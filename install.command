#!/bin/bash
set -euo pipefail
cd "$(dirname "$0")"
trap 's=$?; read -r -p "Press Return to close..." _; exit "$s"' EXIT
for name in "Orbit Adlibs.vst3" "Orbit Adlibs.component"; do
  if [ ! -d "dist/$name" ]; then echo "Missing $name. Run build.command first."; exit 1; fi
done
echo "Close your DAW before continuing."
echo "This installs Orbit Adlibs into your user VST3 and Components folders."
echo "Existing Orbit Adlibs copies are backed up before replacement."
read -r -p "Install now? [y/N] " reply
case "$reply" in y|Y|yes|YES) ;; *) echo "Cancelled."; exit 0;; esac
stamp="$(date +%Y%m%d-%H%M%S)"
for spec in "VST3:Orbit Adlibs.vst3" "Components:Orbit Adlibs.component"; do
  kind="${spec%%:*}"; name="${spec#*:}"
  target="$HOME/Library/Audio/Plug-Ins/$kind"
  mkdir -p "$target"
  if [ -e "$target/$name" ]; then
    mkdir -p "$HOME/Library/Application Support/Orbit Adlibs/Backups/$stamp"
    mv "$target/$name" "$HOME/Library/Application Support/Orbit Adlibs/Backups/$stamp/$name"
  fi
  /usr/bin/ditto "dist/$name" "$target/$name"
done
echo "Installed. Open your DAW and rescan plug-ins."
echo "For the Audio Unit, you can run validate-au.command before opening your project."
