#!/usr/bin/env sh
set -eu

ROOT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)

install_deps_apt() {
    sudo apt update
    sudo apt install -y build-essential cmake pkg-config libevdev-dev wl-clipboard xclip xsel desktop-file-utils shared-mime-info
}

install_deps_dnf() {
    sudo dnf install -y gcc-c++ cmake pkgconf-pkg-config libevdev-devel wl-clipboard xclip xsel desktop-file-utils shared-mime-info
}

install_deps_pacman() {
    sudo pacman -Sy --needed base-devel cmake pkgconf libevdev wl-clipboard xclip xsel desktop-file-utils shared-mime-info
}

install_deps_zypper() {
    sudo zypper install -y gcc-c++ cmake pkg-config libevdev-devel wl-clipboard xclip xsel desktop-file-utils shared-mime-info
}

if command -v apt >/dev/null 2>&1; then
    install_deps_apt
elif command -v dnf >/dev/null 2>&1; then
    install_deps_dnf
elif command -v pacman >/dev/null 2>&1; then
    install_deps_pacman
elif command -v zypper >/dev/null 2>&1; then
    install_deps_zypper
else
    echo "Unsupported package manager. Install dependencies manually, then run:" >&2
    echo "  ./scripts/install-local.sh" >&2
    exit 1
fi

"$ROOT_DIR/scripts/install-local.sh"

echo
echo "Setup complete."
echo "Run:"
echo "  ahkunix-open examples/ads.ahkl"
