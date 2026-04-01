cmake_minimum_required(VERSION 3.16)
project(AnyClaw_LVGL VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ── LVGL ─────────────────────────────────────────────────────────────────
set(LV_CONF_PATH "${CMAKE_SOURCE_DIR}/src/lv_conf.h" CACHE STRING "" FORCE)
set(LV_LVGL_H_INCLUDE_SIMPLE 1)
file(GLOB_RECURSE LVGL_SOURCES
    "${CMAKE_SOURCE_DIR}/thirdparty/lvgl/src/*.c"
    "${CMAKE_SOURCE_DIR}/thirdparty/lvgl/src/*.h"
)
# Exclude platform-specific drivers not supported on Windows
list(FILTER LVGL_SOURCES EXCLUDE REGEX "nema_gfx")
list(FILTER LVGL_SOURCES EXCLUDE REGEX "display/drm")
list(FILTER LVGL_SOURCES EXCLUDE REGEX "display/dsi")
list(FILTER LVGL_SOURCES EXCLUDE REGEX "display/zedboard")
list(FILTER LVGL_SOURCES EXCLUDE REGEX "linux")
list(FILTER LVGL_SOURCES EXCLUDE REGEX "sdl/.*/SDL")
add_library(lvgl_static STATIC ${LVGL_SOURCES})
target_include_directories(lvgl_static PUBLIC
    "${CMAKE_SOURCE_DIR}/thirdparty/lvgl"
    "${CMAKE_SOURCE_DIR}/thirdparty/lvgl/src"
    "${CMAKE_SOURCE_DIR}/src"
    "${CMAKE_SOURCE_DIR}/thirdparty/SDL2/include"
)
target_compile_definitions(lvgl_static PUBLIC
    LV_CONF_INCLUDE_SIMPLE
    _CRT_SECURE_NO_WARNINGS
)
target_compile_options(lvgl_static PRIVATE /FI${CMAKE_SOURCE_DIR}/src/lv_conf.h)

# ── SDL2 (pre-built VC) ──────────────────────────────────────────────────
set(SDL2_DIR "${CMAKE_SOURCE_DIR}/thirdparty/SDL2")
set(SDL2_INCLUDE_DIR "${SDL2_DIR}/include")

if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(SDL2_LIB_DIR "${SDL2_DIR}/lib/x64")
else()
    set(SDL2_LIB_DIR "${SDL2_DIR}/lib/x86")
endif()

find_library(SDL2_LIBRARY NAMES SDL2 PATHS ${SDL2_LIB_DIR} NO_DEFAULT_PATH)
find_library(SDL2_MAIN_LIBRARY NAMES SDL2main PATHS ${SDL2_LIB_DIR} NO_DEFAULT_PATH)

# Copy SDL2.dll to output
configure_file("${SDL2_LIB_DIR}/SDL2.dll" "${CMAKE_BINARY_DIR}/bin/SDL2.dll" COPYONLY)

# ── Core sources ─────────────────────────────────────────────────────────
set(SOURCES
    src/main.cpp
    src/openclaw_mgr.cpp
    src/http_client.cpp
    src/ui_main.cpp
    src/ui_settings.cpp
    src/tray.cpp
    src/health.cpp
    src/autostart.cpp
)

add_executable(AnyClaw_LVGL WIN32 ${SOURCES})

target_include_directories(AnyClaw_LVGL PRIVATE
    src
    "${CMAKE_SOURCE_DIR}/thirdparty/lvgl"
    ${SDL2_INCLUDE_DIR}
)

target_link_libraries(AnyClaw_LVGL PRIVATE
    lvgl_static
    ${SDL2_MAIN_LIBRARY}
    ${SDL2_LIBRARY}
    winhttp
    shell32
    user32
    gdi32
    advapi32
    imm32
    version
    setupapi
    kernel32
)

target_compile_definitions(AnyClaw_LVGL PRIVATE
    LV_CONF_INCLUDE_SIMPLE
    _CRT_SECURE_NO_WARNINGS
    UNICODE
    _UNICODE
)

set_target_properties(AnyClaw_LVGL PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)
