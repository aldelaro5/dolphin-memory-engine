#!/bin/sh
project_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." ; pwd -P)"
package="$project_root/Source/build/dolphin-memory-engine.app"

printf "Enter location of DME package (default=$package): "
read -r package_
if [ ! -z "$package_" ]; then package="$package_"; fi

cd "$package/.."
"$project_root/Externals/Qt-macOS/bin/macdeployqt" "$(basename "$package")" -dmg -codesign=-
