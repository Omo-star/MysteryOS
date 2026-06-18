#pragma once

#include <string>
#include <vector>

enum class ScareKind {
    BlackFlash,
    GreenAfterimage,
    WindowShake,
    FakeError,
    FullscreenMessage,
    ApertureOpen,
    HardJumpscare
};

enum class ScareSound {
    Dread,
    Impact,
    Aperture
};

struct ActiveScare {
    ScareKind kind;
    float start_time = 0.0f;
    float duration = 0.0f;
    float intensity = 1.0f;
    std::string text;
};

struct ScheduledWhisper {
    float due_time = 0.0f;
    std::string text;
};

class ScareDirector {
    public:
        void on_file_open(const std::string& path, int stage, bool corrupted, int files_opened, float now);
        void on_terminal_search(const std::string& command, const std::string& query, int stage, float now);
        void on_stage_unlock(int stage, float now);
        void update(float now);
        std::vector<std::string> drain_terminal_messages(float now);
        std::vector<ScareSound> drain_sound_requests();
        bool has_active(ScareKind kind) const;
        const std::vector<ActiveScare>& active_scares() const { return active_; }
        void render(float now);

    private:
        std::vector<ActiveScare> active_;
        std::vector<ScheduledWhisper> whispers_;
        std::vector<ScareSound> sound_requests_;
        bool saw_corrupted_file_ = false;
        bool saw_e10a_image_ = false;
        bool saw_users_ = false;
        bool saw_stage5_player_folder_ = false;
        bool saw_stage5_door_file_ = false;
        bool stage4_unlock_hit_ = false;
        bool stage5_unlock_hit_ = false;
        bool whispered_deleted_file_ = false;
        bool whispered_users_file_ = false;
        bool whispered_player_folder_ = false;
        bool whispered_model_file_ = false;
        bool search_september_hit_ = false;
        bool search_gerald_hit_ = false;
        bool search_identity_hit_ = false;
        bool timeline_reconstruction_hit_ = false;

        void add(ScareKind kind, float now, float duration, float intensity, const std::string& text = "");
        void schedule_whisper(float now, float delay, const std::string& text);
        void request_sound(ScareSound sound);
};
