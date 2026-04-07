cmake_minimum_required(VERSION 3.20)
project(AnyClaw_LVGL LANGUAGES C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# ═══════════════════════════════════════════════════════════════
# Options
# ═══════════════════════════════════════════════════════════════
set(SDL2_CROSS_PREFIX "/opt/sdl2-mingw" CACHE PATH "SDL2 MinGW cross-compile install prefix")

# ═══════════════════════════════════════════════════════════════
# LVGL sources (GLOB_RECURSE)
# ═══════════════════════════════════════════════════════════════
file(GLOB_RECURSE LVGL_SOURCES
    ${CMAKE_SOURCE_DIR}/thirdparty/lvgl/src/*.c
)

# Exclude project font files (compiled separately as APP sources)
set(EXCLUDE_FONTS
    ${CMAKE_SOURCE_DIR}/thirdparty/lvgl/src/font/lv_font_montserrat_14.c
)
list(REMOVE_ITEM LVGL_SOURCES ${EXCLUDE_FONTS})

# ═══════════════════════════════════════════════════════════════
# App sources
# ═══════════════════════════════════════════════════════════════
set(APP_SOURCES
    src/main.cpp
    src/ui_main.cpp
    src/ui_settings.cpp
    src/tray.cpp
    src/health.cpp
    src/openclaw_mgr.cpp
    src/http_client.cpp
    src/autostart.cpp
    src/selfcheck.cpp
    src/model_manager.cpp
    src/license.cpp
    src/migration.cpp
    src/json_util.cpp
    src/utf8_util.cpp
    src/garlic_dock.cpp
    src/session_manager.cpp
    src/workspace.cpp
    src/permissions.cpp
)

set(APP_FONT_SOURCES
    src/lv_font_mshy_16.c
)

set(APP_RESOURCES
    src/AnyClaw_LVGL.rc
)

# ═══════════════════════════════════════════════════════════════
# Executable
# ═══════════════════════════════════════════════════════════════
add_executable(AnyClaw_LVGL WIN32
    ${LVGL_SOURCES}
    ${APP_SOURCES}
    ${APP_FONT_SOURCES}
    ${APP_RESOURCES}
)

# ═══════════════════════════════════════════════════════════════
# Include paths
# ═══════════════════════════════════════════════════════════════
target_include_directories(AnyClaw_LVGL PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/thirdparty/lvgl
    ${CMAKE_SOURCE_DIR}/thirdparty/lvgl/src
    ${SDL2_CROSS_PREFIX}/include/SDL2
)

# ═══════════════════════════════════════════════════════════════
# SDL2 cross-compiled library
# ═══════════════════════════════════════════════════════════════
target_link_directories(AnyClaw_LVGL PRIVATE
    ${SDL2_CROSS_PREFIX}/lib
)

# ═══════════════════════════════════════════════════════════════
# Link libraries
# ═══════════════════════════════════════════════════════════════
target_link_libraries(AnyClaw_LVGL PRIVATE
    mingw32 SDL2main SDL2
    kernel32 user32 gdi32 winmm imm32 ole32 oleaut32 version uuid
    advapi32 setupapi shell32 dinput8
    dwmapi comctl32 dbghelp ws2_32 comdlg32
    gdiplus winhttp bcrypt
    -static-libgcc -static-libstdc++
)

# ═══════════════════════════════════════════════════════════════
# Compile definitions
# ═══════════════════════════════════════════════════════════════
target_compile_definitions(AnyClaw_LVGL PRIVATE
    LV_CONF_INCLUDE_SIMPLE
    LV_LVGL_H_INCLUDE_SIMPLE
    _WIN32_WINNT=0x0A00
)

# ═══════════════════════════════════════════════════════════════
# Output directory
# ═══════════════════════════════════════════════════════════════
set_target_properties(AnyClaw_LVGL PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

# ═══════════════════════════════════════════════════════════════
# Post-build: copy SDL2.dll
# ═══════════════════════════════════════════════════════════════
add_custom_command(TARGET AnyClaw_LVGL POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${SDL2_CROSS_PREFIX}/bin/SDL2.dll"
        $<TARGET_FILE_DIR:AnyClaw_LVGL>
    COMMENT "Copying SDL2.dll to output directory"
)
