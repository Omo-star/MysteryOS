#pragma once
#include <string>
#include <map>
#include <memory>

struct VFSNode{
    std::string name;
    bool is_dir = false;
    std::string content;
    bool locked = false;
    bool hidden = false;
    bool corrupted = false;
    std::string password_key;
    std::map<std::string, std::shared_ptr<VFSNode>> children;
};

class VFS{
    public:
        bool load(const std::string& json_path);
        VFSNode* root();
        VFSNode* get(const std::string& path);
        bool is_hidden(const std::string& path);
        void unlock(const std::string& path);
        void inject(const std::string& path, const std::string& content, bool corrupted = false);
    private:
        std::shared_ptr<VFSNode> root_;
};
