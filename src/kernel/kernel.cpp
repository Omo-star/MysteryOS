#include "kernel.h"
#include "imgui.h"
#include "fx/glitch.h"
#include "apps/file_explorer.h"
#include "apps/image_viewer.h"
#include "apps/text_editor.h"
#include "apps/terminal.h"
#include "apps/password_dialog.h"
#include "apps/anomaly_message.h"
#include "apps/session_monitor.h"
#include <algorithm>
#include <cmath>
#include <emscripten.h>

using namespace std;

bool Kernel::init() {
    if (!vfs_.load("/data/filesystem.json")) return false;
    if (!puzzle_.load("/data/puzzles.json")) return false;
    puzzle_.set_callback([this](const StageUnlock& u) {
        int level = puzzle_.glitch_level();
        Glitch::set_level(level);
        float freq = 60.0f + level * 18.0f;
        float gain = 0.04f + level * 0.012f;
        char js[256];
        snprintf(js, sizeof(js),
            "if(window._mysteryOsc){"
            "  window._mysteryOsc.osc.frequency.setTargetAtTime(%.1f, window._mysteryOsc.ctx.currentTime, 2);"
            "  window._mysteryOsc.gain.gain.setTargetAtTime(%.3f, window._mysteryOsc.ctx.currentTime, 2);"
            "}",
            freq, gain);
        emscripten_run_script(js);
    });
    emscripten_run_script(
        "window._mysteryOsc = (function(){"
        "  var ctx = new AudioContext();"
        "  var osc = ctx.createOscillator();"
        "  var gain = ctx.createGain();"
        "  osc.type = 'sine';"
        "  osc.frequency.value = 60;"
        "  gain.gain.value = 0.04;"
        "  osc.connect(gain);"
        "  gain.connect(ctx.destination);"
        "  osc.start();"
        "  return {osc: osc, gain: gain, ctx: ctx};"
        "})();"
    );
    return true;
}

static const char* BOOT_LINES[] = {
    "MysteryOS v2.4.1 -- Meridian Analytics",
    "Copyright (c) 2019-2024 Meridian Research Group",
    "",
    "[  0.000] Initializing kernel...",
    "[  0.041] Memory: 8192MB available",
    "[  0.089] Loading filesystem drivers...",
    "[  0.134] Mounting /data ... OK",
    "[  0.201] Loading user profile: evoss",
    "[  0.267] Checking system integrity...",
    "[  0.312] /System/drivers/display.sys ... OK",
    "[  0.318] /System/drivers/input.sys ... OK",
    "[  0.334] /System/drivers/network.sys ... MODIFIED",
    "[  0.356] Starting network services...",
    "[  0.401] Network interface: UP (10.0.0.14)",
    "[  0.445] Starting session monitor...",
    "[  0.489] Checking active processes...",
    "[  0.534] PID 1      init           ... OK",
    "[  0.541] PID 312    file_explorer  ... OK",
    "[  0.548] PID 7741   [no name]      ... running since: [unknown]",
    "[  0.571] Starting desktop environment...",
    "[  0.623] Last session: March 14, 2024  11:48 PM",
    "[  0.634] Session ready.",
};
static const int BOOT_LINE_COUNT = 22;

void Kernel::render_boot() {
    boot_timer_ += ImGui::GetIO().DeltaTime;
    int target = (int)(boot_timer_ / 0.12f);
    if (target > boot_line_) boot_line_ = target;
    if (boot_timer_ > (BOOT_LINE_COUNT + 8) * 0.12f) { booting_ = false; return; }

    ImGui::SetNextWindowPos({0, 0});
    ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
    ImGui::Begin("##boot", nullptr,
        ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoBackground);
    int show = min(boot_line_, BOOT_LINE_COUNT);
    for (int i = 0; i < show; i++) {
        const char* line = BOOT_LINES[i];
        if (strstr(line, "MODIFIED") || strstr(line, "[no name]") || strstr(line, "[unknown]"))
            ImGui::TextColored({1.0f, 0.35f, 0.35f, 1.0f}, "%s", line);
        else if (i < 3)
            ImGui::TextColored({0.55f, 0.55f, 0.55f, 1.0f}, "%s", line);
        else
            ImGui::TextColored({0.6f, 0.85f, 0.6f, 1.0f}, "%s", line);
    }
    ImGui::End();
}

void Kernel::record_file_open(const string& path) {
    files_opened_++;
    activity_log_.push_back({path, session_time_});
    if (activity_log_.size() > 12) {
        activity_log_.erase(activity_log_.begin());
    }

    static const int THRESHOLDS[] = {8, 20, 40, 75, 120, 200};
    static const char* MSGS[] = {
        "SESSION FILE ACCESS COUNT: ELEVATED\nMonitoring in progress.",
        "ACCESS ANOMALY DETECTED\nUnauthorized read pattern observed.\nPID 7741 notified.",
        "WARNING: BEHAVIORAL PROFILE UPDATED\nSession activity logged to /System/logs/",
        "ALERT: READ DEPTH EXCEEDS THRESHOLD\nYou are accessing restricted directories.",
        "CRITICAL: SESSION INTEGRITY COMPROMISED\nThis activity has been recorded.",
        "STOP."
    };
    for (int i = 0; i < 6; i++) {
        if (files_opened_ == THRESHOLDS[i]) {
            show_error_popup_ = true;
            error_popup_msg_ = MSGS[i];
        }
    }
}

void Kernel::render(){
    if (booting_) { Glitch::draw_screen_fx(); render_boot(); return; }

    // Idle tracking for anomaly window
    float dt = ImGui::GetIO().DeltaTime;
    session_time_ += dt;
    ImVec2 md = ImGui::GetIO().MouseDelta;
    bool user_active = (fabsf(md.x) + fabsf(md.y) > 0.5f)
                    || ImGui::GetIO().MouseClicked[0]
                    || ImGui::GetIO().MouseClicked[1]
                    || ImGui::GetIO().InputQueueCharacters.Size > 0;
    if (user_active) idle_timer_ = 0.0f;
    else             idle_timer_ += dt;

    if (puzzle_.stage() >= 3 && !anomaly_spawned_ && idle_timer_ > 90.0f) {
        anomaly_spawned_ = true;
        launch("anomaly_message");
    }

    // Error popup
    if (show_error_popup_) {
        ImGui::OpenPopup("SYSTEM ERROR");
        show_error_popup_ = false;
    }
    if (ImGui::BeginPopupModal("SYSTEM ERROR", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::TextColored({1.0f, 0.3f, 0.3f, 1.0f}, "%s", error_popup_msg_.c_str());
        ImGui::Spacing();
        if (ImGui::Button("   Dismiss   ")) ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
    }

    Glitch::draw_screen_fx();
    render_taskbar();
    for (auto& w : windows_){
        if (!w.open) continue;
        float offset = (w.id % 6) * 30.0f;
        ImGui::SetNextWindowPos({80.0f + offset, 50.0f + offset}, ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize({600, 400}, ImGuiCond_FirstUseEver);
        bool open = true;
        string win_title = string(w.app->title());
        if (Glitch::level() >= 7 && rand() % 25 == 0)
            win_title = Glitch::mangle(win_title);
        win_title += "##win" + to_string(w.id);
        if (ImGui::Begin(win_title.c_str(), &open)) w.app->render(*this);
        ImGui::End();
        if (!open) w.open = false;
    }
    windows_.erase(remove_if(windows_.begin(), windows_.end(), [](const WinEntry& w){return !w.open;}), windows_.end());
    for (auto& [name, arg] : pending_launches_) {
        auto app = make_app(name, arg);
        if (app) windows_.push_back({next_id_++, true, move(app)});
    }
    pending_launches_.clear();
}

void Kernel::render_taskbar(){
    ImGui::SetNextWindowPos({0, 0});
    ImGui::SetNextWindowSize({ImGui::GetIO().DisplaySize.x, 24});
    ImGui::Begin("##taskbar", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus);
    ImGui::Text("MysteryOS");
    ImGui::SameLine();
    if (ImGui::SmallButton("File Explorer")) launch("file_explorer");
    ImGui::SameLine();
    if (ImGui::SmallButton("Terminal")) launch("terminal");
    ImGui::End();
}

unique_ptr<App> Kernel::make_app(const string& name, const string& arg){
    if (name == "file_explorer") return make_unique<FileExplorer>();
    if (name == "text_editor") return make_unique<TextEditor>(arg);
    if (name == "terminal") return make_unique<Terminal>();
    if (name == "password_dialog") return make_unique<PasswordDialog>(arg);
    if (name == "image_viewer") return make_unique<ImageViewer>(arg);
    if (name == "anomaly_message") return make_unique<AnomalyMessage>();
    if (name == "session_monitor") return make_unique<SessionMonitor>();
    return nullptr;
}

void Kernel::launch(const string& name, const string& arg){
    pending_launches_.push_back({name, arg});
}
