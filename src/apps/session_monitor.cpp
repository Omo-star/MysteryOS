#include "session_monitor.h"
#include "kernel/kernel.h"
#include "imgui.h"
#include <algorithm>

using namespace std;

static const char* stage_status(int stage) {
    if (stage >= 5) return "observing current session";
    if (stage >= 4) return "writing without user input";
    if (stage >= 3) return "bound to evoss session";
    if (stage >= 2) return "not registered in process table";
    return "access restricted";
}

void SessionMonitor::render(Kernel& k) {
    int stage = k.puzzle().stage();
    float cpu = min(99.9f, 31.4f + stage * 4.7f + k.files_opened() * 0.08f);

    ImGui::Text("Meridian Session Monitor");
    ImGui::Separator();
    ImGui::Text("session: evoss");
    ImGui::Text("uptime: %.0fs", k.session_time());
    ImGui::Text("file reads this session: %d", k.files_opened());
    ImGui::Spacing();

    ImGui::BeginChild("##processes", {230, 0}, true);
    if (ImGui::Selectable("1001  system_idle", selected_pid_ == 1001)) selected_pid_ = 1001;
    if (ImGui::Selectable("1003  session_manager", selected_pid_ == 1003)) selected_pid_ = 1003;
    if (ImGui::Selectable("1044  user_session_evoss", selected_pid_ == 1044)) selected_pid_ = 1044;
    if (ImGui::Selectable("7741  [UNKNOWN]", selected_pid_ == 7741)) selected_pid_ = 7741;
    ImGui::EndChild();

    ImGui::SameLine();
    ImGui::BeginChild("##details", {0, 0}, true);
    if (selected_pid_ == 7741) {
        ImGui::TextColored({1.0f, 0.35f, 0.35f, 1.0f}, "PID 7741");
        ImGui::Text("name: [UNKNOWN]");
        ImGui::Text("cpu: %.1f%%", cpu);
        ImGui::Text("memory: unbounded");
        ImGui::Text("status: %s", stage_status(stage));
        ImGui::Text("kill policy: denied");
        ImGui::Spacing();
        ImGui::TextWrapped("observer correlation: %s", stage >= 3 ? "active" : "suspected");
        ImGui::TextWrapped("session anchor: %s", stage >= 3 ? "evoss" : "[redacted]");
        ImGui::TextWrapped("classification: %s", stage >= 4 ? "not malware" : "unclassified anomaly");
    } else {
        ImGui::Text("PID %d", selected_pid_);
        ImGui::Text("status: nominal");
        ImGui::Text("kill policy: allowed");
    }

    ImGui::Separator();
    ImGui::Text("recent reads");
    const auto& log = k.activity_log();
    if (log.empty()) {
        ImGui::TextDisabled("no user file reads logged");
    } else {
        for (auto it = log.rbegin(); it != log.rend(); ++it) {
            ImGui::Text("[%.0fs] %s", it->time, it->path.c_str());
        }
    }
    ImGui::EndChild();
}
