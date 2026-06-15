# MysteryOS

![MysteryOS desktop showing the fake OS interface](docs/screenshot.png)

[![Play MysteryOS](https://img.shields.io/badge/play-GitHub%20Pages-111111?style=for-the-badge&logo=github)](https://overcharged-coder.github.io/MysteryOS/)
![C++17](https://img.shields.io/badge/C%2B%2B-17-00599C?style=for-the-badge&logo=cplusplus)
![WebAssembly](https://img.shields.io/badge/WebAssembly-654FF0?style=for-the-badge&logo=webassembly&logoColor=white)
![Emscripten](https://img.shields.io/badge/Emscripten-222222?style=for-the-badge)
![Dear ImGui](https://img.shields.io/badge/Dear%20ImGui-0f172a?style=for-the-badge)
![Groq Optional](https://img.shields.io/badge/Groq-optional%20AI%20anomaly-f55036?style=for-the-badge)

MysteryOS is a browser-playable fake operating system mystery built with C++, Dear ImGui, SDL2, OpenGL ES, and Emscripten.

You are dropped into Dr. Elena Voss's workstation at Meridian Analytics. The computer looks ordinary at first: files, folders, logs, images, a terminal, and a few programs that barely cooperate. But useful parts of the system are locked behind passwords, and each solved stage injects new evidence into the virtual filesystem.

The goal is not just to solve puzzles. It is to understand what happened on this machine, why Elena left it running, and why the process list contains something called PID 7741.

## Play

Play the current GitHub Pages build here:

https://overcharged-coder.github.io/MysteryOS/

If the AI anomaly is not configured, the game still works normally. The optional AI feature only affects `talk 7741 <message>`.

## What It Is

MysteryOS is:

- a narrative puzzle game
- a fake desktop environment
- a terminal mystery
- an explorable filesystem full of story evidence
- a haunted-machine experiment
- optionally, an AI-backed anomaly that can answer and write files inside the fake OS

The main interaction is reading. You open documents, inspect logs, follow hints, try terminal commands, unlock hidden stages, and slowly assemble the history of Project MIRROR.

## Features

- Fake OS desktop built in Dear ImGui
- File Explorer with locked folders and corrupted files
- Text editor for readable documents
- Image viewer for real JPG/PNG files
- Procedural corrupted image rendering
- Terminal commands like `ls`, `cat`, `ps`, `ping`, `kill`, `decode`, `monitor`, `trace`, and `talk`
- Five staged unlocks driven by passwords
- Stage-based file injection
- Session Monitor app for investigating PID 7741
- Activity tracking for files opened during the current session
- Optional Groq-powered `talk 7741` anomaly
- Optional AI-generated files written into the fake filesystem

## Commands

Open the terminal in-game and try:

```text
help
ls /
cat /Desktop/README.txt
unlock <password>
ps
kill 7741
ping 10.0.0.47
decode <file>
monitor
trace 7741
talk 7741 <message>
```

Some commands become more useful as the story progresses.

## The Anomaly

PID 7741 begins as a strange process in the boot log. It becomes more visible as you unlock the archive, open files, and investigate what happened to the researchers before Elena.

The Session Monitor shows PID 7741 as something that is not quite malware:

- it cannot be killed
- its memory is unbounded
- its CPU usage rises
- it is bound to the active session
- it reacts to observation

If the optional AI layer is enabled, `talk 7741 <message>` sends only in-game state to Groq:

- current stage
- session time
- number of files opened
- recent fake file paths
- the message typed by the player

It does **not** send real files, real identity, location, camera, browser history, or anything outside the game.

The AI can reply in the terminal and may create a file in one of these fake OS locations:

```text
/Desktop/
/System/logs/
```

The path is validated in browser JavaScript and again in C++ before the virtual filesystem accepts it.

## Building Locally

Requires:

- Emscripten SDK
- CMake 3.16+
- Git Bash or WSL recommended on Windows

Build:

```bash
source ~/emsdk/emsdk_env.sh
./build.sh
python3 -m http.server 8080 --directory docs
```

Open:

```text
http://localhost:8080
```

The build output lives in `docs/` because GitHub Pages serves the game from there.

## Optional AI Anomaly Setup

The game can run without AI. If no key is configured, `talk 7741` reports no carrier and the rest of MysteryOS still works.

For GitHub Pages:

1. Set Pages source to **GitHub Actions**.
2. Add this repository secret:

```text
GROQ_API_KEY
```

3. Optionally add this Actions variable:

```text
GROQ_MODEL
```

The workflow writes `docs/anomaly-config.js` during deployment.

For local testing, create `docs/anomaly-config.js` yourself:

```js
window.MYSTERYOS_ANOMALY = {
    enabled: true,
    groqApiKey: "YOUR_GROQ_KEY",
    model: "llama-3.1-8b-instant"
};
```

This direct-browser Groq setup exposes the key to players. It is intentionally simple and experimental, not production-secret-safe.

## Native Logic Tests

The non-Emscripten CMake target builds a small logic test executable:

```bash
cmake -S . -B build_native
cmake --build build_native
./build_native/Debug/test_logic.exe
```

The tests cover core puzzle progression and staged filesystem injection.

## Project Structure

```text
src/
  apps/          ImGui apps: terminal, file explorer, image viewer, monitor
  kernel/        fake OS kernel, VFS, puzzle state
  fx/            glitch and corruption effects
data/
  filesystem.json    base virtual filesystem
  puzzles.json       staged unlock content
  images/            embedded image assets
docs/
  GitHub Pages output and shell template
tests/
  native logic tests
third_party/
  Dear ImGui, nlohmann/json, stb_image
```

## Spoiler Warning

Do not read these files unless you are debugging or completely stuck:

```text
data/puzzles.json
data/filesystem.json
docs/data/puzzles.json
docs/data/filesystem.json
```

They contain the structure of the mystery, passwords, stage injections, and major story reveals.

## Status

MysteryOS is heading toward its first public build.

The current focus is making the machine feel deeper, stranger, and more reactive without breaking the core puzzle path.
