#include "fx/glitch.h"
#include <cstdlib>
#include <cstring>
#include <ctime>
#ifndef MYSTERYOS_TEST
#include "imgui.h"
#endif

using namespace std;

namespace Glitch {
    static int g_level = 0;
    void set_level(int level) { g_level = level < 0 ? 0 : level > 10 ? 10 : level; }
    int  level() { return g_level; }

    string mangle(const string& text) {
        if (g_level == 0) return text;
        static const char* noise = "@#$%!?*~^&0";
        const int noise_len = (int)strlen(noise);
        float rate = g_level * 0.05f;
        string out = text;
        srand((unsigned)time(nullptr));
        for (char& c : out)
            if (c != '\n' && (rand() / (float)RAND_MAX) < rate) c = noise[rand() % noise_len];
        return out;
    }

    #ifndef MYSTERYOS_TEST
    void draw_screen_fx() {
        if (g_level == 0) return;
        ImDrawList* dl = ImGui::GetBackgroundDrawList();
        ImVec2 disp = ImGui::GetIO().DisplaySize;

        // scan lines
        if (g_level >= 1)
            for (float y = 0; y < disp.y; y += 4.0f)
                dl->AddLine({0, y}, {disp.x, y}, IM_COL32(0, 0, 0, 30));

        // pixel noise
        if (g_level >= 3) {
            int burst = g_level * 25;
            for (int i = 0; i < burst; i++) {
                float x = (float)(rand() % (int)disp.x);
                float y = (float)(rand() % (int)disp.y);
                dl->AddRectFilled({x, y}, {x + 3, y + 2}, IM_COL32(rand()%255, rand()%255, rand()%255, 180));
            }
        }

        // occasional white flash
        if (g_level >= 5 && (rand() % 120 == 0))
            dl->AddRectFilled({0, 0}, {disp.x, disp.y}, IM_COL32(255, 255, 255, 40));

        // horizontal tear bands
        if (g_level >= 6) {
            int tears = (g_level - 5) * 2;
            for (int i = 0; i < tears; i++) {
                float y = (float)(rand() % (int)disp.y);
                float h = (float)(4 + rand() % 8);
                dl->AddRectFilled({0, y}, {disp.x, y + h}, IM_COL32(0, rand()%60, 0, 120));
            }
        }

        // large colored displacement blocks
        if (g_level >= 8) {
            int blocks = g_level - 6;
            for (int i = 0; i < blocks; i++) {
                float y = (float)(rand() % (int)disp.y);
                float h = (float)(10 + rand() % 30);
                float offset = (float)((rand() % 40) - 20);
                dl->AddRectFilled({offset, y}, {disp.x + offset, y + h}, IM_COL32(rand()%80, rand()%255, rand()%80, 80));
            }
        }

        // heavy flash at level 10
        if (g_level >= 10 && (rand() % 40 == 0))
            dl->AddRectFilled({0, 0}, {disp.x, disp.y}, IM_COL32(0, 255, 0, 25));
    }
    #else
    void draw_screen_fx() {}
    #endif
}