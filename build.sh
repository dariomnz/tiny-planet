#!/bin/bash
set -e

cd "$(dirname "$0")"

if ! command -v emcmake >/dev/null 2>&1; then
  echo "ERROR: emcmake not found. Install the Emscripten SDK and activate the environment:"
  echo "  source ~/emsdk/emsdk_env.sh"
  exit 1
fi

echo "==> Configuring (Emscripten)..."
emcmake cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

echo "==> Building..."
cmake --build build

echo ""
echo "Done! Serve with:"
echo "  python3 -m http.server -d build 8000"
echo "and open: http://localhost:8000/tiny-planet.html"
