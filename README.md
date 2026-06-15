# MysteryOS
MysteryOS requires you to solve puzzles in order to use programs! Even the basic programs are annoying...

**Use:** [https://<your-username>.github.io/MysteryOS/](https://overcharged-coder.github.io/MysteryOS/)

## Building

Requires Emscripten SDK and CMake 3.16+.

Run in Git Bash or WSL:
```bash
source ~/emsdk/emsdk_env.sh
./build.sh
python3 -m http.server 8080 --directory docs
```
Open http://localhost:8080

## Optional AI anomaly

The `talk 7741 <message>` command can call Groq from the browser when `docs/anomaly-config.js` exists.

For GitHub Pages, set Pages source to **GitHub Actions**, then add this repository secret:
```text
GROQ_API_KEY
```

You can optionally add an Actions variable:
```text
GROQ_MODEL
```

If no key is configured, the game still works and `talk 7741` reports no carrier.
When configured, PID 7741 can reply and may write generated files under `/Desktop/` or `/System/logs/`.

## Puzzle spoilers
Don't open `data/puzzles.json` unless you are completely stuck.
