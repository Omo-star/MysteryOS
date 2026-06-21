#include "scare_director.h"
#include <algorithm>
#include <cctype>
#include <cmath>
#include <utility>

#ifndef MYSTERYOS_TEST
#include "imgui.h"
#include <emscripten.h>
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

void ScareDirector::request_sound(ScareSound sound) {
    sound_requests_.push_back(sound);
}

void ScareDirector::on_file_open(const string& path, int stage, bool corrupted, int files_opened, float now) {
    if (corrupted && !saw_corrupted_file_) {
        saw_corrupted_file_ = true;
        add(ScareKind::BlackFlash, now, 0.18f, 0.75f);
        request_sound(ScareSound::Dread);
    }

    if (stage >= 2 && corrupted && !depth_warp_triggered_) {
        depth_warp_triggered_ = true;
        add(ScareKind::DepthWarp, now + 0.5f, 3.0f, 1.0f);
    }

    if (path == "/Pictures/0xE10A.png" && !saw_e10a_image_) {
        saw_e10a_image_ = true;
        add(ScareKind::GreenAfterimage, now, 1.1f, 1.0f);
        add(ScareKind::FullscreenMessage, now, 0.9f, 1.0f, "YOU OPENED IT");
        add(ScareKind::TheEye, now + 0.5f, 4.0f, 1.0f);
        add(ScareKind::HardJumpscare, now + 4.5f, 2.0f, 1.0f, "NO INPUT");
        request_sound(ScareSound::Impact);
    }

    if (stage >= 4 && starts_with(path, "/Users/") && !scene_feed_triggered_) {
        scene_feed_triggered_ = true;
        add(ScareKind::SceneFeed, now, 20.0f, 1.0f);
    }

    if (stage >= 4 && starts_with(path, "/Users/") && !saw_users_) {
        saw_users_ = true;
        add(ScareKind::FakeError, now + 20.5f, 2.4f, 1.0f, "CROSS-CONTAMINATION ACCELERATING");
        add(ScareKind::WindowShake, now + 20.5f, 1.0f, 0.8f);
        request_sound(ScareSound::Dread);
    }

    if (stage >= 3 && starts_with(path, "/Users/") && !hallway_triggered_) {
        hallway_triggered_ = true;
        add(ScareKind::Hallway, now, 4.5f, 1.0f);
        request_sound(ScareSound::Dread);
    }

    if (stage >= 4 && starts_with(path, "/Users/") && !whispered_users_file_) {
        whispered_users_file_ = true;
        schedule_whisper(now, 7.5f, "7741: you opened " + path + " like it was evidence");
    }

    if (stage >= 4 && contains(path, "/.deleted/") && !whispered_deleted_file_) {
        whispered_deleted_file_ = true;
        request_sound(ScareSound::Dread);
        schedule_whisper(now, 11.0f, "7741: deleted does not mean gone");
    }

    if (stage >= 4 && starts_with(path, "/System/models/") && !whispered_model_file_) {
        whispered_model_file_ = true;
        add(ScareKind::ApertureOpen, now, 1.7f, 1.0f, "MODEL ACCESS");
        if (!eye_triggered_) {
            eye_triggered_ = true;
            add(ScareKind::TheEye, now + 1.8f, 4.0f, 1.0f);
        }
        request_sound(ScareSound::Aperture);
        schedule_whisper(now, 8.5f, "7741: models are how i remember");
    }

    if (stage >= 5 && starts_with(path, "/Desktop/you/") && !saw_stage5_player_folder_) {
        saw_stage5_player_folder_ = true;
        add(ScareKind::BlackFlash, now, 0.28f, 1.0f);
        add(ScareKind::FullscreenMessage, now + 0.05f, 1.4f, 1.0f, "THIS ONE IS YOURS");
    }

    if (stage >= 5 && path == "/Desktop/you/a_door_you_did_not_open.txt" && !saw_stage5_door_file_) {
        saw_stage5_door_file_ = true;
        add(ScareKind::ScreenMelt, now, 3.5f, 1.0f);
        add(ScareKind::HardJumpscare, now + 3.0f, 2.0f, 1.0f, "IT OPENED BACK");
        request_sound(ScareSound::Dread);
        request_sound(ScareSound::Impact);
    }

    if (stage >= 5 && path == "/Desktop/you/do_not_open_until_you_are_done.txt" && !saw_do_not_open_file_) {
        saw_do_not_open_file_ = true;
        add(ScareKind::BlackFlash, now, 0.12f, 1.0f);
        add(ScareKind::BunnyJumpscare, now + 0.12f, 5.5f, 1.0f);
        add(ScareKind::HardJumpscare, now + 5.5f, 1.8f, 0.9f, "THANK YOU FOR LOOKING");
        request_sound(ScareSound::Impact);
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
        request_sound(ScareSound::Dread);
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

    if (contains(haystack, "talk") && contains(haystack, "7741") && !scene_interview_triggered_) {
        scene_interview_triggered_ = true;
        add(ScareKind::SceneInterview, now + 2.0f, 25.0f, 1.0f);
    }

    if (stage >= 4 && command == "timeline" && !timeline_reconstruction_hit_) {
        timeline_reconstruction_hit_ = true;
        add(ScareKind::FakeError, now, 2.0f, 0.85f, "TIMELINE RECONSTRUCTION DETECTED");
        if (contains(haystack, "/users")) {
            add(ScareKind::ApertureOpen, now + 0.12f, 1.8f, 1.0f, "USER BRANCH OPEN");
            request_sound(ScareSound::Aperture);
        }
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
        add(ScareKind::HardJumpscare, now + 0.16f, 1.5f, 0.85f, "THE SESSION REMAINS OPEN");
        add(ScareKind::FullscreenMessage, now + 0.08f, 1.5f, 1.0f, "THE SESSION REMAINS OPEN");
        request_sound(ScareSound::Impact);
    }

    if (stage >= 5 && !scene_walk_triggered_) {
        scene_walk_triggered_ = true;
        add(ScareKind::SceneWalk, now + 3.0f, 30.0f, 1.0f);
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

vector<ScareSound> ScareDirector::drain_sound_requests() {
    vector<ScareSound> sounds = move(sound_requests_);
    sound_requests_.clear();
    return sounds;
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
        } else if (scare.kind == ScareKind::ApertureOpen) {
            dl->AddRectFilled({0, 0}, disp, IM_COL32(0, 0, 0, (int)(245.0f * scare.intensity)));
            float open = sinf(t * 3.1415926f);
            float slit = 8.0f + disp.x * 0.22f * open;
            float cx = disp.x * 0.5f + sinf(now * 28.0f) * 8.0f * fade;
            ImVec2 min = {cx - slit * 0.5f, 0.0f};
            ImVec2 max = {cx + slit * 0.5f, disp.y};
            dl->AddRectFilled(min, max, IM_COL32(8, 42, 24, (int)(235.0f * scare.intensity)));
            for (int i = 0; i < 42; i++) {
                float y = fmodf(now * 220.0f + i * 31.0f, disp.y);
                float jitter = sinf(now * 44.0f + i * 17.0f) * 18.0f * open;
                int a = (int)((80.0f + (i % 4) * 30.0f) * fade);
                dl->AddRectFilled({min.x + jitter, y}, {max.x + jitter, y + 2.0f + (i % 5)}, IM_COL32(75, 255, 125, a));
            }
            dl->AddLine({min.x, 0.0f}, {min.x, disp.y}, IM_COL32(170, 255, 190, (int)(255.0f * fade)), 2.0f);
            dl->AddLine({max.x, 0.0f}, {max.x, disp.y}, IM_COL32(170, 255, 190, (int)(255.0f * fade)), 2.0f);
            if (!scare.text.empty() && t > 0.35f && t < 0.72f) {
                ImVec2 size = ImGui::CalcTextSize(scare.text.c_str());
                dl->AddText({disp.x * 0.5f - size.x * 0.5f, disp.y * 0.78f}, IM_COL32(120, 255, 160, (int)(220.0f * fade)), scare.text.c_str());
            }
        } else if (scare.kind == ScareKind::HardJumpscare) {
            int base_alpha = (int)(255.0f * scare.intensity);
            int flash = age < 0.11f ? 255 : 0;
            int green = (int)(90.0f + 90.0f * fabsf(sinf(now * 70.0f)));
            dl->AddRectFilled({0, 0}, disp, IM_COL32(flash, flash, flash, base_alpha));
            dl->AddRectFilled({0, 0}, disp, IM_COL32(0, green, 30, (int)(170.0f * fade)));
            for (int i = 0; i < 76; i++) {
                float y = fmodf(now * 520.0f + i * 19.0f, disp.y);
                float h = 2.0f + (i % 7);
                float xoff = sinf(now * 95.0f + i * 3.0f) * 34.0f;
                int a = (int)((60.0f + (i % 5) * 26.0f) * fade);
                dl->AddRectFilled({xoff, y}, {disp.x + xoff, y + h}, IM_COL32(255, 255, 255, a));
            }
            for (int i = 0; i < 18; i++) {
                float x = fmodf(now * 430.0f + i * 83.0f, disp.x);
                float y = fmodf(now * 310.0f + i * 47.0f, disp.y);
                float s = 18.0f + (i % 6) * 9.0f;
                dl->AddRectFilled({x, y}, {x + s, y + s * 0.35f}, IM_COL32(0, 0, 0, (int)(210.0f * fade)));
            }
            if (!scare.text.empty() && fmodf(now * 18.0f, 1.0f) > 0.45f) {
                float font_size = ImGui::GetFontSize() * 3.8f;
                ImVec2 size = ImGui::CalcTextSize(scare.text.c_str());
                ImVec2 pos = {disp.x * 0.5f - size.x * 1.9f + sinf(now * 100.0f) * 12.0f, disp.y * 0.42f};
                dl->AddText(ImGui::GetFont(), font_size, pos, IM_COL32(0, 0, 0, (int)(255.0f * fade)), scare.text.c_str());
                dl->AddText(ImGui::GetFont(), font_size, {pos.x + 3.0f, pos.y - 2.0f}, IM_COL32(180, 255, 190, (int)(255.0f * fade)), scare.text.c_str());
            }
        } else if (scare.kind == ScareKind::BunnyJumpscare) {
            if (!bunny_launched_) {
                bunny_launched_ = true;
                emscripten_run_script("window._mysteryBunnyScare && window._mysteryBunnyScare()");
            }
            dl->AddRectFilled({0, 0}, disp, IM_COL32(0, 0, 0, (int)(220.0f * fade)));
            if (age > 2.0f) {
                int pulse = (int)(40.0f * fabsf(sinf(now * 35.0f)) * fade);
                dl->AddRectFilled({0, 0}, disp, IM_COL32(120, 0, 0, pulse));
            }
            for (int i = 0; i < 30; i++) {
                float y = fmodf(now * 400.0f + i * 27.0f, disp.y);
                float h = 1.0f + (i % 4);
                int a = (int)((40.0f + (i % 3) * 20.0f) * fade);
                dl->AddRectFilled({0, y}, {disp.x, y + h}, IM_COL32(255, 255, 255, a));
            }
        } else if (scare.kind == ScareKind::ScreenMelt) {
            if (!melt_launched_) {
                melt_launched_ = true;
                emscripten_run_script("window._mysteryScreenMelt && window._mysteryScreenMelt()");
            }
            int drip_alpha = (int)(60.0f * t * scare.intensity);
            for (int i = 0; i < 24; i++) {
                float x = fmodf(i * 57.0f + now * 3.0f, disp.x);
                float drop_h = disp.y * t * (0.4f + (i % 5) * 0.15f);
                float w = 8.0f + (i % 4) * 12.0f;
                dl->AddRectFilled({x, 0}, {x + w, drop_h}, IM_COL32(0, 0, 0, drip_alpha));
            }
            if (t > 0.6f) {
                int fog = (int)(180.0f * (t - 0.6f) * 2.5f * scare.intensity);
                dl->AddRectFilled({0, 0}, disp, IM_COL32(0, 0, 0, fog > 255 ? 255 : fog));
            }
        } else if (scare.kind == ScareKind::DepthWarp) {
            if (!depth_warp_launched_) {
                depth_warp_launched_ = true;
                emscripten_run_script("window._mysteryDepthWarp && window._mysteryDepthWarp()");
            }
        } else if (scare.kind == ScareKind::Hallway) {
            if (!hallway_launched_) {
                hallway_launched_ = true;
                emscripten_run_script("window._mysteryHallway && window._mysteryHallway()");
            }
            if (t > 0.7f) {
                int fog = (int)(80.0f * (1.0f - t) * scare.intensity);
                dl->AddRectFilled({0, 0}, disp, IM_COL32(0, 0, 0, fog));
            }
        } else if (scare.kind == ScareKind::TheEye) {
            if (!eye_launched_) {
                eye_launched_ = true;
                emscripten_run_script("window._mysteryTheEye && window._mysteryTheEye()");
            }
            if (age < 0.3f) {
                int flash = (int)(120.0f * (1.0f - age / 0.3f));
                dl->AddRectFilled({0, 0}, disp, IM_COL32(255, 255, 255, flash));
            }
        } else if (scare.kind == ScareKind::SceneFeed) {
            if (!scene_feed_launched_) {
                scene_feed_launched_ = true;
                emscripten_run_script("window._mysterySceneFeed && window._mysterySceneFeed()");
            }
            dl->AddRectFilled({0, 0}, disp, IM_COL32(0, 0, 0, 255));
        } else if (scare.kind == ScareKind::SceneInterview) {
            if (!scene_interview_launched_) {
                scene_interview_launched_ = true;
                emscripten_run_script("window._mysterySceneInterview && window._mysterySceneInterview()");
            }
            dl->AddRectFilled({0, 0}, disp, IM_COL32(0, 0, 0, 255));
        } else if (scare.kind == ScareKind::SceneWalk) {
            if (!scene_walk_launched_) {
                scene_walk_launched_ = true;
                emscripten_run_script("window._mysterySceneWalk && window._mysterySceneWalk()");
            }
            dl->AddRectFilled({0, 0}, disp, IM_COL32(0, 0, 0, 255));
        }
    }
}
#else
void ScareDirector::render(float now) {
    update(now);
}
#endif
