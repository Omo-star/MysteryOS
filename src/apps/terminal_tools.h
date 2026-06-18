#pragma once

#include "kernel/vfs.h"
#include <string>
#include <vector>

namespace TerminalTools {
    std::vector<std::string> grep(VFS& vfs, const std::string& term, const std::string& path);
    std::vector<std::string> find(VFS& vfs, const std::string& term, const std::string& path);
    std::vector<std::string> timeline(VFS& vfs, const std::string& path);
    std::vector<std::string> diff(VFS& vfs, const std::string& left_path, const std::string& right_path);
    std::vector<std::string> stat(VFS& vfs, const std::string& path);
    std::vector<std::string> strings(VFS& vfs, const std::string& path);
    std::vector<std::string> hash(VFS& vfs, const std::string& path);
}
