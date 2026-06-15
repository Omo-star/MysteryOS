#include "puzzle.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <algorithm>

using namespace std;
using json = nlohmann::json;

bool PuzzleState::load(const string& path){
    ifstream f(path);
    if (!f) return false;
    json j = json::parse(f, nullptr, false);
    if (j.is_discarded() || !j.is_array()) return false;
    for (auto& e : j){
        StageUnlock su;
        su.stage = e.value("stage", 0);
        su.password = e.value("password", string());
        su.glitch_level = e.value("glitch_level", 0);
        for (auto& p : e.value("unlock_paths", json::array())) su.unlock_paths.push_back(p);
        for (auto& a : e.value("unlock_apps", json::array())) su.unlock_apps.push_back(a);
        for (auto& fi : e.value("inject_files", json::array()))
            su.inject_files.push_back({fi.value("path",""), fi.value("content",""), fi.value("corrupted", false)});
        stages_.push_back(move(su));
    }
    return true;
}

bool PuzzleState::try_password(const string& input, VFS& vfs) {
    for (auto& su : stages_) {
        if (su.password == input && su.stage == stage_ + 1) {
            apply(su, vfs);
            return true;
        }
    }
    return false;
}

void PuzzleState::apply(const StageUnlock& su, VFS& vfs) {
    stage_        = su.stage;
    glitch_level_ = su.glitch_level;
    for (auto& p  : su.unlock_paths)  vfs.unlock(p);
    for (auto& a  : su.unlock_apps)   unlocked_apps_.push_back(a);
    for (auto& fi : su.inject_files)  vfs.inject(fi.path, fi.content, fi.corrupted);
    if (callback_) callback_(su);
}

bool PuzzleState::is_app_unlocked(const string& name) const {
    return find(unlocked_apps_.begin(), unlocked_apps_.end(), name) != unlocked_apps_.end();
}
