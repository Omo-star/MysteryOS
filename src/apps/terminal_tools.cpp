#include "terminal_tools.h"

#include <algorithm>
#include <cctype>
#include <iomanip>
#include <sstream>

using namespace std;

namespace {
    const size_t MAX_RESULTS = 40;

    string lower_copy(string value) {
        for (char& c : value) c = (char)tolower((unsigned char)c);
        return value;
    }

    string clean_path(const string& path) {
        if (path.empty()) return "/";
        return path;
    }

    string child_path(const string& parent, const string& name) {
        if (parent.empty() || parent == "/") return "/" + name;
        return parent + "/" + name;
    }

    string trim_cr(string line) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        return line;
    }

    bool contains_case_insensitive(const string& value, const string& term) {
        return lower_copy(value).find(lower_copy(term)) != string::npos;
    }

    bool result_limit_hit(vector<string>& out, const string& command) {
        if (out.size() < MAX_RESULTS) return false;
        if (out.size() == MAX_RESULTS) out.push_back(command + ": result limit reached");
        return true;
    }

    bool line_has_numeric_date(const string& line) {
        for (size_t i = 0; i + 4 < line.size(); i++) {
            if (isdigit((unsigned char)line[i]) &&
                isdigit((unsigned char)line[i + 1]) &&
                (line[i + 2] == '/' || line[i + 2] == '-') &&
                isdigit((unsigned char)line[i + 3]) &&
                isdigit((unsigned char)line[i + 4])) {
                return true;
            }
        }

        for (size_t i = 0; i + 9 < line.size(); i++) {
            if (isdigit((unsigned char)line[i]) &&
                isdigit((unsigned char)line[i + 1]) &&
                isdigit((unsigned char)line[i + 2]) &&
                isdigit((unsigned char)line[i + 3]) &&
                line[i + 4] == '-' &&
                isdigit((unsigned char)line[i + 5]) &&
                isdigit((unsigned char)line[i + 6]) &&
                line[i + 7] == '-' &&
                isdigit((unsigned char)line[i + 8]) &&
                isdigit((unsigned char)line[i + 9])) {
                return true;
            }
        }

        return false;
    }

    bool is_word_char(char c) {
        return isalnum((unsigned char)c) || c == '_';
    }

    bool has_digit_near(const string& line, size_t pos, size_t len) {
        size_t start = pos > 6 ? pos - 6 : 0;
        size_t end = min(line.size(), pos + len + 6);
        for (size_t i = start; i < end; i++) {
            if (isdigit((unsigned char)line[i])) return true;
        }
        return false;
    }

    bool contains_month_marker(const string& lower, const string& month) {
        size_t pos = lower.find(month);
        while (pos != string::npos) {
            bool left_ok = pos == 0 || !is_word_char(lower[pos - 1]);
            size_t after = pos + month.size();
            bool right_ok = after >= lower.size() || !is_word_char(lower[after]);
            if (left_ok && right_ok && (month != "may" || has_digit_near(lower, pos, month.size()))) {
                return true;
            }
            pos = lower.find(month, pos + 1);
        }
        return false;
    }

    bool line_has_timeline_marker(const string& line) {
        static const char* MONTHS[] = {
            "january", "february", "march", "april", "may", "june",
            "july", "august", "september", "october", "november", "december"
        };
        string lower = lower_copy(line);
        for (const char* month : MONTHS) {
            if (contains_month_marker(lower, month)) return true;
        }
        return line_has_numeric_date(line);
    }

    void grep_node(VFSNode* node, const string& path, const string& term, vector<string>& out) {
        if (!node || node->hidden || node->locked || result_limit_hit(out, "grep")) return;

        if (node->is_dir) {
            for (auto& [name, child] : node->children) {
                grep_node(child.get(), child_path(path, name), term, out);
                if (out.size() > MAX_RESULTS) return;
            }
            return;
        }

        istringstream lines(node->content);
        string line;
        while (getline(lines, line)) {
            line = trim_cr(line);
            if (contains_case_insensitive(line, term)) {
                out.push_back(path + ": " + line);
                if (result_limit_hit(out, "grep")) return;
            }
        }
    }

    void find_node(VFSNode* node, const string& path, const string& term, vector<string>& out) {
        if (!node || node->hidden || result_limit_hit(out, "find")) return;

        if (contains_case_insensitive(node->name, term)) {
            out.push_back(path + (node->is_dir ? "/" : ""));
            if (result_limit_hit(out, "find")) return;
        }

        if (!node->is_dir || node->locked) return;
        for (auto& [name, child] : node->children) {
            find_node(child.get(), child_path(path, name), term, out);
            if (out.size() > MAX_RESULTS) return;
        }
    }

    void timeline_node(VFSNode* node, const string& path, vector<string>& out) {
        if (!node || node->hidden || node->locked || result_limit_hit(out, "timeline")) return;

        if (node->is_dir) {
            for (auto& [name, child] : node->children) {
                timeline_node(child.get(), child_path(path, name), out);
                if (out.size() > MAX_RESULTS) return;
            }
            return;
        }

        istringstream lines(node->content);
        string line;
        while (getline(lines, line)) {
            line = trim_cr(line);
            if (line_has_timeline_marker(line)) {
                out.push_back(path + ": " + line);
                if (result_limit_hit(out, "timeline")) return;
            }
        }
    }

    VFSNode* visible_start(VFS& vfs, const string& path) {
        string root = clean_path(path);
        if (vfs.is_hidden(root)) return nullptr;
        return vfs.get(root);
    }

    VFSNode* visible_file(VFS& vfs, const string& path, const string& command, vector<string>& out) {
        string root = clean_path(path);
        if (vfs.is_hidden(root)) {
            out.push_back(command + ": not found: " + root);
            return nullptr;
        }
        VFSNode* node = vfs.get(root);
        if (!node) {
            out.push_back(command + ": not found: " + root);
            return nullptr;
        }
        if (node->locked) {
            out.push_back(command + ": permission denied: " + root);
            return nullptr;
        }
        if (node->is_dir) {
            out.push_back(command + ": is a directory: " + root);
            return nullptr;
        }
        return node;
    }

    vector<string> split_lines(const string& content) {
        vector<string> lines;
        istringstream in(content);
        string line;
        while (getline(in, line)) lines.push_back(trim_cr(line));
        if (!content.empty() && content.back() == '\n') return lines;
        return lines;
    }
}

namespace TerminalTools {
    vector<string> grep(VFS& vfs, const string& term, const string& path) {
        if (term.empty()) return {"grep: missing search term"};
        string root = clean_path(path);
        VFSNode* node = visible_start(vfs, root);
        if (!node) return {"grep: path not found: " + root};

        vector<string> out;
        grep_node(node, root, term, out);
        if (out.empty()) out.push_back("grep: no matches");
        return out;
    }

    vector<string> find(VFS& vfs, const string& term, const string& path) {
        if (term.empty()) return {"find: missing search term"};
        string root = clean_path(path);
        VFSNode* node = visible_start(vfs, root);
        if (!node) return {"find: path not found: " + root};

        vector<string> out;
        find_node(node, root, term, out);
        if (out.empty()) out.push_back("find: no matches");
        return out;
    }

    vector<string> timeline(VFS& vfs, const string& path) {
        string root = clean_path(path);
        VFSNode* node = visible_start(vfs, root);
        if (!node) return {"timeline: path not found: " + root};

        vector<string> out;
        timeline_node(node, root, out);
        if (out.empty()) out.push_back("timeline: no dated lines found");
        return out;
    }

    vector<string> diff(VFS& vfs, const string& left_path, const string& right_path) {
        vector<string> out;
        VFSNode* left = visible_file(vfs, left_path, "diff", out);
        VFSNode* right = visible_file(vfs, right_path, "diff", out);
        if (!left || !right) return out;

        vector<string> left_lines = split_lines(left->content);
        vector<string> right_lines = split_lines(right->content);
        size_t max_lines = max(left_lines.size(), right_lines.size());
        for (size_t i = 0; i < max_lines && out.size() < MAX_RESULTS; i++) {
            string left_line = i < left_lines.size() ? left_lines[i] : "";
            string right_line = i < right_lines.size() ? right_lines[i] : "";
            if (left_line == right_line) continue;
            if (i < left_lines.size()) out.push_back("- " + left_line);
            if (i < right_lines.size() && out.size() < MAX_RESULTS) out.push_back("+ " + right_line);
        }
        if (out.empty()) out.push_back("diff: files match");
        return out;
    }

    vector<string> stat(VFS& vfs, const string& path) {
        string root = clean_path(path);
        if (vfs.is_hidden(root)) return {"stat: not found: " + root};
        VFSNode* node = vfs.get(root);
        if (!node) return {"stat: not found: " + root};

        vector<string> out;
        out.push_back("path: " + root);
        out.push_back(string("type: ") + (node->is_dir ? "directory" : "file"));
        out.push_back(string("locked: ") + (node->locked ? "yes" : "no"));
        out.push_back(string("hidden: ") + (node->hidden ? "yes" : "no"));
        out.push_back(string("corrupted: ") + (node->corrupted ? "yes" : "no"));
        if (node->is_dir) {
            out.push_back("children: " + to_string(node->children.size()));
        } else {
            out.push_back("bytes: " + to_string(node->content.size()));
            out.push_back("lines: " + to_string(split_lines(node->content).size()));
        }
        return out;
    }

    vector<string> strings(VFS& vfs, const string& path) {
        vector<string> out;
        VFSNode* node = visible_file(vfs, path, "strings", out);
        if (!node) return out;

        string run;
        for (unsigned char c : node->content) {
            if (isprint(c) || c == '\t') {
                run.push_back((char)c);
            } else {
                if (run.size() >= 4) out.push_back(run);
                run.clear();
            }
            if (out.size() >= MAX_RESULTS) break;
        }
        if (run.size() >= 4 && out.size() < MAX_RESULTS) out.push_back(run);
        if (out.empty()) out.push_back("strings: no printable runs");
        return out;
    }

    vector<string> hash(VFS& vfs, const string& path) {
        vector<string> out;
        VFSNode* node = visible_file(vfs, path, "hash", out);
        if (!node) return out;

        unsigned long long value = 1469598103934665603ull;
        for (unsigned char c : node->content) {
            value ^= c;
            value *= 1099511628211ull;
        }
        ostringstream hex;
        hex << "fnv1a64: 0x" << std::hex << std::setw(16) << std::setfill('0') << value;
        out.push_back(hex.str());
        return out;
    }
}
