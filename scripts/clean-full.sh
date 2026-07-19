#!/usr/bin/env sh
set -eu

ROOT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)

cd "$ROOT_DIR"

rm -rf build
rm -rf _CPack_Packages
rm -rf Testing
rm -rf CMakeFiles
rm -rf .cache
rm -rf .cmake
rm -rf .pytest_cache
rm -rf .mypy_cache

rm -f CMakeCache.txt
rm -f cmake_install.cmake
rm -f CPackConfig.cmake
rm -f CPackSourceConfig.cmake
rm -f Makefile.cmake
rm -f build.ninja
rm -f .ninja_deps
rm -f .ninja_log
rm -f install_manifest.txt
rm -f compile_commands.json

rm -f ./*.deb
rm -f ./*.rpm
rm -f ./*.AppImage
rm -f ./*.tar.gz
rm -f ./*.zip

find . -name '*.o' -delete
find . -name '*.a' -delete
find . -name '*.so' -delete
find . -name '*.dylib' -delete
find . -name '*.tmp' -delete
find . -name '*.log' -delete
find . -name '*.bak' -delete
find . -name '*.swp' -delete
find . -name '*.swo' -delete
find . -name '*~' -delete
find . -name '.DS_Store' -delete
find . -name '.fuse_hidden*' -delete

echo "Clean full complete. Source tree is ready for a fresh build."
