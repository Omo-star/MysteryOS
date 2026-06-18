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
#include <sstream>

using namespace std;

bool Kernel::init() {
    if (!vfs_.load("/data/filesystem.json")) return false;
    if (!puzzle_.load("/data/puzzles.json")) return false;
    puzzle_.set_callback([this](const StageUnlock& u) {
        int level = puzzle_.glitch_level();
        Glitch::set_level(level);
        scare_director_.on_stage_unlock(u.stage, session_time_);
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

static string quoted_js_string(const string& value) {
    ostringstream out;
    out << '"';
    for (char c : value) {
        switch (c) {
            case '\\': out << "\\\\"; break;
            case '"': out << "\\\""; break;
            case '\n': out << "\\n"; break;
            case '\r': out << "\\r"; break;
            case '\t': out << "\\t"; break;
            default: out << c; break;
        }
    }
    out << '"';
    return out.str();
}

static bool is_allowed_anomaly_path(const string& path) {
    if (path.empty() || path.size() > 96 || path.find("..") != string::npos) return false;
    return path.rfind("/Desktop/", 0) == 0 || path.rfind("/System/logs/", 0) == 0;
}

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

    VFSNode* node = vfs_.get(path);
    bool corrupted = node && node->corrupted;
    scare_director_.on_file_open(path, puzzle_.stage(), corrupted, files_opened_, session_time_);
}

void Kernel::record_terminal_search(const string& command, const string& query) {
    scare_director_.on_terminal_search(command, query, puzzle_.stage(), session_time_);
}

void Kernel::play_scare_sound(ScareSound sound) {
    const char* name = "dread";
    if (sound == ScareSound::Impact) name = "impact";
    else if (sound == ScareSound::Aperture) name = "aperture";

    string js =
        "if(!window._mysteryScareAudio){"
        "window._mysteryScareAudio=(function(){"
        "function ctx(){"
        "var base=window._mysteryOsc&&window._mysteryOsc.ctx;"
        "if(!base)base=new (window.AudioContext||window.webkitAudioContext)();"
        "if(base.state==='suspended')base.resume();"
        "return base;"
        "}"
        "function gainNode(c,g){var n=c.createGain();n.gain.value=g;return n;}"
        "function noise(c,d){"
        "var b=c.createBuffer(1,Math.max(1,Math.floor(c.sampleRate*d)),c.sampleRate);"
        "var a=b.getChannelData(0);"
        "for(var i=0;i<a.length;i++){a[i]=Math.random()*2-1;}"
        "var s=c.createBufferSource();s.buffer=b;return s;"
        "}"
        "function playImpact(){"
        "var c=ctx(),t=c.currentTime;"
        "var n=noise(c,.42),f=c.createBiquadFilter(),g=gainNode(c,0);"
        "f.type='bandpass';f.frequency.setValueAtTime(2800,t);f.Q.value=7;"
        "g.gain.setValueAtTime(0,t);g.gain.linearRampToValueAtTime(.95,t+.012);g.gain.exponentialRampToValueAtTime(.001,t+.38);"
        "n.connect(f);f.connect(g);g.connect(c.destination);n.start(t);n.stop(t+.44);"
        "var o=c.createOscillator(),og=gainNode(c,0);"
        "o.type='sawtooth';o.frequency.setValueAtTime(72,t);o.frequency.exponentialRampToValueAtTime(31,t+.22);"
        "og.gain.setValueAtTime(.55,t);og.gain.exponentialRampToValueAtTime(.001,t+.5);"
        "o.connect(og);og.connect(c.destination);o.start(t);o.stop(t+.52);"
        "}"
        "function playDread(){"
        "var c=ctx(),t=c.currentTime;"
        "var o=c.createOscillator(),g=gainNode(c,0),f=c.createBiquadFilter();"
        "o.type='sine';o.frequency.setValueAtTime(38,t);o.frequency.linearRampToValueAtTime(54,t+1.4);"
        "f.type='lowpass';f.frequency.value=120;"
        "g.gain.setValueAtTime(0,t);g.gain.linearRampToValueAtTime(.22,t+.45);g.gain.setValueAtTime(.22,t+1.05);g.gain.linearRampToValueAtTime(0,t+1.35);"
        "o.connect(f);f.connect(g);g.connect(c.destination);o.start(t);o.stop(t+1.45);"
        "var tick=noise(c,.08),tg=gainNode(c,0),tf=c.createBiquadFilter();"
        "tf.type='highpass';tf.frequency.value=3200;"
        "tg.gain.setValueAtTime(0,t+.72);tg.gain.linearRampToValueAtTime(.28,t+.735);tg.gain.exponentialRampToValueAtTime(.001,t+.82);"
        "tick.connect(tf);tf.connect(tg);tg.connect(c.destination);tick.start(t+.72);tick.stop(t+.84);"
        "}"
        "function playAperture(){"
        "var c=ctx(),t=c.currentTime;"
        "var o=c.createOscillator(),g=gainNode(c,0);"
        "o.type='triangle';o.frequency.setValueAtTime(180,t);o.frequency.exponentialRampToValueAtTime(22,t+1.1);"
        "g.gain.setValueAtTime(0,t);g.gain.linearRampToValueAtTime(.35,t+.08);g.gain.linearRampToValueAtTime(.08,t+.7);g.gain.exponentialRampToValueAtTime(.001,t+1.2);"
        "o.connect(g);g.connect(c.destination);o.start(t);o.stop(t+1.25);"
        "var n=noise(c,1.2),f=c.createBiquadFilter(),ng=gainNode(c,0);"
        "f.type='bandpass';f.frequency.setValueAtTime(900,t);f.frequency.linearRampToValueAtTime(2600,t+1.0);f.Q.value=11;"
        "ng.gain.setValueAtTime(0,t);ng.gain.linearRampToValueAtTime(.26,t+.16);ng.gain.exponentialRampToValueAtTime(.001,t+1.15);"
        "n.connect(f);f.connect(ng);ng.connect(c.destination);n.start(t);n.stop(t+1.2);"
        "}"
        "return{play:function(kind){if(kind==='impact')playImpact();else if(kind==='aperture')playAperture();else playDread();}};"
        "})();"
        "}"
        "window._mysteryScareAudio.play('" + string(name) + "');";
    emscripten_run_script(js.c_str());
}

void Kernel::request_anomaly(const string& prompt, int terminal_id) {
    last_anomaly_terminal_id_ = terminal_id;
    ostringstream payload;
    payload << "{";
    payload << "\"prompt\":" << quoted_js_string(prompt) << ",";
    payload << "\"terminalId\":" << terminal_id << ",";
    payload << "\"stage\":" << puzzle_.stage() << ",";
    payload << "\"filesOpened\":" << files_opened_ << ",";
    payload << "\"sessionTime\":" << (int)session_time_ << ",";
    payload << "\"recentFiles\":[";
    for (size_t i = 0; i < activity_log_.size(); i++) {
        if (i > 0) payload << ",";
        payload << quoted_js_string(activity_log_[i].path);
    }
    payload << "]}";

    string js =
        "if(window.mysteryosRequestAnomaly){"
        "window.mysteryosRequestAnomaly(" + quoted_js_string(payload.str()) + ");"
        "}else if(Module&&Module.ccall){"
        "Module.ccall('mysteryos_anomaly_response_to',null,['number','string'],[" + to_string(terminal_id) + ",'7741: [no carrier] anomaly layer unavailable']);"
        "}";
    emscripten_run_script(js.c_str());
}

void Kernel::receive_anomaly_response(const string& text) {
    receive_anomaly_response(last_anomaly_terminal_id_, text);
}

void Kernel::receive_anomaly_response(int terminal_id, const string& text) {
    anomaly_responses_.push_back({terminal_id, text});
}

void Kernel::receive_anomaly_artifact(const string& reply, const string& path, const string& content) {
    receive_anomaly_artifact(last_anomaly_terminal_id_, reply, path, content);
}

void Kernel::receive_anomaly_artifact(int terminal_id, const string& reply, const string& path, const string& content) {
    if (!reply.empty()) {
        anomaly_responses_.push_back({terminal_id, "7741: " + reply});
    }

    if (path.empty() || content.empty()) return;

    if (!is_allowed_anomaly_path(path)) {
        anomaly_responses_.push_back({terminal_id, "7741: [write blocked] " + path});
        return;
    }

    vfs_.inject(path, content, false);
    anomaly_responses_.push_back({terminal_id, "7741: [wrote " + path + "]"});
}

vector<string> Kernel::drain_anomaly_responses(int terminal_id) {
    vector<string> responses;
    vector<AnomalyResponse> pending;
    for (auto& response : anomaly_responses_) {
        if (response.terminal_id == terminal_id || response.terminal_id < 0) {
            responses.push_back(move(response.text));
        } else {
            pending.push_back(move(response));
        }
    }
    anomaly_responses_ = move(pending);
    return responses;
}

void Kernel::render(){
    if (booting_) { Glitch::draw_screen_fx(); scare_director_.render(session_time_); render_boot(); return; }

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

    for (const auto& whisper : scare_director_.drain_terminal_messages(session_time_)) {
        anomaly_responses_.push_back({-1, whisper});
    }
    for (ScareSound sound : scare_director_.drain_sound_requests()) {
        play_scare_sound(sound);
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
    scare_director_.render(session_time_);
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
