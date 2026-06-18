#pragma once
#include <vector>
#include <memory>
#include <string>
#include "vfs.h"
#include "puzzle.h"
#include "app.h"
#include "fx/scare_director.h"

using namespace std;

struct WinEntry{
    int id;
    bool open = true;
    unique_ptr<App> app;
};

struct ActivityEntry{
    string path;
    float time = 0.0f;
};

struct AnomalyResponse{
    int terminal_id = -1;
    string text;
};

class Kernel{
    public:
        bool init();
        void render();
        void launch(const string& app_name, const string& arg = "");
        VFS& vfs() {return vfs_;}
        PuzzleState& puzzle() {return puzzle_;}
        void record_file_open(const string& path);
        void record_terminal_search(const string& command, const string& query);
        const vector<ActivityEntry>& activity_log() const {return activity_log_;}
        int files_opened() const {return files_opened_;}
        float session_time() const {return session_time_;}
        void request_anomaly(const string& prompt, int terminal_id);
        void receive_anomaly_response(const string& text);
        void receive_anomaly_response(int terminal_id, const string& text);
        void receive_anomaly_artifact(const string& reply, const string& path, const string& content);
        void receive_anomaly_artifact(int terminal_id, const string& reply, const string& path, const string& content);
        vector<string> drain_anomaly_responses(int terminal_id);
    private:
        VFS vfs_;
        PuzzleState puzzle_;
        ScareDirector scare_director_;
        vector<WinEntry> windows_;
        vector<pair<string,string>> pending_launches_;
        bool booting_ = true;
        float boot_timer_ = 0.0f;
        int boot_line_ = 0;
        void render_boot();
        int next_id_ = 0;
        void render_taskbar();
        unique_ptr<App> make_app(const string& name, const string& arg);
        float idle_timer_ = 0.0f;
        float session_time_ = 0.0f;
        bool anomaly_spawned_ = false;
        int files_opened_ = 0;
        void play_scare_sound(ScareSound sound);
        vector<ActivityEntry> activity_log_;
        int last_anomaly_terminal_id_ = -1;
        vector<AnomalyResponse> anomaly_responses_;
        bool show_error_popup_ = false;
        string error_popup_msg_;
};
