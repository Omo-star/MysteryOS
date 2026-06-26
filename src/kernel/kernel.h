#pragma once
#include <vector>
#include <memory>
#include <string>
#include "vfs.h"
#include "puzzle.h"
#include "app.h"
#include "fx/scare_director.h"
#include "player_profile.h"

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

struct PhantomEntry{
    string name;
    bool is_dir = false;
};

struct NotifEntry{
    string sender;
    string msg;
    string color;
    int glitch = 0;
    float time = 0.0f;
};

class Kernel{
    public:
        bool init();
        void render();
        void launch(const string& app_name, const string& arg = "");
        VFS& vfs() {return vfs_;}
        PuzzleState& puzzle() {return puzzle_;}
        void record_file_open(const string& path, const string& source = "gui");
        void record_terminal_command(const string& command);
        void record_terminal_search(const string& command, const string& query);
        vector<string> command_history() const;
        vector<PhantomEntry> phantom_entries_for(const string& path) const;
        const vector<ActivityEntry>& activity_log() const {return activity_log_;}
        int files_opened() const {return files_opened_;}
        float session_time() const {return session_time_;}
        void request_anomaly(const string& prompt, int terminal_id);
        void receive_anomaly_response(const string& text);
        void receive_anomaly_response(int terminal_id, const string& text);
        void receive_anomaly_artifact(const string& reply, const string& path, const string& content);
        void receive_anomaly_artifact(int terminal_id, const string& reply, const string& path, const string& content);
        vector<string> drain_anomaly_responses(int terminal_id);
        const vector<NotifEntry>& notifications() const {return notifications_;}
        void push_notification(const string& sender, const string& msg, const string& color = "#5a5", int glitch = 0, int duration = 4500);
    private:
        VFS vfs_;
        PuzzleState puzzle_;
        ScareDirector scare_director_;
        PlayerProfile player_profile_;
        vector<WinEntry> windows_;
        vector<pair<string,string>> pending_launches_;
        bool booting_ = true;
        float boot_timer_ = 0.0f;
        int boot_line_ = 0;
        int boot_char_ = 0;
        float boot_char_accum_ = 0.0f;
        float boot_line_pause_ = 0.0f;
        float boot_glitch_timer_ = 0.0f;
        float boot_fadeout_timer_ = -1.0f;
        int boot_phase_ = 0; // 0=CRT on, 1=typing, 2=fadeout, 3=done
        int boot_mem_counter_ = 0;
        void render_boot();
        int next_id_ = 0;
        void render_taskbar();
        unique_ptr<App> make_app(const string& name, const string& arg);
        float idle_timer_ = 0.0f;
        float session_time_ = 0.0f;
        bool anomaly_spawned_ = false;
        int files_opened_ = 0;
        void play_scare_sound(ScareSound sound);
        void update_living_files();
        vector<ActivityEntry> activity_log_;
        int last_anomaly_terminal_id_ = -1;
        vector<AnomalyResponse> anomaly_responses_;
        bool show_error_popup_ = false;
        string error_popup_msg_;
        vector<NotifEntry> notifications_;
};
