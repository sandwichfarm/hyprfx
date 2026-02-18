cmake_minimum_required(VERSION 3.27)

project(burn-my-windows
    DESCRIPTION "Burn My Windows effects for Hyprland"
    VERSION 0.1
)

set(CMAKE_CXX_STANDARD 23)

file(GLOB_RECURSE SRC "src/*.cpp")

add_library(burn-my-windows SHARED ${SRC})

find_package(PkgConfig REQUIRED)
pkg_check_modules(deps REQUIRED IMPORTED_TARGET
    hyprland
    libdrm
    libinput
    libudev
    pangocairo
    pixman-1
    wayland-server
    xkbcommon
)
target_link_libraries(burn-my-windows PRIVATE rt PkgConfig::deps)

install(TARGETS burn-my-windows)
