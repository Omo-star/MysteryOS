#include "kernel/puzzle.h"
#include "kernel/vfs.h"
#include <cstdio>

static int expect(bool condition, const char* message) {
    if (!condition) {
        std::printf("FAIL: %s\n", message);
        return 1;
    }
    return 0;
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

    std::printf("All tests passed.\n");
    return 0;
}
