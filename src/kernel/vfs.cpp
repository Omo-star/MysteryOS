#include "vfs.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <sstream>

using namespace std;

using json = nlohmann::json;
static void parse_node(VFSNode& node, const json& j, const string& name){
    node.name = name;
    if (j.contains("content")){
        node.is_dir = false;
        node.content = j.value("content", string{});
        node.locked = j.value("locked", false);
        node.hidden = j.value("hidden", false);
        node.corrupted = j.value("corrupted", false);
        node.password_key = j.value("requires_password", string{});
    }
    else{
        node.is_dir = true;
        node.locked = j.value("locked", false);
        node.hidden = j.value("hidden", false);
        node.password_key = j.value("requires_password", string{});
        for (auto& [child_name, child_val] : j.items()){
            if (child_name == "locked" || child_name == "hidden" || child_name == "requires_password") continue;
            auto child = make_shared<VFSNode>();
            parse_node(*child, child_val, child_name);
            node.children[child_name] = child;
        }
    }
}

bool VFS::load(const string& path){
    ifstream f(path);
    if (!f) return false;
    json j = json::parse(f, nullptr, false);
    if (j.is_discarded()) return false;
    root_ = make_shared<VFSNode>();
    root_->name = "root";
    root_->is_dir = true;
    if (j.contains("root")){
        for (auto& [name, val] : j["root"].items()){
            auto child = make_shared<VFSNode>();
            parse_node(*child, val, name);
            root_->children[name] = child;
        }
    }
    return true;
}

VFSNode* VFS::root() {return root_.get();}

VFSNode* VFS::get(const string& path){
    if (path.empty() || path == "/") return root_.get();
    VFSNode* cur = root_.get();
    string p = (path[0] == '/') ? path.substr(1) : path;
    istringstream ss(p);
    string seg;
    while (getline(ss, seg, '/')){
        if (!cur || !cur->is_dir) return nullptr;
        auto it = cur->children.find(seg);
        if (it == cur->children.end()) return nullptr;
        cur = it->second.get();
    }
    return cur;
}

bool VFS::is_hidden(const string& path) {
    if (path.empty() || path == "/") return false;
    VFSNode* cur = root_.get();
    string p = (path[0] == '/') ? path.substr(1) : path;
    istringstream ss(p);
    string seg;
    while (getline(ss, seg, '/')){
        if (!cur || !cur->is_dir) return false;
        auto it = cur->children.find(seg);
        if (it == cur->children.end()) return false;
        cur = it->second.get();
        if (cur->hidden) return true;
    }
    return false;
}

void VFS::unlock(const string& path){
    VFSNode* n = get(path);
    if (n) {n->locked = false; n->hidden = false; n->password_key.clear();}
}

void VFS::inject(const string& path, const string& content, bool corrupted){
    auto slash = path.rfind('/');
    string dir = (slash == string::npos) ? "/" : path.substr(0, slash);
    string name = (slash == string::npos) ? path : path.substr(slash + 1);
    VFSNode* parent = get(dir);
    if (!parent || !parent->is_dir) return;
    auto node = make_shared<VFSNode>();
    node->name = name;
    node->content = content;
    node->corrupted = corrupted;
    parent->children[name] = node;
}
