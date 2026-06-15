# Terminal Investigation Tools Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add `grep`, `find`, and stage-gated `timeline` commands so late-game MysteryOS puzzles can require real terminal investigation.

**Architecture:** Put recursive VFS search behavior in a small `src/apps/terminal_tools.*` helper module, then call it from `Terminal::execute`. The helpers respect hidden paths, locked content, and result limits so the tools help players investigate without dumping the whole mystery.

**Tech Stack:** C++17, existing MysteryOS VFS, native `test_logic` target.

---

### Task 1: Add Tested Search Helpers

**Files:**
- Create: `src/apps/terminal_tools.h`
- Create: `src/apps/terminal_tools.cpp`
- Modify: `tests/test_main.cpp`
- Modify: `CMakeLists.txt`

- [ ] Add tests for recursive filename search, content search, hidden path exclusion, and month/date timeline scanning.
- [ ] Run the native test target and confirm the new tests fail because the helper module does not exist yet.
- [ ] Implement helper functions returning terminal-ready output lines.
- [ ] Add `src/apps/terminal_tools.cpp` to the native test target.
- [ ] Run the native test target and confirm all tests pass.

### Task 2: Wire Commands Into Terminal

**Files:**
- Modify: `src/apps/terminal.cpp`
- Modify: `CMakeLists.txt`
- Modify: `README.md`

- [ ] Include `terminal_tools.h` in `terminal.cpp`.
- [ ] Add `grep <term> [path]`, `find <term> [path]`, and `timeline [path]` command branches.
- [ ] Gate `timeline` behind stage 4.
- [ ] Update help text and README command list.
- [ ] Compile-check `terminal.cpp` and run native tests.
