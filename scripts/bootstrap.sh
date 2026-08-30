#!/usr/bin/env bash
set -euo pipefail

log() {
    printf '[AI3 bootstrap] %s\n' "$*"
}

command_version() {
    "$1" --version 2>/dev/null | sed -n "1s/^$2 //p"
}

verify_clang_format() {
    local formatter=""
    local version=""
    local candidate

    for candidate in clang-format-21 clang-format; do
        if ! command -v "${candidate}" >/dev/null 2>&1; then
            continue
        fi
        version="$(${candidate} --version 2>/dev/null || true)"
        if [[ "${version}" =~ clang-format[[:space:]]+version[[:space:]]+21([.[:space:]]|$) ]]; then
            formatter="$(command -v "${candidate}")"
            break
        fi
    done

    if [[ -z "${formatter}" ]]; then
        printf 'AI3 bootstrap requires clang-format major 21, but no validated executable was found.\n' >&2
        exit 1
    fi
    log "Validated formatter: ${formatter} ($(${formatter} --version))."
}

cmake_is_supported() {
    local version
    version="$(command_version cmake 'cmake version')"
    [[ "${version}" =~ ^[0-9]+\.[0-9]+([.][0-9]+)?$ ]] &&
        printf '3.25\n%s\n' "${version}" | sort -V -C
}

verify_cmake() {
    if ! cmake_is_supported; then
        printf 'AI3 bootstrap requires CMake >= 3.25, but found "%s".\n' \
            "$(command_version cmake 'cmake version' || true)" >&2
        exit 1
    fi
    log "Validated CMake $(command_version cmake 'cmake version')."
}

verify_commands() {
    local missing=()
    local command_name

    for command_name in "$@"; do
        if ! command -v "${command_name}" >/dev/null 2>&1; then
            missing+=("${command_name}")
        fi
    done

    if (( ${#missing[@]} > 0 )); then
        printf 'AI3 bootstrap did not provide required commands: %s\n' "${missing[*]}" >&2
        exit 1
    fi
}

if [[ -n "${PREFIX:-}" && "${PREFIX}" == *"com.termux"* ]]; then
    log "Detected Termux."
    log "Enabling the Termux X11 package repository if necessary."
    pkg install -y x11-repo

    packages=(
        bash
        clang
        cmake
        coreutils
        findutils
        ninja
        git
        pkg-config
        mesa
        mesa-dev
        libandroid-shmem
        libx11
        libxext
        libxrandr
        libxcursor
        libxi
    )

    log "Installing/updating AI3 prerequisites: ${packages[*]}"
    pkg install -y "${packages[@]}"

    # Termux ships clang-format in the clang package and ctest in cmake.
    verify_commands bash clang clang-format cmake ctest find git ninja pkg-config sort
    verify_clang_format
    verify_cmake
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

    base_packages=(
        build-essential
        ca-certificates
        cmake
        gpg
        ninja-build
        git
        pkg-config
        wget
    )

    if [[ -z "${AI3_BOOTSTRAP_HEADLESS:-}" ]]; then
        base_packages+=(
            libegl1-mesa-dev
            libgles2-mesa-dev
            xorg-dev
            xvfb
        )
    fi

    log "Detected Debian/Ubuntu-style Linux."
    log "Refreshing package metadata."
    "${SUDO[@]}" apt-get update
    log "Installing/updating base AI3 prerequisites: ${base_packages[*]}"
    "${SUDO[@]}" apt-get install -y "${base_packages[@]}"

    . /etc/os-release
    ubuntu_codename="${UBUNTU_CODENAME:-${VERSION_CODENAME:-}}"

    if ! apt-cache show clang-format-21 >/dev/null 2>&1; then
        case "${ubuntu_codename}" in
            jammy|noble) ;;
            *)
                printf 'clang-format-21 is unavailable from configured repositories, and AI3 cannot map "%s" to a supported LLVM Ubuntu repository.\n' \
                    "${ubuntu_codename:-unknown}" >&2
                exit 1
                ;;
        esac

        log "Adding the official LLVM ${ubuntu_codename} repository for clang-format-21."
        temp_dir="$(mktemp -d)"
        trap 'rm -rf "${temp_dir}"' EXIT
        wget -qO "${temp_dir}/llvm.asc" https://apt.llvm.org/llvm-snapshot.gpg.key
        llvm_fingerprint="$(gpg --show-keys --with-colons "${temp_dir}/llvm.asc" | sed -n 's/^fpr:::::::::\([A-F0-9]*\):$/\1/p' | head -n 1)"
        if [[ "${llvm_fingerprint}" != "6084F3CF814B57C1CF12EFD515CF4D18AF4F7421" ]]; then
            printf 'LLVM repository signing key fingerprint did not match the expected key.\n' >&2
            exit 1
        fi
        gpg --dearmor --batch --yes --output "${temp_dir}/llvm.gpg" "${temp_dir}/llvm.asc"
        "${SUDO[@]}" install -m 0644 "${temp_dir}/llvm.gpg" /usr/share/keyrings/ai3-llvm-archive-keyring.gpg
        printf 'deb [signed-by=/usr/share/keyrings/ai3-llvm-archive-keyring.gpg] https://apt.llvm.org/%s/ llvm-toolchain-%s-21 main\n' \
            "${ubuntu_codename}" "${ubuntu_codename}" |
            "${SUDO[@]}" tee /etc/apt/sources.list.d/ai3-llvm-21.list >/dev/null
        "${SUDO[@]}" apt-get update
    fi

    log "Installing clang-format-21."
    "${SUDO[@]}" apt-get install -y clang-format-21

    if ! cmake_is_supported; then
        case "${ubuntu_codename}" in
            jammy|noble) ;;
            *)
                printf 'CMake >= 3.25 is unavailable, and AI3 cannot map "%s" to a supported Kitware Ubuntu repository.\n' \
                    "${ubuntu_codename:-unknown}" >&2
                exit 1
                ;;
        esac

        log "Adding the official Kitware ${ubuntu_codename} repository for CMake >= 3.25."
        temp_dir="${temp_dir:-$(mktemp -d)}"
        trap 'rm -rf "${temp_dir}"' EXIT
        wget -qO "${temp_dir}/kitware.asc" https://apt.kitware.com/keys/kitware-archive-latest.asc
        kitware_fingerprint="$(gpg --show-keys --with-colons "${temp_dir}/kitware.asc" | sed -n 's/^fpr:::::::::\([A-F0-9]*\):$/\1/p' | head -n 1)"
        if [[ "${kitware_fingerprint}" != "4DBEBE3EEC96E7B8C6EC5BE99E92FDC6C5B9BA75" ]]; then
            printf 'Kitware repository signing key fingerprint did not match the expected key.\n' >&2
            exit 1
        fi
        gpg --dearmor --batch --yes --output "${temp_dir}/kitware.gpg" "${temp_dir}/kitware.asc"
        "${SUDO[@]}" install -m 0644 "${temp_dir}/kitware.gpg" /usr/share/keyrings/ai3-kitware-archive-keyring.gpg
        printf 'deb [signed-by=/usr/share/keyrings/ai3-kitware-archive-keyring.gpg] https://apt.kitware.com/ubuntu/ %s main\n' \
            "${ubuntu_codename}" |
            "${SUDO[@]}" tee /etc/apt/sources.list.d/ai3-kitware.list >/dev/null
        "${SUDO[@]}" apt-get update
        "${SUDO[@]}" apt-get install -y cmake
    fi

    verify_commands bash clang-format-21 cmake ctest find git ninja pkg-config sort
    verify_clang_format
    verify_cmake

    log "Linux prerequisites are ready."
    exit 0
fi

printf 'Unsupported environment. AI3 currently bootstraps Termux and Debian/Ubuntu-style Linux.\n' >&2
exit 1
