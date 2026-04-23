cmake_minimum_required(VERSION 3.20)
project(AnyClaw_LVGL LANGUAGES C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# MSVC: force UTF-8 source encoding + disable min/max macro pollution + C11 standard
if(MSVC)
    string(APPEND CMAKE_CXX_FLAGS " /utf-8 /DNOMINMAX /wd5287")
    string(APPEND CMAKE_C_FLAGS " /utf-8 /DNOMINMAX /std:c11 /wd5287")
else()
    string(APPEND CMAKE_CXX_FLAGS " /utf-8 /DNOMINMAX")
    string(APPEND CMAKE_C_FLAGS " /utf-8 /DNOMINMAX")
endif()

# ═══════════════════════════════════════════════════════════════
# Options
# ═══════════════════════════════════════════════════════════════
set(SDL2_CROSS_PREFIX "${CMAKE_SOURCE_DIR}/thirdparty/sdl2-mingw" CACHE PATH "SDL2 MinGW cross-compile install prefix")
set(SDL2_BUNDLED_PREFIX "${CMAKE_SOURCE_DIR}/thirdparty/sdl2-windows")

# Resolve SDL include/lib layout for both Linux cross-build and Windows native build.
set(SDL2_INCLUDE_DIRS "")
set(SDL2_LIB_DIR "")
set(SDL2_DLL_PATH "")

if(MSVC)
    if(EXISTS "${SDL2_BUNDLED_PREFIX}/include/SDL.h")
        list(APPEND SDL2_INCLUDE_DIRS
            "${SDL2_BUNDLED_PREFIX}/include"
        )
    elseif(EXISTS "${SDL2_CROSS_PREFIX}/include/SDL2/SDL.h")
        list(APPEND SDL2_INCLUDE_DIRS
            "${SDL2_CROSS_PREFIX}/include"
            "${SDL2_CROSS_PREFIX}/include/SDL2"
        )
    endif()
else()
    if(EXISTS "${SDL2_CROSS_PREFIX}/include/SDL2/SDL.h")
        list(APPEND SDL2_INCLUDE_DIRS
            "${SDL2_CROSS_PREFIX}/include"
            "${SDL2_CROSS_PREFIX}/include/SDL2"
        )
    elseif(EXISTS "${SDL2_BUNDLED_PREFIX}/include/SDL.h")
        list(APPEND SDL2_INCLUDE_DIRS
            "${SDL2_BUNDLED_PREFIX}/include"
        )
    endif()
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
    if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
        # Native Linux build: try system SDL2 via pkg-config or find_package
        find_package(PkgConfig QUIET)
        if(PkgConfig_FOUND)
            pkg_check_modules(SDL2_PC QUIET sdl2)
        endif()
        if(SDL2_PC_FOUND)
            set(SDL2_INCLUDE_DIRS "${SDL2_PC_INCLUDE_DIRS}")
            set(SDL2_LIB_DIR "")
            set(USE_SYSTEM_SDL2 TRUE)
            message(STATUS "  Using system SDL2 via pkg-config")
        else()
            find_package(SDL2 QUIET)
            if(SDL2_FOUND)
                set(SDL2_INCLUDE_DIRS "${SDL2_INCLUDE_DIRS}")
                set(SDL2_LIB_DIR "")
                set(USE_SYSTEM_SDL2 TRUE)
                message(STATUS "  Using system SDL2 via find_package")
            else()
                message(FATAL_ERROR
                    "SDL2 not found. Install libsdl2-dev or place MinGW SDL2 in:\n"
                    "  ${SDL2_CROSS_PREFIX}/include/SDL2/\n"
                )
            endif()
        endif()
    else()
        message(FATAL_ERROR
            "SDL2 headers not found. Checked:\n"
            "  ${SDL2_CROSS_PREFIX}/include[/SDL2]\n"
            "  ${SDL2_BUNDLED_PREFIX}/include\n"
        )
    endif()
endif()

if(SDL2_LIB_DIR STREQUAL "" AND NOT USE_SYSTEM_SDL2)
    message(FATAL_ERROR
        "SDL2 library directory not found for current toolchain.\n"
        "MSVC expects SDL2.lib under thirdparty/SDL2/lib/x64.\n"
        "MinGW expects libSDL2*.a under ${SDL2_CROSS_PREFIX}/lib."
    )
endif()

# ═══════════════════════════════════════════════════════════════
# FreeType (vector font rendering)
# ═══════════════════════════════════════════════════════════════
set(FREETYPE_CROSS_PREFIX "${CMAKE_SOURCE_DIR}/thirdparty/freetype-mingw" CACHE PATH "FreeType MinGW cross-compile install prefix")

set(FREETYPE_INCLUDE_DIRS "")
set(FREETYPE_LIB_DIR "")

if(NOT MSVC)
    if(EXISTS "${FREETYPE_CROSS_PREFIX}/include/freetype2/freetype/freetype.h")
        list(APPEND FREETYPE_INCLUDE_DIRS "${FREETYPE_CROSS_PREFIX}/include/freetype2")
    elseif(EXISTS "${FREETYPE_CROSS_PREFIX}/include/freetype/freetype.h")
        list(APPEND FREETYPE_INCLUDE_DIRS "${FREETYPE_CROSS_PREFIX}/include/freetype")
    endif()

    if(EXISTS "${FREETYPE_CROSS_PREFIX}/lib/libfreetype.a")
        set(FREETYPE_LIB_DIR "${FREETYPE_CROSS_PREFIX}/lib")
    endif()
endif()

if(FREETYPE_INCLUDE_DIRS STREQUAL "")
    message(STATUS "FreeType headers not found — LV_USE_FREETYPE will compile but link may fail")
    message(STATUS "  Run: bash scripts/build-freetype-mingw.sh to cross-compile FreeType")
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

# Exclude platform-specific drivers not used on Windows (GLOB_RECURSE catches all subdirs).
file(GLOB_RECURSE LVGL_DRIVERS_SOURCES
    ${CMAKE_SOURCE_DIR}/thirdparty/lvgl/src/drivers/*.c
)
set(EXCLUDE_DRIVERS
    ${CMAKE_SOURCE_DIR}/thirdparty/lvgl/src/drivers/nuttx/lv_nuttx_cache.c
    ${CMAKE_SOURCE_DIR}/thirdparty/lvgl/src/drivers/nuttx/lv_nuttx_entry.c
    ${CMAKE_SOURCE_DIR}/thirdparty/lvgl/src/drivers/nuttx/lv_nuttx_fbdev.c
    ${CMAKE_SOURCE_DIR}/thirdparty/lvgl/src/drivers/nuttx/lv_nuttx_image_cache.c
    ${CMAKE_SOURCE_DIR}/thirdparty/lvgl/src/drivers/nuttx/lv_nuttx_lcd.c
    ${CMAKE_SOURCE_DIR}/thirdparty/lvgl/src/drivers/nuttx/lv_nuttx_libuv.c
    ${CMAKE_SOURCE_DIR}/thirdparty/lvgl/src/drivers/nuttx/lv_nuttx_mouse.c
    ${CMAKE_SOURCE_DIR}/thirdparty/lvgl/src/drivers/nuttx/lv_nuttx_profiler.c
    ${CMAKE_SOURCE_DIR}/thirdparty/lvgl/src/drivers/nuttx/lv_nuttx_touchscreen.c
)
list(REMOVE_ITEM LVGL_SOURCES ${EXCLUDE_DRIVERS})

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
    src/anim.cpp
    src/sound.cpp
    src/session_manager.cpp
    src/workspace.cpp
    src/permissions.cpp
    src/feature_flags.cpp
    src/tracing.cpp
    src/lan_chat_protocol.cpp
    src/lan_chat_client.cpp
    src/ftp_client.cpp
    src/stub_ftp.cpp
    src/remote_protocol.cpp
    src/remote_tcp_channel.cpp
    src/kb_store.cpp
    src/app_log.cpp
    src/runtime_mgr.cpp
    src/output_renderer.cpp
    src/async_task.cpp
)

set(APP_FONT_SOURCES
    src/lv_font_mshy_16.c
)

set(APP_RESOURCES
    src/AnyClaw_LVGL.rc
)

# ═══════════════════════════════════════════════════════════════
# FreeType (vector font rendering)
# NOTE: MinGW libfreetype.a is NOT compatible with MSVC — only use for MinGW builds.
# For MSVC, FreeType must come from vcpkg or a native Windows build.
# ═══════════════════════════════════════════════════════════════
if(NOT MSVC)
    set(FREETYPE_PREFIX "${CMAKE_SOURCE_DIR}/thirdparty/freetype-mingw")

    if(EXISTS "${FREETYPE_PREFIX}/lib/libfreetype.a")
        if(EXISTS "${FREETYPE_PREFIX}/include/freetype2/freetype/freetype.h")
            set(FREETYPE_INCLUDE_DIRS "${FREETYPE_PREFIX}/include/freetype2")
        else()
            set(FREETYPE_INCLUDE_DIRS "${FREETYPE_PREFIX}/include")
        endif()
        set(FREETYPE_LIB_DIR "${FREETYPE_PREFIX}/lib")
        set(USE_FREETYPE 1)
        message(STATUS "FreeType: found at ${FREETYPE_PREFIX}")
    else()
        set(USE_FREETYPE 0)
        message(WARNING "FreeType not found at ${FREETYPE_PREFIX}. Build without FreeType. Run scripts/build-freetype-mingw.sh first.")
    endif()
else()
    # MSVC build: use freetype-msvc we built from source
    set(FREETYPE_PREFIX "${CMAKE_SOURCE_DIR}/thirdparty/freetype-msvc")
    if(EXISTS "${FREETYPE_PREFIX}/lib/freetype.lib")
        set(FREETYPE_INCLUDE_DIRS "${FREETYPE_PREFIX}/include")
        set(FREETYPE_LIB_DIR "${FREETYPE_PREFIX}/lib")
        set(USE_FREETYPE 1)
        message(STATUS "FreeType: found MSVC build at ${FREETYPE_PREFIX}")
    else()
        set(USE_FREETYPE 0)
        message(WARNING "FreeType: MSVC lib not found at ${FREETYPE_PREFIX}. Build without FreeType.")
    endif()
endif()

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
    ${FREETYPE_INCLUDE_DIRS}
)

# ═══════════════════════════════════════════════════════════════
# SDL2 cross-compiled library
# ═══════════════════════════════════════════════════════════════
target_link_directories(AnyClaw_LVGL PRIVATE
    ${SDL2_LIB_DIR}
    ${FREETYPE_LIB_DIR}
)

# ═══════════════════════════════════════════════════════════════
# Link libraries
# ═══════════════════════════════════════════════════════════════
if(MSVC)
    target_link_libraries(AnyClaw_LVGL PRIVATE
        SDL2main SDL2
        $<$<BOOL:${USE_FREETYPE}>:freetype>
        kernel32 user32 gdi32 winmm imm32 ole32 oleaut32 version uuid
        advapi32 setupapi shell32 dinput8
        dwmapi comctl32 dbghelp ws2_32 comdlg32
        gdiplus winhttp wininet bcrypt iphlpapi
    )
else()
    if(USE_SYSTEM_SDL2)
        # Native Linux build: use system SDL2
        if(SDL2_PC_FOUND)
            target_link_libraries(AnyClaw_LVGL PRIVATE ${SDL2_PC_LIBRARIES})
        else()
            target_link_libraries(AnyClaw_LVGL PRIVATE SDL2)
        endif()
        target_link_libraries(AnyClaw_LVGL PRIVATE
            $<$<BOOL:${USE_FREETYPE}>:freetype>
            dl pthread
        )
    else()
        # MinGW cross-compile
        target_link_libraries(AnyClaw_LVGL PRIVATE
            mingw32 SDL2main SDL2
            kernel32 user32 gdi32 winmm imm32 ole32 oleaut32 version uuid
            advapi32 setupapi shell32 dinput8
            dwmapi comctl32 dbghelp ws2_32 comdlg32
            gdiplus winhttp wininet bcrypt iphlpapi
            $<$<BOOL:${USE_FREETYPE}>:freetype>
            -static-libgcc -static-libstdc++
        )
    endif()
endif()

# ═══════════════════════════════════════════════════════════════
# Compile definitions
# ═══════════════════════════════════════════════════════════════
target_compile_definitions(AnyClaw_LVGL PRIVATE
    LV_CONF_INCLUDE_SIMPLE
    LV_LVGL_H_INCLUDE_SIMPLE
    _WIN32_WINNT=0x0A00
    NOMINMAX
    $<$<BOOL:${USE_FREETYPE}>:LV_USE_FREETYPE=1>
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

# ═══════════════════════════════════════════════════════════════
# Post-build: copy assets/fonts (CJK + UI fonts)
# ═══════════════════════════════════════════════════════════════
add_custom_command(TARGET AnyClaw_LVGL POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/assets/fonts"
        $<TARGET_FILE_DIR:AnyClaw_LVGL>/assets/fonts
    COMMENT "Copying assets/fonts to output directory"
)

# ═══════════════════════════════════════════════════════════════
# Post-build: copy assets/tray (tray icons for all themes)
# ═══════════════════════════════════════════════════════════════
add_custom_command(TARGET AnyClaw_LVGL POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/assets/tray"
        $<TARGET_FILE_DIR:AnyClaw_LVGL>/assets/tray
    COMMENT "Copying assets/tray to output directory"
)
