cmake_minimum_required(VERSION 3.20)
project(AnyClaw_LVGL LANGUAGES C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# ═══════════════════════════════════════════════════════════════
# Options
# ═══════════════════════════════════════════════════════════════
set(SDL2_CROSS_PREFIX "/opt/sdl2-mingw" CACHE PATH "SDL2 MinGW cross-compile install prefix")
set(SDL2_BUNDLED_PREFIX "${CMAKE_SOURCE_DIR}/thirdparty/SDL2")

# Resolve SDL include/lib layout for both Linux cross-build and Windows native build.
set(SDL2_INCLUDE_DIRS "")
set(SDL2_LIB_DIR "")
set(SDL2_DLL_PATH "")

if(EXISTS "${SDL2_CROSS_PREFIX}/include/SDL2/SDL.h")
    list(APPEND SDL2_INCLUDE_DIRS
        "${SDL2_CROSS_PREFIX}/include"
        "${SDL2_CROSS_PREFIX}/include/SDL2"
    )
endif()

if(EXISTS "${SDL2_BUNDLED_PREFIX}/include/SDL.h")
    list(APPEND SDL2_INCLUDE_DIRS
        "${SDL2_BUNDLED_PREFIX}/include"
    )
endif()

if(MSVC)
    if(EXISTS "${SDL2_BUNDLED_PREFIX}/lib/x64/SDL2.lib")
        set(SDL2_LIB_DIR "${SDL2_BUNDLED_PREFIX}/lib/x64")
    elseif(EXISTS "${SDL2_CROSS_PREFIX}/lib/SDL2.lib")
        set(SDL2_LIB_DIR "${SDL2_CROSS_PREFIX}/lib")
    endif()

    if(EXISTS "${SDL2_BUNDLED_PREFIX}/lib/x64/SDL2.dll")
        set(SDL2_DLL_PATH "${SDL2_BUNDLED_PREFIX}/lib/x64/SDL2.dll")
    elseif(EXISTS "${SDL2_CROSS_PREFIX}/bin/SDL2.dll")
        set(SDL2_DLL_PATH "${SDL2_CROSS_PREFIX}/bin/SDL2.dll")
    endif()
else()
    if(EXISTS "${SDL2_CROSS_PREFIX}/lib/libSDL2.a" OR EXISTS "${SDL2_CROSS_PREFIX}/lib/libSDL2.dll.a")
        set(SDL2_LIB_DIR "${SDL2_CROSS_PREFIX}/lib")
    endif()

    if(EXISTS "${SDL2_CROSS_PREFIX}/bin/SDL2.dll")
        set(SDL2_DLL_PATH "${SDL2_CROSS_PREFIX}/bin/SDL2.dll")
    endif()
endif()

if(SDL2_INCLUDE_DIRS STREQUAL "")
    message(FATAL_ERROR
        "SDL2 headers not found. Checked:\n"
        "  ${SDL2_CROSS_PREFIX}/include[/SDL2]\n"
        "  ${SDL2_BUNDLED_PREFIX}/include\n"
    )
endif()

if(SDL2_LIB_DIR STREQUAL "")
    message(FATAL_ERROR
        "SDL2 library directory not found for current toolchain.\n"
        "MSVC expects SDL2.lib under thirdparty/SDL2/lib/x64.\n"
        "MinGW expects libSDL2*.a under ${SDL2_CROSS_PREFIX}/lib."
    )
endif()

# ═══════════════════════════════════════════════════════════════
# LVGL sources (GLOB_RECURSE)
# ═══════════════════════════════════════════════════════════════
file(GLOB_RECURSE LVGL_SOURCES
    ${CMAKE_SOURCE_DIR}/thirdparty/lvgl/src/*.c
)

# Exclude project-private duplicated font symbols from upstream LVGL bundle.
set(EXCLUDE_FONTS
    ${CMAKE_SOURCE_DIR}/thirdparty/lvgl/src/font/lv_font_mshy_16.c
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
    src/boot_check.cpp
    src/model_manager.cpp
    src/license.cpp
    src/migration.cpp
    src/json_util.cpp
    src/utf8_util.cpp
    src/garlic_dock.cpp
    src/session_manager.cpp
    src/workspace.cpp
    src/permissions.cpp
    src/boot_check.cpp
    src/feature_flags.cpp
    src/tracing.cpp
    src/lan_chat_protocol.cpp
    src/lan_chat_client.cpp
    src/ftp_client.cpp
    src/remote_protocol.cpp
    src/kb_store.cpp
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
    ${SDL2_INCLUDE_DIRS}
)

# ═══════════════════════════════════════════════════════════════
# SDL2 cross-compiled library
# ═══════════════════════════════════════════════════════════════
target_link_directories(AnyClaw_LVGL PRIVATE
    ${SDL2_LIB_DIR}
)

# ═══════════════════════════════════════════════════════════════
# Link libraries
# ═══════════════════════════════════════════════════════════════
if(MSVC)
    target_link_libraries(AnyClaw_LVGL PRIVATE
        SDL2main SDL2
        kernel32 user32 gdi32 winmm imm32 ole32 oleaut32 version uuid
        advapi32 setupapi shell32 dinput8
        dwmapi comctl32 dbghelp ws2_32 comdlg32
        gdiplus winhttp wininet bcrypt
    )
else()
    target_link_libraries(AnyClaw_LVGL PRIVATE
        mingw32 SDL2main SDL2
        kernel32 user32 gdi32 winmm imm32 ole32 oleaut32 version uuid
        advapi32 setupapi shell32 dinput8
        dwmapi comctl32 dbghelp ws2_32 comdlg32
        gdiplus winhttp wininet bcrypt
        -static-libgcc -static-libstdc++
    )
endif()

# ═══════════════════════════════════════════════════════════════
# Compile definitions
# ═══════════════════════════════════════════════════════════════
target_compile_definitions(AnyClaw_LVGL PRIVATE
    LV_CONF_INCLUDE_SIMPLE
    LV_LVGL_H_INCLUDE_SIMPLE
    _WIN32_WINNT=0x0A00
    NOMINMAX
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
if(SDL2_DLL_PATH)
    add_custom_command(TARGET AnyClaw_LVGL POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${SDL2_DLL_PATH}"
            $<TARGET_FILE_DIR:AnyClaw_LVGL>
        COMMENT "Copying SDL2.dll to output directory"
    )
else()
    message(WARNING "SDL2.dll not found. Build will succeed but runtime needs SDL2.dll in PATH/output.")
endif()
