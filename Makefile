<<<<<<< HEAD
.PHONY: configure build test dev setup install-local deb package clean-artifacts clean-full run-example help
=======
.PHONY: configure build test dev setup install-local deb package clean-artifacts clean-full run-example help uninstall reinstall
>>>>>>> master

BUILD_DIR ?= build
SCRIPT ?= examples/ads.ahkl
DEVICE ?=

help:
	@echo "AHKUnix targets:"
	@echo "  make setup          Install deps, build, install launcher locally"
	@echo "  make configure      Configure CMake"
	@echo "  make build          Build ahkunixd"
	@echo "  make test           Run tests"
	@echo "  make install-local  Build and install into ~/.local"
	@echo "  make deb            Build Debian package"
	@echo "  make run-example    Run examples/ads.ahkl"
	@echo "  make clean-artifacts Remove build and generated packages"
	@echo "  make clean-full     Remove all generated/temp files for a fresh build"
<<<<<<< HEAD
=======
	@echo "  make uninstall      Remove installed files from ~/.local"
	@echo "  make reinstall      Nuke everything, build and install fresh"
>>>>>>> master

configure:
	cmake -S . -B $(BUILD_DIR)

build: configure
	cmake --build $(BUILD_DIR)

test: build
	ctest --test-dir $(BUILD_DIR) --output-on-failure

dev: build

setup:
	./scripts/setup.sh

install-local: build
	./scripts/install-local.sh --no-build

deb: build
	cpack -G DEB --config $(BUILD_DIR)/CPackConfig.cmake

package: deb

clean-artifacts:
	rm -rf $(BUILD_DIR) *.deb *.rpm *.AppImage *.tar.gz *.zip

clean-full:
	./scripts/clean-full.sh

run-example: build
	@if [ -n "$(DEVICE)" ]; then \
		sudo --preserve-env=DISPLAY,WAYLAND_DISPLAY,XDG_RUNTIME_DIR,DBUS_SESSION_BUS_ADDRESS ./$(BUILD_DIR)/ahkunixd --device "$(DEVICE)" "$(SCRIPT)"; \
	else \
		sudo --preserve-env=DISPLAY,WAYLAND_DISPLAY,XDG_RUNTIME_DIR,DBUS_SESSION_BUS_ADDRESS ./$(BUILD_DIR)/ahkunixd "$(SCRIPT)"; \
	fi
<<<<<<< HEAD
=======

uninstall:
	@echo "Stopping daemon if running..."
	-ahkunixctl stop 2>/dev/null || sudo pkill ahkunixd || true
	@echo "Removing local binaries and desktop files..."
	rm -f $(HOME)/.local/bin/ahkunixd
	rm -f $(HOME)/.local/bin/ahkunixctl
	rm -f $(HOME)/.local/share/applications/ahkunix.desktop
	rm -f $(HOME)/.local/share/mime/packages/application-x-ahkunix.xml

reinstall: uninstall clean-full install-local
	@echo "Reinstall complete. Fresh build deployed."
>>>>>>> master
