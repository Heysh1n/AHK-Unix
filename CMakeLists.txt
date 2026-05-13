cmake_minimum_required(VERSION 3.20)

project(AHKUnix VERSION 0.5.7 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(PkgConfig REQUIRED)
find_package(Threads REQUIRED)
pkg_check_modules(LIBEVDEV REQUIRED IMPORTED_TARGET libevdev)

set(AHKUNIX_CORE_SOURCES
    src/AhkParser.cpp
    src/Clipboard.cpp
    src/Daemon.cpp
    src/Errors.cpp
    src/EvdevKeyboard.cpp
    src/Fd.cpp
    src/KeyboardDetect.cpp
    src/LayoutProfile.cpp
    src/RingBuffer.cpp
    src/Signals.cpp
    src/StringUtil.cpp
    src/UinputKeyboard.cpp
    src/commands/CancelCommand.cpp
    src/commands/TextCommand.cpp
    src/commands/SendInputCommand.cpp
    src/commands/SleepCommand.cpp
    src/commands/IfCommand.cpp
    src/commands/ScriptParser.cpp
)

add_library(ahkunix_core STATIC ${AHKUNIX_CORE_SOURCES})

target_include_directories(ahkunix_core
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/include"
)

target_link_libraries(ahkunix_core
    PUBLIC
        PkgConfig::LIBEVDEV
)

target_compile_options(ahkunix_core PRIVATE
    -Wall
    -Wextra
    -Wpedantic
)

add_executable(ahkunixd
    src/daemon/main.cpp
    src/daemon/Daemonizer.cpp
    src/daemon/IpcServer.cpp
)

target_link_libraries(ahkunixd
    PRIVATE
        ahkunix_core
        Threads::Threads
)

target_compile_options(ahkunixd PRIVATE
    -Wall
    -Wextra
    -Wpedantic
)

add_executable(ahkunixctl
    src/cli/main.cpp
)

target_link_libraries(ahkunixctl
    PRIVATE
        ahkunix_core
)

target_compile_options(ahkunixctl PRIVATE
    -Wall
    -Wextra
    -Wpedantic
)

enable_testing()

add_test(NAME cli_usage
    COMMAND "${CMAKE_CURRENT_SOURCE_DIR}/tests/cli_usage.sh" "$<TARGET_FILE:ahkunixctl>"
)

add_custom_target(dev
    DEPENDS ahkunixd ahkunixctl
    COMMENT "Build AHKUnix for local development"
)

add_custom_target(install-local
    COMMAND "${CMAKE_CURRENT_SOURCE_DIR}/scripts/install-local.sh" --no-build
    DEPENDS ahkunixd ahkunixctl
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    COMMENT "Install AHKUnix into ~/.local"
)

add_custom_target(setup
    COMMAND "${CMAKE_CURRENT_SOURCE_DIR}/scripts/setup.sh"
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    COMMENT "Install dependencies, build, and install AHKUnix locally"
)

add_custom_target(package-deb
    COMMAND "${CMAKE_CPACK_COMMAND}" -G DEB --config "${CMAKE_BINARY_DIR}/CPackConfig.cmake"
    DEPENDS ahkunixd ahkunixctl
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    COMMENT "Build Debian package"
)

add_custom_target(clean-artifacts
    COMMAND "${CMAKE_COMMAND}" -E rm -rf "${CMAKE_BINARY_DIR}" "${CMAKE_CURRENT_SOURCE_DIR}/ahkunix-${PROJECT_VERSION}-Linux.deb"
    COMMENT "Remove build directory and generated packages"
)

add_custom_target(clean-full
    COMMAND "${CMAKE_CURRENT_SOURCE_DIR}/scripts/clean-full.sh"
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    COMMENT "Remove all generated and temporary files for a fresh build"
)

install(TARGETS ahkunixd ahkunixctl RUNTIME DESTINATION bin)
install(FILES data/ahkunix.desktop
    DESTINATION share/applications
    PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ
)
install(FILES data/application-x-ahkunix.xml
    DESTINATION share/mime/packages
    PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ
)
install(FILES
    examples/ads.ahkl
    examples/weather_broadcast.ahkl
    examples/worker_cancel.ahkl
    DESTINATION share/doc/ahkunix/examples
    PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ
)

set(CPACK_PACKAGE_NAME "ahkunix")
set(CPACK_PACKAGE_VENDOR "AHKUnix")
set(CPACK_PACKAGE_CONTACT "local")
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Linux evdev/uinput hotstring daemon")
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "AHKUnix")
set(CPACK_DEBIAN_PACKAGE_DEPENDS "libevdev2, sudo, wl-clipboard | xclip | xsel")
set(CPACK_DEBIAN_PACKAGE_SECTION "utils")
set(CPACK_DEBIAN_PACKAGE_PRIORITY "optional")
set(CPACK_DEBIAN_PACKAGE_CONTROL_EXTRA
    "${CMAKE_CURRENT_SOURCE_DIR}/packaging/deb/postinst;${CMAKE_CURRENT_SOURCE_DIR}/packaging/deb/postrm"
)

include(CPack)
