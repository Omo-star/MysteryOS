#include "notification_manager.h"
#include "kernel/kernel.h"
#include "imgui.h"
#include "fx/glitch.h"
#include <cmath>
#include <cstring>

void NotificationManager::render(Kernel& k) {
    auto& notifs = k.notifications();
    int stage = k.puzzle().stage();
    float now = k.session_time();

    if (notifs.empty()) {
        ImGui::TextColored({0.5f, 0.5f, 0.5f, 1.0f}, "No notifications.");
        return;
    }

    // Header gets corrupted at higher stages
    if (stage >= 4) {
        Glitch::TextCorruptor corruptor(0.3f);
        string header = corruptor.render("Notification History", now);
        ImGui::TextColored({0.7f, 0.7f, 0.7f, 1.0f}, "%s", header.c_str());
    } else {
        ImGui::TextColored({0.7f, 0.7f, 0.7f, 1.0f}, "Notification History");
    }
    ImGui::Separator();
    ImGui::Spacing();

    // Show notifications newest-first
    for (int i = (int)notifs.size() - 1; i >= 0; i--) {
        auto& n = notifs[i];

        // Parse the hex color string into ImVec4
        unsigned int r = 0x5a, g = 0xaa, b = 0x5a;
        if (n.color.size() >= 4 && n.color[0] == '#') {
            sscanf(n.color.c_str() + 1, "%02x%02x%02x", &r, &g, &b);
        }
        float cr = r / 255.0f, cg = g / 255.0f, cb = b / 255.0f;

        // Timestamp
        int mins = (int)(n.time / 60.0f);
        int secs = (int)n.time % 60;
        ImGui::TextColored({0.4f, 0.4f, 0.4f, 1.0f}, "[%02d:%02d]", mins, secs);
        ImGui::SameLine();

        // Sender — colored, gets mangled at high glitch
        string sender = n.sender;
        string msg = n.msg;
        if (n.glitch > 0) {
            int saved = Glitch::level();
            Glitch::set_level(n.glitch);
            sender = Glitch::mangle(sender);
            msg = Glitch::mangle(msg);
            Glitch::set_level(saved);
        }

        ImGui::TextColored({cr, cg, cb, 1.0f}, "%s", sender.c_str());
        ImGui::TextWrapped("%s", msg.c_str());

        // At stage 5+, some old notifications "mutate" — show altered text
        if (stage >= 5 && i < (int)notifs.size() - 3 && n.glitch < 3) {
            float flicker = sinf(now * 2.0f + i * 1.3f);
            if (flicker > 0.7f) {
                ImGui::SameLine();
                ImGui::TextColored({0.6f, 0.15f, 0.15f, 0.5f}, " [amended]");
            }
        }

        ImGui::Spacing();
        if (i > 0) {
            ImGui::PushStyleColor(ImGuiCol_Separator, ImVec4(0.2f, 0.2f, 0.2f, 0.3f));
            ImGui::Separator();
            ImGui::PopStyleColor();
        }
    }

    // At stage 4+, phantom notifications appear at the bottom
    if (stage >= 4) {
        ImGui::Spacing();
        ImGui::Separator();
        float pulse = 0.3f + 0.2f * sinf(now * 3.0f);
        ImGui::TextColored({pulse, 0.05f, 0.05f, pulse}, "[entries below this line were not sent by any known process]");

        if (stage >= 5) {
            ImGui::Spacing();
            string phantom = Glitch::mangle("you opened the notification manager to understand what is watching you. that is also being watched.");
            ImGui::TextColored({0.5f, 0.12f, 0.12f, 0.7f}, "%s", phantom.c_str());
        }
    }
}
