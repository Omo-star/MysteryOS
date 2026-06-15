#include "kernel/puzzle.h"
#include "kernel/vfs.h"
#include "apps/terminal_tools.h"
#include <algorithm>
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

    std::printf("All tests passed.\n");
    return 0;
}
