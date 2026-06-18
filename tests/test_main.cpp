#include "kernel/puzzle.h"
#include "kernel/vfs.h"
#include "kernel/player_profile.h"
#include "apps/terminal_tools.h"
#include "fx/glitch.h"
#include "fx/scare_director.h"
#include <algorithm>
#include <cstdlib>
#include <cstdio>
#include <string>
#include <vector>

static int expect(bool condition, const char* message) {
    if (!condition) {
        std::printf("FAIL: %s\n", message);
        return 1;
    }
    return 0;
}

static bool has_line_containing(const std::vector<std::string>& lines, const std::string& text) {
    return std::any_of(lines.begin(), lines.end(), [&](const std::string& line) {
        return line.find(text) != std::string::npos;
    });
}

static bool has_sound(const std::vector<ScareSound>& sounds, ScareSound sound) {
    return std::any_of(sounds.begin(), sounds.end(), [&](ScareSound value) {
        return value == sound;
    });
}

int main() {
    VFS vfs;
    PuzzleState puzzle;

    if (expect(vfs.load("data/filesystem.json"), "filesystem should load")) return 1;
    if (expect(puzzle.load("data/puzzles.json"), "puzzles should load")) return 1;

    if (expect(!puzzle.try_password("overwritten", vfs), "stage 2 password should not unlock from stage 0")) return 1;
    if (expect(puzzle.stage() == 0, "failed skipped password should leave stage at 0")) return 1;

    if (expect(puzzle.try_password("mirror", vfs), "stage 1 password should unlock from stage 0")) return 1;
    if (expect(puzzle.stage() == 1, "stage 1 password should advance to stage 1")) return 1;

    if (expect(!puzzle.try_password("elena", vfs), "stage 3 password should not unlock from stage 1")) return 1;
    if (expect(puzzle.stage() == 1, "failed skipped password should leave stage at 1")) return 1;

    if (expect(puzzle.try_password("overwritten", vfs), "stage 2 password should unlock from stage 1")) return 1;
    if (expect(puzzle.stage() == 2, "stage 2 password should advance to stage 2")) return 1;
    if (expect(vfs.get("/System/tools/session_monitor.txt") != nullptr, "stage 2 should inject session monitor instructions")) return 1;

    vfs.inject("/Desktop/search_alpha.txt", "First line\nNeedle appears here\nSeptember 1 happened", false);
    vfs.inject("/Desktop/no_date.txt", "The anomaly may create files", false);
    vfs.inject("/System/logs/needle_log.txt", "needle from logs", false);

    auto grep_lines = TerminalTools::grep(vfs, "needle", "/");
    if (expect(has_line_containing(grep_lines, "/Desktop/search_alpha.txt: Needle appears here"), "grep should find matching file content")) return 1;
    if (expect(has_line_containing(grep_lines, "/System/logs/needle_log.txt: needle from logs"), "grep should search recursively through visible directories")) return 1;

    auto find_lines = TerminalTools::find(vfs, "needle", "/");
    if (expect(has_line_containing(find_lines, "/System/logs/needle_log.txt"), "find should match filenames recursively")) return 1;

    auto hidden_find_lines = TerminalTools::find(vfs, "mkato", "/");
    if (expect(!has_line_containing(hidden_find_lines, "/Users/mkato"), "find should not reveal hidden paths before unlock")) return 1;

    auto timeline_lines = TerminalTools::timeline(vfs, "/Desktop");
    if (expect(has_line_containing(timeline_lines, "/Desktop/search_alpha.txt: September 1 happened"), "timeline should surface month/date evidence")) return 1;
    if (expect(!has_line_containing(timeline_lines, "/Desktop/no_date.txt"), "timeline should not treat modal may as a date")) return 1;

    vfs.inject("/Desktop/diff_a.txt", "same\nleft only\nshared\n", false);
    vfs.inject("/Desktop/diff_b.txt", "same\nright only\nshared\n", false);
    auto diff_lines = TerminalTools::diff(vfs, "/Desktop/diff_a.txt", "/Desktop/diff_b.txt");
    if (expect(has_line_containing(diff_lines, "- left only"), "diff should show removed lines")) return 1;
    if (expect(has_line_containing(diff_lines, "+ right only"), "diff should show added lines")) return 1;

    auto stat_lines = TerminalTools::stat(vfs, "/Desktop/diff_a.txt");
    if (expect(has_line_containing(stat_lines, "type: file"), "stat should report file type")) return 1;
    if (expect(has_line_containing(stat_lines, "bytes:"), "stat should report byte count")) return 1;

    vfs.inject("/Desktop/blob.bin", "abc\001\002VISIBLE_TEXT\003tail", false);
    auto strings_lines = TerminalTools::strings(vfs, "/Desktop/blob.bin");
    if (expect(has_line_containing(strings_lines, "VISIBLE_TEXT"), "strings should extract printable runs")) return 1;

    auto hash_lines = TerminalTools::hash(vfs, "/Desktop/diff_a.txt");
    if (expect(has_line_containing(hash_lines, "fnv1a64:"), "hash should report deterministic file hash")) return 1;

    VFSNode* live_node = vfs.get("/Desktop/diff_a.txt");
    live_node->content = "changed by test";
    if (expect(vfs.get("/Desktop/diff_a.txt")->content == "changed by test", "VFS content should be mutable for living files")) return 1;

    PlayerProfile profile;
    profile.record_command("cat /System/config.cfg");
    profile.record_file_open("/System/config.cfg", "terminal");
    profile.record_file_open("/System/config.cfg", "terminal");
    profile.record_file_open("/Users/mkato/.deleted/transfer_second_thoughts.txt", "gui");
    profile.record_search("grep", "september");
    auto observations = profile.observations(5);
    if (expect(has_line_containing(observations, "returned to /System/config.cfg"), "profile should notice rereads")) return 1;
    if (expect(has_line_containing(observations, "deleted files"), "profile should notice deleted-file focus")) return 1;
    if (expect(has_line_containing(observations, "commands more than windows"), "profile should notice terminal preference")) return 1;
    if (expect(profile.live_profile_content(5).find("september") != std::string::npos, "live profile should include searched terms")) return 1;

    srand(12345);
    int expected_next_rand = rand();
    srand(12345);
    Glitch::set_level(5);
    (void)Glitch::mangle("corrupted render should not reseed the OS glitch clock");
    if (expect(rand() == expected_next_rand, "mangle should not disturb global rand state")) return 1;

    Glitch::TextCorruptor corruptor(0.25f);
    Glitch::set_level(10);
    std::string corruption_source(400, 'A');
    std::string first_corruption = corruptor.render(corruption_source, 1.0f);
    std::string same_frame_corruption = corruptor.render(corruption_source, 1.1f);
    if (expect(first_corruption == same_frame_corruption, "corrupted text should be cached between refresh ticks")) return 1;
    std::string refreshed_corruption = corruptor.render(corruption_source, 1.4f);
    if (expect(refreshed_corruption != first_corruption, "corrupted text should refresh after its own timer")) return 1;

    ScareDirector scares;
    scares.on_file_open("/System/config.cfg", 2, true, 8, 10.0f);
    if (expect(scares.has_active(ScareKind::BlackFlash), "corrupted file should trigger a black flash")) return 1;
    scares.update(10.4f);
    if (expect(!scares.has_active(ScareKind::BlackFlash), "black flash should expire quickly")) return 1;

    scares.on_file_open("/Pictures/0xE10A.png", 3, true, 12, 20.0f);
    if (expect(scares.has_active(ScareKind::GreenAfterimage), "0xE10A image should trigger green afterimage")) return 1;
    if (expect(scares.has_active(ScareKind::FullscreenMessage), "0xE10A image should trigger fullscreen message")) return 1;
    if (expect(scares.has_active(ScareKind::HardJumpscare), "0xE10A image should trigger a hard jumpscare")) return 1;
    auto e10a_sounds = scares.drain_sound_requests();
    if (expect(has_sound(e10a_sounds, ScareSound::Impact), "0xE10A image should request impact sound")) return 1;

    ScareDirector users_scares;
    users_scares.on_file_open("/Users/mkato/Documents/first_week_notes.txt", 4, false, 30, 30.0f);
    if (expect(users_scares.has_active(ScareKind::FakeError), "first /Users file should trigger fake error")) return 1;
    users_scares.update(40.0f);
    users_scares.on_file_open("/Users/cshin/Documents/final_day.txt", 4, false, 31, 40.0f);
    if (expect(!users_scares.has_active(ScareKind::FakeError), "/Users fake error should only happen once")) return 1;

    ScareDirector deleted_whisper_scares;
    deleted_whisper_scares.on_file_open("/Users/mkato/.deleted/transfer_second_thoughts.txt", 4, false, 42, 100.0f);
    auto deleted_sounds = deleted_whisper_scares.drain_sound_requests();
    if (expect(has_sound(deleted_sounds, ScareSound::Dread), "deleted file should request dread sound")) return 1;
    auto early_whispers = deleted_whisper_scares.drain_terminal_messages(105.0f);
    if (expect(early_whispers.empty(), "deleted-file whisper should wait before speaking")) return 1;
    auto late_whispers = deleted_whisper_scares.drain_terminal_messages(114.0f);
    if (expect(has_line_containing(late_whispers, "deleted does not mean gone"), "deleted file should schedule a delayed whisper")) return 1;

    ScareDirector search_scares;
    search_scares.on_terminal_search("grep", "september", 4, 200.0f);
    if (expect(search_scares.has_active(ScareKind::FakeError), "september search should trigger fake profiling error")) return 1;
    auto september_sounds = search_scares.drain_sound_requests();
    if (expect(has_sound(september_sounds, ScareSound::Dread), "september search should request dread sound")) return 1;
    auto search_whispers = search_scares.drain_terminal_messages(209.0f);
    if (expect(has_line_containing(search_whispers, "first month"), "september search should schedule terminal whisper")) return 1;

    ScareDirector timeline_scares;
    timeline_scares.on_terminal_search("timeline", "/Users", 4, 300.0f);
    if (expect(timeline_scares.has_active(ScareKind::FakeError), "timeline use should trigger reconstruction warning")) return 1;
    if (expect(timeline_scares.has_active(ScareKind::ApertureOpen), "timeline /Users should open an aperture scare")) return 1;
    auto timeline_sounds = timeline_scares.drain_sound_requests();
    if (expect(has_sound(timeline_sounds, ScareSound::Aperture), "timeline /Users should request aperture sound")) return 1;

    ScareDirector model_scares;
    model_scares.on_file_open("/System/models/README.txt", 5, false, 101, 400.0f);
    if (expect(model_scares.has_active(ScareKind::ApertureOpen), "first model file should open an aperture scare")) return 1;
    auto model_sounds = model_scares.drain_sound_requests();
    if (expect(has_sound(model_sounds, ScareSound::Aperture), "first model file should request aperture sound")) return 1;

    ScareDirector player_folder_scares;
    player_folder_scares.on_file_open("/Desktop/you/a_door_you_did_not_open.txt", 5, false, 150, 500.0f);
    if (expect(player_folder_scares.has_active(ScareKind::HardJumpscare), "door file should trigger hard jumpscare")) return 1;
    auto door_sounds = player_folder_scares.drain_sound_requests();
    if (expect(has_sound(door_sounds, ScareSound::Impact), "door file should request impact sound")) return 1;

    ScareDirector do_not_open_scares;
    do_not_open_scares.on_file_open("/Desktop/you/do_not_open_until_you_are_done.txt", 5, false, 151, 600.0f);
    if (expect(do_not_open_scares.has_active(ScareKind::HardJumpscare), "do-not-open file should trigger a setpiece scare")) return 1;

    std::printf("All tests passed.\n");
    return 0;
}
