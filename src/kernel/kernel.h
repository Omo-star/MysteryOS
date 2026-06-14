#pragma once
#include <vector>
#include <memory>
#include <string>
#include "vfs.h"
#include "puzzle.h"
#include "app.h"

using namespace std;

struct WinEntry{
    int id;
    bool open = true;
    unique_ptr<App> app;
};

class Kernel{
    public:
        bool init();
        void render();
        void launch(const string& app_name, const string& arg = "");
        VFS& vfs() {return vfs_;}
        PuzzleState& puzzle() {return puzzle_;}
    private:
        VFS vfs_;
        PuzzleState puzzle_;
        vector<WinEntry> windows_;
        vector<pair<string,string>> pending_launches_;
        bool booting_ = true;
        float boot_timer_ = 0.0f;
        int boot_line_ = 0;
        void render_boot();
        int next_id_ = 0;
        void render_taskbar();
        unique_ptr<App> make_app(const string& name, const string& arg);
};
