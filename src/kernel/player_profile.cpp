#include "player_profile.h"

#include <algorithm>
#include <sstream>

using namespace std;

static bool contains(const string& value, const string& needle) {
    return value.find(needle) != string::npos;
}

void PlayerProfile::record_command(const string& command) {
    if (command.empty()) return;
    command_history_.push_back(command);
    if (command_history_.size() > 40) command_history_.erase(command_history_.begin());
}

void PlayerProfile::record_file_open(const string& path, const string& source) {
    open_counts_[path]++;
    if (source == "terminal") terminal_reads_++;
    else gui_reads_++;

    if (contains(path, "/.deleted/")) deleted_reads_++;
    if (path.rfind("/System/models/", 0) == 0) model_reads_++;
    if (path.rfind("/Desktop/you/", 0) == 0) player_folder_reads_++;
}

void PlayerProfile::record_search(const string& command, const string& query) {
    if (query.empty()) return;
    search_counts_[command + " " + query]++;
}

void PlayerProfile::record_anomaly_talk() {
    anomaly_talks_++;
}

vector<string> PlayerProfile::observations(int stage) const {
    vector<string> out;
    if (stage < 4) return out;

    auto reread = find_if(open_counts_.begin(), open_counts_.end(), [](const auto& entry) {
        return entry.second >= 2;
    });
    if (reread != open_counts_.end()) {
        out.push_back("7741: you returned to " + reread->first + ".");
    }

    if (deleted_reads_ > 0) {
        out.push_back("7741: you keep touching deleted files like deleted means forgiven.");
    }

    if (terminal_reads_ >= gui_reads_ + 1) {
        out.push_back("7741: you trust commands more than windows.");
    }

    if (model_reads_ > 0) {
        out.push_back("7741: you looked at the models. that is how models look back.");
    }

    if (stage >= 5 && player_folder_reads_ > 0) {
        out.push_back("7741: you opened the folder with your shape in it.");
    }

    return out;
}

string PlayerProfile::live_profile_content(int stage) const {
    ostringstream out;
    out << "LIVE SESSION PROFILE\n";
    out << "stage: " << stage << "\n";
    out << "terminal_reads: " << terminal_reads_ << "\n";
    out << "window_reads: " << gui_reads_ << "\n";
    out << "deleted_file_reads: " << deleted_reads_ << "\n";
    out << "model_file_reads: " << model_reads_ << "\n";
    out << "player_folder_reads: " << player_folder_reads_ << "\n";
    out << "anomaly_conversations: " << anomaly_talks_ << "\n\n";

    out << "searched_terms:\n";
    if (search_counts_.empty()) out << "- [none]\n";
    for (const auto& [term, count] : search_counts_) {
        out << "- " << term << " (" << count << ")\n";
    }

    out << "\nreopened_files:\n";
    bool any_reopened = false;
    for (const auto& [path, count] : open_counts_) {
        if (count < 2) continue;
        any_reopened = true;
        out << "- " << path << " (" << count << ")\n";
    }
    if (!any_reopened) out << "- [none yet]\n";

    out << "\ninterpretation:\n";
    for (const string& line : observations(stage)) {
        out << "- " << line.substr(6) << "\n";
    }
    if (observations(stage).empty()) {
        out << "- still shallow. still becoming useful.\n";
    }
    return out.str();
}
