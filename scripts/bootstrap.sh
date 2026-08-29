#!/usr/bin/env bash
set -euo pipefail

log() {
    printf '[AI3 bootstrap] %s\n' "$*"
}

if [[ -n "${PREFIX:-}" && "${PREFIX}" == *"com.termux"* ]]; then
    log "Detected Termux."
    log "Enabling the Termux X11 package repository if necessary."
    pkg install -y x11-repo

    packages=(
        clang
        cmake
        ninja
        git
        pkg-config
        mesa
        mesa-dev
        libx11
        libxext
        libxrandr
        libxcursor
        libxi
    )

    log "Installing/updating AI3 prerequisites: ${packages[*]}"
    pkg install -y "${packages[@]}"

    log "Termux prerequisites are ready."
    exit 0
fi

if command -v apt-get >/dev/null 2>&1; then
    if [[ "$(id -u)" -eq 0 ]]; then
        SUDO=()
    elif command -v sudo >/dev/null 2>&1; then
        SUDO=(sudo)
    else
        printf 'AI3 bootstrap requires root privileges or sudo for apt-get.\n' >&2
        exit 1
    fi

    packages=(
        build-essential
        cmake
        ninja-build
        git
        pkg-config
        libegl1-mesa-dev
        libgles2-mesa-dev
        xorg-dev
    )

    log "Detected Debian/Ubuntu-style Linux."
    log "Refreshing package metadata."
    "${SUDO[@]}" apt-get update
    log "Installing/updating AI3 prerequisites: ${packages[*]}"
    "${SUDO[@]}" apt-get install -y "${packages[@]}"

    log "Linux prerequisites are ready."
    exit 0
fi

printf 'Unsupported environment. AI3 currently bootstraps Termux and Debian/Ubuntu-style Linux.\n' >&2
exit 1
