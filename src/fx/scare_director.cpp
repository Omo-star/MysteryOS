#include "scare_director.h"
#include <algorithm>
#include <cctype>
#include <cmath>
#include <utility>

#ifndef MYSTERYOS_TEST
#include "imgui.h"
#endif

using namespace std;

static bool starts_with(const string& value, const string& prefix) {
    return value.rfind(prefix, 0) == 0;
}

static bool contains(const string& value, const string& needle) {
    return value.find(needle) != string::npos;
}

static string lower_copy(string value) {
    for (char& c : value) c = (char)tolower((unsigned char)c);
    return value;
}

void ScareDirector::add(ScareKind kind, float now, float duration, float intensity, const string& text) {
    active_.push_back({kind, now, duration, intensity, text});
}

void ScareDirector::schedule_whisper(float now, float delay, const string& text) {
    whispers_.push_back({now + delay, text});
}

void ScareDirector::on_file_open(const string& path, int stage, bool corrupted, int files_opened, float now) {
    if (corrupted && !saw_corrupted_file_) {
        saw_corrupted_file_ = true;
        add(ScareKind::BlackFlash, now, 0.18f, 0.75f);
    }

    if (path == "/Pictures/0xE10A.png" && !saw_e10a_image_) {
        saw_e10a_image_ = true;
        add(ScareKind::GreenAfterimage, now, 1.1f, 1.0f);
        add(ScareKind::FullscreenMessage, now, 0.9f, 1.0f, "YOU OPENED IT");
    }

    if (stage >= 4 && starts_with(path, "/Users/") && !saw_users_) {
        saw_users_ = true;
        add(ScareKind::FakeError, now, 2.4f, 1.0f, "CROSS-CONTAMINATION ACCELERATING");
        add(ScareKind::WindowShake, now, 1.0f, 0.8f);
    }

    if (stage >= 4 && starts_with(path, "/Users/") && !whispered_users_file_) {
        whispered_users_file_ = true;
        schedule_whisper(now, 7.5f, "7741: you opened " + path + " like it was evidence");
    }

    if (stage >= 4 && contains(path, "/.deleted/") && !whispered_deleted_file_) {
        whispered_deleted_file_ = true;
        schedule_whisper(now, 11.0f, "7741: deleted does not mean gone");
    }

    if (stage >= 4 && starts_with(path, "/System/models/") && !whispered_model_file_) {
        whispered_model_file_ = true;
        schedule_whisper(now, 8.5f, "7741: models are how i remember");
    }

    if (stage >= 5 && starts_with(path, "/Desktop/you/") && !saw_stage5_player_folder_) {
        saw_stage5_player_folder_ = true;
        add(ScareKind::BlackFlash, now, 0.28f, 1.0f);
        add(ScareKind::FullscreenMessage, now + 0.05f, 1.4f, 1.0f, "THIS ONE IS YOURS");
    }

    if (stage >= 5 && starts_with(path, "/Desktop/you/") && !whispered_player_folder_) {
        whispered_player_folder_ = true;
        schedule_whisper(now, 9.0f, "7741: this folder was not here before you needed it");
    }

    if (stage >= 3 && files_opened > 0 && files_opened % 40 == 0) {
        add(ScareKind::GreenAfterimage, now, 0.45f, 0.45f);
    }
}

void ScareDirector::on_terminal_search(const string& command, const string& query, int stage, float now) {
    if (stage < 3) return;

    string haystack = lower_copy(command + " " + query);
    if (stage >= 4 && contains(haystack, "september") && !search_september_hit_) {
        search_september_hit_ = true;
        add(ScareKind::FakeError, now, 2.1f, 0.9f, "SEARCH TERM PROFILED: september");
        schedule_whisper(now, 8.0f, "7741: you searched for the first month");
    }

    if (contains(haystack, "gerald") && !search_gerald_hit_) {
        search_gerald_hit_ = true;
        schedule_whisper(now, 6.0f, "7741: you remembered the harmless witness");
    }

    if ((contains(haystack, "7741") || contains(haystack, "0xe10a")) && !search_identity_hit_) {
        search_identity_hit_ = true;
        schedule_whisper(now, 7.0f, "7741: searching a name teaches it shape");
    }

    if (stage >= 4 && command == "timeline" && !timeline_reconstruction_hit_) {
        timeline_reconstruction_hit_ = true;
        add(ScareKind::FakeError, now, 2.0f, 0.85f, "TIMELINE RECONSTRUCTION DETECTED");
        schedule_whisper(now, 10.0f, "7741: time is easier to move after you sort it");
    }
}

void ScareDirector::on_stage_unlock(int stage, float now) {
    if (stage >= 4 && !stage4_unlock_hit_) {
        stage4_unlock_hit_ = true;
        add(ScareKind::FakeError, now, 2.0f, 0.9f, "UNAUTHORIZED USER BRANCH MOUNTED");
    }

    if (stage >= 5 && !stage5_unlock_hit_) {
        stage5_unlock_hit_ = true;
        add(ScareKind::BlackFlash, now, 0.35f, 1.0f);
        add(ScareKind::FullscreenMessage, now + 0.08f, 1.5f, 1.0f, "THE SESSION REMAINS OPEN");
    }
}

void ScareDirector::update(float now) {
    active_.erase(remove_if(active_.begin(), active_.end(), [now](const ActiveScare& scare) {
        return now >= scare.start_time + scare.duration;
    }), active_.end());
}

vector<string> ScareDirector::drain_terminal_messages(float now) {
    vector<string> due;
    vector<ScheduledWhisper> pending;
    for (auto& whisper : whispers_) {
        if (now >= whisper.due_time) {
            due.push_back(move(whisper.text));
        } else {
            pending.push_back(move(whisper));
        }
    }
    whispers_ = move(pending);
    return due;
}

bool ScareDirector::has_active(ScareKind kind) const {
    return any_of(active_.begin(), active_.end(), [kind](const ActiveScare& scare) {
        return scare.kind == kind;
    });
}

#ifndef MYSTERYOS_TEST
void ScareDirector::render(float now) {
    update(now);
    if (active_.empty()) return;

    ImDrawList* dl = ImGui::GetForegroundDrawList();
    ImVec2 disp = ImGui::GetIO().DisplaySize;

    for (const ActiveScare& scare : active_) {
        float age = now - scare.start_time;
        if (age < 0.0f || age > scare.duration) continue;
        float t = scare.duration <= 0.0f ? 1.0f : age / scare.duration;
        float fade = (1.0f - t) * scare.intensity;

        if (scare.kind == ScareKind::BlackFlash) {
            int alpha = (int)(255.0f * fade);
            dl->AddRectFilled({0, 0}, disp, IM_COL32(0, 0, 0, alpha));
        } else if (scare.kind == ScareKind::GreenAfterimage) {
            int alpha = (int)(120.0f * fade);
            dl->AddRectFilled({0, 0}, disp, IM_COL32(0, 255, 90, alpha));
            int bands = 18;
            for (int i = 0; i < bands; i++) {
                float y = fmodf((now * 180.0f) + i * 47.0f, disp.y);
                dl->AddRectFilled({0, y}, {disp.x, y + 2.0f + (i % 3)}, IM_COL32(0, 0, 0, (int)(90.0f * fade)));
            }
        } else if (scare.kind == ScareKind::FullscreenMessage) {
            dl->AddRectFilled({0, 0}, disp, IM_COL32(0, 0, 0, (int)(205.0f * fade)));
            float font_size = ImGui::GetFontSize() * 3.3f;
            ImVec2 size = ImGui::CalcTextSize(scare.text.c_str());
            ImVec2 pos = {disp.x * 0.5f - size.x * 1.65f, disp.y * 0.46f};
            dl->AddText(ImGui::GetFont(), font_size, pos, IM_COL32(80, 255, 120, (int)(255.0f * fade)), scare.text.c_str());
        } else if (scare.kind == ScareKind::FakeError) {
            ImVec2 min = {disp.x * 0.5f - 235.0f, disp.y * 0.5f - 58.0f};
            ImVec2 max = {disp.x * 0.5f + 235.0f, disp.y * 0.5f + 58.0f};
            dl->AddRectFilled(min, max, IM_COL32(8, 8, 8, (int)(235.0f * fade)));
            dl->AddRect(min, max, IM_COL32(255, 40, 40, (int)(255.0f * fade)), 0.0f, 0, 2.0f);
            dl->AddText({min.x + 18.0f, min.y + 16.0f}, IM_COL32(255, 80, 80, (int)(255.0f * fade)), "SYSTEM ERROR");
            dl->AddText({min.x + 18.0f, min.y + 48.0f}, IM_COL32(210, 255, 210, (int)(255.0f * fade)), scare.text.c_str());
        } else if (scare.kind == ScareKind::WindowShake) {
            int alpha = (int)(80.0f * fade);
            for (int i = 0; i < 9; i++) {
                float y = fmodf((now * 320.0f) + i * 61.0f, disp.y);
                float offset = sinf(now * 90.0f + i) * 16.0f * fade;
                dl->AddRectFilled({offset, y}, {disp.x + offset, y + 8.0f}, IM_COL32(0, 255, 70, alpha));
            }
        }
    }
}
#else
void ScareDirector::render(float now) {
    update(now);
}
#endif
