#!/usr/bin/env sh
set -eu

ROOT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
BUILD=1

if [ "${1:-}" = "--no-build" ]; then
    BUILD=0
fi

if [ "$BUILD" -eq 1 ]; then
    cmake -S "$ROOT_DIR" -B "$ROOT_DIR/build"
    cmake --build "$ROOT_DIR/build"
fi

mkdir -p "$HOME/.local/bin"
mkdir -p "$HOME/.local/share/applications"
mkdir -p "$HOME/.local/share/mime/packages"

install -m 0755 "$ROOT_DIR/build/ahkunixd" "$HOME/.local/bin/ahkunixd"
install -m 0755 "$ROOT_DIR/scripts/ahkunix-open" "$HOME/.local/bin/ahkunix-open"
install -m 0644 "$ROOT_DIR/data/ahkunix.desktop" "$HOME/.local/share/applications/ahkunix.desktop"
install -m 0644 "$ROOT_DIR/data/application-x-ahkunix.xml" "$HOME/.local/share/mime/packages/application-x-ahkunix.xml"

if command -v update-mime-database >/dev/null 2>&1; then
    update-mime-database "$HOME/.local/share/mime"
fi

if command -v update-desktop-database >/dev/null 2>&1; then
    update-desktop-database "$HOME/.local/share/applications" || true
fi

xdg-mime default ahkunix.desktop application/x-ahkunix

echo "Installed:"
echo "  $HOME/.local/bin/ahkunixd"
echo "  $HOME/.local/bin/ahkunix-open"
echo
echo "Make sure ~/.local/bin is in PATH."
echo "Run scripts with:"
echo "  ahkunix-open file.ahkl"
