set -e
mkdir -p build docs/data
cp data/*.json docs/data/ 2>/dev/null || true
cp data/*.mp4 data/*.gif data/*.png data/*.obj data/*.mtl data/bark_tree*.jpg data/leaves_*.jpg data/ground.jpg docs/data/ 2>/dev/null || true
emcmake cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j4
cp build/mysteryos.js build/mysteryos.wasm docs/
cp build/mysteryos.data docs/ 2>/dev/null || true
cp build/mysteryos.html docs/index.html
echo "Done. Run: python3 -m http.server 8080 --directory docs"
