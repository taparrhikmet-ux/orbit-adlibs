#!/bin/bash
set -euo pipefail
cd "$(dirname "$0")"
export ORBIT_ARCHS="arm64;x86_64"
exec /bin/bash ./build.command
