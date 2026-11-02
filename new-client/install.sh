#!/usr/bin/env bash
set -euo pipefail

# Install the build tools and Qt modules used by CMakeLists.txt.
if [[ ! -r /etc/os-release ]]; then
    printf 'Cannot identify this Linux distribution (/etc/os-release is missing).\n' >&2
    exit 1
fi

# shellcheck source=/dev/null
. /etc/os-release

if [[ ${EUID} -eq 0 ]]; then
    elevate=()
elif command -v sudo >/dev/null 2>&1; then
    elevate=(sudo)
else
    printf 'Run this script as root or install sudo first.\n' >&2
    exit 1
fi

case " ${ID:-} ${ID_LIKE:-} " in
    *" fedora "*|*" rhel "*)
        "${elevate[@]}" dnf install \
            cmake gcc-c++ make \
            qt6-qtbase-devel qt6-qttools-devel qt6-qtwebsockets-devel qt6-qtwayland
        ;;
    *" debian "*|*" ubuntu "*)
        "${elevate[@]}" apt-get update
        "${elevate[@]}" apt-get install \
            build-essential cmake \
            qt6-base-dev qt6-tools-dev qt6-websockets-dev qt6-wayland
        ;;
    *" arch "*)
        "${elevate[@]}" pacman -S --needed \
            base-devel cmake qt6-base qt6-tools qt6-websockets qt6-wayland
        ;;
    *)
        printf 'Unsupported distribution: %s. Install Qt 6 Widgets, Network, UiTools, WebSockets, CMake and a C++17 compiler.\n' "${PRETTY_NAME:-unknown}" >&2
        exit 1
        ;;
esac

qt_version=$(qmake6 -query QT_VERSION)
IFS=. read -r qt_major qt_minor _ <<< "$qt_version"
if (( qt_major < 6 || (qt_major == 6 && qt_minor < 5) )); then
    printf 'Qt %s is installed, but the WebSocket client requires Qt 6.5 or newer.\n' "$qt_version" >&2
    exit 1
fi

printf '\nDependencies installed. Build from the project root:\n'
printf '  cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug\n'
printf '  cmake --build build\n'
printf '  ./build/abyss-client\n'
