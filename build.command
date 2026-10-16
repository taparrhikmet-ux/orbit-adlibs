#!/bin/bash
set -euo pipefail
cd "$(dirname "$0")"
trap 's=$?; if [ "$s" -ne 0 ]; then echo; echo "Build failed. Read the error above. See README.md for prerequisites."; fi; read -r -p "Press Return to close..." _; exit "$s"' EXIT
if [ "$(uname -s)" != "Darwin" ]; then echo "Run this script on macOS."; exit 1; fi
if ! xcrun --find clang++ >/dev/null 2>&1; then
  echo "Apple Command Line Tools are required. Run: xcode-select --install"; exit 1
fi
if ! command -v cmake >/dev/null 2>&1; then
  echo "CMake 3.22 or newer is required. Install CMake, then rerun this script."
  echo "If installed as an app, add /Applications/CMake.app/Contents/bin to PATH."
  echo "If you already use Homebrew, you can run: brew install cmake"
  exit 1
fi
ARCHS="${ORBIT_ARCHS:-$(uname -m)}"
echo "Building Orbit Adlibs for $ARCHS; minimum macOS 12.0."
echo "First build downloads JUCE 7.0.12 from its official GitHub repository."
cmake -S . -B build -G "Unix Makefiles" \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_OSX_DEPLOYMENT_TARGET=12.0 \
  "-DCMAKE_OSX_ARCHITECTURES=$ARCHS"
cmake --build build --config Release --target OrbitAdlibs_VST3 OrbitAdlibs_AU --parallel 2
mkdir -p dist
for spec in "VST3:Orbit Adlibs.vst3" "AU:Orbit Adlibs.component"; do
  kind="${spec%%:*}"
  name="${spec#*:}"
  /usr/bin/ditto "build/OrbitAdlibs_artefacts/Release/$kind/$name" "dist/$name"
  /usr/bin/codesign --force --deep --sign - "dist/$name"
done
echo
echo "Build completed. VST3 and Audio Unit are in the dist folder."
echo "Close your DAW, then run install.command to install for your macOS user."
open dist
