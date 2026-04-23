#include "app.h"
#include "app_config.h"
#include "icon_config.h"
#include "model_manager.h"
#include "app_log.h"
#include "theme.h"
#include "lang.h"
#include "SDL.h"
#include "SDL_syswm.h"
#include <cstdio>
#include <cstring>
#include <cstdarg>
#include <string>
#include <vector>
#include <thread>
#include <atomic>
#include <mutex>
#include <deque>
#include <fstream>
#include <filesystem>
#include <algorithm>
#include <unordered_set>
#include <sstream>
#include <cctype>
#include <windows.h>
#include <commdlg.h>
#include <shlobj.h>
#include <shellapi.h>

/* Extern for tray minimize */
#include "tray.h"
#include "anim.h"
#include "sound.h"
#include "health.h"
#include "license.h"
#include "session_manager.h"
#include "boot_check.h"
#include "tracing.h"
#include "lan_chat_client.h"
#include "ftp_client.h"
#include "remote_protocol.h"
#include "remote_tcp_channel.h"
#include "kb_store.h"
#include "output_renderer.h"
#include "workspace.h"
#include "permissions.h"
#include "runtime_mgr.h"
#include "cjk_font_data.h"
#include "markdown.h"
#include "widgets/aw_form.h"

/* ═══ System Font: TinyTTF embedded data + bitmap fallback ═══ */
#ifdef __cplusplus
extern "C" {
#endif
extern const lv_font_t lv_font_mshy_16;
#ifdef __cplusplus
}
#endif
lv_font_t* g_cjk_font = nullptr;
lv_font_t* g_cjk_font_small = nullptr;
lv_font_t* g_cjk_font_chat = nullptr;   /* Chat text: 4/5 of default (13px at 100%) */
ThemeFonts g_theme_fonts = {};            /* Per-theme font pointers */
static DWORD g_ui_thread_id = 0;
static bool g_restore_last_session = false; /* default: start fresh every launch */
static LanChatClient g_lan_chat_client;
static std::atomic<bool> g_ftp_running(false);
static std::atomic<bool> g_ftp_cancel(false);
static std::mutex g_ftp_last_mtx;
static bool g_ftp_has_last = false;
static FtpTransferConfig g_ftp_last_cfg;

/* Global startup error title (set by selfcheck, displayed by UI after LVGL init) */
std::string g_startup_error_title;
std::string g_status_reason;  /* Failure reason shown when status == Error */
/* FIX Bug 8: Mutex for cross-thread access to startup error strings */
std::mutex g_startup_error_mtx;

static void init_system_font() {
    int dpi = app_get_dpi_scale();
    int cjk_size = 16 * dpi / 100;    /* 16px at 100%, 32px at 200% */
    int small_size = 12 * dpi / 100;   /* 12px at 100%, 24px at 200% */
    int chat_size = 13 * dpi / 100;    /* 13px at 100% — 4/5 of default for chat text */

    /* Priority 1: Embedded font data (no file system dependency) */
    g_cjk_font = lv_tiny_ttf_create_data(cjk_font_data, cjk_font_data_len, cjk_size);
    if (g_cjk_font) {
        g_cjk_font_small = lv_tiny_ttf_create_data(cjk_font_data, cjk_font_data_len, small_size);
        g_cjk_font_chat = lv_tiny_ttf_create_data(cjk_font_data, cjk_font_data_len, chat_size);
        LOG_I("Font", "TinyTTF embedded data loaded, size=%dpx (DPI %d%%)", cjk_size, dpi);
    } else {
        /* Priority 2: File-based system font (Windows msyh / Wine) */
        const char* font_paths[] = {
            "C:\\Windows\\Fonts\\msyh.ttc",
            "C:\\Windows\\Fonts\\msyh.ttf",
            "A:\\Windows\\Fonts\\msyh.ttc",
            nullptr
        };
        bool loaded = false;
        for (int i = 0; font_paths[i] != nullptr; i++) {
            FILE* f = fopen(font_paths[i], "rb");
            if (f) {
                fclose(f);
                g_cjk_font = lv_tiny_ttf_create_file(font_paths[i], cjk_size);
                if (g_cjk_font) {
                    g_cjk_font_small = lv_tiny_ttf_create_file(font_paths[i], small_size);
                    g_cjk_font_chat = lv_tiny_ttf_create_file(font_paths[i], chat_size);
                    LOG_I("Font", "TinyTTF file loaded: %s, size=%dpx", font_paths[i], cjk_size);
                    loaded = true;
                    break;
                }
            }
        }
        if (!loaded) {
            /* Priority 3: Compiled bitmap font (fixed 16px, no DPI scaling) */
            g_cjk_font = (lv_font_t*)&lv_font_mshy_16;
            g_cjk_font_small = (lv_font_t*)&lv_font_montserrat_12;
            g_cjk_font_chat = (lv_font_t*)&lv_font_montserrat_12;  /* fallback: 12px */
            LOG_W("Font", "Fallback to bitmap lv_font_mshy_16 (16px fixed)");
        }
    }

    /* ═══ Emoji/Symbol font: try Windows fonts as fallback ═══ */
    {
        const char* emoji_paths[] = {
            "C:\\Windows\\Fonts\\segoeui.ttf",        /* Segoe UI regular (basic symbols) */
            "C:\\Windows\\Fonts\\arial.ttf",           /* Arial fallback */
            "A:\\Windows\\Fonts\\segoeui.ttf",
            nullptr
        };
        for (int i = 0; emoji_paths[i] != nullptr; i++) {
            FILE* ef = fopen(emoji_paths[i], "rb");
            if (ef) {
                fclose(ef);
                int sym_size = 16 * dpi / 100;
                lv_font_t* sym_font = lv_tiny_ttf_create_file(emoji_paths[i], sym_size);
                if (sym_font) {
                    if (g_cjk_font)       g_cjk_font->fallback = sym_font;
                    if (g_cjk_font_small)  g_cjk_font_small->fallback = sym_font;
                    if (g_cjk_font_chat)   g_cjk_font_chat->fallback = sym_font;
                    LOG_I("Font", "Symbol fallback font loaded: %s size=%dpx", emoji_paths[i], sym_size);
                    break;
                }
            }
        }
    }

    fflush(stdout);
}

/* ═══════════════════════════════════════════════════════════════
 *  init_theme_fonts — per-theme font initialization
 *
 *  Fills g_theme_fonts with DPI-scaled font pointers.
 *  When theme-specific .ttf/.otf files are available in assets/fonts/,
 *  this function loads them per theme. Currently uses embedded CJK
 *  font data as universal fallback.
 *
 *  Font files expected per theme (see Design §3.4):
 *    Matcha:  Inter-*.ttf + SourceHanSansSC-*.otf
 *    Peachy:  Nunito-{Regular,Bold}.ttf + HarmonyOS_Sans_SC_*.otf
 *    Mochi:   PlusJakartaSans-*.ttf + Lora-*.ttf + 思源宋体_SC_*.otf
 *    All:     JetBrainsMono-Regular.ttf
 * ═══════════════════════════════════════════════════════════════ */

/* Helper: create a TinyTTF font from embedded data at given pixel size */
static lv_font_t* make_embedded_font(int px) {
    int dpi = app_get_dpi_scale();
    int sz = px * dpi / 100;
    if (sz < 8) sz = 8;
    return lv_tiny_ttf_create_data(cjk_font_data, cjk_font_data_len, sz);
}

/* Helper: create a TinyTTF font from file at given pixel size, fallback to embedded */
static lv_font_t* try_load_ttf(const char* path, int px, lv_font_t* fallback_font) {
    int dpi = app_get_dpi_scale();
    int sz = px * dpi / 100;
    if (sz < 8) sz = 8;
    FILE* f = fopen(path, "rb");
    if (f) {
        fclose(f);
        lv_font_t* font = lv_tiny_ttf_create_file(path, sz);
        if (font) return font;
    }
    /* Fallback: use embedded data */
    if (fallback_font) return fallback_font;
    return make_embedded_font(px);
}

/* Helper: create a FreeType vector font (high-DPI crisp rendering).
 * Returns NULL if file not found or FreeType init fails.
 * Caller should use this as primary when available, with TinyTTF as fallback. */
static lv_font_t* try_load_freetype(const char* path, int px, lv_freetype_font_style_t style) {
#if LV_USE_FREETYPE
    int dpi = app_get_dpi_scale();
    int sz = px * dpi / 100;
    if (sz < 8) sz = 8;
    FILE* f = fopen(path, "rb");
    if (!f) return nullptr;
    fclose(f);
    lv_font_t* ft = lv_freetype_font_create(path, LV_FREETYPE_FONT_RENDER_MODE_OUTLINE,
                                             (uint32_t)sz, style);
    if (ft) {
        LOG_I("Font", "FreeType loaded: %s @ %dpx (dpi %d)", path, sz, dpi);
        return ft;
    }
    LOG_W("Font", "FreeType create failed for %s, falling back to TinyTTF", path);
#else
    (void)path; (void)px; (void)style;
#endif
    return nullptr;
}

/* Theme font file mapping (relative to executable, Design §3.1) */
struct ThemeFontPaths {
    const char* body_regular;    /* English UI body font (Regular) */
    const char* body_semibold;   /* English UI emphasis (SemiBold/Bold) */
    const char* body_bold;       /* English headings (Bold) */
    const char* cjk_regular;     /* CJK body font */
    const char* cjk_bold;        /* CJK title font */
    const char* code_font;       /* Monospace font */
};

static const ThemeFontPaths FONT_PATHS[] = {
                /* Theme::Matcha — Inter + Noto Sans SC */
    { "assets/fonts/Inter-Regular.ttf", "assets/fonts/Inter-SemiBold.ttf",
            "assets/fonts/Inter-Bold.ttf", "assets/fonts/NotoSansSC-Regular.otf",
            "assets/fonts/NotoSansSC-Bold.otf", "assets/fonts/JetBrainsMono-Regular.ttf" },
        /* Theme::Peachy — Nunito + Noto Sans SC */
    { "assets/fonts/Nunito-Regular.ttf", "assets/fonts/Nunito-Bold.ttf",
            "assets/fonts/Nunito-Bold.ttf", "assets/fonts/NotoSansSC-Regular.otf",
            "assets/fonts/NotoSansSC-Bold.otf", "assets/fonts/JetBrainsMono-Regular.ttf" },
        /* Theme::Mochi — PJS + Lora + Noto Sans SC */
    { "assets/fonts/PlusJakartaSans-Regular.ttf", "assets/fonts/PlusJakartaSans-SemiBold.ttf",
            "assets/fonts/Lora-Bold.ttf", "assets/fonts/NotoSansSC-Regular.otf",
            "assets/fonts/NotoSansSC-Bold.otf", "assets/fonts/JetBrainsMono-Regular.ttf" },
};

void init_theme_fonts(Theme theme) {
    /* Base pixel sizes at 800h (Design §3.2) */
    static const int SZ_DISPLAY = 28;
    static const int SZ_H1      = 22;
    static const int SZ_H2      = 18;
    static const int SZ_H3      = 15;
    static const int SZ_BODY    = 13;
    static const int SZ_SMALL   = 11;
    static const int SZ_CAPTION = 10;
    static const int SZ_CODE    = 12;

    int ti = (int)theme;
    if (ti < 0 || ti > 2) ti = 0;
    const ThemeFontPaths& paths = FONT_PATHS[ti];

    /* Create embedded fallback for when theme files are not available */
    lv_font_t* emb_body    = make_embedded_font(SZ_BODY);
    lv_font_t* emb_small   = make_embedded_font(SZ_SMALL);
    lv_font_t* emb_caption = make_embedded_font(SZ_CAPTION);

    /* Try loading theme-specific fonts; fall back to embedded CJK */
    lv_font_t* body_font    = try_load_ttf(paths.body_regular, SZ_BODY, emb_body);
    lv_font_t* bold_font    = try_load_ttf(paths.body_bold, SZ_H1, emb_body);
    lv_font_t* semibold_font= try_load_ttf(paths.body_semibold, SZ_H3, emb_body);
    lv_font_t* cjk_regular  = try_load_ttf(paths.cjk_regular, SZ_BODY, emb_body);
    lv_font_t* cjk_bold     = try_load_ttf(paths.cjk_bold, SZ_H1, emb_body);
    lv_font_t* code_font    = try_load_ttf(paths.code_font, SZ_CODE, emb_body);

    /* Build 9-level English/通用 font set */
    g_theme_fonts.display     = try_load_ttf(paths.body_bold, SZ_DISPLAY, emb_body);
    g_theme_fonts.h1          = bold_font;
    g_theme_fonts.h2          = try_load_ttf(paths.body_semibold, SZ_H2, emb_body);
    g_theme_fonts.h3          = semibold_font;
    g_theme_fonts.body        = body_font;
    g_theme_fonts.body_strong = semibold_font;
    g_theme_fonts.small       = try_load_ttf(paths.body_regular, SZ_SMALL, emb_small);
    g_theme_fonts.caption     = try_load_ttf(paths.body_regular, SZ_CAPTION, emb_caption);
    g_theme_fonts.code        = code_font;

    /* CJK fonts */
    g_theme_fonts.cjk_body    = cjk_regular;
    g_theme_fonts.cjk_title   = cjk_bold;

    /* Set fallback chain: English font → CJK font
     * When theme-specific font files are available, this links English→CJK.
     * When using embedded fallback (same pointer), skip to avoid self-loop. */
    if (g_theme_fonts.body && g_theme_fonts.cjk_body && g_theme_fonts.body != g_theme_fonts.cjk_body)
        g_theme_fonts.body->fallback = g_theme_fonts.cjk_body;
    if (g_theme_fonts.body_strong && g_theme_fonts.cjk_body && g_theme_fonts.body_strong != g_theme_fonts.cjk_body)
        g_theme_fonts.body_strong->fallback = g_theme_fonts.cjk_body;
    if (g_theme_fonts.h1 && g_theme_fonts.cjk_title && g_theme_fonts.h1 != g_theme_fonts.cjk_title)
        g_theme_fonts.h1->fallback = g_theme_fonts.cjk_title;
    if (g_theme_fonts.h2 && g_theme_fonts.cjk_title && g_theme_fonts.h2 != g_theme_fonts.cjk_title)
        g_theme_fonts.h2->fallback = g_theme_fonts.cjk_title;
    if (g_theme_fonts.display && g_theme_fonts.cjk_title && g_theme_fonts.display != g_theme_fonts.cjk_title)
        g_theme_fonts.display->fallback = g_theme_fonts.cjk_title;

    /* ══ FreeType vector fonts (5T-02.5) ══
     * Try loading high-DPI vector fonts via FreeType.
     * When available, FreeType replaces TinyTTF as primary;
     * TinyTTF font becomes the fallback for CJK glyph coverage. */
    lv_font_t* ft_body = try_load_freetype(paths.body_regular, SZ_BODY, LV_FREETYPE_FONT_STYLE_NORMAL);
    lv_font_t* ft_bold = try_load_freetype(paths.body_bold, SZ_H1, LV_FREETYPE_FONT_STYLE_BOLD);

    if (ft_body) {
        /* FreeType body → fallback to TinyTTF body (which has CJK) */
        ft_body->fallback = g_theme_fonts.body;
        g_theme_fonts.freetype_body = ft_body;
        g_theme_fonts.body = ft_body;
        /* Also upgrade body_strong/small/caption to FreeType if same file */
        lv_font_t* ft_strong = try_load_freetype(paths.body_semibold, SZ_H3, LV_FREETYPE_FONT_STYLE_BOLD);
        if (ft_strong) {
            ft_strong->fallback = g_theme_fonts.body_strong;
            g_theme_fonts.body_strong = ft_strong;
        }
        lv_font_t* ft_small = try_load_freetype(paths.body_regular, SZ_SMALL, LV_FREETYPE_FONT_STYLE_NORMAL);
        if (ft_small) {
            ft_small->fallback = g_theme_fonts.small;
            g_theme_fonts.small = ft_small;
        }
        lv_font_t* ft_caption = try_load_freetype(paths.body_regular, SZ_CAPTION, LV_FREETYPE_FONT_STYLE_NORMAL);
        if (ft_caption) {
            ft_caption->fallback = g_theme_fonts.caption;
            g_theme_fonts.caption = ft_caption;
        }
    }
    if (ft_bold) {
        ft_bold->fallback = g_theme_fonts.h1;
        g_theme_fonts.freetype_title = ft_bold;
        g_theme_fonts.h1 = ft_bold;
        /* Also upgrade display/h2 with FreeType bold/semibold */
        lv_font_t* ft_display = try_load_freetype(paths.body_bold, SZ_DISPLAY, LV_FREETYPE_FONT_STYLE_BOLD);
        if (ft_display) {
            ft_display->fallback = g_theme_fonts.display;
            g_theme_fonts.display = ft_display;
        }
        lv_font_t* ft_h2 = try_load_freetype(paths.body_semibold, SZ_H2, LV_FREETYPE_FONT_STYLE_BOLD);
        if (ft_h2) {
            ft_h2->fallback = g_theme_fonts.h2;
            g_theme_fonts.h2 = ft_h2;
        }
    }
    /* FreeType code font */
    lv_font_t* ft_code = try_load_freetype(paths.code_font, SZ_CODE, LV_FREETYPE_FONT_STYLE_NORMAL);
    if (ft_code) {
        ft_code->fallback = g_theme_fonts.code;
        g_theme_fonts.code = ft_code;
    }

    /* Re-apply fallback chain with FreeType as primary → TinyTTF → CJK embedded */
    if (g_theme_fonts.body && g_theme_fonts.cjk_body && g_theme_fonts.body != g_theme_fonts.cjk_body)
        g_theme_fonts.body->fallback = g_theme_fonts.cjk_body;
    if (g_theme_fonts.body_strong && g_theme_fonts.cjk_body && g_theme_fonts.body_strong != g_theme_fonts.cjk_body)
        g_theme_fonts.body_strong->fallback = g_theme_fonts.cjk_body;
    if (g_theme_fonts.h1 && g_theme_fonts.cjk_title && g_theme_fonts.h1 != g_theme_fonts.cjk_title)
        g_theme_fonts.h1->fallback = g_theme_fonts.cjk_title;
    if (g_theme_fonts.h2 && g_theme_fonts.cjk_title && g_theme_fonts.h2 != g_theme_fonts.cjk_title)
        g_theme_fonts.h2->fallback = g_theme_fonts.cjk_title;
    if (g_theme_fonts.display && g_theme_fonts.cjk_title && g_theme_fonts.display != g_theme_fonts.cjk_title)
        g_theme_fonts.display->fallback = g_theme_fonts.cjk_title;

    /* Legacy globals: prioritize CJK fonts to avoid garbled Chinese text on some stacks */
    g_cjk_font = g_theme_fonts.cjk_body ? g_theme_fonts.cjk_body : g_theme_fonts.body;
    g_cjk_font_small = g_theme_fonts.small ? g_theme_fonts.small : g_cjk_font;
    g_cjk_font_chat = g_theme_fonts.cjk_body ? g_theme_fonts.cjk_body : g_cjk_font;

    LOG_I("Font", "Theme fonts initialized for theme %d (FreeType %s)",
          ti, (ft_body || ft_bold) ? "active" : "unavailable");
}

/* ═══════════════════════════════════════════════════════════════
 *  apply_btn_gradient — apply theme gradient to a button (G2/G3)
 *  If btn_grad_enable=1, sets horizontal gradient from start→end.
 *  If btn_grad_enable=0, does nothing (solid color via bg_color).
 * ═══════════════════════════════════════════════════════════════ */
static void apply_btn_gradient(lv_obj_t* btn) {
    if (!btn || !g_colors || !g_colors->btn_grad_enable) return;
    lv_obj_set_style_bg_grad_color(btn, g_colors->btn_grad_end, 0);
    lv_obj_set_style_bg_grad_dir(btn, LV_GRAD_DIR_HOR, 0);
    lv_obj_set_style_bg_grad_stop(btn, 255, 0);
}

#define CJK_FONT (g_theme_fonts.body ? g_theme_fonts.body : g_cjk_font)
#define CJK_FONT_SMALL (g_theme_fonts.small ? g_theme_fonts.small : g_cjk_font_small)
#define CJK_FONT_CHAT (g_theme_fonts.body ? g_theme_fonts.body : (g_cjk_font_chat ? g_cjk_font_chat : g_cjk_font))

/* ═══ Unified font size macros ═══ */
/* All UI text uses these macros. Change once, apply everywhere. */
#define FONT_SIZE_DEFAULT   16   /* Default body text / buttons / labels */
#define FONT_SIZE_SMALL     12   /* Hints, timestamps, dim text */
#define FONT_SIZE_TITLE     20   /* Panel titles, section headers */
#define FONT_SIZE_LARGE     24   /* Big headings, about page */
#define FONT_SIZE_ICON      14   /* Symbol icons (LV_SYMBOL_*) */

#define FONT_DEFAULT    CJK_FONT                /* Main body font */
#define FONT_SMALL      CJK_FONT_SMALL          /* Small/hint font */
#define FONT_TITLE      CJK_FONT                /* Title font (same family, same size) */
#define FONT_ICON       FONT(FONT_SIZE_ICON)    /* Icon/symbol font */

/* Font helper — DPI-scaled font selection (Montserrat bitmap) */
static const lv_font_t* FONT(int base_px) {
    /* Use DPI-scaled size for font selection */
    int sz = base_px * app_get_dpi_scale() / 100;
    if (sz <= 12) return &lv_font_montserrat_10;
    if (sz <= 14) return &lv_font_montserrat_12;
    if (sz <= 16) return &lv_font_montserrat_14;
    if (sz <= 18) return &lv_font_montserrat_16;
    if (sz <= 20) return &lv_font_montserrat_18;
    if (sz <= 22) return &lv_font_montserrat_20;
    if (sz <= 24) return &lv_font_montserrat_22;
    if (sz <= 26) return &lv_font_montserrat_24;
    if (sz <= 28) return &lv_font_montserrat_26;
    if (sz <= 30) return &lv_font_montserrat_28;
    if (sz <= 32) return &lv_font_montserrat_30;
    if (sz <= 36) return &lv_font_montserrat_32;
    if (sz <= 40) return &lv_font_montserrat_36;
    if (sz <= 48) return &lv_font_montserrat_40;
    return &lv_font_montserrat_48;  /* Max available */
}

/* Layout constants — base values (96dpi). DPI-scaled in app_ui_init(). */
static int WIN_W = 1088;    /* Updated at runtime in app_ui_init() */
static int WIN_H = 680;     /* Updated at runtime in app_ui_init() */
static int TITLE_H = TITLE_BAR_H_BASE;
static int PANEL_GAP = PANEL_GAP_BASE;
static int SPLITTER_W = SPLITTER_W_BASE;
static int GAP = GAP_BASE;
static int CHAT_GAP = CHAT_GAP_BASE;
static int CHAT_MSG_MARGIN = CHAT_MSG_MARGIN_BASE;
static int LEFT_PANEL_W = 240; /* Overwritten by pct calc in init */
static int RIGHT_PANEL_W = 830;
static int PANEL_TOP = 56;
static int PANEL_H = 450;
static int FOOTER_H = 30;

/* ═══ Model & API Key globals ═══ */
char g_selected_model[256] = {0};   /* Currently selected model name */
char g_api_key[256] = {0};          /* OpenRouter API key */

/* Safe zero wrappers for cross-unit cleanup (avoid sizeof on extern globals) */
void app_secure_zero_sensitive() {
    secure_zero(g_api_key, sizeof(g_api_key));
    secure_zero(g_selected_model, sizeof(g_selected_model));
}
enum ControlMode { CONTROL_USER = 0, CONTROL_AI = 1 };
enum LlmAccessMode { LLM_GATEWAY = 0, LLM_DIRECT_API = 1 };
enum AiInteractionMode { AIMODE_AUTONOMOUS = 0, AIMODE_ASK = 1, AIMODE_PLAN = 2 };
static ControlMode g_control_mode = CONTROL_USER;
static LlmAccessMode g_llm_access_mode = LLM_GATEWAY;
static AiInteractionMode g_ai_interaction_mode = AIMODE_AUTONOMOUS;
static bool g_proactive_startup_enabled = true;
static bool g_proactive_idle_enabled = true;
static bool g_proactive_memory_enabled = true;
static bool g_proactive_summary_enabled = true;
static DWORD g_proactive_last_idle_action_tick = 0;
static int g_proactive_idle_threshold_minutes = 30;
static int g_proactive_last_summary_yday = -1;
static int g_proactive_last_memory_yday = -1;
static bool g_work_show_advanced = false;
static char g_profile_user_name[64] = "User";
static char g_profile_user_role[64] = "Owner";
static char g_profile_user_persona[128] = "";
static char g_profile_ai_name[64] = "AI";
static char g_profile_ai_role[64] = "Assistant";
static char g_profile_ai_persona[128] = "";
static char g_profile_ai_skills[192] = "";
static char g_profile_user_avatar[64] = "garlic";
static bool g_remote_guard_armed = false;
bool g_app_auth_email = false;
bool g_app_auth_calendar = false;
static char g_profile_ai_avatar[64] = "lobster";
static bool g_gemma_install_opt_in = false;
/* bit0=2B, bit1=9B, bit2=27B (multi-select) */
static int g_gemma_model_mask = 0;
static std::atomic<bool> g_gemma_install_started(false);
static uint32_t g_wiz_gemma_progress_start_ms = 0;

/* ═══ Theme Helpers ═══ */
/* Blend a base color with a tint color at ~12% opacity (for toast bg etc.) */
static lv_color_t blend_with_tint(lv_color_t base, lv_color_t tint) {
    return lv_color_make(
        (uint8_t)((base.red * 224u + tint.red * 32u) >> 8),
        (uint8_t)((base.green * 224u + tint.green * 32u) >> 8),
        (uint8_t)((base.blue * 224u + tint.blue * 32u) >> 8)
    );
}

/* ═══ Theme System (P2-4) ═══ */
Theme g_theme = Theme::Matcha;
static const ThemeColors THEME_DARK = {
    /* ══ 🍵 Matcha v1 (default) ══ */
    /* ── Backgrounds ── */
    /* bg */                  {0x0F, 0x11, 0x17},
    /* surface */             {0x13, 0x15, 0x1E},
    /* panel */               {0x1A, 0x1D, 0x27},
    /* raised */              {0x1E, 0x22, 0x30},
    /* overlay */             {0x25, 0x2A, 0x38},
    /* ── Text ── */
    /* text */                {0xE8, 0xEC, 0xF4},
    /* text_dim */            {0x8B, 0x92, 0xA8},
    /* text_hint */           {0x6B, 0x72, 0x80},
    /* text_inverse */        {0x0F, 0x11, 0x17},
    /* ── Accent ── */
    /* accent */              {0x3D, 0xD6, 0x8C},
    /* accent_hover */        {0x2B, 0xB6, 0x73},
    /* accent_active */       {0x1F, 0xA3, 0x5E},
    /* accent_subtle */       {0x3D, 0xD6, 0x8C},   /* +10% opacity applied at use site */
    /* accent_secondary */    {0x6C, 0x9F, 0xFF},
    /* ── Accent tertiary ── */
    /* accent_tertiary */     {0x00, 0x00, 0x00},   /* Matcha: not used */
    /* ── Semantic ── */
    /* success */             {0x3D, 0xD6, 0x8C},
    /* warning */             {0xFF, 0xBE, 0x3D},
    /* danger */              {0xFF, 0x6B, 0x6B},
    /* info */                {0x6C, 0x9F, 0xFF},
    /* ── Functional ── */
    /* border */              {0x3D, 0xD6, 0x8C},   /* +0A opacity at use site */
    /* border_strong */       {0x3D, 0xD6, 0x8C},   /* +20 opacity at use site */
    /* divider */             {0x3D, 0xD6, 0x8C},   /* +08 opacity at use site */
    /* focus_glow */          {0x3D, 0xD6, 0x8C},   /* +30 opacity at use site */
    /* hover_overlay */       {0xFF, 0xFF, 0xFF},   /* +0A opacity at use site */
    /* active_overlay */      {0xFF, 0xFF, 0xFF},   /* +14 opacity at use site */
    /* disabled_bg */         {0x1A, 0x1D, 0x27},   /* +80 opacity at use site */
    /* disabled_text */       {0x6B, 0x72, 0x80},   /* +80 opacity at use site */
    /* ── Bubble ── */
    /* bubble_user_bg */      {0x2B, 0x5C, 0x3E},
    /* bubble_user_bg_end */  {0x1F, 0x4A, 0x30},
    /* bubble_ai_bg */        {0x1E, 0x22, 0x30},
    /* bubble_ai_accent_bar */{0x3D, 0xD6, 0x8C},
    /* ── Buttons ── */
    /* btn_action */          {0x3D, 0xD6, 0x8C},
    /* btn_secondary */       {0x2B, 0x31, 0x40},
    /* btn_close */           {0xFF, 0x6B, 0x6B},
    /* btn_add */             {0x2B, 0xB6, 0x73},
    /* ── Log ── */
    /* log_bg */              {0x12, 0x14, 0x1B},
    /* log_text */            {0xB6, 0xF2, 0xCC},
    /* ── Shadow colors ── */
    /* shadow_sm */           {0x00, 0x00, 0x00},
    /* shadow_md */           {0x00, 0x00, 0x00},
    /* shadow_lg */           {0x00, 0x00, 0x00},
    /* shadow_glow */         {0x3D, 0xD6, 0x8C},
    /* ── Opacity ── */
    /* mask_opacity */        153,  /* 60% */
    /* toast_opacity */       230,
    /* loading_opacity */     204,  /* 80% */
    /* ── Structural ── */
    /* btn_grad_enable */     1,
    /* icon_stroke_width */   2,
    /* btn_grad_start */      {0x3D, 0xD6, 0x8C},  /* #3DD68C accent */
    /* btn_grad_end */        {0x2B, 0xB6, 0x73},  /* #2BB673 accent_hover */
    /* ── Radius ── */
    /* radius_sm */           4,
    /* radius_md */           8,
    /* radius_lg */           12,
    /* radius_xl */           16,
    /* radius_2xl */          20,
};

static const ThemeColors THEME_PEACHY = {
    /* ══ 🍑 Peachy v2 ══ */
    /* ── Backgrounds ── */
    /* bg */                  {0xFF, 0xF8, 0xF3},
    /* surface */             {0xFF, 0xF1, 0xE8},
    /* panel */               {0xFF, 0xFF, 0xFF},
    /* raised */              {0xFF, 0xF5, 0xEE},
    /* overlay */             {0xFF, 0xE8, 0xD6},
    /* ── Text ── */
    /* text */                {0x2D, 0x1B, 0x14},
    /* text_dim */            {0x8B, 0x73, 0x55},
    /* text_hint */           {0xB8, 0xA0, 0x89},
    /* text_inverse */        {0xFF, 0xFF, 0xFF},
    /* ── Accent ── */
    /* accent */              {0xFF, 0x7F, 0x50},
    /* accent_hover */        {0xE8, 0x6D, 0x3C},
    /* accent_active */       {0xCC, 0x5A, 0x2A},
    /* accent_subtle */       {0xFF, 0x7F, 0x50},   /* +1A opacity at use site */
    /* accent_secondary */    {0xFF, 0xB3, 0x47},
    /* ── Accent tertiary ── */
    /* accent_tertiary */     {0xFF, 0x6B, 0x8A},
    /* ── Semantic ── */
    /* success */             {0x7E, 0xCF, 0xB3},
    /* warning */             {0xFF, 0xB3, 0x47},
    /* danger */              {0xFF, 0x5C, 0x5C},
    /* info */                {0xB8, 0xA9, 0xE8},
    /* ── Functional ── */
    /* border */              {0xF0, 0xE0, 0xD0},
    /* border_strong */       {0xE0, 0xC8, 0xB0},
    /* divider */             {0xF5, 0xE6, 0xD8},
    /* focus_glow */          {0xFF, 0x7F, 0x50},   /* +30 opacity at use site */
    /* hover_overlay */       {0xFF, 0x7F, 0x50},   /* +0A opacity at use site */
    /* active_overlay */      {0xFF, 0x7F, 0x50},   /* +14 opacity at use site */
    /* disabled_bg */         {0xFF, 0xF1, 0xE8},   /* +80 opacity at use site */
    /* disabled_text */       {0xB8, 0xA0, 0x89},   /* +80 opacity at use site */
    /* ── Bubble ── */
    /* bubble_user_bg */      {0xFF, 0xE8, 0xD6},
    /* bubble_user_bg_end */  {0xFF, 0xD6, 0xBE},
    /* bubble_ai_bg */        {0xFF, 0xFF, 0xFF},
    /* bubble_ai_accent_bar */{0xFF, 0x7F, 0x50},
    /* ── Buttons ── */
    /* btn_action */          {0xFF, 0x7F, 0x50},
    /* btn_secondary */       {0xF7, 0xE8, 0xDF},
    /* btn_close */           {0xE5, 0x64, 0x64},
    /* btn_add */             {0xF5, 0x8A, 0x62},
    /* ── Log ── */
    /* log_bg */              {0xFF, 0xF1, 0xE8},
    /* log_text */            {0x9A, 0x4E, 0x32},
    /* ── Shadow colors ── */
    /* shadow_sm */           {0x8B, 0x50, 0x32},   /* warm brown */
    /* shadow_md */           {0x8B, 0x50, 0x32},
    /* shadow_lg */           {0x8B, 0x50, 0x32},
    /* shadow_glow */         {0xFF, 0x7F, 0x50},
    /* ── Opacity ── */
    /* mask_opacity */        77,   /* 30% */
    /* toast_opacity */       240,
    /* loading_opacity */     217,  /* 85% */
    /* ── Structural ── */
    /* btn_grad_enable */     1,
    /* icon_stroke_width */   2,
    /* btn_grad_start */      {0xFF, 0x7F, 0x50},  /* #FF7F50 accent */
    /* btn_grad_end */        {0xFF, 0xB3, 0x47},  /* #FFB347 warm orange */
    /* ── Radius ── */
    /* radius_sm */           6,
    /* radius_md */           12,
    /* radius_lg */           20,
    /* radius_xl */           22,
    /* radius_2xl */          28,
};

static const ThemeColors THEME_CLASSIC = {
    /* ══ 🌑 经典暗色 VS Code 风 ══ */
    /* ── Backgrounds ── */
    /* bg */                  {0x1E, 0x1E, 0x1E},
    /* surface */             {0x25, 0x25, 0x26},
    /* panel */               {0x2D, 0x2D, 0x30},
    /* raised */              {0x38, 0x38, 0x38},
    /* overlay */             {0x40, 0x40, 0x40},
    /* ── Text ── */
    /* text */                {0xCC, 0xCC, 0xCC},
    /* text_dim */            {0x80, 0x80, 0x80},
    /* text_hint */           {0x55, 0x55, 0x55},
    /* text_inverse */        {0x1E, 0x1E, 0x1E},
    /* ── Accent ── */
    /* accent */              {0x00, 0x7A, 0xCC},
    /* accent_hover */        {0x1A, 0x8A, 0xD8},
    /* accent_active */       {0x00, 0x60, 0xA8},
    /* accent_subtle */       {0x00, 0x7A, 0xCC},   /* +18 opacity at use site */
    /* accent_secondary */    {0x4F, 0xC1, 0xFF},
    /* ── Accent tertiary ── */
    /* accent_tertiary */     {0xC5, 0x86, 0xC0},   /* VS Code purple/pink */
    /* ── Semantic ── */
    /* success */             {0x4E, 0xC9, 0xB0},
    /* warning */             {0xCE, 0x91, 0x78},
    /* danger */              {0xF4, 0x43, 0x36},
    /* info */                {0x56, 0x9C, 0xD6},
    /* ── Functional ── */
    /* border */              {0x3E, 0x3E, 0x42},
    /* border_strong */       {0x55, 0x55, 0x55},
    /* divider */             {0x33, 0x33, 0x33},
    /* focus_glow */          {0x00, 0x7A, 0xCC},   /* +30 opacity at use site */
    /* hover_overlay */       {0xFF, 0xFF, 0xFF},   /* +0A opacity at use site */
    /* active_overlay */      {0xFF, 0xFF, 0xFF},   /* +14 opacity at use site */
    /* disabled_bg */         {0x2D, 0x2D, 0x30},   /* +80 opacity at use site */
    /* disabled_text */       {0x55, 0x55, 0x55},   /* +80 opacity at use site */
    /* ── Bubble ── */
    /* bubble_user_bg */      {0x26, 0x4F, 0x78},
    /* bubble_user_bg_end */  {0x1E, 0x40, 0x65},
    /* bubble_ai_bg */        {0x2D, 0x2D, 0x30},
    /* bubble_ai_accent_bar */{0x00, 0x7A, 0xCC},
    /* ── Buttons ── */
    /* btn_action */          {0x00, 0x7A, 0xCC},
    /* btn_secondary */       {0x3E, 0x3E, 0x42},
    /* btn_close */           {0xF4, 0x43, 0x36},
    /* btn_add */             {0x1A, 0x8A, 0xD8},
    /* ── Log ── */
    /* log_bg */              {0x1E, 0x1E, 0x1E},
    /* log_text */            {0x4E, 0xC9, 0xB0},
    /* ── Shadow colors ── */
    /* shadow_sm */           {0x00, 0x00, 0x00},
    /* shadow_md */           {0x00, 0x00, 0x00},
    /* shadow_lg */           {0x00, 0x00, 0x00},
    /* shadow_glow */         {0x00, 0x7A, 0xCC},
    /* ── Opacity ── */
    /* mask_opacity */        153,  /* 60% */
    /* toast_opacity */       217,
    /* loading_opacity */     204,  /* 80% */
    /* ── Structural ── */
    /* btn_grad_enable */     0,
    /* icon_stroke_width */   2,
    /* btn_grad_start */      {0x00, 0x78, 0xD4},  /* #0078D4 accent (solid mode) */
    /* btn_grad_end */        {0x1A, 0x8A, 0xD8},  /* #1A8AD8 accent_hover */
    /* ── Radius ── */
    /* radius_sm */           4,
    /* radius_md */           8,
    /* radius_lg */           12,
    /* radius_xl */           16,
    /* radius_2xl */          20,
};

static const ThemeColors THEME_MOCHI = {
    /* ══ 🍡 Mochi v3 ══ */
    /* ── Backgrounds ── */
    /* bg */                  {0xFA, 0xF6, 0xF0},
    /* surface */             {0xF3, 0xED, 0xE4},
    /* panel */               {0xFF, 0xFD, 0xF9},
    /* raised */              {0xED, 0xE7, 0xDC},
    /* overlay */             {0xE4, 0xDD, 0xD0},
    /* ── Text ── */
    /* text */                {0x3D, 0x32, 0x26},
    /* text_dim */            {0x8B, 0x7D, 0x6B},
    /* text_hint */           {0xB0, 0xA3, 0x94},
    /* text_inverse */        {0xFF, 0xFD, 0xF9},
    /* ── Accent ── */
    /* accent */              {0xA6, 0x7B, 0x5B},
    /* accent_hover */        {0x8F, 0x6A, 0x4D},
    /* accent_active */       {0x7A, 0x5A, 0x40},
    /* accent_subtle */       {0xA6, 0x7B, 0x5B},   /* +08 opacity at use site */
    /* accent_secondary */    {0xC9, 0xA9, 0x6E},
    /* ── Accent tertiary ── */
    /* accent_tertiary */     {0xC4, 0x86, 0x8C},
    /* ── Semantic ── */
    /* success */             {0x7E, 0xA8, 0x7A},
    /* warning */             {0xC9, 0xA9, 0x6E},
    /* danger */              {0xC4, 0x70, 0x70},
    /* info */                {0x7B, 0x98, 0xA6},
    /* ── Functional ── */
    /* border */              {0xD6, 0xCF, 0xC4},
    /* border_strong */       {0xC4, 0xBB, 0xAD},
    /* divider */             {0xE8, 0xE2, 0xD8},
    /* focus_glow */          {0xA6, 0x7B, 0x5B},   /* +25 opacity at use site */
    /* hover_overlay */       {0xA6, 0x7B, 0x5B},   /* +08 opacity at use site */
    /* active_overlay */      {0xA6, 0x7B, 0x5B},   /* +12 opacity at use site */
    /* disabled_bg */         {0xF3, 0xED, 0xE4},   /* +80 opacity at use site */
    /* disabled_text */       {0xB0, 0xA3, 0x94},   /* +80 opacity at use site */
    /* ── Bubble ── */
    /* bubble_user_bg */      {0xE8, 0xDF, 0xD0},
    /* bubble_user_bg_end */  {0xDD, 0xD2, 0xC0},
    /* bubble_ai_bg */        {0xFF, 0xFD, 0xF9},
    /* bubble_ai_accent_bar */{0xA6, 0x7B, 0x5B},
    /* ── Buttons ── */
    /* btn_action */          {0xA6, 0x7B, 0x5B},
    /* btn_secondary */       {0xE4, 0xDD, 0xD0},
    /* btn_close */           {0xC4, 0x70, 0x70},
    /* btn_add */             {0x8F, 0x6A, 0x4D},
    /* ── Log ── */
    /* log_bg */              {0x2A, 0x24, 0x1D},
    /* log_text */            {0xC9, 0xA9, 0x6E},
    /* ── Shadow colors ── */
    /* shadow_sm */           {0x78, 0x64, 0x50},   /* warm brown */
    /* shadow_md */           {0x78, 0x64, 0x50},
    /* shadow_lg */           {0x78, 0x64, 0x50},
    /* shadow_glow */         {0xA6, 0x7B, 0x5B},
    /* ── Opacity ── */
    /* mask_opacity */        89,   /* 35% */
    /* toast_opacity */       230,
    /* loading_opacity */     200,  /* 78% */
    /* ── Structural ── */
    /* btn_grad_enable */     0,
    /* icon_stroke_width */   1,
    /* btn_grad_start */      {0xA6, 0x7B, 0x5B},  /* #A67B5B accent (solid mode) */
    /* btn_grad_end */        {0x8F, 0x6A, 0x4D},  /* #8F6A4D accent_hover */
    /* ── Radius ── */
    /* radius_sm */           4,
    /* radius_md */           8,
    /* radius_lg */           10,
    /* radius_xl */           14,
    /* radius_2xl */          16,
};

static const ThemeColors THEME_LIGHT = {
    /* ══ ☀️ 亮色 Light ══ */
    /* ── Backgrounds ── */
    /* bg */                  {0xF5, 0xF7, 0xFA},
    /* surface */             {0xEB, 0xEE, 0xF2},
    /* panel */               {0xFF, 0xFF, 0xFF},
    /* raised */              {0xFF, 0xFF, 0xFF},
    /* overlay */             {0xE2, 0xE5, 0xEA},
    /* ── Text ── */
    /* text */                {0x1A, 0x1D, 0x27},
    /* text_dim */            {0x6B, 0x72, 0x80},
    /* text_hint */           {0x9C, 0xA3, 0xAF},
    /* text_inverse */        {0xFF, 0xFF, 0xFF},
    /* ── Accent ── */
    /* accent */              {0x2B, 0xB6, 0x73},
    /* accent_hover */        {0x22, 0x99, 0x60},
    /* accent_active */       {0x1A, 0x7D, 0x4D},
    /* accent_subtle */       {0x2B, 0xB6, 0x73},   /* +12 opacity at use site */
    /* accent_secondary */    {0x3B, 0x82, 0xF6},
    /* ── Accent tertiary ── */
    /* accent_tertiary */     {0xEC, 0x48, 0x99},   /* pink accent */
    /* ── Semantic ── */
    /* success */             {0x16, 0xA3, 0x4A},
    /* warning */             {0xD9, 0x77, 0x06},
    /* danger */              {0xDC, 0x26, 0x26},
    /* info */                {0x3B, 0x82, 0xF6},
    /* ── Functional ── */
    /* border */              {0xE2, 0xE5, 0xEA},
    /* border_strong */       {0xD1, 0xD5, 0xDB},
    /* divider */             {0xF1, 0xF3, 0xF5},
    /* focus_glow */          {0x2B, 0xB6, 0x73},   /* +30 opacity at use site */
    /* hover_overlay */       {0x00, 0x00, 0x00},   /* +08 opacity at use site */
    /* active_overlay */      {0x00, 0x00, 0x00},   /* +12 opacity at use site */
    /* disabled_bg */         {0xEB, 0xEE, 0xF2},   /* +80 opacity at use site */
    /* disabled_text */       {0x9C, 0xA3, 0xAF},   /* +80 opacity at use site */
    /* ── Bubble ── */
    /* bubble_user_bg */      {0xDC, 0xFC, 0xE7},
    /* bubble_user_bg_end */  {0xBB, 0xF7, 0xD0},
    /* bubble_ai_bg */        {0xF1, 0xF5, 0xF9},
    /* bubble_ai_accent_bar */{0x2B, 0xB6, 0x73},
    /* ── Buttons ── */
    /* btn_action */          {0x2B, 0xB6, 0x73},
    /* btn_secondary */       {0xF1, 0xF3, 0xF5},
    /* btn_close */           {0xDC, 0x26, 0x26},
    /* btn_add */             {0x22, 0x99, 0x60},
    /* ── Log ── */
    /* log_bg */              {0x1A, 0x1D, 0x27},
    /* log_text */            {0x4A, 0xDE, 0x80},
    /* ── Shadow colors ── */
    /* shadow_sm */           {0x00, 0x00, 0x00},
    /* shadow_md */           {0x00, 0x00, 0x00},
    /* shadow_lg */           {0x00, 0x00, 0x00},
    /* shadow_glow */         {0x2B, 0xB6, 0x73},
    /* ── Opacity ── */
    /* mask_opacity */        51,   /* 20% */
    /* toast_opacity */       240,
    /* loading_opacity */     217,  /* 85% */
    /* ── Structural ── */
    /* btn_grad_enable */     0,
    /* icon_stroke_width */   2,
    /* btn_grad_start */      {0x2E, 0xCC, 0x71},  /* #2ECC71 accent (solid mode) */
    /* btn_grad_end */        {0x22, 0x99, 0x60},  /* #229960 accent_hover */
    /* ── Radius ── */
    /* radius_sm */           4,
    /* radius_md */           8,
    /* radius_lg */           12,
    /* radius_xl */           16,
    /* radius_2xl */          20,
};

const ThemeColors* g_colors = &THEME_DARK;

/* ═══ Exit confirmation state ═══ */
bool g_exit_confirm_enabled = true;

bool is_exit_confirmation_enabled() {
    return g_exit_confirm_enabled;
}

void set_exit_confirmation(bool enabled) {
    g_exit_confirm_enabled = enabled;
}

/* ═══ DPI scale state (100, 125, 150) ═══ */
int g_dpi_scale = 100;

int app_get_dpi_scale() {
    return g_dpi_scale;
}

void app_set_dpi_scale(int scale_percent) {
    if (scale_percent < 100) scale_percent = 100;
    if (scale_percent > 250) scale_percent = 250;
    g_dpi_scale = scale_percent;

    /* Layout constants scaled by DPI */
    TITLE_H = SCALE(48); FOOTER_H = SCALE(30); PANEL_GAP = SCALE(12); SPLITTER_W = SCALE(6);
    GAP = SCALE(12); CHAT_GAP = SCALE(6); CHAT_MSG_MARGIN = SCALE(8); PANEL_TOP = TITLE_H + SCALE(8);

    /* Only save config if window exists (skip during startup before window creation) */
    if (app_get_window()) {
        save_theme_config();
    }
    ui_log("[DPI] Scale set to %d%%", scale_percent);
}

/* P2-01: Language mode - PRD 2.1 single language only, auto-detect system */
static Lang detect_system_lang() {
    /* P2-01: Auto-detect system language via Windows API */
    wchar_t buf[LOCALE_NAME_MAX_LENGTH] = {};
    if (GetSystemDefaultLocaleName(buf, LOCALE_NAME_MAX_LENGTH)) {
        if (wcsncmp(buf, L"zh", 2) == 0) return Lang::CN;
    }
    LANGID lid = GetSystemDefaultUILanguage();
    if (PRIMARYLANGID(lid) == LANG_CHINESE) return Lang::CN;
    return Lang::EN;
}
Lang g_lang = detect_system_lang();

/* Forward declarations for P2-03/P2-04 persistence */
extern int g_refresh_interval_ms;
extern lv_timer_t* g_refresh_timer;
static bool is_wizard_completed();

static std::string get_userdata_dir() {
    const char* appdata = std::getenv("APPDATA");
    if (appdata) {
        std::string dir = std::string(appdata) + "\\AnyClaw_LVGL";
        std::filesystem::create_directories(dir);
        return dir;
    }
    return ".";
}

static std::string get_config_path() {
    return get_userdata_dir() + "\\config.json";
}

static std::string get_chat_history_path() {
    return get_userdata_dir() + "\\chat_history.json";
}

/* Forward declarations used by early chat-history helpers */
static lv_obj_t* chat_cont = nullptr;              /* Chat container */
static char chat_history[CHAT_HISTORY_MAX] = {0};  /* Buffered plain-text history */
static void chat_add_user_bubble(const char* text);
static void chat_add_ai_bubble(const char* text);
static void chat_force_scroll_bottom();
static void chat_start_stream(const char* user_message);
static void update_send_button_state();
static bool is_streaming_now();

/* ═══ Chat History Persistence ═══ */

/* Simple JSON array writer for chat messages.
 * Format: [{"role":"user","text":"...","ts":"..."}, ...]
 * Max CHAT_PERSIST_MAX (50) messages, oldest trimmed.
 */
#include <vector>
struct ChatMessage {
    std::string role;  /* "user" or "ai" */
    std::string text;
    std::string ts;    /* "YYYY-MM-DD HH:MM:SS" */
};
static std::vector<ChatMessage> g_chat_messages;

/* ═══ Chat Search State ═══ */
static lv_obj_t* g_search_bar = nullptr;       /* Search bar container (hidden by default) */
static lv_obj_t* g_search_input = nullptr;      /* Search text input */
static lv_obj_t* g_search_btn = nullptr;        /* Toggle search button */
static lv_obj_t* g_search_info = nullptr;       /* "3 of 12" results label */
static bool g_search_visible = false;
static std::vector<int> g_search_results;       /* Indices into chat_cont children */
static int g_search_current = -1;               /* Current result index */

static void search_toggle_cb(lv_event_t* e);    /* Forward */
static void search_execute_cb(lv_event_t* e);   /* Forward */
static void search_prev_cb(lv_event_t* e);      /* Forward */
static void search_next_cb(lv_event_t* e);      /* Forward */
static void relayout_panels();                  /* Forward */
static void apply_mode_switch_visuals();
static void apply_nav_module_visuals();
static void update_main_title();
static void refresh_tasks_module_data(bool run_pending);
static void refresh_files_module_data(bool scan_workspace);
static void run_tasks_template(const char* template_name, const char* prompt);
static void work_send_cb(lv_event_t* e);
static void work_prompt_input_cb(lv_event_t* e);
static void chat_send_cb(lv_event_t* e);
static void work_chat_toggle_cb(lv_event_t* e);
static void work_chat_send_cb(lv_event_t* e);
static void work_trace_refresh_cb(lv_event_t* e);
static void work_trace_toggle_sort_cb(lv_event_t* e);
static void work_trace_toggle_fail_filter_cb(lv_event_t* e);
static void work_retry_failed_attachments_cb(lv_event_t* e);
static void work_boot_open_report_cb(lv_event_t* e);
static void work_remote_disconnect_cb(lv_event_t* e);
static void work_lan_start_host_cb(lv_event_t* e);
static void work_lan_connect_cb(lv_event_t* e);
static void work_lan_discover_cb(lv_event_t* e);
static void work_lan_send_global_cb(lv_event_t* e);
static void work_lan_send_private_cb(lv_event_t* e);
static void work_lan_create_group_cb(lv_event_t* e);
static void work_lan_join_group_cb(lv_event_t* e);
static void work_lan_send_group_cb(lv_event_t* e);
static void work_ftp_upload_cb(lv_event_t* e);
static void work_ftp_download_cb(lv_event_t* e);
static void work_ftp_cancel_cb(lv_event_t* e);
static void work_ftp_retry_cb(lv_event_t* e);
static void work_kb_add_source_cb(lv_event_t* e);
static void work_kb_import_dir_cb(lv_event_t* e);
static void work_kb_export_manifest_cb(lv_event_t* e);
static void work_kb_search_cb(lv_event_t* e);
static void work_cron_refresh_cb(lv_event_t* e);
static void work_cron_run_cb(lv_event_t* e);
static void work_cron_enable_cb(lv_event_t* e);
static void work_cron_disable_cb(lv_event_t* e);
static void work_cron_delete_cb(lv_event_t* e);
static lv_obj_t* create_dialog(lv_obj_t* parent, const char* title, int w, int h, lv_obj_t** out_overlay);
static void work_add_step_card(const char* action, const char* detail, bool done, bool write_op);
static void work_append_md_block(const char* title, const char* text);

/* Forward declarations for globals used by config persistence before their definitions. */
static bool g_work_chat_collapsed = false;
static bool g_work_chat_collapsed_pref = false;
static bool g_c2_dual_result = true;
static bool g_wizard_leader_mode = true;
static Runtime g_wizard_active_runtime = Runtime::OpenClaw;
static bool g_wizard_hermes_enabled = false;
static char g_wizard_claude_code_path[260] = {0};

/* Escape a string for JSON (handles \, ", newlines) */
static std::string json_escape(const char* s) {
    std::string out;
    for (const char* p = s; *p; p++) {
        switch (*p) {
            case '"':  out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\n': out += "\\n"; break;
            case '\r': out += "\\r"; break;
            case '\t': out += "\\t"; break;
            default:   out += *p; break;
        }
    }
    return out;
}

/* Save all chat messages to disk (overwrite) */
static void save_chat_history_to_disk() {
    std::string path = get_chat_history_path();
    std::ofstream f(path);
    if (!f.is_open()) return;

    f << "[\n";
    for (size_t i = 0; i < g_chat_messages.size(); i++) {
        const auto& m = g_chat_messages[i];
        f << "  {\"role\":\"" << m.role << "\",\"text\":\"" << json_escape(m.text.c_str())
          << "\",\"ts\":\"" << m.ts << "\"}";
        if (i + 1 < g_chat_messages.size()) f << ",";
        f << "\n";
    }
    f << "]\n";
    f.close();
}

/* Append one message to chat history and persist */
static void persist_chat_message(const char* role, const char* text, const char* ts) {
    g_chat_messages.push_back({role, text, ts});

    /* Trim to max */
    while ((int)g_chat_messages.size() > CHAT_PERSIST_MAX) {
        g_chat_messages.erase(g_chat_messages.begin());
    }

    save_chat_history_to_disk();
}

/* Load chat history from disk and restore bubbles in UI */
static void load_chat_history() {
    std::string path = get_chat_history_path();
    std::ifstream f(path);
    if (!f.is_open()) {
        LOG_D("Chat", "No chat history file found");
        return;
    }
    std::string content((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
    f.close();

    g_chat_messages.clear();

    /* Parse JSON array manually (simple, no external dependency) */
    size_t pos = 0;
    while (pos < content.size()) {
        size_t obj_start = content.find('{', pos);
        if (obj_start == std::string::npos) break;
        size_t obj_end = content.find('}', obj_start);
        if (obj_end == std::string::npos) break;

        std::string obj = content.substr(obj_start, obj_end - obj_start + 1);

        /* Extract role */
        char role[16] = {0};
        size_t rp = obj.find("\"role\"");
        if (rp != std::string::npos) {
            rp = obj.find(':', rp);
            if (rp != std::string::npos) {
                rp++;
                while (rp < obj.size() && (obj[rp] == ' ' || obj[rp] == '"')) rp++;
                size_t re = obj.find('"', rp);
                if (re != std::string::npos && re - rp < 15) {
                    memcpy(role, obj.c_str() + rp, re - rp);
                }
            }
        }

        /* Extract text (handle escaped chars) */
        std::string text;
        size_t tp = obj.find("\"text\"");
        if (tp != std::string::npos) {
            tp = obj.find(':', tp);
            if (tp != std::string::npos) {
                tp++;
                while (tp < obj.size() && obj[tp] != '"') tp++;
                if (tp < obj.size()) tp++; /* skip opening quote */
                size_t te = tp;
                while (te < obj.size()) {
                    if (obj[te] == '\\' && te + 1 < obj.size()) {
                        switch (obj[te + 1]) {
                            case 'n': text += '\n'; break;
                            case 'r': break;
                            case 't': text += '\t'; break;
                            default: text += obj[te + 1]; break;
                        }
                        te += 2;
                    } else if (obj[te] == '"') {
                        break;
                    } else {
                        text += obj[te];
                        te++;
                    }
                }
            }
        }

        /* Extract ts */
        char ts[32] = {0};
        size_t sp = obj.find("\"ts\"");
        if (sp != std::string::npos) {
            sp = obj.find(':', sp);
            if (sp != std::string::npos) {
                sp++;
                while (sp < obj.size() && obj[sp] != '"') sp++;
                if (sp < obj.size()) sp++;
                size_t se = obj.find('"', sp);
                if (se != std::string::npos && se - sp < 31) {
                    memcpy(ts, obj.c_str() + sp, se - sp);
                }
            }
        }

        if (role[0] && !text.empty()) {
            g_chat_messages.push_back({role, text, ts});
        }

        pos = obj_end + 1;
    }

    /* Restore bubbles in UI */
    LOG_I("Chat", "Loaded %zu messages from history", g_chat_messages.size());
    for (const auto& m : g_chat_messages) {
        if (m.role == "user") {
            chat_add_user_bubble(m.text.c_str());
        } else {
            chat_add_ai_bubble(m.text.c_str());
        }
    }
}

/* Clear chat history (file + memory + UI) */
void clear_chat_history() {
    g_chat_messages.clear();
    std::string path = get_chat_history_path();
    std::remove(path.c_str());

    /* Clear chat UI */
    if (chat_cont) {
        uint32_t cnt = lv_obj_get_child_count(chat_cont);
        for (uint32_t i = cnt; i > 0; i--) {
            lv_obj_t* child = lv_obj_get_child(chat_cont, i - 1);
            if (child) lv_obj_del(child);
        }
    }
    chat_history[0] = '\0';
    LOG_I("Chat", "Chat history cleared");
}

/* ═══ Chat Search Implementation (SF-01) ═══ */

/* Show/hide search bar */
static void search_toggle_cb(lv_event_t* e) {
    (void)e;
    g_search_visible = !g_search_visible;
    if (g_search_bar) {
        if (g_search_visible) {
            lv_obj_clear_flag(g_search_bar, LV_OBJ_FLAG_HIDDEN);
            if (g_search_input) lv_group_focus_obj(g_search_input);
        } else {
            lv_obj_add_flag(g_search_bar, LV_OBJ_FLAG_HIDDEN);
            /* Clear search highlights */
            g_search_results.clear();
            g_search_current = -1;
        }
    }
    /* Reposition chat area */
    relayout_panels();
}

/* Search through chat messages for matching text */
static void search_execute_cb(lv_event_t* e) {
    (void)e;
    if (!g_search_input || !chat_cont) return;

    const char* query = lv_textarea_get_text(g_search_input);
    if (!query || !query[0]) {
        g_search_results.clear();
        g_search_current = -1;
        if (g_search_info) lv_label_set_text(g_search_info, "");
        return;
    }

    g_search_results.clear();
    g_search_current = -1;

    /* Search through g_chat_messages */
    std::string q(query);
    /* Case-insensitive search: convert query to lowercase for comparison */
    std::string q_lower = q;
    for (auto& c : q_lower) c = tolower(c);

    for (int i = 0; i < (int)g_chat_messages.size(); i++) {
        std::string text_lower = g_chat_messages[i].text;
        for (auto& c : text_lower) c = tolower(c);
        if (text_lower.find(q_lower) != std::string::npos) {
            g_search_results.push_back(i);
        }
    }

    if (!g_search_results.empty()) {
        g_search_current = 0;
        /* Scroll to first result */
        int msg_idx = g_search_results[0];
        if (msg_idx >= 0 && msg_idx < (int)lv_obj_get_child_count(chat_cont)) {
            lv_obj_t* target = lv_obj_get_child(chat_cont, msg_idx);
            if (target) lv_obj_scroll_to_view(target, LV_ANIM_ON);
        }
    }

    /* Update info label */
    if (g_search_info) {
        if (g_search_results.empty()) {
            lv_label_set_text(g_search_info, (g_lang == Lang::CN) ? "无结果" : "No results");
        } else {
            char buf[32];
            snprintf(buf, sizeof(buf), "%d/%d", g_search_current + 1, (int)g_search_results.size());
            lv_label_set_text(g_search_info, buf);
        }
    }

    LOG_D("Chat", "Search '%s': %zu results", query, g_search_results.size());
}

/* Navigate to previous search result */
static void search_prev_cb(lv_event_t* e) {
    (void)e;
    if (g_search_results.empty()) return;
    g_search_current--;
    if (g_search_current < 0) g_search_current = (int)g_search_results.size() - 1;

    int msg_idx = g_search_results[g_search_current];
    if (msg_idx >= 0 && msg_idx < (int)lv_obj_get_child_count(chat_cont)) {
        lv_obj_t* target = lv_obj_get_child(chat_cont, msg_idx);
        if (target) lv_obj_scroll_to_view(target, LV_ANIM_ON);
    }

    if (g_search_info) {
        char buf[32];
        snprintf(buf, sizeof(buf), "%d/%d", g_search_current + 1, (int)g_search_results.size());
        lv_label_set_text(g_search_info, buf);
    }
}

/* Navigate to next search result */
static void search_next_cb(lv_event_t* e) {
    (void)e;
    if (g_search_results.empty()) return;
    g_search_current++;
    if (g_search_current >= (int)g_search_results.size()) g_search_current = 0;

    int msg_idx = g_search_results[g_search_current];
    if (msg_idx >= 0 && msg_idx < (int)lv_obj_get_child_count(chat_cont)) {
        lv_obj_t* target = lv_obj_get_child(chat_cont, msg_idx);
        if (target) lv_obj_scroll_to_view(target, LV_ANIM_ON);
    }

    if (g_search_info) {
        char buf[32];
        snprintf(buf, sizeof(buf), "%d/%d", g_search_current + 1, (int)g_search_results.size());
        lv_label_set_text(g_search_info, buf);
    }
}

/* ═══ Pre-window-creation config loaders ═══ */
/* Read config once, extract multiple fields (avoid repeated file I/O) */
static std::string g_preconfig_cache;

static const std::string& get_preconfig() {
    if (g_preconfig_cache.empty()) {
        std::ifstream f(get_config_path());
        if (f.is_open()) {
            g_preconfig_cache.assign((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
            f.close();
        }
    }
    return g_preconfig_cache;
}

void load_window_config(int* w, int* h) {
    const std::string& content = get_preconfig();
    if (content.empty()) return;
    int ww = json_extract_int(content.c_str(), "window_w", -1);
    int wh = json_extract_int(content.c_str(), "window_h", -1);
    if (ww >= 400 && ww <= 5000 && wh >= 300 && wh <= 4000) {
        *w = ww;
        *h = wh;
        LOG_I("CONFIG", "Restored window size: %dx%d", *w, *h);
    }
}

void load_exit_confirm_config() {
    const std::string& content = get_preconfig();
    if (content.empty()) return;
    int val = json_extract_int(content.c_str(), "exit_confirm", -1);
    if (val >= 0) g_exit_confirm_enabled = (val == 1);
    LOG_I("CONFIG", "Exit confirmation: %s", g_exit_confirm_enabled ? "ON" : "OFF");
}

void load_dpi_config() {
    const std::string& content = get_preconfig();
    if (content.empty()) return;
    int val = json_extract_int(content.c_str(), "dpi_scale", -1);
    if (val == 100 || val == 125 || val == 150) {
        g_dpi_scale = val;
        LOG_I("CONFIG", "DPI scale: %d%%", g_dpi_scale);
    }
}

/* P2-03/P2-04: Full data persistence - theme, language, window position, refresh interval */
static bool g_wizard_completed = false;  /* Set when wizard completes */
void save_theme_config() {
    std::ofstream f(get_config_path());
    if (f.is_open()) {
        int theme_idx = (int)g_theme;
        int lang_idx = (int)g_lang;

        /* Get window position and size */
        int wx = 0, wy = 0, ww = 1450, wh = 800;
        SDL_Window* win = app_get_window();
        if (win) {
            SDL_GetWindowPosition(win, &wx, &wy);
            SDL_GetWindowSize(win, &ww, &wh);
        }

        f << "{\n";
        f << "  \"theme\": " << theme_idx << ",\n";
        f << "  \"language\": " << lang_idx << ",\n";
        f << "  \"window_x\": " << wx << ",\n";
        f << "  \"window_y\": " << wy << ",\n";
        f << "  \"window_w\": " << ww << ",\n";
        f << "  \"window_h\": " << wh << ",\n";
        f << "  \"refresh_interval_ms\": " << g_refresh_interval_ms << ",\n";
        f << "  \"always_top\": " << (app_is_topmost() ? 1 : 0) << ",\n";
        f << "  \"exit_confirm\": " << (g_exit_confirm_enabled ? 1 : 0) << ",\n";
        f << "  \"dpi_scale\": " << g_dpi_scale << ",\n";
        f << "  \"log_enabled\": " << (g_log_enabled ? 1 : 0) << ",\n";
        f << "  \"log_level\": " << g_log_level << ",\n";
        f << "  \"wizard_completed\": " << (g_wizard_completed ? "true" : "false") << ",\n";
        f << "  \"control_mode\": " << (int)g_control_mode << ",\n";
        f << "  \"llm_access_mode\": " << (int)g_llm_access_mode << ",\n";
        f << "  \"ai_interaction_mode\": " << (int)g_ai_interaction_mode << ",\n";
        f << "  \"proactive_startup_enabled\": " << (g_proactive_startup_enabled ? 1 : 0) << ",\n";
        f << "  \"proactive_idle_enabled\": " << (g_proactive_idle_enabled ? 1 : 0) << ",\n";
        f << "  \"proactive_memory_enabled\": " << (g_proactive_memory_enabled ? 1 : 0) << ",\n";
        f << "  \"proactive_summary_enabled\": " << (g_proactive_summary_enabled ? 1 : 0) << ",\n";
        f << "  \"proactive_idle_threshold_minutes\": " << g_proactive_idle_threshold_minutes << ",\n";
        f << "  \"work_show_advanced\": " << (g_work_show_advanced ? 1 : 0) << ",\n";
        f << "  \"c2_dual_result\": " << (g_c2_dual_result ? 1 : 0) << ",\n";
        f << "  \"work_chat_collapsed\": " << (g_work_chat_collapsed_pref ? 1 : 0) << ",\n";
        f << "  \"profile_user_name\": \"" << json_escape(g_profile_user_name) << "\",\n";
        f << "  \"profile_user_role\": \"" << json_escape(g_profile_user_role) << "\",\n";
        f << "  \"profile_user_persona\": \"" << json_escape(g_profile_user_persona) << "\",\n";
        f << "  \"profile_ai_name\": \"" << json_escape(g_profile_ai_name) << "\",\n";
        f << "  \"profile_ai_role\": \"" << json_escape(g_profile_ai_role) << "\",\n";
        f << "  \"profile_ai_persona\": \"" << json_escape(g_profile_ai_persona) << "\",\n";
        f << "  \"profile_ai_skills\": \"" << json_escape(g_profile_ai_skills) << "\",\n";
        f << "  \"profile_user_avatar\": \"" << json_escape(g_profile_user_avatar) << "\",\n";
        f << "  \"profile_ai_avatar\": \"" << json_escape(g_profile_ai_avatar) << "\",\n";
        f << "  \"gemma_install_opt_in\": " << (g_gemma_install_opt_in ? 1 : 0) << ",\n";
        f << "  \"gemma_model_mask\": " << g_gemma_model_mask << ",\n";
        f << "  \"remote_guard_armed\": " << (g_remote_guard_armed ? 1 : 0) << ",\n";
                f << "  \"agent_mode\": \"" << (g_wizard_leader_mode ? "leader" : "single") << "\",\n";
                f << "  \"leader_mode\": " << (g_wizard_leader_mode ? 1 : 0) << ",\n";
                f << "  \"active_runtime\": \""
                    << ((g_wizard_active_runtime == Runtime::Hermes) ? "hermes"
                            : (g_wizard_active_runtime == Runtime::Claude) ? "claude"
                            : "openclaw")
                    << "\",\n";
                f << "  \"hermes_enabled\": " << (g_wizard_hermes_enabled ? 1 : 0) << ",\n";
                f << "  \"claude_code_path\": \"" << json_escape(g_wizard_claude_code_path) << "\",\n";
        f << "  \"app_auth_email\": " << (g_app_auth_email ? 1 : 0) << ",\n";
        f << "  \"app_auth_calendar\": " << (g_app_auth_calendar ? 1 : 0) << ",\n";
        f << "  \"model_name\": \"" << json_escape(g_selected_model) << "\",\n";
        f << "  \"api_key\": \"" << json_escape(g_api_key) << "\",\n";
        f << "  \"restore_last_session\": " << (g_restore_last_session ? 1 : 0) << "\n";
        f << "}\n";
        f.close();
    }
}

void load_theme_config() {
    std::ifstream f(get_config_path());
    if (!f.is_open()) return;
    std::string content((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
    f.close();

    /* FIX 1: Use robust JSON extraction instead of hand-written lambda */
    int theme = json_extract_int(content.c_str(), "theme", -1);
    if (theme >= 0) {
        /* Backward compatibility for legacy 5-theme config:
         * 0=Dark -> Matcha, 1=Peachy -> Peachy, 2=Classic -> Matcha,
         * 3=Mochi -> Mochi, 4=Light -> Peachy */
        switch (theme) {
            case 0:
            case 2:
                g_theme = Theme::Matcha;
                break;
            case 1:
            case 4:
                g_theme = Theme::Peachy;
                break;
            case 3:
                g_theme = Theme::Mochi;
                break;
            default:
                break;
        }

        switch (g_theme) {
            case Theme::Matcha: g_colors = &THEME_DARK; break;
            case Theme::Peachy: g_colors = &THEME_PEACHY; break;
            case Theme::Mochi:  g_colors = &THEME_MOCHI; break;
        }
    }
    /* Language forced to English — do not restore from config file */
    /* int lang = json_extract_int(content.c_str(), "language", -1); */
    /* if (lang >= 0 && lang <= 3) { g_lang = (Lang)lang; } */

    /* P2-03: Restore window position */
    int wx = json_extract_int(content.c_str(), "window_x", -1);
    int wy = json_extract_int(content.c_str(), "window_y", -1);
    if (wx >= 0 && wy >= 0 && wx < 3000 && wy < 2000) {
        SDL_Window* win = app_get_window();
        if (win) {
            SDL_SetWindowPosition(win, wx, wy);
        }
    }

    /* Restore window size */
    int ww = json_extract_int(content.c_str(), "window_w", -1);
    int wh = json_extract_int(content.c_str(), "window_h", -1);
    if (ww >= 400 && ww <= 5000 && wh >= 300 && wh <= 4000) {
        SDL_Window* win = app_get_window();
        if (win) {
            SDL_SetWindowSize(win, ww, wh);
        }
    }

    /* Restore exit confirmation */
    int ec = json_extract_int(content.c_str(), "exit_confirm", -1);
    if (ec >= 0) {
        g_exit_confirm_enabled = (ec == 1);
    }

    /* P2-07: Restore refresh interval */
    int ri = json_extract_int(content.c_str(), "refresh_interval_ms", -1);
    if (ri >= 5000 && ri <= 300000) {
        g_refresh_interval_ms = ri;
        if (g_refresh_timer) {
            lv_timer_set_period(g_refresh_timer, ri);
        }
    }

    /* P2-24: Restore window topmost state */
    int at = json_extract_int(content.c_str(), "always_top", -1);
    if (at == 1) {
        app_set_topmost(true);
    }

    /* Restore log settings */
    int le = json_extract_int(content.c_str(), "log_enabled", -1);
    if (le >= 0) {
        g_log_enabled = (le == 1);
    }
    int ll = json_extract_int(content.c_str(), "log_level", -1);
    if (ll >= 0 && ll <= 3) {
        g_log_level = ll;
    }
    int cm = json_extract_int(content.c_str(), "control_mode", -1);
    if (cm >= 0 && cm <= 1) g_control_mode = (ControlMode)cm;
    int lm = json_extract_int(content.c_str(), "llm_access_mode", -1);
    if (lm >= 0 && lm <= 1) g_llm_access_mode = (LlmAccessMode)lm;
    int aim = json_extract_int(content.c_str(), "ai_interaction_mode", -1);
    if (aim >= 0 && aim <= 2) g_ai_interaction_mode = (AiInteractionMode)aim;
    int ps = json_extract_int(content.c_str(), "proactive_startup_enabled", -1);
    if (ps >= 0) g_proactive_startup_enabled = (ps == 1);
    int pi = json_extract_int(content.c_str(), "proactive_idle_enabled", -1);
    if (pi >= 0) g_proactive_idle_enabled = (pi == 1);
    int pm = json_extract_int(content.c_str(), "proactive_memory_enabled", -1);
    if (pm >= 0) g_proactive_memory_enabled = (pm == 1);
    int psm = json_extract_int(content.c_str(), "proactive_summary_enabled", -1);
    if (psm >= 0) g_proactive_summary_enabled = (psm == 1);
    int pit = json_extract_int(content.c_str(), "proactive_idle_threshold_minutes", -1);
    if (pit >= 5 && pit <= 240) g_proactive_idle_threshold_minutes = pit;
    int wsa = json_extract_int(content.c_str(), "work_show_advanced", -1);
    if (wsa >= 0) g_work_show_advanced = (wsa == 1);
    int c2dr = json_extract_int(content.c_str(), "c2_dual_result", -1);
    if (c2dr >= 0) g_c2_dual_result = (c2dr == 1);
    int wcc = json_extract_int(content.c_str(), "work_chat_collapsed", -1);
    if (wcc >= 0) g_work_chat_collapsed_pref = (wcc == 1);
    g_work_chat_collapsed = g_c2_dual_result ? false : g_work_chat_collapsed_pref;
    if (g_llm_access_mode == LLM_DIRECT_API) {
        /* User requested Direct API mode to be paused for now. */
        g_llm_access_mode = LLM_GATEWAY;
    }
    json_extract_string(content.c_str(), "profile_user_name", g_profile_user_name, sizeof(g_profile_user_name));
    json_extract_string(content.c_str(), "profile_user_role", g_profile_user_role, sizeof(g_profile_user_role));
    json_extract_string(content.c_str(), "profile_user_persona", g_profile_user_persona, sizeof(g_profile_user_persona));
    json_extract_string(content.c_str(), "profile_ai_name", g_profile_ai_name, sizeof(g_profile_ai_name));
    json_extract_string(content.c_str(), "profile_ai_role", g_profile_ai_role, sizeof(g_profile_ai_role));
    json_extract_string(content.c_str(), "profile_ai_persona", g_profile_ai_persona, sizeof(g_profile_ai_persona));
    json_extract_string(content.c_str(), "profile_ai_skills", g_profile_ai_skills, sizeof(g_profile_ai_skills));
    json_extract_string(content.c_str(), "profile_user_avatar", g_profile_user_avatar, sizeof(g_profile_user_avatar));
    json_extract_string(content.c_str(), "profile_ai_avatar", g_profile_ai_avatar, sizeof(g_profile_ai_avatar));
    int gio = json_extract_int(content.c_str(), "gemma_install_opt_in", -1);
    if (gio >= 0) g_gemma_install_opt_in = (gio == 1);
    int gmm = json_extract_int(content.c_str(), "gemma_model_mask", -1);
    if (gmm >= 0 && gmm <= 7) g_gemma_model_mask = gmm;
    int rg = json_extract_int(content.c_str(), "remote_guard_armed", -1);
    if (rg >= 0) g_remote_guard_armed = (rg == 1);
    int lm2 = json_extract_int(content.c_str(), "leader_mode", -1);
    if (lm2 >= 0) g_wizard_leader_mode = (lm2 == 1);
    int he = json_extract_int(content.c_str(), "hermes_enabled", -1);
    if (he >= 0) g_wizard_hermes_enabled = (he == 1);
    char mode_buf[32] = {0};
    if (json_extract_string(content.c_str(), "agent_mode", mode_buf, sizeof(mode_buf))) {
        if (strcmp(mode_buf, "single") == 0) g_wizard_leader_mode = false;
        if (strcmp(mode_buf, "leader") == 0) g_wizard_leader_mode = true;
    }
    char rt_buf[32] = {0};
    if (json_extract_string(content.c_str(), "active_runtime", rt_buf, sizeof(rt_buf))) {
        if (strcmp(rt_buf, "hermes") == 0) g_wizard_active_runtime = Runtime::Hermes;
        else if (strcmp(rt_buf, "claude") == 0) g_wizard_active_runtime = Runtime::Claude;
        else g_wizard_active_runtime = Runtime::OpenClaw;
    }
    json_extract_string(content.c_str(), "claude_code_path", g_wizard_claude_code_path, sizeof(g_wizard_claude_code_path));
    int ae = json_extract_int(content.c_str(), "app_auth_email", -1);
    if (ae >= 0) g_app_auth_email = (ae == 1);
    int ac = json_extract_int(content.c_str(), "app_auth_calendar", -1);
    if (ac >= 0) g_app_auth_calendar = (ac == 1);

    /* Restore wizard_completed flag */
    g_wizard_completed = is_wizard_completed();

    /* FIX 1: Use robust JSON extraction for model_name and api_key */
    json_extract_string(content.c_str(), "model_name", g_selected_model, sizeof(g_selected_model));
    json_extract_string(content.c_str(), "api_key", g_api_key, sizeof(g_api_key));
    int rls = json_extract_int(content.c_str(), "restore_last_session", -1);
    if (rls >= 0) g_restore_last_session = (rls == 1);

    LOG_I("CONFIG", "log_enabled=%d log_level=%d model=%s control=%d llm=%d ai_mode=%d remote_guard=%d",
          g_log_enabled, g_log_level,
          g_selected_model[0] ? g_selected_model : "(none)",
          (int)g_control_mode, (int)g_llm_access_mode, (int)g_ai_interaction_mode, g_remote_guard_armed ? 1 : 0);
}

/* Forward declaration for title bar creation (called last for z-order) */
static void create_title_bar(lv_obj_t* scr);

/* Theme change callback - forward declaration */
void apply_theme_to_all();

static void theme_dropdown_cb(lv_event_t* e) {
    lv_obj_t* dropdown = (lv_obj_t*)lv_event_get_target(e);
    uint16_t sel = lv_dropdown_get_selected(dropdown);
    if (sel == (uint16_t)g_theme) return;
    g_theme = (Theme)sel;
    switch (g_theme) {
        case Theme::Matcha: g_colors = &THEME_DARK; break;
        case Theme::Peachy: g_colors = &THEME_PEACHY; break;
        case Theme::Mochi:  g_colors = &THEME_MOCHI; break;
    }
    save_theme_config();
    init_theme_fonts(g_theme); /* Reload per-theme fonts */
    apply_theme_to_all();
    static const char* names[] = {"Matcha", "Peachy", "Mochi"};
    const char* theme_name = (sel < 3) ? names[sel] : "?";
    ui_log("[Theme] Switched to %s", theme_name);
    ui_toast_success(g_lang == Lang::CN ? "主题已切换" : "Theme switched");
}

/* Called from settings UI to add theme dropdown */
lv_obj_t* ui_settings_add_theme_dropdown(lv_obj_t* tab) {
    const ThemeColors* c = g_colors;
    lv_obj_t* dd = lv_dropdown_create(tab);
    lv_dropdown_set_options(dd, "Matcha\nPeachy\nMochi");
    lv_dropdown_set_selected(dd, (uint16_t)g_theme);
    lv_obj_set_width(dd, SCALE(160));
    lv_obj_set_style_bg_color(dd, c->surface, 0);
    lv_obj_set_style_text_color(dd, c->text, 0);
    lv_obj_set_style_border_color(dd, c->border, 0);
    lv_obj_set_style_text_font(dd, CJK_FONT, 0);
    lv_obj_set_style_text_font(dd, FONT(14), LV_PART_INDICATOR);  /* Symbol font for ▼ */
    lv_obj_set_style_text_font(dd, CJK_FONT, LV_PART_ITEMS);
    lv_obj_set_style_text_color(dd, c->text, LV_PART_ITEMS);
    lv_obj_set_style_bg_color(dd, c->panel, LV_PART_ITEMS);
    lv_obj_add_event_cb(dd, theme_dropdown_cb, LV_EVENT_VALUE_CHANGED, nullptr);
    return dd;
}

/* I18n string pair */
struct I18n { const char* en; const char* cn; };

/* Get translated string based on current language */
static const char* tr(const I18n& s) {
    return (g_lang == Lang::CN) ? s.cn : s.en;
}

/* P2-01: Single language title per PRD 2.1 */
static const char* tr_title(const I18n& s) {
    return (g_lang == Lang::CN) ? s.cn : s.en;
}

/* ── I18n Strings ── */
static const I18n STR_TITLE       = {"AnyClaw Desktop Manager", "AnyClaw 桌面管理器"};
static const I18n STR_TITLE_BOT_CHAT = {"Bot · Chat", "Bot · 对话"};
static const I18n STR_TITLE_BOT_WORK = {"Bot · Work", "Bot · 任务"};
static const I18n STR_TITLE_TASKS = {"Tasks", "任务"};
static const I18n STR_TITLE_FILES = {"Resources", "资源"};

static const I18n STR_LOG         = {"Log", "日志"};
// STR_VERSION, STR_PORT removed — no longer displayed on main panel
static const I18n STR_PATH        = {"Path", "路径"};
static const I18n STR_FOOTER      = {"AnyClaw v2.2.1 | LVGL 9.6 + SDL2", "AnyClaw v2.2.1 | LVGL 9.6 + SDL2"};
static const I18n STR_AUTOREFRESH = {"Auto-refresh: 30s", "自动刷新: 30s"};
static const I18n STR_CHECKING    = {"Checking...", "检查中..."};
static const I18n STR_READY       = {"Ready", "就绪"};
static const I18n STR_BUSY        = {"Busy", "处理中"};
static const I18n STR_NOTFOUND    = {"Not Found", "未找到"};
static const I18n STR_NOTINST     = {"Not Installed", "未安装"};
static const I18n STR_IDLE        = {"Idle", "空闲"};
static const I18n STR_RUNNING     = {"Running", "运行中"};
static const I18n STR_ERROR       = {"Error", "错误"};
static const I18n STR_UNKNOWN     = {"Unknown", "未知"};
static const I18n STR_TASK_LIST   = {"Task List", "任务列表"};
static const I18n STR_CHAT        = {"Chat", "聊天"};
static const I18n STR_WIFI        = {"Connected", "已连接"};
static const I18n STR_BATTERY     = {"Power", "电源"};
static const I18n STR_NO_TASKS    = {"No pending tasks", "暂无待处理任务"};
static const I18n STR_GW_RUNNING  = {"Gateway Service", "Gateway 服务"};
static const I18n STR_CHAT_INPUT  = {"Type message...", "输入消息..."};
static const I18n STR_SEND        = {"Send", "发送"};

/* ── UI widgets ── */
static lv_obj_t* status_label = nullptr;
// version_label, port_label removed from main panel
static lv_obj_t* path_label = nullptr;
/* Buttons moved to tray menu */
static lv_obj_t* led_ok = nullptr;
static lv_obj_t* title_bar = nullptr;
static lv_obj_t* g_lang_toggle_label = nullptr;  /* P2-01: language toggle button label */
static lv_obj_t* title_label = nullptr;

/* Window control buttons */
static lv_obj_t* btn_minimize = nullptr;
static lv_obj_t* btn_maximize = nullptr;
static lv_obj_t* btn_close = nullptr;
static lv_obj_t* top_mode_chat = nullptr;
static lv_obj_t* top_mode_work = nullptr;
static lv_obj_t* top_btn_settings = nullptr;
static lv_obj_t* lbl_maximize = nullptr;  /* Label inside maximize button, for icon swap */
static bool g_maximized = false;          /* Track maximize state */

/* Loading overlay (shown while OpenClaw is starting) */
static lv_obj_t* g_loading_overlay = nullptr;
static lv_obj_t* g_loading_icon = nullptr;
static lv_obj_t* g_loading_label = nullptr;
static lv_obj_t* g_loading_bar = nullptr;
static lv_obj_t* g_loading_stage_list = nullptr;
static lv_obj_t* g_loading_hide_btn = nullptr;
static lv_obj_t* g_loading_toggle_btn = nullptr;
static lv_timer_t* g_loading_timer = nullptr;
static DWORD g_loading_start_tick = 0;
static float g_loading_angle = 0.0f;
static std::mutex g_loading_live_mtx;
static std::string g_loading_live_step;
static int g_loading_live_pct = -1;
static std::atomic<bool> g_loading_startup_done(false);
static bool g_loading_hidden_by_user = false;
static bool g_loading_details_collapsed = false;
static bool g_loading_fade_active = false;
static int g_loading_fade_opa = 220;

/* Hide loading overlay */
static void loading_hide() {
    if (g_loading_timer) { lv_timer_del(g_loading_timer); g_loading_timer = nullptr; }
    if (g_loading_overlay) { lv_obj_add_flag(g_loading_overlay, LV_OBJ_FLAG_HIDDEN); }
}

static void loading_manual_hide_cb(lv_event_t* e) {
    (void)e;
    g_loading_hidden_by_user = true;
    loading_hide();
}

static void loading_apply_layout_mode() {
    if (!g_loading_overlay) return;
    if (g_loading_stage_list) {
        if (g_loading_details_collapsed) lv_obj_add_flag(g_loading_stage_list, LV_OBJ_FLAG_HIDDEN);
        else lv_obj_clear_flag(g_loading_stage_list, LV_OBJ_FLAG_HIDDEN);
    }
    lv_obj_set_height(g_loading_overlay, g_loading_details_collapsed ? SCALE(210) : SCALE(300));
    if (g_loading_toggle_btn) {
        lv_obj_t* lbl = lv_obj_get_child(g_loading_toggle_btn, 0);
        if (lbl) lv_label_set_text(lbl, g_loading_details_collapsed ? "▸" : "▾");
    }
}

static void loading_toggle_details_cb(lv_event_t* e) {
    (void)e;
    g_loading_details_collapsed = !g_loading_details_collapsed;
    loading_apply_layout_mode();
}

static void loading_start_fade() {
    if (!g_loading_overlay || g_loading_fade_active) return;
    g_loading_fade_active = true;
}

static const char* loading_stage_reset_text() {
    return (g_lang == Lang::CN)
        ? "○ 网关\n○ 许可证\n○ 工作区\n○ 功能开关\n○ 收尾完成"
        : "○ Gateway\n○ License\n○ Workspace\n○ Feature flags\n○ Finalize";
}

/* Loading timer: rotate icon + check status */
static void loading_timer_cb(lv_timer_t* t) {
    (void)t;
    if (!g_loading_overlay) return;
    if (g_loading_fade_active) {
        g_loading_fade_opa -= 12;
        if (g_loading_fade_opa < 0) g_loading_fade_opa = 0;
        lv_obj_set_style_opa(g_loading_overlay, (lv_opa_t)g_loading_fade_opa, 0);
        if (g_loading_fade_opa <= 0) {
            g_loading_fade_active = false;
            loading_hide();
        }
        return;
    }

    DWORD now = GetTickCount();

    /* Rotate garlic icon */
    g_loading_angle += 6.0f;  /* 6° per tick = full rotation in ~3s at 50ms interval */
    if (g_loading_angle >= 360.0f) g_loading_angle -= 360.0f;
    if (g_loading_icon) {
        lv_image_set_rotation(g_loading_icon, (int32_t)(g_loading_angle * 10));
    }

    /* Update loading text dots animation */
    std::string live_step;
    int live_pct = -1;
    bool startup_done = g_loading_startup_done.load();
    {
        std::lock_guard<std::mutex> lk(g_loading_live_mtx);
        live_step = g_loading_live_step;
        live_pct = g_loading_live_pct;
    }
    if (startup_done && now - g_loading_start_tick > 700) {
        loading_start_fade();
        return;
    }

    if (g_loading_label) {
        DWORD elapsed = GetTickCount() - g_loading_start_tick;
        char buf[192];
        float elapsed_s = (elapsed <= 0) ? 0.0f : (elapsed / 1000.0f);
        const char* eta_text = "ETA: calculating...";
        const char* speed_text = "Speed: estimating...";
        char eta_buf[48] = {0};
        char speed_buf[48] = {0};
        int eta_sec = -1;
        float rate = 0.0f;
        int pct_for_eta = (live_pct < 0) ? 0 : live_pct;
        if (elapsed_s > 0.2f && pct_for_eta > 0) {
            rate = (float)pct_for_eta / elapsed_s;
            if (rate > 0.01f && pct_for_eta < 100) {
                eta_sec = (int)((100.0f - (float)pct_for_eta) / rate);
                if (eta_sec < 0) eta_sec = 0;
                snprintf(eta_buf, sizeof(eta_buf), "ETA: %ds", eta_sec);
                eta_text = eta_buf;
            } else if (pct_for_eta >= 100) {
                eta_text = "ETA: done";
            }
            snprintf(speed_buf, sizeof(speed_buf), "Speed: %.1f%%/s", rate);
            speed_text = speed_buf;
        }
        if (!live_step.empty()) {
            int pct = (live_pct < 0) ? 0 : live_pct;
            snprintf(buf, sizeof(buf), "%s (%d%%)\n%s  |  %s", live_step.c_str(), pct, speed_text, eta_text);
        } else {
            int dots = (int)((elapsed / 500) % 4);
            const char* phase = "Starting OpenClaw";
            if (elapsed > 9000) phase = "Checking Gateway status";
            else if (elapsed > 3000) phase = "Initializing runtime modules";
            snprintf(buf, sizeof(buf), "%s%.*s\n%s  |  %s", phase, dots, "...", speed_text, eta_text);
        }
        lv_label_set_text(g_loading_label, buf);
    }
    if (g_loading_bar) {
        int bar_pct = live_pct;
        if (bar_pct < 0) {
            DWORD elapsed = GetTickCount() - g_loading_start_tick;
            bar_pct = (elapsed > 12000) ? 95 : (int)(elapsed / 140);
        }
        if (bar_pct < 0) bar_pct = 0;
        if (bar_pct > 100) bar_pct = 100;
        lv_bar_set_value(g_loading_bar, bar_pct, LV_ANIM_ON);
    }
    if (g_loading_stage_list) {
        int p = (live_pct < 0) ? 0 : live_pct;
        const char* gateway = (p >= 30) ? "✓" : (p >= 10 ? "•" : "○");
        const char* license = (p >= 56) ? "✓" : (p >= 40 ? "•" : "○");
        const char* workspace = (p >= 80) ? "✓" : (p >= 60 ? "•" : "○");
        const char* features = (p >= 90) ? "✓" : (p >= 82 ? "•" : "○");
        const char* finish = startup_done ? "✓" : (p >= 95 ? "•" : "○");
        char stage_buf[320];
        if (g_lang == Lang::CN) {
            snprintf(stage_buf, sizeof(stage_buf),
                     "%s 网关\n%s 许可证\n%s 工作区\n%s 功能开关\n%s 收尾完成",
                     gateway, license, workspace, features, finish);
        } else {
            snprintf(stage_buf, sizeof(stage_buf),
                     "%s Gateway\n%s License\n%s Workspace\n%s Feature flags\n%s Finalize",
                     gateway, license, workspace, features, finish);
        }
        lv_label_set_text(g_loading_stage_list, stage_buf);
    }

    /* UI-thread friendly: avoid network probing here. Worker startup events drive hide timing. */
    if (now - g_loading_start_tick > 12000) {
        LOG_W("Loading", "Startup card timeout after 12s, hiding");
        loading_start_fade();
    }
}

/* Show loading overlay */
static void loading_show() {
    if (!g_loading_overlay) {
        lv_obj_t* scr = lv_screen_active();
        g_loading_overlay = lv_obj_create(scr);
        int overlay_w = WIN_W * LOADING_OVERLAY_W_PCT / 100;
        int overlay_h = WIN_H * LOADING_OVERLAY_H_PCT / 100;
        lv_obj_set_size(g_loading_overlay, overlay_w, overlay_h);
        lv_obj_align(g_loading_overlay, LV_ALIGN_TOP_RIGHT, -SCALE(16), TITLE_H + SCALE(8));
        lv_obj_set_style_bg_color(g_loading_overlay, g_colors->raised, 0);
        lv_obj_set_style_bg_opa(g_loading_overlay, g_colors->loading_opacity, 0);
        lv_obj_set_style_border_width(g_loading_overlay, 1, 0);
        lv_obj_set_style_border_color(g_loading_overlay, g_colors->border_strong, 0);
        lv_obj_set_style_radius(g_loading_overlay, SCALE(g_colors->radius_lg), 0);
        lv_obj_set_style_pad_all(g_loading_overlay, SCALE(14), 0);
        lv_obj_set_style_pad_gap(g_loading_overlay, SCALE(8), 0);
        lv_obj_clear_flag(g_loading_overlay, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_flex_flow(g_loading_overlay, LV_FLEX_FLOW_COLUMN);
        lv_obj_set_flex_align(g_loading_overlay, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
        lv_obj_move_foreground(g_loading_overlay);

        lv_obj_t* top_row = lv_obj_create(g_loading_overlay);
        lv_obj_set_size(top_row, LV_PCT(100), LV_SIZE_CONTENT);
        lv_obj_set_style_bg_opa(top_row, LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_width(top_row, 0, 0);
        lv_obj_set_style_pad_all(top_row, 0, 0);
        lv_obj_set_style_pad_gap(top_row, SCALE(8), 0);
        lv_obj_set_flex_flow(top_row, LV_FLEX_FLOW_ROW);
        lv_obj_set_flex_align(top_row, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        lv_obj_clear_flag(top_row, LV_OBJ_FLAG_SCROLLABLE);

        lv_obj_t* top_title = lv_label_create(top_row);
        lv_label_set_text(top_title, g_lang == Lang::CN ? "后台启动中" : "Background startup");
        lv_obj_set_style_text_color(top_title, g_colors->text_dim, 0);
        lv_obj_set_style_text_font(top_title, CJK_FONT_SMALL, 0);

        g_loading_hide_btn = lv_button_create(top_row);
        lv_obj_set_size(g_loading_hide_btn, SCALE(28), SCALE(22));
        lv_obj_set_style_bg_color(g_loading_hide_btn, g_colors->btn_secondary, 0);
        lv_obj_set_style_radius(g_loading_hide_btn, SCALE(g_colors->radius_sm), 0);
        lv_obj_add_event_cb(g_loading_hide_btn, loading_manual_hide_cb, LV_EVENT_CLICKED, nullptr);
        lv_obj_t* hide_lbl = lv_label_create(g_loading_hide_btn);
        lv_label_set_text(hide_lbl, "×");
        lv_obj_set_style_text_font(hide_lbl, CJK_FONT_SMALL, 0);
        lv_obj_center(hide_lbl);

        g_loading_toggle_btn = lv_button_create(top_row);
        lv_obj_set_size(g_loading_toggle_btn, SCALE(28), SCALE(22));
        lv_obj_set_style_bg_color(g_loading_toggle_btn, g_colors->btn_secondary, 0);
        lv_obj_set_style_radius(g_loading_toggle_btn, SCALE(g_colors->radius_sm), 0);
        lv_obj_add_event_cb(g_loading_toggle_btn, loading_toggle_details_cb, LV_EVENT_CLICKED, nullptr);
        lv_obj_t* toggle_lbl = lv_label_create(g_loading_toggle_btn);
        lv_label_set_text(toggle_lbl, "▾");
        lv_obj_set_style_text_font(toggle_lbl, CJK_FONT_SMALL, 0);
        lv_obj_center(toggle_lbl);

        g_loading_icon = lv_image_create(g_loading_overlay);
        lv_image_set_src(g_loading_icon, "A:assets/garlic_48.png");
        int loading_icon_px = overlay_h * LOADING_ICON_PCT / 100;
        lv_obj_set_size(g_loading_icon, loading_icon_px, loading_icon_px);
        lv_image_set_scale(g_loading_icon, 256); /* 100% */
        lv_obj_align(g_loading_icon, LV_ALIGN_TOP_MID, 0, 0);

        /* Loading text */
        g_loading_label = lv_label_create(g_loading_overlay);
        lv_label_set_text(g_loading_label,
                          g_lang == Lang::CN
                            ? "正在启动 OpenClaw...\n界面已可用，后台继续初始化。"
                            : "Starting OpenClaw...\nUI is ready. Finishing background startup.");
        lv_obj_set_style_text_color(g_loading_label, g_colors->text, 0);
        lv_obj_set_style_text_font(g_loading_label, CJK_FONT_SMALL, 0);
        lv_label_set_long_mode(g_loading_label, LV_LABEL_LONG_WRAP);
        lv_obj_set_width(g_loading_label, LV_PCT(100));

        g_loading_bar = lv_bar_create(g_loading_overlay);
        lv_obj_set_size(g_loading_bar, LV_PCT(100), SCALE(8));
        lv_bar_set_range(g_loading_bar, 0, 100);
        lv_bar_set_value(g_loading_bar, 2, LV_ANIM_OFF);
        lv_obj_set_style_radius(g_loading_bar, 999, 0);
        lv_obj_set_style_bg_color(g_loading_bar, g_colors->btn_secondary, LV_PART_MAIN);
        lv_obj_set_style_bg_color(g_loading_bar, g_colors->accent, LV_PART_INDICATOR);

        g_loading_stage_list = lv_label_create(g_loading_overlay);
        lv_label_set_text(g_loading_stage_list, loading_stage_reset_text());
        lv_obj_set_style_text_color(g_loading_stage_list, g_colors->text_dim, 0);
        lv_obj_set_style_text_font(g_loading_stage_list, CJK_FONT_SMALL, 0);
        lv_obj_set_width(g_loading_stage_list, LV_PCT(100));
    } else {
        lv_obj_clear_flag(g_loading_overlay, LV_OBJ_FLAG_HIDDEN);
        lv_obj_align(g_loading_overlay, LV_ALIGN_TOP_RIGHT, -SCALE(16), TITLE_H + SCALE(8));
        lv_obj_move_foreground(g_loading_overlay);
    }

    g_loading_start_tick = GetTickCount();
    g_loading_angle = 0.0f;
    g_loading_hidden_by_user = false;
    g_loading_details_collapsed = false;
    g_loading_fade_active = false;
    g_loading_fade_opa = 220;
    g_loading_startup_done.store(false);
    {
        std::lock_guard<std::mutex> lk(g_loading_live_mtx);
        g_loading_live_step.clear();
        g_loading_live_pct = -1;
    }
    if (g_loading_bar) lv_bar_set_value(g_loading_bar, 2, LV_ANIM_OFF);
    if (g_loading_stage_list) lv_label_set_text(g_loading_stage_list, loading_stage_reset_text());
    lv_obj_set_style_opa(g_loading_overlay, (lv_opa_t)g_loading_fade_opa, 0);
    loading_apply_layout_mode();

    /* Start rotation + status check timer (60fps) */
    if (g_loading_timer) lv_timer_del(g_loading_timer);
    g_loading_timer = lv_timer_create(loading_timer_cb, 50, nullptr);
}

/* Splitter / Resizable panels */
static lv_obj_t* splitter = nullptr;
static lv_obj_t* left_nav = nullptr;     /* icon-only module nav */
static lv_obj_t* nav_btn_bot = nullptr;
static lv_obj_t* nav_btn_tasks = nullptr;
static lv_obj_t* nav_btn_files = nullptr;
static lv_obj_t* nav_session = nullptr;   /* bottom nav: Chat/Work toggle */
static lv_obj_t* left_panel = nullptr;
static lv_obj_t* right_panel = nullptr;
static lv_obj_t* module_placeholder = nullptr;
static lv_obj_t* module_placeholder_title = nullptr;
static lv_obj_t* module_placeholder_desc = nullptr;
static lv_obj_t* module_tasks_panel = nullptr;
static lv_obj_t* module_files_panel = nullptr;
static lv_obj_t* module_tasks_state = nullptr;
static lv_obj_t* module_files_state = nullptr;
static lv_obj_t* module_tasks_view = nullptr;
static lv_obj_t* module_files_view = nullptr;
static lv_obj_t* module_files_filter_all = nullptr;
static lv_obj_t* module_files_filter_fonts = nullptr;
static lv_obj_t* module_files_filter_icons = nullptr;
static lv_obj_t* module_files_filter_sounds = nullptr;
static lv_obj_t* module_files_search_input = nullptr;
static lv_obj_t* module_files_ext_dd = nullptr;
enum ModuleFileFilter { MODULE_FILE_ALL = 0, MODULE_FILE_FONTS = 1, MODULE_FILE_ICONS = 2, MODULE_FILE_SOUNDS = 3 };
static ModuleFileFilter g_module_file_filter = MODULE_FILE_ALL;
static int g_module_file_ext_filter = 0;  /* 0=all */
static char g_module_files_search[128] = "";
static lv_obj_t* mode_bar = nullptr;
static lv_obj_t* mode_btn_chat = nullptr;
static lv_obj_t* mode_btn_voice = nullptr;
static lv_obj_t* mode_btn_work = nullptr;
static lv_obj_t* mode_panel_chat = nullptr;
static lv_obj_t* mode_panel_work = nullptr;
static lv_obj_t* mode_trace_chat_panel = nullptr;
static lv_obj_t* mode_lbl_chat_next_step = nullptr;
static lv_obj_t* mode_ta_chat_script = nullptr;
static lv_obj_t* mode_lbl_work_next_step = nullptr;
static lv_obj_t* mode_ta_work_script = nullptr;
static lv_obj_t* mode_ta_trace_recent = nullptr;
static lv_obj_t* mode_lbl_boot_report_hint = nullptr;
static lv_obj_t* mode_boot_progress_panel = nullptr;
static lv_obj_t* mode_boot_progress_task = nullptr;
static lv_obj_t* mode_boot_progress_step = nullptr;
static lv_obj_t* mode_boot_progress_result = nullptr;
static lv_obj_t* mode_boot_progress_bar = nullptr;
static lv_obj_t* mode_boot_progress_list = nullptr;
static lv_obj_t* mode_ta_work_prompt = nullptr;
static lv_obj_t* mode_lbl_work_md_output = nullptr;
static lv_obj_t* mode_lbl_work_renderer = nullptr;
static lv_obj_t* mode_lbl_work_output_title = nullptr;
static lv_obj_t* mode_work_step_stream = nullptr;
static lv_obj_t* mode_chat_empty_card = nullptr;
static lv_obj_t* mode_work_empty_card = nullptr;
static lv_obj_t* mode_sec_step_stream = nullptr;
static lv_obj_t* mode_sec_work_console = nullptr;
static lv_obj_t* mode_work_output_wrap = nullptr;
static lv_obj_t* mode_work_splitter = nullptr;
static lv_obj_t* mode_work_chat_panel = nullptr;
static lv_obj_t* mode_lbl_work_chat_state = nullptr;
static lv_obj_t* mode_ta_work_chat_feed = nullptr;
static lv_obj_t* mode_ta_work_chat_input = nullptr;
static lv_obj_t* mode_btn_work_chat_toggle = nullptr;
static lv_obj_t* mode_btn_work_chat_send = nullptr;
static int g_work_step_stream_h = 160;
static int g_work_output_h = 180;
static lv_obj_t* mode_ta_lan_host = nullptr;
static lv_obj_t* mode_ta_lan_port = nullptr;
static lv_obj_t* mode_ta_lan_nick = nullptr;
static lv_obj_t* mode_ta_lan_to = nullptr;
static lv_obj_t* mode_ta_lan_group = nullptr;
static lv_obj_t* mode_ta_lan_msg = nullptr;
static lv_obj_t* mode_ta_lan_feed = nullptr;
static lv_obj_t* mode_ta_lan_users = nullptr;
static lv_obj_t* mode_ta_lan_groups = nullptr;
static lv_obj_t* mode_ta_ftp_host = nullptr;
static lv_obj_t* mode_ta_ftp_port = nullptr;
static lv_obj_t* mode_ta_ftp_user = nullptr;
static lv_obj_t* mode_ta_ftp_pass = nullptr;
static lv_obj_t* mode_ta_ftp_remote = nullptr;
static lv_obj_t* mode_ta_ftp_local = nullptr;
static lv_obj_t* mode_sw_ftp_ftps = nullptr;
static lv_obj_t* mode_sw_ftp_recursive = nullptr;
static lv_obj_t* mode_ta_kb_source = nullptr;
static lv_obj_t* mode_ta_kb_keyword = nullptr;
static lv_obj_t* mode_ta_kb_result = nullptr;
static lv_obj_t* mode_ta_cron_list = nullptr;
static lv_obj_t* mode_ta_cron_id = nullptr;
static lv_obj_t* mode_sec_lan = nullptr;
static lv_obj_t* mode_sec_ftp = nullptr;
static lv_obj_t* mode_sec_kb = nullptr;
static lv_obj_t* mode_sw_work_advanced = nullptr;
static lv_obj_t* mode_btn_ftp_upload = nullptr;
static lv_obj_t* mode_btn_ftp_download = nullptr;
static lv_obj_t* mode_btn_ftp_cancel = nullptr;
static lv_obj_t* mode_btn_ftp_retry = nullptr;
static lv_obj_t* mode_dd_control = nullptr;
static lv_obj_t* mode_dd_llm = nullptr;
static lv_obj_t* mode_dd_ai_mode = nullptr;
static lv_obj_t* mode_lbl_work_hint = nullptr;
static lv_obj_t* ctrl_bar = nullptr;
static lv_obj_t* ctrl_dd_agent = nullptr;
static lv_obj_t* ctrl_dd_ai_host = nullptr;
static lv_obj_t* ctrl_btn_text_voice = nullptr;
static lv_obj_t* mode_dd_chat_ai_mode = nullptr;
static lv_obj_t* mode_lbl_chat_status = nullptr;
static lv_obj_t* mode_ta_user_name = nullptr;
static lv_obj_t* mode_ta_user_role = nullptr;
static lv_obj_t* mode_ta_ai_name = nullptr;
static lv_obj_t* mode_ta_ai_role = nullptr;
static lv_obj_t* mode_ta_ai_persona = nullptr;
static lv_obj_t* mode_ta_ai_skills = nullptr;
static lv_obj_t* mode_dd_user_avatar = nullptr;
static lv_obj_t* mode_dd_ai_avatar = nullptr;
static lv_obj_t* mode_remote_warning_bar = nullptr;
static lv_obj_t* mode_sw_remote_guard = nullptr;
static lv_obj_t* mode_lbl_remote_state = nullptr;
static lv_obj_t* mode_lbl_remote_session_state = nullptr;
static lv_obj_t* mode_ta_remote_host = nullptr;
static lv_obj_t* mode_ta_remote_port = nullptr;
static lv_obj_t* mode_btn_remote_request = nullptr;
static lv_obj_t* mode_btn_remote_accept = nullptr;
static lv_obj_t* mode_btn_remote_reject = nullptr;
static lv_obj_t* mode_btn_remote_disconnect = nullptr;
static lv_obj_t* mode_sw_gemma_install = nullptr;
static lv_obj_t* mode_cb_gemma_2b = nullptr;
static lv_obj_t* mode_cb_gemma_9b = nullptr;
static lv_obj_t* mode_cb_gemma_27b = nullptr;
static lv_obj_t* mode_lbl_gemma_recommend = nullptr;
static lv_obj_t* mode_profile_user_avatar_preview = nullptr;
static lv_obj_t* mode_profile_user_text_preview = nullptr;
static lv_obj_t* mode_profile_ai_avatar_preview = nullptr;
static lv_obj_t* mode_profile_ai_text_preview = nullptr;
static lv_obj_t* g_voice_input_overlay = nullptr;
static lv_obj_t* g_voice_input_ta = nullptr;
static HANDLE g_voice_stt_thread = nullptr;
static lv_timer_t* g_voice_stt_poll_timer = nullptr;
static volatile LONG g_voice_stt_running = 0;
static volatile LONG g_voice_stt_done = 0;
static volatile LONG g_voice_stt_active_token = 0;
static DWORD g_voice_stt_start_tick = 0;
static bool g_voice_stt_ok = false;
static char g_voice_stt_text[2048] = {0};
static char g_voice_stt_err[128] = {0};
static volatile LONG g_ask_feedback_pending = 0;
static char g_ask_feedback_reason[256] = {0};
static char g_ask_feedback_suggestion[256] = {0};
static char g_ask_feedback_options[256] = {0};
static char g_pending_feedback_payload[512] = {0};
static bool g_streaming = false;
static char g_ai_next_step[256] = "Idle";
static char g_ai_script_log[8192] = "";
static char g_work_md_doc[32768] = "";
static char g_work_last_prompt[2048] = "";
static bool g_work_waiting_ai = false;
static lv_obj_t* g_remote_arm_modal = nullptr;
static lv_obj_t* g_remote_arm_confirm_btn = nullptr;
static lv_obj_t* g_remote_arm_count_label = nullptr;
static lv_timer_t* g_remote_arm_timer = nullptr;
static int g_remote_arm_countdown = 0;
enum RemoteSessionState { REMOTE_IDLE = 0, REMOTE_REQUESTING = 1, REMOTE_PENDING_ACCEPT = 2, REMOTE_CONNECTED = 3 };
static RemoteSessionState g_remote_state = REMOTE_IDLE;
static lv_timer_t* g_remote_session_timer = nullptr;
static int g_remote_session_left = 0;
static int g_remote_retry_attempt = 0;
static int g_remote_link_down_ticks = 0;
static char g_remote_last_reason[160] = "";
static const int REMOTE_RETRY_MAX = 2;
static const int CTRL_BAR_H = 40;
static int MODE_BAR_H = 36;
enum UiMainMode { UI_MODE_CHAT = 0, UI_MODE_WORK = 1 };
static UiMainMode g_ui_mode = UI_MODE_CHAT;
enum UiNavModule { UI_NAV_BOT = 0, UI_NAV_TASKS = 1, UI_NAV_FILES = 2 };
static UiNavModule g_ui_nav_module = UI_NAV_BOT;
/* chat_cont is declared above for early chat-history restore helpers */
static lv_obj_t* btn_send_widget = nullptr; /* Send button */
static lv_obj_t* btn_upload_widget = nullptr; /* Upload button */
static lv_obj_t* btn_voice_widget = nullptr; /* Voice mode quick button */
static lv_obj_t* btn_work_widget = nullptr; /* Send-as-task quick button */
static lv_obj_t* ctrl_btn_dual_view = nullptr;
static constexpr int CHAT_ACTION_BTN_BASE = 40;
static constexpr int CHAT_ACTION_BTN_MARGIN = 6;
static constexpr int CHAT_ACTION_BTN_GAP = 6;
static constexpr int CHAT_INPUT_MIN_H_BASE = 96;
static constexpr int CHAT_INPUT_MAX_H_BASE = 220;

struct AttachmentQueueItem {
    std::string path;
    bool is_dir = false;
    lv_obj_t* status_lbl = nullptr;
    bool done = false;
};
struct SentAttachmentItem {
    std::string path;
    bool is_dir = false;
};
struct AppliedWriteBackup {
    std::string path;
    std::string bak_path;
};
struct AttachmentRetryCtx {
    char* path = nullptr;
    bool is_dir = false;
    lv_obj_t* status_lbl = nullptr;
};
static std::vector<AttachmentQueueItem> g_attachment_queue;
static lv_timer_t* g_attachment_queue_timer = nullptr;
static std::vector<AttachmentQueueItem> g_attachment_failed_cache;
static std::vector<SentAttachmentItem> g_sent_attachments;
static std::vector<AppliedWriteBackup> g_step_write_backups;
static std::atomic<bool> g_boot_check_running(false);
static std::atomic<bool> g_wizard_gate_running(false);
static std::atomic<bool> g_wizard_gate_ready(false);
static std::mutex g_wizard_gate_mtx;
static EnvCheckResult g_wizard_gate_env{};
static std::mutex g_boot_detail_mtx;
static std::string g_boot_detail_text;
static std::atomic<bool> g_boot_detail_dirty(false);
static unsigned int g_remote_stub_session_seq = 1;
static char g_remote_stub_session_id[64] = "";
static char g_remote_stub_auth_token[64] = "";
static DWORD g_remote_stub_auth_expire_tick = 0;
static bool g_trace_sort_latency = false;
static bool g_trace_fail_only = false;

/* Left panel children that need resize on splitter drag */
static lv_obj_t* lp_panel_title = nullptr;   /* "Gateway Status" label */
static lv_obj_t* lp_status_row = nullptr;
static lv_obj_t* lp_separator = nullptr;
static lv_obj_t* lp_task_title = nullptr;
static lv_obj_t* lp_task_count = nullptr;  /* task count badge "(N)" */
static lv_obj_t* lp_hint = nullptr;
static lv_obj_t* lp_progress_panel = nullptr;
static lv_obj_t* lp_progress_title = nullptr;
static lv_obj_t* lp_progress_step = nullptr;
static lv_obj_t* lp_progress_result = nullptr;
static lv_obj_t* lp_progress_bar = nullptr;

/* P2-3: Task list dynamic widgets */
struct TaskItem {
    char name[128];
    char status[32];    /* Running / Idle / Done / Error */
    char detail[256];   /* Hover detail text (multi-line) */
    char session_key[256]; /* Session key for abort action (empty if not abortable) */
    lv_obj_t* widget;   /* Container widget */
    lv_obj_t* label;    /* Task name label */
    lv_obj_t* status_lbl; /* Status label */
    lv_obj_t* abort_btn;  /* Abort/reset button (nullptr if no action) */
    lv_obj_t* tooltip;  /* Hover detail popup (nullptr when hidden) */
};
#define MAX_TASK_WIDGETS 12
static TaskItem g_tasks[MAX_TASK_WIDGETS];
static int g_task_count = 0;
static int g_task_next_y = 0;  /* Next task item y position (consistent with rg=12) */
static lv_obj_t* task_empty_label = nullptr;  /* "No tasks yet" placeholder */

/* ── TASK-054: Task hover detail popup ── */
static bool g_tooltip_busy = false;  /* re-entrancy guard */
static lv_timer_t* g_hover_timer = nullptr;  /* debounce timer */
static TaskItem* g_pending_hover_item = nullptr;  /* item pending tooltip */

static void task_tooltip_destroy(TaskItem* t) {
    if (g_tooltip_busy) return;
    if (t->tooltip) {
        lv_obj_t* tip = t->tooltip;
        t->tooltip = nullptr;  /* clear BEFORE delete to prevent re-entry */
        lv_obj_del(tip);
        LOG_D("TOOLTIP", "Destroyed tooltip for: %s", t->name);
    }
}

/* Cancel any pending hover timer */
static void cancel_hover_timer(void) {
    if (g_hover_timer) {
        lv_timer_del(g_hover_timer);
        g_hover_timer = nullptr;
    }
    g_pending_hover_item = nullptr;
}

/* Forward declaration */
static void task_tooltip_create(TaskItem* t);

/* Timer callback: show tooltip after debounce delay */
static void on_hover_timer(lv_timer_t* t) {
    (void)t;
    if (g_pending_hover_item && !g_tooltip_busy) {
        task_tooltip_create(g_pending_hover_item);
    }
    g_pending_hover_item = nullptr;
    g_hover_timer = nullptr;
}

static void task_tooltip_create(TaskItem* t) {
    if (t->tooltip || !t->detail[0] || !t->widget || g_tooltip_busy) return;

    g_tooltip_busy = true;

    /* Create tooltip as child of screen (not left_panel) so it floats above */
    lv_obj_t* tip = lv_obj_create(lv_scr_act());
    t->tooltip = tip;

    /* Disable input on tooltip — let mouse events pass through to task items */
    lv_obj_add_flag(tip, LV_OBJ_FLAG_EVENT_BUBBLE);
    lv_obj_clear_flag(tip, LV_OBJ_FLAG_CLICKABLE);

    /* Size based on content — 22% of window width */
    lv_obj_set_width(tip, LV_PCT(22));
    lv_obj_set_height(tip, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_color(tip, g_colors->raised, 0);
    lv_obj_set_style_border_color(tip, g_colors->border_strong, 0);
    lv_obj_set_style_border_width(tip, 1, 0);
    lv_obj_set_style_radius(tip, SCALE(g_colors->radius_md), 0);
    lv_obj_set_style_pad_all(tip, 12, 0);
    lv_obj_set_style_shadow_width(tip, 16, 0);
    lv_obj_set_style_shadow_color(tip, g_colors->shadow_md, 0);
    lv_obj_set_style_shadow_opa(tip, LV_OPA_40, 0);
    lv_obj_clear_flag(tip, LV_OBJ_FLAG_SCROLLABLE);

    /* Title: task name */
    lv_obj_t* title = lv_label_create(tip);
    lv_label_set_text(title, t->name);
    lv_obj_set_style_text_color(title, g_colors->accent_secondary, 0);
    lv_obj_set_style_text_font(title, CJK_FONT_SMALL, 0);
    lv_label_set_long_mode(title, LV_LABEL_LONG_MODE_DOTS);
    lv_obj_set_width(title, lv_pct(100));

    /* Detail text */
    lv_obj_t* detail_lbl = lv_label_create(tip);
    lv_label_set_text(detail_lbl, t->detail);
    lv_obj_set_style_text_color(detail_lbl, g_colors->text, 0);
    lv_obj_set_style_text_font(detail_lbl, CJK_FONT_SMALL, 0);
    lv_label_set_long_mode(detail_lbl, LV_LABEL_LONG_MODE_WRAP);
    lv_obj_set_width(detail_lbl, lv_pct(100));
    lv_obj_set_style_pad_top(detail_lbl, 6, 0);

    /* Position: right of task item, vertically aligned.
     * Use lv_obj_get_coords (cheap) instead of lv_obj_update_layout (expensive). */
    lv_area_t widget_area;
    lv_obj_get_coords(t->widget, &widget_area);
    int32_t tx = widget_area.x2 + 8;
    int32_t ty = widget_area.y1;

    /* Estimate tooltip width from content (avoids full layout pass) */
    lv_point_t txt_sz;
    lv_text_get_size(&txt_sz, t->detail, CJK_FONT_SMALL, 0, 0, LV_COORD_MAX, LV_TEXT_FLAG_NONE);
    lv_point_t name_sz;
    lv_text_get_size(&name_sz, t->name, CJK_FONT_SMALL, 0, 0, LV_COORD_MAX, LV_TEXT_FLAG_NONE);
    int32_t est_tip_w = (txt_sz.x > name_sz.x ? txt_sz.x : name_sz.x) + 24;  /* + padding/border */
    int32_t scr_w = lv_obj_get_width(lv_scr_act());
    if (tx + est_tip_w > scr_w - 8) {
        /* Fall back: show below the task item */
        tx = widget_area.x1;
        ty = widget_area.y2 + 4;
    }
    lv_obj_set_pos(tip, tx, ty);

    g_tooltip_busy = false;
}

static void task_hover_cb(lv_event_t* e) {
    if (g_tooltip_busy) return;

    lv_obj_t* target = lv_event_get_target_obj(e);
    lv_event_code_t code = lv_event_get_code(e);

    /* Find which TaskItem this widget belongs to */
    TaskItem* found = nullptr;
    for (int i = 0; i < g_task_count; i++) {
        if (g_tasks[i].widget == target) { found = &g_tasks[i]; break; }
    }
    if (!found) return;

    if (code == LV_EVENT_HOVER_OVER) {
        /* Debounce: cancel any pending timer, start a new 100ms delay.
         * Only the final resting position creates the tooltip. */
        cancel_hover_timer();
        g_pending_hover_item = found;
        g_hover_timer = lv_timer_create(on_hover_timer, 100, nullptr);
    } else if (code == LV_EVENT_HOVER_LEAVE) {
        /* Immediate destroy on leave — no need to debounce. */
        cancel_hover_timer();
        task_tooltip_destroy(found);
    }
}

/* Destroy all tooltips (call before task_clear) */
static void task_destroy_all_tooltips() {
    cancel_hover_timer();
    g_tooltip_busy = true;
    for (int i = 0; i < g_task_count; i++) {
        task_tooltip_destroy(&g_tasks[i]);
    }
    g_tooltip_busy = false;
}

/* P2: Chat area widgets */
static lv_obj_t* chat_display = nullptr;
lv_obj_t* chat_input = nullptr;
/* chat_history is declared above for early clear/load helpers */

/* ── Log system (buffered with level) ── */
/* LOG_MAX_LINES defined in app_config.h */
#define LOG_LINE_LEN   256

/* Log entry: text + level for filtering */
struct UiLogEntry {
    char text[LOG_LINE_LEN];
    int  level; /* 0=DEBUG, 1=INFO, 2=WARN, 3=ERROR */
};
static UiLogEntry log_entries[LOG_MAX_LINES];
static int  log_count = 0;
static int  log_filter_level = 0; /* Show all levels >= this (UI filter) */
struct PendingUiLogEntry {
    char text[LOG_LINE_LEN];
    int level;
};
static std::mutex g_ui_log_queue_mtx;
static std::deque<PendingUiLogEntry> g_ui_log_queue;
static lv_timer_t* g_ui_log_flush_timer = nullptr;
struct PendingProgressEvent {
    int type; /* 0 begin/update, 1 finish */
    bool ok;
    int percent;
    char task[96];
    char step[192];
    char result[192];
};
static void wizard_progress_mirror_from_event(const PendingProgressEvent& ev, int pct);
static std::mutex g_progress_queue_mtx;
static std::deque<PendingProgressEvent> g_progress_queue;

/* Log display refresh (no-op: UI panel not in main view) */
static void log_refresh_display() { }

static void ui_log_append_entry(const char* timed, int level) {
    if (!timed) return;
    if (log_count >= LOG_MAX_LINES) {
        for (int i = 0; i < LOG_MAX_LINES - 1; i++) log_entries[i] = log_entries[i + 1];
        log_count = LOG_MAX_LINES - 1;
    }
    strncpy(log_entries[log_count].text, timed, LOG_LINE_LEN - 1);
    log_entries[log_count].text[LOG_LINE_LEN - 1] = 0;
    log_entries[log_count].level = level;
    log_count++;
}

static void ui_log_flush_pending() {
    std::deque<PendingUiLogEntry> local;
    {
        std::lock_guard<std::mutex> lk(g_ui_log_queue_mtx);
        if (g_ui_log_queue.empty()) return;
        local.swap(g_ui_log_queue);
    }
    for (const auto& it : local) ui_log_append_entry(it.text, it.level);
    log_refresh_display();
}

static void toast_flush_pending();

static void ui_log_flush_timer_cb(lv_timer_t* t) {
    (void)t;
    const ThemeColors* c = g_colors ? g_colors : &THEME_DARK;
    if (g_ui_thread_id != 0 && GetCurrentThreadId() == g_ui_thread_id) {
        ui_log_flush_pending();
        toast_flush_pending();
        std::deque<PendingProgressEvent> local;
        {
            std::lock_guard<std::mutex> lk(g_progress_queue_mtx);
            if (!g_progress_queue.empty()) local.swap(g_progress_queue);
        }
        for (const auto& ev : local) {
            int pct = ev.percent;
            if (pct < 0) pct = 0;
            if (pct > 100) pct = 100;
            if (lp_progress_panel && lp_progress_bar) {
                lv_obj_clear_flag(lp_progress_panel, LV_OBJ_FLAG_HIDDEN);
                if (lp_progress_title) lv_label_set_text_fmt(lp_progress_title, "Task: %s", ev.task[0] ? ev.task : "Long Task");
                if (lp_progress_step) lv_label_set_text_fmt(lp_progress_step, "Step: %s", ev.step[0] ? ev.step : "Running...");
                lv_bar_set_value(lp_progress_bar, pct, LV_ANIM_ON);
                if (lp_progress_result) {
                    if (ev.type == 1) {
                        lv_label_set_text(lp_progress_result, ev.result[0] ? ev.result : (ev.ok ? "Done" : "Failed"));
                        lv_obj_set_style_text_color(lp_progress_result, ev.ok ? c->success : c->danger, 0);
                    } else {
                        lv_label_set_text(lp_progress_result, "");
                    }
                }
            }

            wizard_progress_mirror_from_event(ev, pct);

            if (strstr(ev.task, "Startup")) {
                {
                    std::lock_guard<std::mutex> lk(g_loading_live_mtx);
                    if (ev.type == 1) {
                        g_loading_live_step = ev.result[0] ? ev.result : "Startup complete";
                        g_loading_live_pct = 100;
                    } else {
                        g_loading_live_step = ev.step[0] ? ev.step : "Background startup";
                        g_loading_live_pct = pct;
                    }
                }
                if (status_label && ev.step[0]) {
                    lv_label_set_text(status_label, ev.step);
                }
                if (ev.type == 1) {
                    g_loading_startup_done.store(true);
                }
            }

            if (mode_boot_progress_panel && strstr(ev.task, "Boot Check")) {
                lv_obj_clear_flag(mode_boot_progress_panel, LV_OBJ_FLAG_HIDDEN);
                if (mode_boot_progress_task) lv_label_set_text_fmt(mode_boot_progress_task, "Task: %s", ev.task[0] ? ev.task : "Boot Check");
                if (mode_boot_progress_step) lv_label_set_text_fmt(mode_boot_progress_step, "Step: %s", ev.step[0] ? ev.step : "Running...");
                if (mode_boot_progress_bar) lv_bar_set_value(mode_boot_progress_bar, pct, LV_ANIM_ON);
                if (mode_boot_progress_result) {
                    if (ev.type == 1) {
                        lv_label_set_text(mode_boot_progress_result, ev.result[0] ? ev.result : (ev.ok ? "Done" : "Failed"));
                        lv_obj_set_style_text_color(mode_boot_progress_result, ev.ok ? g_colors->success : g_colors->danger, 0);
                    } else {
                        lv_label_set_text(mode_boot_progress_result, "");
                    }
                }
            }
        }

        if (g_boot_detail_dirty.exchange(false) && mode_boot_progress_list) {
            std::string local_detail;
            {
                std::lock_guard<std::mutex> lk(g_boot_detail_mtx);
                local_detail = g_boot_detail_text;
            }
            lv_textarea_set_text(mode_boot_progress_list, local_detail.c_str());
        }
    }
}

/* Get total log entry count */
int ui_log_get_count() { return log_count; }

/* Get log entry by visible index (after filter), returns nullptr if out of range */
const char* ui_log_get_line(int visible_index) {
    int vis = 0;
    for (int i = 0; i < log_count; i++) {
        if (log_entries[i].level >= log_filter_level) {
            if (vis == visible_index) return log_entries[i].text;
            vis++;
        }
    }
    return nullptr;
}

/* Get log level for a visible index */
int ui_log_get_level(int visible_index) {
    int vis = 0;
    for (int i = 0; i < log_count; i++) {
        if (log_entries[i].level >= log_filter_level) {
            if (vis == visible_index) return log_entries[i].level;
            vis++;
        }
    }
    return -1;
}

/* Get visible count (after filter) */
int ui_log_get_visible_count() {
    int vis = 0;
    for (int i = 0; i < log_count; i++) {
        if (log_entries[i].level >= log_filter_level) vis++;
    }
    return vis;
}

/* Set UI filter level */
void ui_log_set_filter(int level) { log_filter_level = level; }
int  ui_log_get_filter() { return log_filter_level; }

/* Clear all UI log entries */
void ui_log_clear_entries() {
    log_count = 0;
    memset(log_entries, 0, sizeof(log_entries));
    log_refresh_display();
}

void ui_log(const char* fmt, ...) {
    char buf[LOG_LINE_LEN];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    char timed[LOG_LINE_LEN + 32];
    SYSTEMTIME st;
    GetLocalTime(&st);
    snprintf(timed, sizeof(timed), "[%04d-%02d-%02d %02d:%02d:%02d] %s", st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond, buf);

    /* Determine level from prefix: [ERROR], [WARN], otherwise INFO */
    int level = 1; /* default INFO */
    if (strstr(buf, "[ERROR]")) level = 3;
    else if (strstr(buf, "[WARN]")) level = 2;
    else if (strstr(buf, "[DEBUG]")) level = 0;

    /* Cross-thread safe: workers enqueue, UI thread flushes periodically. */
    if (g_ui_thread_id != 0 && GetCurrentThreadId() != g_ui_thread_id) {
        PendingUiLogEntry e{};
        strncpy(e.text, timed, LOG_LINE_LEN - 1);
        e.text[LOG_LINE_LEN - 1] = '\0';
        e.level = level;
        std::lock_guard<std::mutex> lk(g_ui_log_queue_mtx);
        if (g_ui_log_queue.size() > 512) g_ui_log_queue.pop_front();
        g_ui_log_queue.push_back(e);
        return;
    }

    ui_log_flush_pending();
    ui_log_append_entry(timed, level);
    log_refresh_display();
}

static void enqueue_progress_event(int type, const char* task, const char* step, int percent, bool ok, const char* result) {
    PendingProgressEvent ev{};
    ev.type = type;
    ev.ok = ok;
    ev.percent = percent;
    snprintf(ev.task, sizeof(ev.task), "%s", task ? task : "");
    snprintf(ev.step, sizeof(ev.step), "%s", step ? step : "");
    snprintf(ev.result, sizeof(ev.result), "%s", result ? result : "");
    std::lock_guard<std::mutex> lk(g_progress_queue_mtx);
    if (g_progress_queue.size() > 256) g_progress_queue.pop_front();
    g_progress_queue.push_back(ev);
}

void ui_progress_begin(const char* task, const char* step, int percent) {
    enqueue_progress_event(0, task, step, percent, true, "");
}

void ui_progress_update(const char* task, const char* step, int percent) {
    enqueue_progress_event(0, task, step, percent, true, "");
}

void ui_progress_finish(const char* task, bool ok, const char* result) {
    enqueue_progress_event(1, task, "", 100, ok, result ? result : "");
}

void update_ui_language();

/* ════════════════════════════════════════════════════════════
 *  Toast Notification System (UI-37)
 *
 *  轻量级应用内通知：底部弹出，自动消失。
 *  线程安全：任意线程调用 ui_toast()，UI 线程渲染。
 * ════════════════════════════════════════════════════════════ */
/* ToastType defined in app.h */

struct PendingToast {
    char text[512];
    ToastType type;
    int duration_ms;
};
static std::mutex g_toast_queue_mtx;
static std::deque<PendingToast> g_toast_queue;
static lv_obj_t* g_toast_container = nullptr;
static lv_obj_t* g_toast_label = nullptr;
static lv_timer_t* g_toast_timer = nullptr;

static void toast_timer_cb(lv_timer_t* t) {
    (void)t;
    if (g_toast_container) {
        lv_obj_add_flag(g_toast_container, LV_OBJ_FLAG_HIDDEN);
    }
    if (g_toast_timer) {
        lv_timer_del(g_toast_timer);
        g_toast_timer = nullptr;
    }
}

static void toast_render(const char* text, ToastType type, int duration_ms) {
    const ThemeColors* c = g_colors;
    if (!g_toast_container) {
        /* Create once at screen bottom center */
        lv_obj_t* scr = lv_screen_active();
        g_toast_container = lv_obj_create(scr);
        lv_obj_set_style_bg_opa(g_toast_container, LV_OPA_COVER, 0);
        lv_obj_set_style_border_width(g_toast_container, 1, 0);
        lv_obj_set_style_radius(g_toast_container, SCALE(g_colors->radius_md), 0);
        lv_obj_set_style_pad_all(g_toast_container, SCALE(10), 0);
        lv_obj_set_style_pad_hor(g_toast_container, SCALE(16), 0);
        lv_obj_set_style_shadow_width(g_toast_container, SCALE(12), 0);
        lv_obj_set_style_shadow_color(g_toast_container, g_colors->shadow_sm, 0);
        lv_obj_set_style_shadow_opa(g_toast_container, LV_OPA_30, 0);
        lv_obj_clear_flag(g_toast_container, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_add_flag(g_toast_container, LV_OBJ_FLAG_FLOATING);

        g_toast_label = lv_label_create(g_toast_container);
        lv_label_set_long_mode(g_toast_label, LV_LABEL_LONG_WRAP);
        lv_obj_set_style_text_font(g_toast_label, CJK_FONT_SMALL, 0);
    }

    /* Set text */
    lv_label_set_text(g_toast_label, text);

    /* Color by type */
    lv_color_t bg, border, txt;
    switch (type) {
        case ToastType::Success:
            bg = blend_with_tint(c->bg, c->success);
            border = c->success;
            txt = c->success;
            break;
        case ToastType::Warning:
            bg = blend_with_tint(c->bg, c->warning);
            border = c->warning;
            txt = c->warning;
            break;
        case ToastType::Error:
            bg = blend_with_tint(c->bg, c->danger);
            border = c->danger;
            txt = c->danger;
            break;
        default: /* Info */
            bg = c->panel;
            border = c->border;
            txt = c->text;
            break;
    }
    lv_obj_set_style_bg_color(g_toast_container, bg, 0);
    lv_obj_set_style_border_color(g_toast_container, border, 0);
    lv_obj_set_style_text_color(g_toast_label, txt, 0);

    /* Size: content-width, max 40% of window */
    int max_w = WIN_W * 40 / 100;
    lv_obj_set_width(g_toast_label, max_w);
    lv_obj_set_width(g_toast_container, LV_SIZE_CONTENT);
    lv_obj_set_height(g_toast_container, LV_SIZE_CONTENT);

    /* Position: bottom center */
    lv_obj_align(g_toast_container, LV_ALIGN_BOTTOM_MID, 0, -SCALE(24));

    /* Show */
    lv_obj_clear_flag(g_toast_container, LV_OBJ_FLAG_HIDDEN);
    lv_obj_move_foreground(g_toast_container);

    /* Auto-dismiss timer */
    if (g_toast_timer) lv_timer_del(g_toast_timer);
    g_toast_timer = lv_timer_create(toast_timer_cb, duration_ms, nullptr);
    lv_timer_set_repeat_count(g_toast_timer, 1);
}

static void toast_flush_pending() {
    std::lock_guard<std::mutex> lk(g_toast_queue_mtx);
    while (!g_toast_queue.empty()) {
        const auto& t = g_toast_queue.front();
        toast_render(t.text, t.type, t.duration_ms);
        g_toast_queue.pop_front();
        /* Only render the latest toast if multiple queued */
    }
}

/* Public API: show a toast notification. Thread-safe. */
void ui_toast(const char* text, ToastType type, int duration_ms) {
    if (!text || !text[0]) return;
    if (duration_ms <= 0) duration_ms = 3000;

    /* If called from UI thread, render directly */
    if (g_ui_thread_id != 0 && GetCurrentThreadId() == g_ui_thread_id) {
        toast_render(text, type, duration_ms);
        return;
    }
    /* Otherwise enqueue for UI thread to flush */
    std::lock_guard<std::mutex> lk(g_toast_queue_mtx);
    PendingToast t{};
    strncpy(t.text, text, sizeof(t.text) - 1);
    t.type = type;
    t.duration_ms = duration_ms;
    g_toast_queue.push_back(t);
}

/* Convenience wrappers */
void ui_toast_info(const char* text)    { ui_toast(text, ToastType::Info, 3000); }
void ui_toast_success(const char* text) { ui_toast(text, ToastType::Success, 3000); }
void ui_toast_warn(const char* text)    { ui_toast(text, ToastType::Warning, 4000); }
void ui_toast_error(const char* text)   { ui_toast(text, ToastType::Error, 5000); }

/* ── Window control button callbacks ── */
static void btn_minimize_cb(lv_event_t* e) {
    (void)e;
    /* Use Win32 API directly - SDL_MinimizeWindow may not work on all setups */
    extern SDL_Window* app_get_window();
    SDL_Window* sdlWin = app_get_window();
    if (sdlWin) {
        SDL_SysWMinfo wmInfo;
        SDL_VERSION(&wmInfo.version);
        if (SDL_GetWindowWMInfo(sdlWin, &wmInfo)) {
            ShowWindow(wmInfo.info.win.window, SW_MINIMIZE);
        }
    }
    ui_log("[Window] Minimized");
}

static void btn_maximize_cb(lv_event_t* e) {
    (void)e;
    SDL_Window* win = app_get_window();
    if (!win) return;

    if (g_maximized) {
        /* Restore to normal size */
        SDL_RestoreWindow(win);
        g_maximized = false;
        if (lbl_maximize) lv_label_set_text(lbl_maximize, "\xE2\x96\xA1");  /* □ */
        ui_log("[Window] Restored to normal size");
    } else {
        /* Maximize */
        SDL_MaximizeWindow(win);
        g_maximized = true;
        if (lbl_maximize) lv_label_set_text(lbl_maximize, "\xE2\xA7\xA9");  /* ⧉ */
        ui_log("[Window] Maximized");
    }
}

static void btn_close_cb(lv_event_t* e) {
    (void)e;
    /* Save config before minimizing to tray */
    extern void save_theme_config();
    save_theme_config();
    /* Minimize to tray instead of quitting */
    tray_show_window(false);
    ui_log("[Window] Minimized to tray");
}

/* Forward declarations */
static void relayout_panels();
static void apply_mode_switch_visuals();
static void update_work_mode_hint();
static bool ask_mode_confirm_action(const char* reason, const char* suggestion, const char* options_csv = nullptr, char* out_answer = nullptr, size_t out_answer_cap = 0);
static void update_remote_session_visuals();
static void stop_remote_session_timer();
static bool remote_begin_request(bool retry_mode);
static void remote_reset_to_idle(const char* reason, bool stop_tcp, bool close_proto);
void ui_relayout_all();
static int mode_content_h() { return std::max(120, PANEL_H - CHAT_GAP * 2); }
static int mode_content_w() { return std::max(200, RIGHT_PANEL_W - CHAT_GAP * 2); }
static int chat_trace_panel_h(int content_w) { return (content_w < SCALE(900)) ? SCALE(128) : SCALE(96); }
static int chat_top_y_with_trace(int content_w) {
    if (!mode_trace_chat_panel) return CHAT_GAP;
    return CHAT_GAP + chat_trace_panel_h(content_w) + CHAT_GAP;
}
extern void save_theme_config();
static const char* profile_user_name() { return g_profile_user_name[0] ? g_profile_user_name : "User"; }
static const char* profile_ai_name() { return g_profile_ai_name[0] ? g_profile_ai_name : "AI"; }
static const char* profile_user_avatar_src() {
    return (strcmp(g_profile_user_avatar, "lobster") == 0) ? "A:assets/icons/ai/lobster_01_24.png" : "A:assets/garlic_32.png";
}
static const char* profile_ai_avatar_src() {
    return (strcmp(g_profile_ai_avatar, "garlic") == 0) ? "A:assets/garlic_32.png" : "A:assets/icons/ai/lobster_01_24.png";
}

static bool attachment_is_text_ext(const std::string& path) {
    std::string lower = path;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
    return (lower.find(".txt") != std::string::npos) ||
           (lower.find(".md") != std::string::npos) ||
           (lower.find(".json") != std::string::npos) ||
           (lower.find(".cpp") != std::string::npos) ||
           (lower.find(".h") != std::string::npos) ||
           (lower.find(".py") != std::string::npos) ||
           (lower.find(".log") != std::string::npos) ||
           (lower.find(".yaml") != std::string::npos) ||
           (lower.find(".yml") != std::string::npos) ||
           (lower.find(".ini") != std::string::npos);
}

static bool attachment_is_probably_binary(const std::string& path) {
    std::ifstream f(path, std::ios::binary);
    if (!f.is_open()) return true;
    char buf[1024] = {0};
    f.read(buf, sizeof(buf));
    std::streamsize n = f.gcount();
    if (n <= 0) return true;
    int bad = 0;
    for (int i = 0; i < (int)n; i++) {
        unsigned char ch = (unsigned char)buf[i];
        if (ch == 0) return true;
        if (ch < 9 || (ch > 13 && ch < 32)) bad++;
    }
    return bad > (int)(n / 8);
}

static void attachment_collect_files(const std::string& root,
                                     std::vector<std::string>& out,
                                     int max_files,
                                     int max_depth) {
    std::error_code ec;
    if (!std::filesystem::exists(root, ec) || ec) return;
    std::filesystem::recursive_directory_iterator it(root, ec), end;
    while (!ec && it != end) {
        if ((int)out.size() >= max_files) break;
        if ((int)it.depth() > max_depth) {
            it.disable_recursion_pending();
            ++it;
            continue;
        }
        if (it->is_regular_file(ec) && !ec) {
            std::string fp = it->path().string();
            if (permissions().is_path_allowed(fp.c_str(), false)) {
                out.push_back(fp);
            }
        }
        ++it;
    }
}

/* Truncate text snippet: prefer line boundary (max 50 lines), fallback to byte limit */
static std::string truncate_snippet(const std::string& raw, uint64_t max_bytes) {
    if (raw.size() <= max_bytes) return raw;
    int line_count = 0;
    uint64_t cut = 0;
    for (uint64_t i = 0; i < raw.size() && i < max_bytes; i++) {
        if (raw[i] == '\n') {
            line_count++;
            if (line_count >= 50) { cut = i; break; }
        }
    }
    if (cut == 0) cut = max_bytes; /* no line boundary found, hard cut */
    return raw.substr(0, cut);
}

static void submit_prompt_to_chat(const char* text) {
    if (!text || !text[0]) return;
    if (is_streaming_now()) {
        ui_log("[Chat] A response is still streaming, ignore new submit");
        return;
    }
    chat_add_user_bubble(text);
    chat_force_scroll_bottom();
    std::string payload = text;
    if (!g_sent_attachments.empty()) {
        constexpr int kMaxAttachItems = 10;
        constexpr int kMaxDirExpandFiles = 20;
        constexpr int kMaxDirDepth = 2;
        constexpr uint64_t kMaxTotalSnippetBytes = 32768;
        constexpr uint64_t kMaxSingleSnippetBytes = 4096;
        std::string att = "\n\nAttached files for this request:\n";
        std::unordered_set<std::string> seen;
        uint64_t total_snippet_bytes = 0;
        int take = 0;
        for (const auto& a : g_sent_attachments) {
            if (take >= kMaxAttachItems) break;
            if (a.is_dir) {
                std::vector<std::string> files;
                attachment_collect_files(a.path, files, kMaxDirExpandFiles, kMaxDirDepth);
                att += "- " + a.path + " [directory, expanded files=" + std::to_string((int)files.size()) + "]\n";
                for (const auto& fp : files) {
                    if (take >= kMaxAttachItems) break;
                    if (!seen.insert(fp).second) continue;
                    if (!permissions().is_path_allowed(fp.c_str(), false)) {
                        att += "  - " + fp + " (skipped: permission denied)\n";
                        continue;
                    }
                    std::error_code fec;
                    uint64_t sz = (uint64_t)std::filesystem::file_size(fp, fec);
                    if (fec) continue;
                    att += "  - " + fp + " (" + std::to_string((unsigned long long)sz) + " bytes)\n";
                    bool can_snip = attachment_is_text_ext(fp) && !attachment_is_probably_binary(fp) &&
                                    sz <= (256 * 1024) && total_snippet_bytes < kMaxTotalSnippetBytes;
                    if (can_snip) {
                        std::ifstream f(fp, std::ios::binary);
                        std::string snippet((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
                        snippet = truncate_snippet(snippet, kMaxSingleSnippetBytes);
                        total_snippet_bytes += (uint64_t)snippet.size();
                        att += "    snippet:\n    ```text\n" + snippet + "\n    ```\n";
                    }
                    take++;
                }
            } else {
                const std::string fp = a.path;
                if (!seen.insert(fp).second) continue;
                if (!permissions().is_path_allowed(fp.c_str(), false)) {
                    att += "- " + fp + " [skipped: permission denied]\n";
                    continue;
                }
                std::error_code fec;
                uint64_t sz = (uint64_t)std::filesystem::file_size(fp, fec);
                if (fec) sz = 0;
                att += "- " + fp + " [file, " + std::to_string((unsigned long long)sz) + " bytes]\n";
                bool can_snip = attachment_is_text_ext(fp) && !attachment_is_probably_binary(fp) &&
                                sz <= (256 * 1024) && total_snippet_bytes < kMaxTotalSnippetBytes;
                if (can_snip) {
                    std::ifstream f(fp, std::ios::binary);
                    if (f.is_open()) {
                        std::string snippet((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
                        snippet = truncate_snippet(snippet, kMaxSingleSnippetBytes);
                        total_snippet_bytes += (uint64_t)snippet.size();
                        att += "  snippet:\n  ```text\n" + snippet + "\n  ```\n";
                    }
                } else if (!attachment_is_text_ext(fp) || attachment_is_probably_binary(fp)) {
                    att += "  snippet: skipped (binary or unsupported type)\n";
                } else {
                    att += "  snippet: skipped (size limit)\n";
                }
                take++;
            }
        }
        payload += att;
        g_sent_attachments.clear();
        ui_log("[Share] Attached files injected into chat payload");
    }
    std::string kb_ctx = KbStore::instance().build_context_snippet(text, 3, 1000);
    if (!kb_ctx.empty()) {
        /* Prepend KB context but preserve any attachment info already appended to payload */
        payload = kb_ctx + "\nUser request:\n" + payload;
        ui_log("[KB] Attached local context to chat request");
    }
    chat_start_stream(payload.c_str());
}

static void update_remote_guard_visuals() {
    if (mode_remote_warning_bar) {
        if (g_remote_guard_armed) {
            lv_obj_clear_flag(mode_remote_warning_bar, LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_obj_add_flag(mode_remote_warning_bar, LV_OBJ_FLAG_HIDDEN);
        }
    }
    if (mode_lbl_remote_state) {
        lv_label_set_text(mode_lbl_remote_state,
                          g_remote_guard_armed
                              ? "Remote guard: ARMED (double confirm + countdown required)"
                              : "Remote guard: DISARMED");
    }
}

static void ui_alert_with_sound(const char* title, const char* message) {
    const char* t = (title && title[0]) ? title : "AnyClaw Alert";
    const char* m = (message && message[0]) ? message : "Unknown error";
    static DWORD s_last_tick = 0;
    static char s_last_msg[160] = {0};
    DWORD now = GetTickCount();
    if (s_last_msg[0] && strcmp(s_last_msg, m) == 0 && (now - s_last_tick) < 8000) {
        return;
    }
    s_last_tick = now;
    snprintf(s_last_msg, sizeof(s_last_msg), "%s", m);
    MessageBeep(MB_ICONERROR);
    MessageBoxA(nullptr, m, t, MB_OK | MB_ICONERROR | MB_TOPMOST);
}

static void update_profile_preview_cards() {
    if (mode_profile_user_avatar_preview) lv_img_set_src(mode_profile_user_avatar_preview, profile_user_avatar_src());
    if (mode_profile_ai_avatar_preview) lv_img_set_src(mode_profile_ai_avatar_preview, profile_ai_avatar_src());
    if (mode_profile_user_text_preview) {
        lv_label_set_text_fmt(mode_profile_user_text_preview, "%s\n%s",
                              profile_user_name(),
                              g_profile_user_role[0] ? g_profile_user_role : "Owner");
    }
    if (mode_profile_ai_text_preview) {
        lv_label_set_text_fmt(mode_profile_ai_text_preview, "%s\n%s",
                              profile_ai_name(),
                              g_profile_ai_role[0] ? g_profile_ai_role : "Assistant");
    }
}

static void close_remote_arm_modal() {
    if (g_remote_arm_timer) {
        lv_timer_del(g_remote_arm_timer);
        g_remote_arm_timer = nullptr;
    }
    if (g_remote_arm_modal) {
        lv_obj_del(g_remote_arm_modal);
        g_remote_arm_modal = nullptr;
        g_remote_arm_confirm_btn = nullptr;
        g_remote_arm_count_label = nullptr;
    }
}

static void remote_arm_modal_confirm_cb(lv_event_t* e) {
    (void)e;
    g_remote_guard_armed = true;
    if (mode_sw_remote_guard) lv_obj_add_state(mode_sw_remote_guard, LV_STATE_CHECKED);
    update_remote_guard_visuals();
    update_work_mode_hint();
    save_theme_config();
    close_remote_arm_modal();
    LOG_W("REMOTE", "Remote guard armed after double-confirm countdown");
}

static void remote_arm_modal_cancel_cb(lv_event_t* e) {
    (void)e;
    if (mode_sw_remote_guard) lv_obj_clear_state(mode_sw_remote_guard, LV_STATE_CHECKED);
    g_remote_guard_armed = false;
    update_remote_guard_visuals();
    update_work_mode_hint();
    save_theme_config();
    close_remote_arm_modal();
    LOG_I("REMOTE", "Remote guard arming canceled");
}

static void remote_arm_countdown_cb(lv_timer_t* timer) {
    (void)timer;
    if (!g_remote_arm_confirm_btn || !g_remote_arm_count_label) return;
    g_remote_arm_countdown--;
    if (g_remote_arm_countdown <= 0) {
        lv_label_set_text(g_remote_arm_count_label, "Countdown done. Confirm is enabled.");
        lv_obj_clear_state(g_remote_arm_confirm_btn, LV_STATE_DISABLED);
        if (g_remote_arm_timer) {
            lv_timer_del(g_remote_arm_timer);
            g_remote_arm_timer = nullptr;
        }
        return;
    }
    lv_label_set_text_fmt(g_remote_arm_count_label, "Enable confirm in %d second(s)...", g_remote_arm_countdown);
}

static void open_remote_arm_modal() {
    close_remote_arm_modal();
    lv_obj_t* scr = lv_screen_active();
    if (!scr) return;
    const ThemeColors* c = g_colors ? g_colors : &THEME_DARK;
    g_remote_arm_modal = lv_obj_create(scr);
    lv_obj_set_size(g_remote_arm_modal, WIN_W, WIN_H);
    lv_obj_set_style_bg_color(g_remote_arm_modal, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(g_remote_arm_modal, LV_OPA_60, 0);
    lv_obj_set_style_border_width(g_remote_arm_modal, 0, 0);
    lv_obj_set_style_pad_all(g_remote_arm_modal, 0, 0);
    lv_obj_clear_flag(g_remote_arm_modal, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* box = lv_obj_create(g_remote_arm_modal);
    int remote_box_w = std::min(WIN_W * 39 / 100, WIN_W - 48);
    int remote_box_h = WIN_H * 31 / 100;
    lv_obj_set_size(box, remote_box_w, remote_box_h);
    lv_obj_center(box);
    lv_obj_set_style_bg_color(box, c->panel, 0);
    lv_obj_set_style_border_color(box, c->border, 0);
    lv_obj_set_style_border_width(box, 1, 0);
    lv_obj_set_style_radius(box, SCALE(g_colors->radius_lg), 0);
    lv_obj_set_style_pad_all(box, 14, 0);
    lv_obj_set_style_pad_gap(box, 10, 0);
    lv_obj_set_flex_flow(box, LV_FLEX_FLOW_COLUMN);
    lv_obj_clear_flag(box, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* t = lv_label_create(box);
    lv_label_set_text(t, "Remote guard arming requires second confirmation");
    lv_obj_set_style_text_color(t, c->text, 0);
    lv_obj_set_style_text_font(t, CJK_FONT, 0);

    lv_obj_t* d = lv_label_create(box);
    lv_label_set_text(d, "Safety policy: double confirmation + countdown.\nOnly arm if you are ready for remote control workflow.");
    lv_obj_set_style_text_color(d, c->text_dim, 0);
    lv_obj_set_style_text_font(d, CJK_FONT_SMALL, 0);

    g_remote_arm_count_label = lv_label_create(box);
    lv_obj_set_style_text_color(g_remote_arm_count_label, c->accent, 0);
    lv_obj_set_style_text_font(g_remote_arm_count_label, CJK_FONT_SMALL, 0);

    lv_obj_t* row = lv_obj_create(box);
    lv_obj_set_size(row, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(row, 0, 0);
    lv_obj_set_style_pad_all(row, 0, 0);
    lv_obj_set_style_pad_gap(row, 10, 0);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(row, LV_FLEX_ALIGN_END, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* btn_cancel = aw_btn_create(row, "Cancel", BTN_SECONDARY, SCALE(110), SCALE(34));
    lv_obj_add_event_cb(btn_cancel, remote_arm_modal_cancel_cb, LV_EVENT_CLICKED, nullptr);
    g_remote_arm_confirm_btn = aw_btn_create(row, "Confirm Arm", BTN_DANGER, SCALE(140), SCALE(34));
    lv_obj_add_event_cb(g_remote_arm_confirm_btn, remote_arm_modal_confirm_cb, LV_EVENT_CLICKED, nullptr);
    lv_obj_add_state(g_remote_arm_confirm_btn, LV_STATE_DISABLED);

    g_remote_arm_countdown = 3;
    lv_label_set_text_fmt(g_remote_arm_count_label, "Enable confirm in %d second(s)...", g_remote_arm_countdown);
    g_remote_arm_timer = lv_timer_create(remote_arm_countdown_cb, 1000, nullptr);
}

static void remote_session_timer_cb(lv_timer_t* t) {
    (void)t;
    if (g_remote_state == REMOTE_CONNECTED) {
        if (!RemoteTcpChannel::instance().is_connected()) {
            g_remote_link_down_ticks++;
            if (g_remote_link_down_ticks >= 2) {
                remote_reset_to_idle("Connection lost, session closed", true, true);
            } else {
                update_remote_session_visuals();
            }
        } else {
            g_remote_link_down_ticks = 0;
        }
        return;
    }
    if (g_remote_state != REMOTE_PENDING_ACCEPT) return;
    g_remote_session_left--;
    if (g_remote_session_left <= 0) {
        stop_remote_session_timer();
        if (g_remote_guard_armed && g_remote_retry_attempt < REMOTE_RETRY_MAX) {
            g_remote_retry_attempt++;
            ui_log("[Remote] Request timed out, auto-retry %d/%d", g_remote_retry_attempt, REMOTE_RETRY_MAX);
            remote_reset_to_idle("", true, true);
            if (!remote_begin_request(true)) {
                remote_reset_to_idle("Auto-retry failed, back to idle", true, true);
            }
        } else {
            remote_reset_to_idle("Request timed out", true, true);
        }
    }
    update_remote_session_visuals();
}

static void stop_remote_session_timer() {
    if (g_remote_session_timer) {
        lv_timer_del(g_remote_session_timer);
        g_remote_session_timer = nullptr;
    }
}

static void remote_reset_to_idle(const char* reason, bool stop_tcp, bool close_proto) {
    stop_remote_session_timer();
    if (stop_tcp) RemoteTcpChannel::instance().stop();
    if (close_proto && g_remote_stub_session_id[0]) {
        RemoteProtocolManager::instance().close_session(g_remote_stub_session_id);
    }
    g_remote_state = REMOTE_IDLE;
    g_remote_link_down_ticks = 0;
    snprintf(g_remote_last_reason, sizeof(g_remote_last_reason), "%s", reason ? reason : "");
    g_remote_stub_session_id[0] = '\0';
    g_remote_stub_auth_token[0] = '\0';
    if (reason && reason[0]) {
        ui_log("[Remote] %s", reason);
        if (strstr(reason, "failed") || strstr(reason, "lost") || strstr(reason, "timeout")) {
            ui_alert_with_sound("Remote Session Alert", reason);
        }
    }
    update_remote_session_visuals();
}

static void update_remote_session_visuals() {
    if (mode_lbl_remote_session_state) {
        const char* text = "Remote session: idle";
        if (g_remote_state == REMOTE_REQUESTING) text = "Remote session: requesting...";
        else if (g_remote_state == REMOTE_PENDING_ACCEPT) {
            static char buf[96] = {0};
            snprintf(buf, sizeof(buf), "Remote session: awaiting acceptance (%ds, retry %d/%d)",
                     g_remote_session_left, g_remote_retry_attempt, REMOTE_RETRY_MAX);
            text = buf;
        } else if (g_remote_state == REMOTE_CONNECTED) {
            static char buf[128] = {0};
            snprintf(buf, sizeof(buf), "Remote session: CONNECTED (tcp=%s, downTicks=%d)",
                     RemoteTcpChannel::instance().is_connected() ? "up" : "down", g_remote_link_down_ticks);
            text = buf;
        } else if (g_remote_last_reason[0]) {
            static char buf[192] = {0};
            snprintf(buf, sizeof(buf), "Remote session: idle (%s)", g_remote_last_reason);
            text = buf;
        }
        lv_label_set_text(mode_lbl_remote_session_state, text);
    }
    if (mode_btn_remote_request) {
        if (g_remote_state == REMOTE_IDLE) lv_obj_clear_state(mode_btn_remote_request, LV_STATE_DISABLED);
        else lv_obj_add_state(mode_btn_remote_request, LV_STATE_DISABLED);
    }
    if (mode_btn_remote_accept) {
        if (g_remote_state == REMOTE_PENDING_ACCEPT) lv_obj_clear_state(mode_btn_remote_accept, LV_STATE_DISABLED);
        else lv_obj_add_state(mode_btn_remote_accept, LV_STATE_DISABLED);
    }
    if (mode_btn_remote_reject) {
        if (g_remote_state == REMOTE_PENDING_ACCEPT) lv_obj_clear_state(mode_btn_remote_reject, LV_STATE_DISABLED);
        else lv_obj_add_state(mode_btn_remote_reject, LV_STATE_DISABLED);
    }
    if (mode_btn_remote_disconnect) {
        if (g_remote_state == REMOTE_CONNECTED) lv_obj_clear_state(mode_btn_remote_disconnect, LV_STATE_DISABLED);
        else lv_obj_add_state(mode_btn_remote_disconnect, LV_STATE_DISABLED);
    }
}

static void remote_stub_prepare_auth_token() {
    auto rec = RemoteProtocolManager::instance().create_session("work_mode", 120000);
    snprintf(g_remote_stub_session_id, sizeof(g_remote_stub_session_id), "%s", rec.session_id.c_str());
    snprintf(g_remote_stub_auth_token, sizeof(g_remote_stub_auth_token), "%s", rec.auth_token.c_str());
    g_remote_stub_auth_expire_tick = (DWORD)rec.expire_tick;
}

static void remote_copy_text_to_clipboard(const char* text) {
    if (!text || !text[0]) return;
    if (!OpenClipboard(nullptr)) return;
    EmptyClipboard();
    size_t len = strlen(text) + 1;
    HGLOBAL h = GlobalAlloc(GMEM_MOVEABLE, len);
    if (h) {
        void* p = GlobalLock(h);
        if (p) {
            memcpy(p, text, len);
            GlobalUnlock(h);
            SetClipboardData(CF_TEXT, h);
        } else {
            GlobalFree(h);
        }
    }
    CloseClipboard();
}

static bool remote_launch_quick_assist(char* err, int err_cap) {
    if (err && err_cap > 0) err[0] = '\0';
    STARTUPINFOA si{};
    PROCESS_INFORMATION pi{};
    si.cb = sizeof(si);
    char cmd[] = "cmd /c start \"\" quickassist:";
    BOOL ok = CreateProcessA(nullptr, cmd, nullptr, nullptr, FALSE, CREATE_NO_WINDOW, nullptr, nullptr, &si, &pi);
    if (!ok) {
        if (err && err_cap > 0) snprintf(err, err_cap, "Quick Assist launch failed: %lu", GetLastError());
        return false;
    }
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    return true;
}

static std::string remote_build_share_code() {
    if (!g_remote_stub_auth_token[0]) return "";
    std::string tok = g_remote_stub_auth_token;
    std::string digits;
    for (char ch : tok) {
        if (ch >= '0' && ch <= '9') digits.push_back(ch);
    }
    while (digits.size() < 6) digits.push_back('0');
    return digits.substr(0, 6);
}

static bool remote_begin_request(bool retry_mode) {
    remote_stub_prepare_auth_token();
    g_remote_last_reason[0] = '\0';
    const char* reason = nullptr;
    if (!RemoteProtocolManager::instance().try_update_state(g_remote_stub_session_id, "requesting", &reason)) {
        ui_log("[Remote] Request rejected by protocol manager: %s", reason ? reason : "unknown");
        return false;
    }
    g_remote_state = REMOTE_REQUESTING;
    update_remote_session_visuals();
    ui_log("[Remote] Session request sent (%s, session=%s)",
           retry_mode ? "retry" : "initial",
           g_remote_stub_session_id);

    g_remote_state = REMOTE_PENDING_ACCEPT;
    reason = nullptr;
    if (!RemoteProtocolManager::instance().try_update_state(g_remote_stub_session_id, "pending_accept", &reason)) {
        ui_log("[Remote] Failed to enter pending_accept: %s", reason ? reason : "unknown");
        remote_reset_to_idle("Failed to enter pending_accept", true, true);
        return false;
    }
    g_remote_session_left = 15;
    stop_remote_session_timer();
    g_remote_session_timer = lv_timer_create(remote_session_timer_cb, 1000, nullptr);
    update_remote_session_visuals();
    return true;
}

static void remote_request_cb(lv_event_t* e) {
    (void)e;
    TraceSpan span("remote_request", "work_mode");
    if (!ask_mode_confirm_action("Start remote collaboration session request", "Confirm host/port and send request")) return;
    if (!g_remote_guard_armed) {
        span.set_fail();
        ui_log("[Remote] Cannot request session: guard is disarmed");
        return;
    }
    if (g_remote_state != REMOTE_IDLE) return;
    g_remote_retry_attempt = 0;
    if (!remote_begin_request(false)) span.set_fail();
    else {
        if (!RemoteTcpChannel::instance().send_text_frame("ctrl", "request")) {
            ui_log("[Remote] ctrl/request frame send failed: %s", RemoteTcpChannel::instance().last_error().c_str());
        }
        char qa_err[160] = {0};
        if (remote_launch_quick_assist(qa_err, sizeof(qa_err))) {
            std::string code = remote_build_share_code();
            if (!code.empty()) {
                char payload[128] = {0};
                snprintf(payload, sizeof(payload), "quickassist_code=%s;session=%s", code.c_str(), g_remote_stub_session_id);
                if (!RemoteTcpChannel::instance().send_text_frame("im", payload)) {
                    ui_log("[Remote] quickassist code IM send failed: %s", RemoteTcpChannel::instance().last_error().c_str());
                }
                remote_copy_text_to_clipboard(code.c_str());
                ui_log("[Remote] Quick Assist started, code=%s (已复制到剪贴板，并通过IM通道发送)", code.c_str());
            } else {
                ui_log("[Remote] Quick Assist started, waiting for code...");
            }
        } else {
            ui_log("[Remote] %s (fall back to manual code sharing)", qa_err[0] ? qa_err : "Quick Assist launch failed");
        }
    }
}

static void remote_accept_cb(lv_event_t* e) {
    (void)e;
    TraceSpan span("remote_accept", g_remote_stub_session_id[0] ? g_remote_stub_session_id : "no-session");
    if (g_remote_state != REMOTE_PENDING_ACCEPT) return;
    bool expired = false;
    bool auth_ok = RemoteProtocolManager::instance().verify_token(
        g_remote_stub_session_id,
        g_remote_stub_auth_token,
        "pending_accept",
        &expired
    );
    if (!auth_ok) {
        span.set_fail();
        ui_log("[Remote] Session accept failed: auth token %s", expired ? "expired" : "invalid");
        remote_reset_to_idle("Session closed after auth failure", true, true);
        return;
    }
    stop_remote_session_timer();
    g_remote_state = REMOTE_CONNECTED;
    const char* reason = nullptr;
    if (!RemoteProtocolManager::instance().try_update_state(g_remote_stub_session_id, "connected", &reason)) {
        span.set_fail();
        ui_log("[Remote] Cannot enter connected state: %s", reason ? reason : "unknown");
        remote_reset_to_idle("Cannot enter connected state, session closed", true, true);
        return;
    }
    RemoteTcpConfig tcp_cfg;
    if (mode_ta_remote_host) {
        const char* host = lv_textarea_get_text(mode_ta_remote_host);
        if (host && host[0]) tcp_cfg.host = host;
    }
    if (mode_ta_remote_port) {
        int p = atoi(lv_textarea_get_text(mode_ta_remote_port));
        if (p > 0 && p < 65536) tcp_cfg.port = p;
    }
    if (!RemoteTcpChannel::instance().start(tcp_cfg)) {
        span.set_fail();
        ui_log("[Remote] TCP channel start failed: %s",
               RemoteTcpChannel::instance().last_error().c_str());
        remote_reset_to_idle("TCP channel start failed, session closed", true, true);
        return;
    }
    g_remote_link_down_ticks = 0;
    stop_remote_session_timer();
    g_remote_session_timer = lv_timer_create(remote_session_timer_cb, 1000, nullptr);
    ui_log("[Remote] Session accepted and connected (session=%s, auth=ok, tcp=%s:%d)",
           g_remote_stub_session_id[0] ? g_remote_stub_session_id : "n/a",
           tcp_cfg.host.c_str(), tcp_cfg.port);
    g_remote_last_reason[0] = '\0';
    if (!RemoteTcpChannel::instance().send_text_frame("ctrl", "accept")) {
        ui_log("[Remote] ctrl/accept frame send failed: %s", RemoteTcpChannel::instance().last_error().c_str());
    }
    update_remote_session_visuals();
}

static void remote_reject_cb(lv_event_t* e) {
    (void)e;
    TraceSpan span("remote_reject", g_remote_stub_session_id[0] ? g_remote_stub_session_id : "no-session");
    if (g_remote_state != REMOTE_PENDING_ACCEPT) return;
    if (!RemoteTcpChannel::instance().send_text_frame("ctrl", "reject")) {
        ui_log("[Remote] ctrl/reject frame send failed: %s", RemoteTcpChannel::instance().last_error().c_str());
    }
    remote_reset_to_idle("Session rejected", true, true);
}

static void remote_disconnect_cb(lv_event_t* e) {
    (void)e;
    TraceSpan span("remote_disconnect", g_remote_stub_session_id[0] ? g_remote_stub_session_id : "no-session");
    if (!RemoteTcpChannel::instance().send_text_frame("ctrl", "disconnect")) {
        ui_log("[Remote] ctrl/disconnect frame send failed: %s", RemoteTcpChannel::instance().last_error().c_str());
    }
    remote_reset_to_idle("Session disconnected", true, true);
}

static int get_total_ram_gb() {
    MEMORYSTATUSEX ms = {};
    ms.dwLength = sizeof(ms);
    if (!GlobalMemoryStatusEx(&ms)) return 0;
    return (int)(ms.ullTotalPhys / (1024ULL * 1024ULL * 1024ULL));
}

static int gemma_recommended_mask() {
    int ram_gb = get_total_ram_gb();
    if (ram_gb <= 0) return 1;     /* 2B */
    if (ram_gb < 16) return 1;     /* 2B */
    if (ram_gb < 32) return 2;     /* 9B */
    return 4;                      /* 27B */
}

static const char* gemma_mask_to_text(int mask) {
    if (mask == 0) return "none";
    static char buf[64] = {0};
    buf[0] = '\0';
    if (mask & 1) strcat(buf, "2B");
    if (mask & 2) strcat(buf, buf[0] ? ",9B" : "9B");
    if (mask & 4) strcat(buf, buf[0] ? ",27B" : "27B");
    return buf;
}

static void update_gemma_recommend_visuals() {
    const int rec = gemma_recommended_mask();
    if (mode_lbl_gemma_recommend) {
        int ram_gb = get_total_ram_gb();
        const char* rec_name = (rec == 4) ? "Gemma 4 27B" : ((rec == 2) ? "Gemma 4 9B" : "Gemma 4 2B");
        lv_label_set_text_fmt(mode_lbl_gemma_recommend,
                              "RAM: %d GB, recommended: %s\nSelected count: %d",
                              ram_gb, rec_name,
                              ((g_gemma_model_mask & 1) ? 1 : 0) + ((g_gemma_model_mask & 2) ? 1 : 0) + ((g_gemma_model_mask & 4) ? 1 : 0));
    }
    if (mode_cb_gemma_2b) {
        if (g_gemma_install_opt_in) lv_obj_clear_state(mode_cb_gemma_2b, LV_STATE_DISABLED);
        else lv_obj_add_state(mode_cb_gemma_2b, LV_STATE_DISABLED);
        if (g_gemma_model_mask & 1) lv_obj_add_state(mode_cb_gemma_2b, LV_STATE_CHECKED);
        else lv_obj_clear_state(mode_cb_gemma_2b, LV_STATE_CHECKED);
    }
    if (mode_cb_gemma_9b) {
        if (g_gemma_install_opt_in) lv_obj_clear_state(mode_cb_gemma_9b, LV_STATE_DISABLED);
        else lv_obj_add_state(mode_cb_gemma_9b, LV_STATE_DISABLED);
        if (g_gemma_model_mask & 2) lv_obj_add_state(mode_cb_gemma_9b, LV_STATE_CHECKED);
        else lv_obj_clear_state(mode_cb_gemma_9b, LV_STATE_CHECKED);
    }
    if (mode_cb_gemma_27b) {
        if (g_gemma_install_opt_in) lv_obj_clear_state(mode_cb_gemma_27b, LV_STATE_DISABLED);
        else lv_obj_add_state(mode_cb_gemma_27b, LV_STATE_DISABLED);
        if (g_gemma_model_mask & 4) lv_obj_add_state(mode_cb_gemma_27b, LV_STATE_CHECKED);
        else lv_obj_clear_state(mode_cb_gemma_27b, LV_STATE_CHECKED);
    }
}

static void gemma_install_sw_cb(lv_event_t* e) {
    lv_obj_t* sw = lv_event_get_target_obj(e);
    if (!sw) return;
    g_gemma_install_opt_in = lv_obj_has_state(sw, LV_STATE_CHECKED);
    if (g_gemma_install_opt_in && g_gemma_model_mask == 0) {
        g_gemma_model_mask = gemma_recommended_mask();
    } else if (!g_gemma_install_opt_in) {
        g_gemma_model_mask = 0;
    }
    update_gemma_recommend_visuals();
    save_theme_config();
    LOG_I("GEMMA", "Local Gemma install opt-in=%d mask=%d", g_gemma_install_opt_in ? 1 : 0, g_gemma_model_mask);
}

static void gemma_model_checkbox_cb(lv_event_t* e) {
    lv_obj_t* cb = lv_event_get_target_obj(e);
    if (!cb) return;
    int bit = 0;
    if (cb == mode_cb_gemma_2b) bit = 1;
    else if (cb == mode_cb_gemma_9b) bit = 2;
    else if (cb == mode_cb_gemma_27b) bit = 4;
    if (bit == 0) return;
    if (lv_obj_has_state(cb, LV_STATE_CHECKED)) g_gemma_model_mask |= bit;
    else g_gemma_model_mask &= ~bit;
    if (g_gemma_install_opt_in && g_gemma_model_mask == 0) {
        /* Keep at least one model selected when installation is enabled. */
        g_gemma_model_mask = gemma_recommended_mask();
    }
    update_gemma_recommend_visuals();
    save_theme_config();
}

static void start_gemma_install_async(int model_mask) {
    if (g_gemma_install_started.exchange(true)) {
        ui_log("[Gemma] Installer already running, skip duplicate start");
        return;
    }
    app_reset_setup_cancel();
    g_wiz_gemma_progress_start_ms = 0;
    std::thread([model_mask]() {
        char out[1024] = {0};
        ui_log("[Gemma] Installer started. mask=%d", model_mask);
        bool ok = app_install_gemma_models(model_mask, out, sizeof(out));
        if (ok) {
            ui_log("[Gemma] %s", out);
        } else {
            ui_log("[Gemma] Install failed: %s", out[0] ? out : "unknown error");
            ui_alert_with_sound("Gemma Install Failed", out[0] ? out : "unknown error");
        }
        g_gemma_install_started.store(false);
    }).detach();
}

static void update_agent_trace_views() {
    if (mode_lbl_chat_next_step) lv_label_set_text_fmt(mode_lbl_chat_next_step, "Next Step: %s", g_ai_next_step);
    if (mode_ta_chat_script) lv_textarea_set_text(mode_ta_chat_script, g_ai_script_log);
    if (mode_lbl_work_next_step) lv_label_set_text_fmt(mode_lbl_work_next_step, "Next Step: %s", g_ai_next_step);
    if (mode_ta_work_script) lv_textarea_set_text(mode_ta_work_script, g_ai_script_log);
}

static void set_ai_next_step(const char* step) {
    snprintf(g_ai_next_step, sizeof(g_ai_next_step), "%s", (step && step[0]) ? step : "Idle");
    update_agent_trace_views();
}

static void append_ai_script_log(const char* line) {
    if (!line || !line[0]) return;
    SYSTEMTIME st;
    GetLocalTime(&st);
    char stamped[1536];
    snprintf(stamped, sizeof(stamped), "[%02d:%02d:%02d] %s", st.wHour, st.wMinute, st.wSecond, line);
    size_t llen = strlen(stamped);
    if (strlen(g_ai_script_log) + llen + 2 >= sizeof(g_ai_script_log)) {
        /* Drop oldest half to keep UI responsive and log bounded. */
        memmove(g_ai_script_log, g_ai_script_log + sizeof(g_ai_script_log) / 2, sizeof(g_ai_script_log) / 2);
        g_ai_script_log[sizeof(g_ai_script_log) / 2] = '\0';
    }
    strcat(g_ai_script_log, stamped);
    strcat(g_ai_script_log, "\n");
    update_agent_trace_views();
}

static void render_work_md_doc() {
    if (!mode_lbl_work_md_output) return;
    render_markdown_to_label(mode_lbl_work_md_output,
                             g_work_md_doc[0] ? g_work_md_doc : "# Work Output\n\n等待输入...",
                             CJK_FONT_CHAT);
}

static void work_emit_step_decision(const char* decision, const char* action) {
    char safe_action[96] = {0};
    int si = 0;
    for (const char* p = action ? action : ""; *p && si < (int)sizeof(safe_action) - 1; ++p) {
        if (*p == '"' || *p == '\\' || *p == '\n' || *p == '\r') continue;
        safe_action[si++] = *p;
    }
    char payload[320] = {0};
    snprintf(payload, sizeof(payload),
             "{\"type\":\"feedback\",\"answer\":\"step_%s:%s\"}",
             decision ? decision : "none",
             safe_action[0] ? safe_action : "unknown");
    if (g_streaming) {
        snprintf(g_pending_feedback_payload, sizeof(g_pending_feedback_payload), "%s", payload);
    } else {
        submit_prompt_to_chat(payload);
    }
}

struct StepBtnPayload {
    char log_line[96];
    char action[96];
    char detail[512];
    char decision[24];
};

static std::string json_unescape_min(const char* s) {
    std::string out;
    if (!s) return out;
    for (const char* p = s; *p; ++p) {
        if (*p == '\\' && *(p + 1)) {
            ++p;
            if (*p == 'n') out.push_back('\n');
            else if (*p == 'r') {}
            else if (*p == 't') out.push_back('\t');
            else out.push_back(*p);
        } else {
            out.push_back(*p);
        }
    }
    return out;
}

static std::string build_plan_preview_text(const char* raw) {
    if (!raw || !raw[0]) return "";
    const char* p = strstr(raw, "\"steps\"");
    if (!p) return "";
    std::vector<std::string> steps;
    const char* end = p + 1600;
    const char* cur = p;
    while (*cur && cur < end && (int)steps.size() < 6) {
        if (*cur == '"') {
            cur++;
            std::string token;
            while (*cur && *cur != '"' && token.size() < 120) {
                token.push_back(*cur++);
            }
            if (token.size() >= 4 &&
                token != "steps" &&
                token != "type" &&
                token != "plan" &&
                token != "risk" &&
                token != "est_time" &&
                token != "action" &&
                token != "title") {
                bool dup = false;
                for (const auto& s : steps) {
                    if (s == token) { dup = true; break; }
                }
                if (!dup) steps.push_back(token);
            }
        } else {
            cur++;
        }
    }
    if (steps.empty()) return "";
    std::string out = "Plan Preview:\n";
    for (size_t i = 0; i < steps.size(); ++i) {
        out += std::to_string((int)i + 1);
        out += ". ";
        out += steps[i];
        out += "\n";
    }
    return out;
}

static bool work_try_apply_write_from_detail(const char* detail, std::string* out_path) {
    if (!detail || !strstr(detail, "\"name\":\"write")) return false;
    char path[320] = {0};
    char content[4096] = {0};
    json_extract_string(detail, "path", path, sizeof(path));
    json_extract_string(detail, "content", content, sizeof(content));
    if (!path[0]) return false;
    std::string full = path;
    if (full.find(':') == std::string::npos && !(full.size() > 1 && full[0] == '\\' && full[1] == '\\')) {
        full = workspace_get_root() + "\\" + full;
    }
    if (!permissions().is_path_allowed(full.c_str(), true)) {
        ui_log("[Work] Step apply denied by permission boundary: %s", full.c_str());
        return false;
    }
    std::string bak = full + ".anyclaw.bak";
    try {
        if (std::filesystem::exists(full)) {
            std::filesystem::copy_file(full, bak, std::filesystem::copy_options::overwrite_existing);
        }
        std::ofstream f(full, std::ios::binary);
        if (!f.is_open()) return false;
        std::string decoded = json_unescape_min(content);
        f << decoded;
        f.close();
        g_step_write_backups.push_back({full, bak});
        if (out_path) *out_path = full;
        return true;
    } catch (...) {
        return false;
    }
}

static bool work_try_rollback_write_from_detail(const char* detail, std::string* out_path) {
    char path[320] = {0};
    json_extract_string(detail, "path", path, sizeof(path));
    if (!path[0]) return false;
    std::string full = path;
    if (full.find(':') == std::string::npos && !(full.size() > 1 && full[0] == '\\' && full[1] == '\\')) {
        full = workspace_get_root() + "\\" + full;
    }
    for (int i = (int)g_step_write_backups.size() - 1; i >= 0; --i) {
        if (g_step_write_backups[i].path == full && std::filesystem::exists(g_step_write_backups[i].bak_path)) {
            try {
                std::filesystem::copy_file(g_step_write_backups[i].bak_path, full, std::filesystem::copy_options::overwrite_existing);
                if (out_path) *out_path = full;
                return true;
            } catch (...) {
                return false;
            }
        }
    }
    return false;
}

static void step_btn_clicked_cb(lv_event_t* e) {
    StepBtnPayload* data = (StepBtnPayload*)lv_event_get_user_data(e);
    if (!data) return;
    if (data->log_line[0]) ui_log("%s", data->log_line);
    if (strcmp(data->decision, "accept") == 0) {
        std::string p;
        if (work_try_apply_write_from_detail(data->detail, &p)) {
            ui_log("[Work] Step local write applied: %s", p.c_str());
        }
    } else if (strcmp(data->decision, "rollback") == 0) {
        std::string p;
        if (work_try_rollback_write_from_detail(data->detail, &p)) {
            ui_log("[Work] Step rollback restored: %s", p.c_str());
        }
    } else if (strcmp(data->decision, "retry") == 0) {
        if (mode_ta_work_prompt && g_work_last_prompt[0]) {
            lv_textarea_set_text(mode_ta_work_prompt, g_work_last_prompt);
            work_send_cb(nullptr);
            ui_log("[Work] Step retry triggered for last work prompt");
        } else {
            ui_log("[Work] Step retry skipped: no last prompt");
        }
    }
    work_emit_step_decision(data->decision, data->action);
}

static void step_btn_delete_cb(lv_event_t* e) {
    StepBtnPayload* data = (StepBtnPayload*)lv_event_get_user_data(e);
    delete data;
}

static void update_chat_empty_state() {
    if (!mode_chat_empty_card || !chat_cont) return;
    uint32_t cnt = lv_obj_get_child_cnt(chat_cont);
    if (cnt == 0) lv_obj_clear_flag(mode_chat_empty_card, LV_OBJ_FLAG_HIDDEN);
    else lv_obj_add_flag(mode_chat_empty_card, LV_OBJ_FLAG_HIDDEN);
}

static void update_work_empty_state() {
    if (!mode_work_empty_card || !mode_work_step_stream) return;
    uint32_t cnt = lv_obj_get_child_cnt(mode_work_step_stream);
    if (cnt <= 1) lv_obj_clear_flag(mode_work_empty_card, LV_OBJ_FLAG_HIDDEN);
    else lv_obj_add_flag(mode_work_empty_card, LV_OBJ_FLAG_HIDDEN);
}

static void c2_refresh_dual_view_button() {
    if (!ctrl_btn_dual_view) return;
    lv_obj_t* lbl = lv_obj_get_child(ctrl_btn_dual_view, 0);
    if (!lbl) return;
    lv_label_set_text(lbl, g_c2_dual_result ? tr({"Dual: ON", "双结果: 开"}) : tr({"Dual: OFF", "双结果: 关"}));
}

static void c2_refresh_work_chat_toggle_button() {
    if (!mode_btn_work_chat_toggle) return;
    lv_obj_t* lbl = lv_obj_get_child(mode_btn_work_chat_toggle, 0);
    if (lbl) {
        if (g_c2_dual_result) lv_label_set_text(lbl, "=");
        else lv_label_set_text(lbl, g_work_chat_collapsed ? ">" : "<");
    }
    if (g_c2_dual_result) lv_obj_add_state(mode_btn_work_chat_toggle, LV_STATE_DISABLED);
    else lv_obj_clear_state(mode_btn_work_chat_toggle, LV_STATE_DISABLED);
}

static void c2_set_work_state_label(const I18n& i18n_text) {
    if (!mode_lbl_work_chat_state) return;
    lv_label_set_text(mode_lbl_work_chat_state, tr(i18n_text));
}

static void c2_refresh_work_chat_state_label() {
    if (!mode_lbl_work_chat_state) return;
    if (g_work_waiting_ai) {
        c2_set_work_state_label({"State: executing task", "状态：正在执行任务"});
    } else if (g_c2_dual_result) {
        c2_set_work_state_label({"State: dual-result pinned (single input)", "状态：双结果固定（单入口）"});
    } else {
        c2_set_work_state_label(g_work_chat_collapsed
            ? I18n{"State: collapsed", "状态：已折叠"}
            : I18n{"State: ready", "状态：就绪"});
    }
}

static void c2_apply_work_panel_policy() {
    if (!mode_work_chat_panel) return;
    if (g_c2_dual_result) {
        g_work_chat_collapsed = false;
        if (mode_ta_work_chat_feed) lv_obj_clear_flag(mode_ta_work_chat_feed, LV_OBJ_FLAG_HIDDEN);
    } else {
        if (g_work_chat_collapsed) {
            if (mode_ta_work_chat_feed) lv_obj_add_flag(mode_ta_work_chat_feed, LV_OBJ_FLAG_HIDDEN);
        } else {
            if (mode_ta_work_chat_feed) lv_obj_clear_flag(mode_ta_work_chat_feed, LV_OBJ_FLAG_HIDDEN);
        }
    }
    if (mode_ta_work_chat_input) lv_obj_add_flag(mode_ta_work_chat_input, LV_OBJ_FLAG_HIDDEN);
    if (mode_btn_work_chat_send) lv_obj_add_flag(mode_btn_work_chat_send, LV_OBJ_FLAG_HIDDEN);
    c2_refresh_work_chat_toggle_button();
    c2_refresh_work_chat_state_label();
}

static void c2_commit_work_panel_policy(bool persist_pref) {
    c2_apply_work_panel_policy();
    relayout_panels();
    if (persist_pref) save_theme_config();
}

static void c2_mirror_prompt_to_work(const char* text) {
    if (!text || !text[0]) return;
    if (mode_ta_work_prompt) lv_textarea_set_text(mode_ta_work_prompt, text);
    snprintf(g_work_last_prompt, sizeof(g_work_last_prompt), "%s", text);
    g_work_waiting_ai = true;
    work_append_md_block("Unified Input", text);
    work_add_step_card("统一入口任务", text, false, false);
    if (mode_ta_work_chat_feed) {
        lv_textarea_add_text(mode_ta_work_chat_feed, "\n[Unified] ");
        lv_textarea_add_text(mode_ta_work_chat_feed, text);
    }
    c2_refresh_work_chat_state_label();
}

static void c2_dual_view_toggle_cb(lv_event_t* e) {
    (void)e;
    if (!g_c2_dual_result) {
        /* Turning Dual ON: remember current collapse preference. */
        g_work_chat_collapsed_pref = g_work_chat_collapsed;
    }
    g_c2_dual_result = !g_c2_dual_result;
    if (!g_c2_dual_result) {
        /* Turning Dual OFF: restore user's previous preference. */
        g_work_chat_collapsed = g_work_chat_collapsed_pref;
    }
    c2_commit_work_panel_policy(true);
    c2_refresh_dual_view_button();
    ui_log("[C2] Dual result view: %s", g_c2_dual_result ? "on" : "off");
}

static void layout_chat_action_buttons(int content_w, int input_y, int input_h) {
    const int btn_size = SCALE(CHAT_ACTION_BTN_BASE);
    const int btn_margin = SCALE(CHAT_ACTION_BTN_MARGIN);
    const int btn_gap = SCALE(CHAT_ACTION_BTN_GAP);
    if (btn_send_widget) {
        lv_obj_set_pos(btn_send_widget, content_w - CHAT_GAP - btn_size - btn_margin, input_y + input_h - btn_size - btn_margin);
    }
    if (btn_upload_widget) {
        lv_obj_set_pos(btn_upload_widget, content_w - CHAT_GAP - btn_size - btn_margin - btn_size - btn_gap, input_y + input_h - btn_size - btn_margin);
    }
    if (btn_voice_widget) {
        lv_obj_set_pos(btn_voice_widget, content_w - CHAT_GAP - btn_size - btn_margin - (btn_size + btn_gap) * 2, input_y + input_h - btn_size - btn_margin);
    }
    if (btn_work_widget) {
        lv_obj_set_pos(btn_work_widget, content_w - CHAT_GAP - btn_size - btn_margin - (btn_size + btn_gap) * 3, input_y + input_h - btn_size - btn_margin);
    }
}

static void chat_empty_suggestion_cb(lv_event_t* e) {
    const char* prompt = (const char*)lv_event_get_user_data(e);
    if (!chat_input || !prompt || !prompt[0]) return;
    lv_textarea_set_text(chat_input, prompt);
    lv_textarea_set_cursor_pos(chat_input, LV_TEXTAREA_CURSOR_LAST);
    lv_obj_scroll_to_view(chat_input, LV_ANIM_OFF);
}

static void work_empty_example_cb(lv_event_t* e) {
    const char* sample = (const char*)lv_event_get_user_data(e);
    if (!mode_ta_work_prompt || !sample || !sample[0]) return;
    lv_textarea_set_text(mode_ta_work_prompt, sample);
    lv_textarea_set_cursor_pos(mode_ta_work_prompt, LV_TEXTAREA_CURSOR_LAST);
    lv_obj_scroll_to_view(mode_ta_work_prompt, LV_ANIM_OFF);
}

static bool work_find_outside_path_in_text(const char* detail, std::string& path_out) {
    path_out.clear();
    if (!detail || !detail[0]) return false;
    std::string s = detail;
    std::replace(s.begin(), s.end(), '\n', ' ');
    std::replace(s.begin(), s.end(), '\t', ' ');
    std::istringstream iss(s);
    std::string tok;
    while (iss >> tok) {
        while (!tok.empty() && (tok.front() == '"' || tok.front() == '\'' || tok.front() == '(' || tok.front() == '[' || tok.front() == '{')) {
            tok.erase(tok.begin());
        }
        while (!tok.empty() && (tok.back() == '"' || tok.back() == '\'' || tok.back() == ')' || tok.back() == ']' || tok.back() == '}' || tok.back() == ',' || tok.back() == ';')) {
            tok.pop_back();
        }
        if (tok.empty()) continue;
        bool looks_path = (tok.size() >= 3 && std::isalpha((unsigned char)tok[0]) && tok[1] == ':' && (tok[2] == '\\' || tok[2] == '/')) ||
                          (!tok.empty() && (tok[0] == '\\' || tok[0] == '/'));
        if (!looks_path) continue;
        if (!permissions().is_path_allowed(tok.c_str(), false)) {
            path_out = tok;
            return true;
        }
    }
    return false;
}

static void work_add_step_card(const char* action, const char* detail, bool done, bool write_op) {
    if (!mode_work_step_stream) return;
    const ThemeColors* c = g_colors ? g_colors : &THEME_DARK;
    const char* act = action ? action : "";
    const char* det = detail ? detail : "";
    bool exec_op = strstr(act, "exec") || strstr(act, "shell") || strstr(act, "command");
    bool read_op = strstr(act, "read") || strstr(act, "list") || strstr(act, "glob");
    bool failed = strstr(act, "fail") || strstr(act, "error") || strstr(act, "timeout") ||
                  strstr(act, "失败") || strstr(act, "错误") ||
                  strstr(det, "fail") || strstr(det, "error") || strstr(det, "timeout") ||
                  strstr(det, "失败") || strstr(det, "错误");
    lv_color_t card_border = c->border;
    lv_color_t card_bg = c->surface;
    const char* kind = "general";
    if (write_op) { card_border = c->warning; card_bg = c->surface; kind = "write"; }
    else if (exec_op) { card_border = c->info; card_bg = c->surface; kind = "exec"; }
    else if (read_op) { card_border = c->success; card_bg = c->surface; kind = "read"; }
    lv_obj_t* card = lv_obj_create(mode_work_step_stream);
    lv_obj_set_width(card, LV_PCT(100));
    lv_obj_set_height(card, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_color(card, card_bg, 0);
    lv_obj_set_style_border_color(card, card_border, 0);
    lv_obj_set_style_border_width(card, 1, 0);
    lv_obj_set_style_radius(card, SCALE(g_colors->radius_lg), 0);
    lv_obj_set_style_pad_all(card, 12, 0);
    lv_obj_set_style_pad_gap(card, 8, 0);
    lv_obj_set_flex_flow(card, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(card, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* title = lv_label_create(card);
    const char* status_icon = failed ? "✗" : (done ? "✓" : "●");
    lv_color_t status_color = failed ? c->btn_close : (done ? c->accent : c->btn_add);
    lv_label_set_text_fmt(title, "%s  %s", status_icon, action ? action : "Step");
    lv_obj_set_style_text_color(title, status_color, 0);
    lv_obj_set_style_text_font(title, CJK_FONT_SMALL, 0);

    lv_obj_t* kind_lbl = lv_label_create(card);
    lv_label_set_text_fmt(kind_lbl, "kind: %s", kind);
    lv_obj_set_style_text_color(kind_lbl, c->text, 0);
    lv_obj_set_style_text_font(kind_lbl, CJK_FONT_SMALL, 0);

    lv_obj_t* info = lv_label_create(card);
    lv_label_set_text(info, detail && detail[0] ? detail : "");
    std::string out_path;
    bool outside_path = work_find_outside_path_in_text(detail, out_path);
    lv_obj_set_style_text_color(info, outside_path ? c->btn_close : c->text_dim, 0);
    lv_obj_set_style_text_font(info, CJK_FONT_SMALL, 0);
    lv_label_set_long_mode(info, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(info, LV_PCT(100));
    if (outside_path) {
        lv_obj_t* warn = lv_label_create(card);
        lv_label_set_text_fmt(warn, "⚠ Path outside workspace: %s", out_path.c_str());
        lv_obj_set_style_text_color(warn, c->btn_close, 0);
        lv_obj_set_style_text_font(warn, CJK_FONT_SMALL, 0);
        lv_label_set_long_mode(warn, LV_LABEL_LONG_WRAP);
        lv_obj_set_width(warn, LV_PCT(100));
    }

    if (write_op) {
        lv_obj_t* row = lv_obj_create(card);
        lv_obj_set_size(row, LV_PCT(100), LV_SIZE_CONTENT);
        lv_obj_set_style_bg_opa(row, LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_width(row, 0, 0);
        lv_obj_set_style_pad_all(row, 0, 0);
        lv_obj_set_style_pad_gap(row, 6, 0);
        lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
        lv_obj_set_flex_align(row, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);

        auto bind_btn = [&](const char* txt, AwBtnStyle bs, const char* log_line, const char* decision) {
            lv_obj_t* btn = aw_btn_create(row, txt, bs, SCALE(90), SCALE(30));
            StepBtnPayload* payload = new StepBtnPayload{};
            snprintf(payload->log_line, sizeof(payload->log_line), "%s", log_line ? log_line : "");
            snprintf(payload->action, sizeof(payload->action), "%s", action ? action : "step");
            snprintf(payload->detail, sizeof(payload->detail), "%s", detail ? detail : "");
            snprintf(payload->decision, sizeof(payload->decision), "%s", decision ? decision : "none");
            lv_obj_add_event_cb(btn, step_btn_clicked_cb, LV_EVENT_CLICKED, payload);
            lv_obj_add_event_cb(btn, step_btn_delete_cb, LV_EVENT_DELETE, payload);
        };
        bind_btn("接受", BTN_PRIMARY, "[Work] Step accepted", "accept");
        bind_btn("拒绝", BTN_DANGER, "[Work] Step rejected", "reject");
        bind_btn("回退", BTN_SECONDARY, "[Work] Step rollback requested", "rollback");
    } else if (exec_op && failed) {
        lv_obj_t* row = lv_obj_create(card);
        lv_obj_set_size(row, LV_PCT(100), LV_SIZE_CONTENT);
        lv_obj_set_style_bg_opa(row, LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_width(row, 0, 0);
        lv_obj_set_style_pad_all(row, 0, 0);
        lv_obj_set_style_pad_gap(row, 6, 0);
        lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
        lv_obj_set_flex_align(row, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_t* btn = aw_btn_create(row, "重试", BTN_SECONDARY, SCALE(90), SCALE(30));
        StepBtnPayload* payload = new StepBtnPayload{};
        snprintf(payload->log_line, sizeof(payload->log_line), "%s", "[Work] Step retry requested");
        snprintf(payload->action, sizeof(payload->action), "%s", action ? action : "step");
        snprintf(payload->detail, sizeof(payload->detail), "%s", detail ? detail : "");
        snprintf(payload->decision, sizeof(payload->decision), "%s", "retry");
        lv_obj_add_event_cb(btn, step_btn_clicked_cb, LV_EVENT_CLICKED, payload);
        lv_obj_add_event_cb(btn, step_btn_delete_cb, LV_EVENT_DELETE, payload);
    }
    lv_obj_scroll_to_y(mode_work_step_stream, lv_obj_get_scroll_bottom(mode_work_step_stream), LV_ANIM_ON);
    update_work_empty_state();
}

static void work_append_md_block(const char* title, const char* text) {
    if (!title || !text || !text[0]) return;
    const ThemeColors* c = g_colors ? g_colors : &THEME_DARK;
    WorkRenderType rt = output_renderer_pick(title, text);
    bool is_plan = strstr(text, "\"type\":\"plan\"") || strstr(text, "\"steps\"") || strstr(text, "approve_plan");
    std::string render_text = text;
    std::string plan_preview;
    if (is_plan) {
        plan_preview = build_plan_preview_text(text);
        if (!plan_preview.empty()) {
            render_text = std::string("## Plan Summary\n\n```text\n") + plan_preview + "```\n\n## Raw Plan\n\n" + text;
        }
    }
    output_renderer_append_markdown(g_work_md_doc, sizeof(g_work_md_doc), title, render_text.c_str());
    if (mode_lbl_work_output_title) {
        const char* pfx = "";
        if (is_plan) pfx = "[Plan] ";
        else if (rt == WorkRenderType::Diff) pfx = "[Diff] ";
        else if (rt == WorkRenderType::Table) pfx = "[Table] ";
        else if (rt == WorkRenderType::Web) pfx = "[Web] ";
        else if (rt == WorkRenderType::File) pfx = "[File] ";
        else if (rt == WorkRenderType::Terminal) pfx = "[Terminal] ";
        lv_label_set_text_fmt(mode_lbl_work_output_title, "Output: %s%s", pfx, title);
    }
    if (mode_lbl_work_renderer) {
        lv_label_set_text_fmt(mode_lbl_work_renderer, "Renderer: %s", is_plan ? "plan" : output_renderer_name(rt));
        lv_color_t rc = c->accent_secondary;
        if (is_plan) rc = c->warning;
        else if (rt == WorkRenderType::Diff) rc = c->info;
        else if (rt == WorkRenderType::Table) rc = c->success;
        else if (rt == WorkRenderType::Web) rc = c->info;
        else if (rt == WorkRenderType::File) rc = c->warning;
        else if (rt == WorkRenderType::Terminal) rc = c->text_dim;
        lv_obj_set_style_text_color(mode_lbl_work_renderer, rc, 0);
    }
    const bool write_op = strstr(title, "write") || strstr(title, "Write") || strstr(text, "write_file");
    work_add_step_card(is_plan ? "计划输出" : title, plan_preview.empty() ? text : plan_preview.c_str(), true, write_op);
    render_work_md_doc();
}

static void update_work_mode_hint() {
    if (!mode_lbl_work_hint) return;
    const char* control = (g_control_mode == CONTROL_AI) ? "AI controls AnyClaw" : "User controls AnyClaw";
    const char* llm = "Gateway mode (OpenClaw, Direct API paused)";
    const char* ai_mode = (g_ai_interaction_mode == AIMODE_PLAN) ? "Plan" :
                          (g_ai_interaction_mode == AIMODE_ASK) ? "Ask" : "Autonomous";
    lv_label_set_text_fmt(mode_lbl_work_hint, "Control: %s\nLLM Access: %s\nAI Interaction: %s\nRemote Guard: %s\nProactive: startup=%s idle=%s\nLocal Gemma4: %s\nUser: %s (%s)\nAI: %s (%s)",
                          control, llm,
                          ai_mode,
                          g_remote_guard_armed ? "Armed" : "Disarmed",
                          g_proactive_startup_enabled ? "on" : "off",
                          g_proactive_idle_enabled ? "on" : "off",
                          g_gemma_install_opt_in ? "Enabled" : "Disabled",
                          profile_user_name(), g_profile_user_role[0] ? g_profile_user_role : "Owner",
                          profile_ai_name(), g_profile_ai_role[0] ? g_profile_ai_role : "Assistant");
}

static void update_chat_status_label(const char* text, bool busy) {
    if (!mode_lbl_chat_status) return;
    lv_label_set_text(mode_lbl_chat_status, text ? text : "");
    const ThemeColors* c = g_colors ? g_colors : &THEME_DARK;
    lv_obj_set_style_text_color(mode_lbl_chat_status, busy ? c->accent : c->text_dim, 0);
    /* Keep Work state label owned by C2 panel policy to avoid Chat status overriding it. */
    c2_refresh_work_chat_state_label();
}

static void update_work_advanced_visibility() {
    auto apply = [](lv_obj_t* obj, bool show) {
        if (!obj) return;
        if (show) lv_obj_clear_flag(obj, LV_OBJ_FLAG_HIDDEN);
        else lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
    };
    apply(mode_sec_lan, g_work_show_advanced);
    apply(mode_sec_ftp, g_work_show_advanced);
    apply(mode_sec_kb, g_work_show_advanced);
}

static void sync_ai_mode_dropdowns() {
    if (mode_dd_ai_mode) lv_dropdown_set_selected(mode_dd_ai_mode, (uint16_t)g_ai_interaction_mode);
    if (mode_dd_chat_ai_mode) lv_dropdown_set_selected(mode_dd_chat_ai_mode, (uint16_t)g_ai_interaction_mode);
}

static const char* ai_mode_system_prompt() {
    switch (g_ai_interaction_mode) {
        case AIMODE_ASK:
            return "You are in ASK mode. Continue autonomously for normal tasks, but for decision points or risky actions, emit compact JSON blocks like {\"type\":\"ask_feedback\",\"reason\":\"...\",\"suggestion\":\"...\",\"options\":[\"...\",\"...\",\"...\"]}.";
        case AIMODE_PLAN:
            return "You are in PLAN mode. First output a compact JSON plan block {\"type\":\"plan\",\"steps\":[...],\"risk\":\"...\",\"est_time\":\"...\"} before execution details.";
        default:
            return "You are in autonomous mode. Execute tasks directly and keep responses concise.";
    }
}

static void run_proactive_startup_once() {
    static bool done = false;
    if (done || !g_proactive_startup_enabled) return;
    done = true;
    ui_log("[Proactive] Startup check executed");
    append_ai_script_log("[proactive] startup check complete");
}

static DWORD get_idle_ms_win32() {
    LASTINPUTINFO lii;
    lii.cbSize = sizeof(lii);
    if (!GetLastInputInfo(&lii)) return 0;
    DWORD now = GetTickCount();
    return now - lii.dwTime;
}

static void proactive_tick() {
    DWORD now = GetTickCount();
    if (g_proactive_idle_enabled) {
        DWORD idle_ms = get_idle_ms_win32();
        DWORD threshold = (DWORD)(g_proactive_idle_threshold_minutes * 60 * 1000);
        if (idle_ms >= threshold && (now - g_proactive_last_idle_action_tick) > threshold) {
            g_proactive_last_idle_action_tick = now;
            if (g_proactive_memory_enabled) {
                workspace_sync_managed_sections();
                ui_log("[Proactive] Idle maintenance: synced managed docs");
                append_ai_script_log("[proactive] idle maintenance finished");
            } else {
                ui_log("[Proactive] Idle detected, memory maintenance disabled");
            }
        }
    }
    if (g_proactive_summary_enabled) {
        time_t tt = time(NULL);
        struct tm tmv;
        localtime_s(&tmv, &tt);
        if (g_proactive_memory_enabled && tmv.tm_hour == 18 && g_proactive_last_memory_yday != tmv.tm_yday) {
            g_proactive_last_memory_yday = tmv.tm_yday;
            workspace_sync_managed_sections();
            ui_log("[Proactive] 18:00 memory maintenance executed");
        }
        if (tmv.tm_hour == 23 && g_proactive_last_summary_yday != tmv.tm_yday) {
            g_proactive_last_summary_yday = tmv.tm_yday;
            ui_log("[Proactive] Daily summary: health monitor active, workspace synced");
        }
    }
}

static void work_control_mode_cb(lv_event_t* e) {
    lv_obj_t* dd = lv_event_get_target_obj(e);
    if (!dd) return;
    g_control_mode = (lv_dropdown_get_selected(dd) == 1) ? CONTROL_AI : CONTROL_USER;
    update_work_mode_hint();
    save_theme_config();
    LOG_I("MODE", "Control mode switched to %s", g_control_mode == CONTROL_AI ? "AI" : "User");
}

static void ai_emit_ui_action_feedback(const char* action, const char* target, bool ok, const char* reason) {
    char safe_action[96] = {0};
    char safe_target[96] = {0};
    char safe_reason[160] = {0};
    auto sanitize = [](const char* in, char* out, size_t out_sz) {
        if (!out || out_sz == 0) return;
        out[0] = '\0';
        if (!in) return;
        size_t oi = 0;
        for (size_t i = 0; in[i] && oi + 1 < out_sz; i++) {
            char c = in[i];
            if (c == '"' || c == '\\' || c == '\n' || c == '\r') continue;
            out[oi++] = c;
        }
        out[oi] = '\0';
    };
    sanitize(action, safe_action, sizeof(safe_action));
    sanitize(target, safe_target, sizeof(safe_target));
    sanitize(reason, safe_reason, sizeof(safe_reason));
    char payload[460] = {0};
    snprintf(payload, sizeof(payload),
             "{\"type\":\"feedback\",\"answer\":\"ui_action:%s:%s:%s:%s\"}",
             safe_action[0] ? safe_action : "unknown",
             safe_target[0] ? safe_target : "-",
             ok ? "ok" : "fail",
             safe_reason[0] ? safe_reason : (ok ? "done" : "error"));
    if (g_streaming) snprintf(g_pending_feedback_payload, sizeof(g_pending_feedback_payload), "%s", payload);
    else submit_prompt_to_chat(payload);
}

static bool ai_simulate_text_input(const char* text) {
    if (!text || !text[0]) return false;
    for (const unsigned char* p = (const unsigned char*)text; *p; ++p) {
        INPUT in[2] = {};
        in[0].type = INPUT_KEYBOARD;
        in[0].ki.wVk = 0;
        in[0].ki.wScan = *p;
        in[0].ki.dwFlags = KEYEVENTF_UNICODE;
        in[1] = in[0];
        in[1].ki.dwFlags = KEYEVENTF_UNICODE | KEYEVENTF_KEYUP;
        if (SendInput(2, in, sizeof(INPUT)) != 2) return false;
    }
    return true;
}

static bool ai_simulate_enter_key() {
    INPUT in[2] = {};
    in[0].type = INPUT_KEYBOARD;
    in[0].ki.wVk = VK_RETURN;
    in[1] = in[0];
    in[1].ki.dwFlags = KEYEVENTF_KEYUP;
    return SendInput(2, in, sizeof(INPUT)) == 2;
}

static bool ai_simulate_mouse_left_click_at(int x, int y, bool move_first) {
    if (move_first) SetCursorPos(x, y);
    INPUT in[2] = {};
    in[0].type = INPUT_MOUSE;
    in[0].mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
    in[1] = in[0];
    in[1].mi.dwFlags = MOUSEEVENTF_LEFTUP;
    return SendInput(2, in, sizeof(INPUT)) == 2;
}

static bool ui_action_is_high_risk(const char* action, const char* target) {
    if (!action || !action[0]) return false;
    if (strcmp(action, "run_work") == 0) return true;
    if (strcmp(action, "send_chat_message") == 0) return true;
    if (strcmp(action, "send_work_message") == 0) return true;
    if (strcmp(action, "simulate_input") == 0) return true;
    if (strcmp(action, "click_button") == 0 && target) {
        return strcmp(target, "remote_request") == 0 ||
               strcmp(target, "remote_accept") == 0 ||
               strcmp(target, "remote_reject") == 0 ||
               strcmp(target, "remote_disconnect") == 0 ||
               strcmp(target, "work_remote_disconnect") == 0 ||
               strcmp(target, "ftp_upload") == 0 ||
               strcmp(target, "ftp_download") == 0 ||
               strcmp(target, "cron_delete") == 0;
    }
    return false;
}

static bool ask_mode_confirm_ui_action(const char* action, const char* target, const char* value) {
    if (g_ai_interaction_mode != AIMODE_ASK) return true;
    if (!ui_action_is_high_risk(action, target)) return true;
    char reason[320] = {0};
    snprintf(reason, sizeof(reason), "AI control requests ui_action: %s target=%s value=%s",
             action ? action : "", target ? target : "", (value && value[0]) ? value : "-");
    bool ok = ask_mode_confirm_action(reason, "Allow this UI action to continue?", "allow_once,deny");
    if (!ok) {
        snprintf(g_pending_feedback_payload, sizeof(g_pending_feedback_payload),
                 "{\"type\":\"feedback\",\"answer\":\"deny_ui_action:%s\"}",
                 action && action[0] ? action : "unknown");
    }
    return ok;
}

static void execute_ai_ui_action_if_any(const char* text) {
    if (g_control_mode != CONTROL_AI || !text || !text[0]) return;
    const char* tag = strstr(text, "\"type\":\"ui_action\"");
    if (!tag) return;
    char action[64] = {0};
    char target[64] = {0};
    char value[512] = {0};
    json_extract_string(tag, "action", action, sizeof(action));
    json_extract_string(tag, "target", target, sizeof(target));
    json_extract_string(tag, "value", value, sizeof(value));
    if (!ask_mode_confirm_ui_action(action, target, value)) {
        ui_log("[CTRL] AI ui_action blocked by Ask mode");
        ai_emit_ui_action_feedback(action, target, false, "blocked_by_ask");
        return;
    }

    bool handled = false;
    bool success = true;
    const char* fail_reason = "";
    struct FieldMap { const char* key; lv_obj_t** ta; };
    static FieldMap k_field_map[] = {
        {"lan_host", &mode_ta_lan_host}, {"lan_port", &mode_ta_lan_port}, {"lan_nick", &mode_ta_lan_nick},
        {"lan_msg", &mode_ta_lan_msg}, {"ftp_host", &mode_ta_ftp_host}, {"ftp_port", &mode_ta_ftp_port},
        {"ftp_user", &mode_ta_ftp_user}, {"ftp_pass", &mode_ta_ftp_pass}, {"ftp_remote", &mode_ta_ftp_remote},
        {"kb_query", &mode_ta_kb_keyword}, {"cron_id", &mode_ta_cron_id}, {nullptr, nullptr}
    };
    struct ClickMap { const char* key; void (*cb)(lv_event_t*); };
    static ClickMap k_click_map[] = {
        {"work_send", work_send_cb}, {"chat_send", chat_send_cb}, {"work_chat_send", work_chat_send_cb},
        {"work_chat_toggle", work_chat_toggle_cb}, {"remote_request", remote_request_cb},
        {"remote_accept", remote_accept_cb}, {"remote_reject", remote_reject_cb},
        {"remote_disconnect", remote_disconnect_cb}, {"work_remote_disconnect", work_remote_disconnect_cb},
        {"trace_refresh", work_trace_refresh_cb}, {"trace_sort", work_trace_toggle_sort_cb},
        {"trace_fail_filter", work_trace_toggle_fail_filter_cb}, {"boot_open_report", work_boot_open_report_cb},
        {"retry_attachments", work_retry_failed_attachments_cb}, {"lan_start_host", work_lan_start_host_cb},
        {"lan_connect", work_lan_connect_cb}, {"lan_discover", work_lan_discover_cb},
        {"lan_send_global", work_lan_send_global_cb}, {"lan_send_private", work_lan_send_private_cb},
        {"lan_create_group", work_lan_create_group_cb}, {"lan_join_group", work_lan_join_group_cb},
        {"lan_send_group", work_lan_send_group_cb}, {"ftp_upload", work_ftp_upload_cb},
        {"ftp_download", work_ftp_download_cb}, {"ftp_cancel", work_ftp_cancel_cb},
        {"ftp_retry", work_ftp_retry_cb}, {"kb_add_source", work_kb_add_source_cb},
        {"kb_import_dir", work_kb_import_dir_cb}, {"kb_export_manifest", work_kb_export_manifest_cb},
        {"kb_search", work_kb_search_cb}, {"cron_refresh", work_cron_refresh_cb},
        {"cron_run", work_cron_run_cb}, {"cron_enable", work_cron_enable_cb},
        {"cron_disable", work_cron_disable_cb}, {"cron_delete", work_cron_delete_cb},
        {nullptr, nullptr}
    };

    if (strcmp(action, "switch_mode") == 0) {
        if (strcmp(target, "work") == 0) g_ui_mode = UI_MODE_WORK;
        else g_ui_mode = UI_MODE_CHAT;
        apply_mode_switch_visuals();
        relayout_panels();
        ui_log("[CTRL] AI ui_action switch_mode -> %s", target[0] ? target : "chat");
        handled = true;
    } else if (strcmp(action, "set_work_prompt") == 0) {
        if (mode_ta_work_prompt && value[0]) {
            lv_textarea_set_text(mode_ta_work_prompt, value);
            ui_log("[CTRL] AI ui_action set_work_prompt");
            handled = true;
        } else { success = false; fail_reason = "work_prompt_unavailable"; }
    } else if (strcmp(action, "set_field") == 0) {
        if (value[0]) {
            bool matched = false;
            for (int i = 0; k_field_map[i].key; i++) {
                if (strcmp(target, k_field_map[i].key) == 0) {
                    matched = true;
                    if (k_field_map[i].ta && *k_field_map[i].ta) {
                        lv_textarea_set_text(*k_field_map[i].ta, value);
                        handled = true;
                        ui_log("[CTRL] AI ui_action set_field(%s)", target);
                    } else {
                        success = false;
                        fail_reason = "field_widget_unavailable";
                    }
                    break;
                }
            }
            if (!matched) { success = false; fail_reason = "unknown_field_target"; }
        } else { success = false; fail_reason = "empty_field_value"; }
    } else if (strcmp(action, "set_chat_prompt") == 0) {
        if (chat_input && value[0]) {
            lv_textarea_set_text(chat_input, value);
            ui_log("[CTRL] AI ui_action set_chat_prompt");
            handled = true;
        } else { success = false; fail_reason = "chat_input_unavailable"; }
    } else if (strcmp(action, "send_chat_message") == 0) {
        if (chat_input && value[0]) {
            lv_textarea_set_text(chat_input, value);
            chat_send_cb(nullptr);
            ui_log("[CTRL] AI ui_action send_chat_message");
            handled = true;
        } else { success = false; fail_reason = "chat_send_input_empty"; }
    } else if (strcmp(action, "send_work_message") == 0) {
        if (mode_ta_work_prompt && value[0]) {
            lv_textarea_set_text(mode_ta_work_prompt, value);
            work_send_cb(nullptr);
            ui_log("[CTRL] AI ui_action send_work_message");
            handled = true;
        } else { success = false; fail_reason = "work_prompt_unavailable"; }
    } else if (strcmp(action, "toggle_work_chat") == 0) {
        work_chat_toggle_cb(nullptr);
        ui_log("[CTRL] AI ui_action toggle_work_chat");
        handled = true;
    } else if (strcmp(action, "simulate_input") == 0) {
        if (strcmp(target, "keyboard_text") == 0) {
            if (ai_simulate_text_input(value)) {
                ui_log("[CTRL] AI ui_action simulate_input(keyboard_text)");
                handled = true;
            } else {
                success = false;
                fail_reason = "sendinput_text_failed";
            }
        } else if (strcmp(target, "key_enter") == 0) {
            if (ai_simulate_enter_key()) {
                ui_log("[CTRL] AI ui_action simulate_input(key_enter)");
                handled = true;
            } else {
                success = false;
                fail_reason = "sendinput_enter_failed";
            }
        } else if (strcmp(target, "mouse_left_click") == 0) {
            POINT p{};
            GetCursorPos(&p);
            if (ai_simulate_mouse_left_click_at((int)p.x, (int)p.y, false)) {
                ui_log("[CTRL] AI ui_action simulate_input(mouse_left_click)");
                handled = true;
            } else {
                success = false;
                fail_reason = "sendinput_mouse_click_failed";
            }
        } else if (strcmp(target, "mouse_move_click") == 0) {
            int x = 0, y = 0;
            if (sscanf(value, "%d,%d", &x, &y) == 2) {
                if (ai_simulate_mouse_left_click_at(x, y, true)) {
                    ui_log("[CTRL] AI ui_action simulate_input(mouse_move_click:%d,%d)", x, y);
                    handled = true;
                } else {
                    success = false;
                    fail_reason = "sendinput_mouse_move_click_failed";
                }
            } else {
                success = false;
                fail_reason = "invalid_mouse_coordinates";
            }
        } else {
            success = false;
            fail_reason = "unknown_simulate_target";
        }
    } else if (strcmp(action, "click_button") == 0) {
        bool matched = false;
        for (int i = 0; k_click_map[i].key; i++) {
            if (strcmp(target, k_click_map[i].key) == 0) {
                matched = true;
                if (k_click_map[i].cb) {
                    k_click_map[i].cb(nullptr);
                    ui_log("[CTRL] AI ui_action click_button(%s)", target);
                    handled = true;
                } else {
                    success = false;
                    fail_reason = "button_callback_missing";
                }
                break;
            }
        }
        if (!matched) { success = false; fail_reason = "unknown_button_target"; }
    } else if (strcmp(action, "run_work") == 0) {
        work_send_cb(nullptr);
        ui_log("[CTRL] AI ui_action run_work executed");
        handled = true;
    } else {
        success = false;
        fail_reason = "unknown_action";
    }
    if (!handled && success) {
        success = false;
        fail_reason = "not_executed";
    }
    ai_emit_ui_action_feedback(action, target, success, fail_reason);
}

static void work_llm_mode_cb(lv_event_t* e) {
    lv_obj_t* dd = lv_event_get_target_obj(e);
    if (!dd) return;
    uint16_t selected = lv_dropdown_get_selected(dd);
    if (selected == 1) {
        /* Direct API is temporarily paused by product decision. */
        g_llm_access_mode = LLM_GATEWAY;
        lv_dropdown_set_selected(dd, 0);
        ui_log("[Mode] Direct API paused, fallback to Gateway");
    } else {
        g_llm_access_mode = LLM_GATEWAY;
    }
    update_work_mode_hint();
    save_theme_config();
    LOG_I("MODE", "LLM access switched to Gateway (Direct API paused)");
}

static void ai_mode_dropdown_cb(lv_event_t* e) {
    lv_obj_t* dd = lv_event_get_target_obj(e);
    if (!dd) return;
    uint16_t sel = lv_dropdown_get_selected(dd);
    if (sel > 2) sel = 0;
    g_ai_interaction_mode = (AiInteractionMode)sel;
    sync_ai_mode_dropdowns();
    update_work_mode_hint();
    save_theme_config();
    ui_log("[Mode] AI interaction mode: %s", sel == 0 ? "Autonomous" : (sel == 1 ? "Ask" : "Plan"));
}

static void work_advanced_sw_cb(lv_event_t* e) {
    lv_obj_t* sw = lv_event_get_target_obj(e);
    if (!sw) return;
    g_work_show_advanced = lv_obj_has_state(sw, LV_STATE_CHECKED);
    update_work_advanced_visibility();
    save_theme_config();
    ui_log("[Work] Advanced modules %s", g_work_show_advanced ? "shown" : "hidden");
}

struct AskFeedbackDialogCtx {
    volatile int* decision; /* 0=pending 1=reject 2=allow */
    char* answer;
    size_t answer_cap;
    lv_obj_t* input_ta;
};

static void ask_feedback_choice_cb(lv_event_t* e) {
    AskFeedbackDialogCtx* ctx = (AskFeedbackDialogCtx*)lv_event_get_user_data(e);
    if (!ctx || !ctx->decision || !ctx->answer || ctx->answer_cap == 0) return;
    const char* choice = (const char*)lv_event_get_user_data(e);
    snprintf(ctx->answer, ctx->answer_cap, "%s", choice ? choice : "allow");
    *ctx->decision = 2;
}

static void ask_feedback_reject_cb(lv_event_t* e) {
    AskFeedbackDialogCtx* ctx = (AskFeedbackDialogCtx*)lv_event_get_user_data(e);
    if (!ctx || !ctx->decision || !ctx->answer || ctx->answer_cap == 0) return;
    snprintf(ctx->answer, ctx->answer_cap, "%s", "reject");
    *ctx->decision = 1;
}

static void ask_feedback_submit_cb(lv_event_t* e) {
    AskFeedbackDialogCtx* ctx = (AskFeedbackDialogCtx*)lv_event_get_user_data(e);
    if (!ctx || !ctx->decision || !ctx->answer || ctx->answer_cap == 0) return;
    const char* text = (ctx->input_ta) ? lv_textarea_get_text(ctx->input_ta) : "";
    snprintf(ctx->answer, ctx->answer_cap, "%s", (text && text[0]) ? text : "allow");
    *ctx->decision = 2;
}

static bool ask_mode_confirm_action(const char* reason, const char* suggestion, const char* options_csv, char* out_answer, size_t out_answer_cap) {
    if (g_ai_interaction_mode != AIMODE_ASK) return true;
    if (g_ui_thread_id == 0 || GetCurrentThreadId() != g_ui_thread_id) {
        char msg[768];
        snprintf(msg, sizeof(msg),
                 "AI Ask Mode decision required.\n\nReason: %s\nSuggestion: %s\n\nYes = Continue\nNo = Reject",
                 reason ? reason : "(none)",
                 suggestion ? suggestion : "(none)");
        bool ok = MessageBoxA(NULL, msg, "AnyClaw Ask Feedback", MB_ICONQUESTION | MB_YESNO | MB_TOPMOST) == IDYES;
        if (out_answer && out_answer_cap > 0) snprintf(out_answer, out_answer_cap, "%s", ok ? "allow" : "reject");
        return ok;
    }

    {
        char default_choice[64] = "allow";
        if (options_csv && options_csv[0]) {
            char opts[256];
            snprintf(opts, sizeof(opts), "%s", options_csv);
            for (char* p = opts; *p; ++p) {
                if (*p == ',') *p = '|';
            }
            char* ctxp = nullptr;
            char* tok = strtok_s(opts, "|", &ctxp);
            if (tok && tok[0]) {
                while (*tok == ' ' || *tok == '\t') tok++;
                snprintf(default_choice, sizeof(default_choice), "%s", tok);
            }
        }

        char msg[896];
        snprintf(msg, sizeof(msg),
                 "AI Ask Mode decision required.\n\nReason: %s\nSuggestion: %s\n\nYes = continue\nNo = reject",
                 reason ? reason : "(none)",
                 suggestion ? suggestion : "(none)");
        bool ok = MessageBoxA(nullptr, msg, "AnyClaw Ask Feedback", MB_ICONQUESTION | MB_YESNO | MB_TOPMOST | MB_SETFOREGROUND) == IDYES;
        if (out_answer && out_answer_cap > 0) {
            snprintf(out_answer, out_answer_cap, "%s", ok ? default_choice : "reject");
        }
        return ok;
    }

    lv_obj_t* overlay = nullptr;
    lv_obj_t* box = create_dialog(lv_screen_active(), "AnyClaw Ask Feedback", 0, 0, &overlay);
    if (!box || !overlay) return false;
    const ThemeColors* c = g_colors ? g_colors : &THEME_DARK;

    lv_obj_t* lbl_suggestion = lv_label_create(box);
    lv_label_set_text_fmt(lbl_suggestion, "建议：%s", suggestion ? suggestion : "(none)");
    lv_obj_set_style_text_color(lbl_suggestion, c->accent, 0);
    lv_obj_set_style_text_font(lbl_suggestion, CJK_FONT, 0);
    lv_label_set_long_mode(lbl_suggestion, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(lbl_suggestion, LV_PCT(100));

    lv_obj_t* info_box = lv_obj_create(box);
    lv_obj_set_size(info_box, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_style_bg_color(info_box, c->surface, 0);
    lv_obj_set_style_border_color(info_box, c->border, 0);
    lv_obj_set_style_border_width(info_box, 1, 0);
    lv_obj_set_style_radius(info_box, SCALE(g_colors->radius_lg), 0);
    lv_obj_set_style_pad_all(info_box, SCALE(10), 0);
    lv_obj_set_style_pad_gap(info_box, SCALE(6), 0);
    lv_obj_set_flex_flow(info_box, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(info_box, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_clear_flag(info_box, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* lbl_reason = lv_label_create(info_box);
    lv_label_set_text_fmt(lbl_reason, "原因：%s", reason ? reason : "(none)");
    lv_obj_set_style_text_color(lbl_reason, c->text, 0);
    lv_obj_set_style_text_font(lbl_reason, CJK_FONT_SMALL, 0);
    lv_label_set_long_mode(lbl_reason, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(lbl_reason, LV_PCT(100));

    lv_obj_t* row_opt = lv_obj_create(box);
    lv_obj_set_size(row_opt, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(row_opt, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(row_opt, 0, 0);
    lv_obj_set_style_pad_all(row_opt, 0, 0);
    lv_obj_set_style_pad_gap(row_opt, 8, 0);
    lv_obj_set_flex_flow(row_opt, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(row_opt, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_clear_flag(row_opt, LV_OBJ_FLAG_SCROLLABLE);

    volatile int decision = 0;
    static thread_local char answer[256];
    answer[0] = '\0';
    AskFeedbackDialogCtx ctx = {&decision, answer, sizeof(answer), nullptr};
    auto add_opt = [&](const char* txt) {
        lv_obj_t* b = aw_btn_create(row_opt, txt, BTN_SECONDARY, LV_PCT(100), SCALE(34));
        lv_obj_add_event_cb(b, [](lv_event_t* ev) {
            AskFeedbackDialogCtx* cctx = (AskFeedbackDialogCtx*)lv_event_get_user_data(ev);
            if (!cctx || !cctx->decision || !cctx->answer) return;
            lv_obj_t* btn = lv_event_get_target_obj(ev);
            lv_obj_t* lbl = lv_obj_get_child(btn, 0);
            const char* t = lbl ? lv_label_get_text(lbl) : "allow";
            snprintf(cctx->answer, cctx->answer_cap, "%s", t ? t : "allow");
            *cctx->decision = 2;
        }, LV_EVENT_CLICKED, &ctx);
    };
    int added = 0;
    char default_choice[64] = "allow";
    if (options_csv && options_csv[0]) {
        char opts[256];
        snprintf(opts, sizeof(opts), "%s", options_csv);
        for (char* p = opts; *p; ++p) {
            if (*p == ',') *p = '|';
        }
        char* ctxp = nullptr;
        char* tok = strtok_s(opts, "|", &ctxp);
        while (tok && added < 4) {
            while (*tok == ' ' || *tok == '\t') tok++;
            char* end = tok + strlen(tok);
            while (end > tok && (end[-1] == ' ' || end[-1] == '\t')) {
                end[-1] = '\0';
                end--;
            }
            if (tok[0]) {
                add_opt(tok);
                if (added == 0) snprintf(default_choice, sizeof(default_choice), "%s", tok);
                added++;
            }
            tok = strtok_s(nullptr, "|", &ctxp);
        }
    }
    if (added == 0) {
        add_opt("继续执行");
        add_opt("先给计划");
        add_opt("改后执行");
        snprintf(default_choice, sizeof(default_choice), "%s", "继续执行");
    }

    lv_obj_t* lbl_custom = lv_label_create(box);
    lv_label_set_text(lbl_custom, "自定义回复：");
    lv_obj_set_style_text_color(lbl_custom, c->text_dim, 0);
    lv_obj_set_style_text_font(lbl_custom, CJK_FONT_SMALL, 0);
    lv_obj_set_width(lbl_custom, LV_PCT(100));

    lv_obj_t* ta = lv_textarea_create(box);
    lv_textarea_set_placeholder_text(ta, "自定义反馈（可选）");
    lv_textarea_set_one_line(ta, false);
    lv_obj_set_size(ta, LV_PCT(100), SCALE(84));
    lv_obj_set_style_bg_color(ta, c->surface, 0);
    lv_obj_set_style_text_color(ta, c->text, 0);
    lv_obj_set_style_border_color(ta, c->border, 0);
    lv_obj_set_style_border_width(ta, 1, 0);
    lv_obj_set_style_border_color(ta, c->accent, LV_STATE_FOCUSED);
    lv_obj_set_style_border_width(ta, 2, LV_STATE_FOCUSED);
    lv_obj_set_style_radius(ta, SCALE(g_colors->radius_lg), 0);
    lv_obj_set_style_pad_all(ta, SCALE(10), 0);
    ctx.input_ta = ta;

    lv_obj_t* row_btn = lv_obj_create(box);
    lv_obj_set_size(row_btn, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(row_btn, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(row_btn, 0, 0);
    lv_obj_set_style_pad_all(row_btn, 0, 0);
    lv_obj_set_style_pad_gap(row_btn, 8, 0);
    lv_obj_set_flex_flow(row_btn, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(row_btn, LV_FLEX_ALIGN_END, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag(row_btn, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* btn_reject = aw_btn_create(row_btn, "拒绝", BTN_DANGER, SCALE(110), SCALE(34));
    lv_obj_add_event_cb(btn_reject, ask_feedback_reject_cb, LV_EVENT_CLICKED, &ctx);
    lv_obj_t* btn_submit = aw_btn_create(row_btn, "发送", BTN_PRIMARY, SCALE(120), SCALE(34));
    lv_obj_add_event_cb(btn_submit, ask_feedback_submit_cb, LV_EVENT_CLICKED, &ctx);
    lv_obj_add_state(btn_submit, LV_STATE_FOCUSED);

    lv_obj_t* lbl_timeout = lv_label_create(box);
    lv_label_set_text(lbl_timeout, "⏱ 60秒未选择将自动拒绝");
    lv_obj_set_style_text_color(lbl_timeout, c->text_dim, 0);
    lv_obj_set_style_text_font(lbl_timeout, CJK_FONT_SMALL, 0);
    lv_obj_set_width(lbl_timeout, LV_PCT(100));

    lv_obj_move_foreground(overlay);
    uint32_t start = GetTickCount();
    while (decision == 0) {
        lv_timer_handler();
        Sleep(16);
        if (GetTickCount() - start > 60000) {
            decision = 1;
            snprintf(answer, sizeof(answer), "%s", "timeout_reject");
        }
    }
    if (overlay) lv_obj_del(overlay);

    if (out_answer && out_answer_cap > 0) {
        snprintf(out_answer, out_answer_cap, "%s", answer[0] ? answer : (decision == 2 ? default_choice : "reject"));
    }
    if (decision == 2) {
        ui_log("[AIMODE] ask_feedback approved: %s | answer=%s", reason ? reason : "(none)", answer);
        append_ai_script_log("[ask] approved");
        return true;
    }
    ui_log("[AIMODE] ask_feedback rejected: %s | answer=%s", reason ? reason : "(none)", answer);
    append_ai_script_log("[ask] rejected");
    return false;
}

static void work_profile_save_cb(lv_event_t* e) {
    (void)e;
    auto cp = [](lv_obj_t* ta, char* dst, size_t cap) {
        if (!ta || !dst || cap == 0) return;
        const char* t = lv_textarea_get_text(ta);
        if (!t) t = "";
        snprintf(dst, cap, "%s", t);
    };
    cp(mode_ta_user_name, g_profile_user_name, sizeof(g_profile_user_name));
    cp(mode_ta_user_role, g_profile_user_role, sizeof(g_profile_user_role));
    cp(mode_ta_ai_name, g_profile_ai_name, sizeof(g_profile_ai_name));
    cp(mode_ta_ai_role, g_profile_ai_role, sizeof(g_profile_ai_role));
    cp(mode_ta_ai_persona, g_profile_ai_persona, sizeof(g_profile_ai_persona));
    cp(mode_ta_ai_skills, g_profile_ai_skills, sizeof(g_profile_ai_skills));
    if (mode_dd_user_avatar) {
        uint16_t s = lv_dropdown_get_selected(mode_dd_user_avatar);
        snprintf(g_profile_user_avatar, sizeof(g_profile_user_avatar), "%s", (s == 1) ? "lobster" : "garlic");
    }
    if (mode_dd_ai_avatar) {
        uint16_t s = lv_dropdown_get_selected(mode_dd_ai_avatar);
        snprintf(g_profile_ai_avatar, sizeof(g_profile_ai_avatar), "%s", (s == 1) ? "garlic" : "lobster");
    }
    update_profile_preview_cards();
    update_work_mode_hint();
    save_theme_config();
    LOG_I("PROFILE", "Profile saved user=%s ai=%s", profile_user_name(), profile_ai_name());
}

static void work_remote_guard_sw_cb(lv_event_t* e) {
    lv_obj_t* sw = lv_event_get_target_obj(e);
    if (!sw) return;
    bool want_arm = lv_obj_has_state(sw, LV_STATE_CHECKED);
    if (want_arm && !g_remote_guard_armed) {
        /* Enforce double-confirm and countdown before entering armed state. */
        lv_obj_clear_state(sw, LV_STATE_CHECKED);
        open_remote_arm_modal();
        return;
    }
    g_remote_guard_armed = want_arm;
    update_remote_guard_visuals();
    update_work_mode_hint();
    save_theme_config();
    LOG_I("REMOTE", "Remote safety guard %s", g_remote_guard_armed ? "armed" : "disarmed");
}

static void work_remote_disconnect_cb(lv_event_t* e) {
    (void)e;
    close_remote_arm_modal();
    stop_remote_session_timer();
    RemoteTcpChannel::instance().stop();
    g_remote_state = REMOTE_IDLE;
    g_remote_guard_armed = false;
    if (mode_sw_remote_guard) lv_obj_clear_state(mode_sw_remote_guard, LV_STATE_CHECKED);
    update_remote_guard_visuals();
    update_remote_session_visuals();
    update_work_mode_hint();
    save_theme_config();
    ui_log("[Remote] Emergency disconnect triggered by user");
    LOG_W("REMOTE", "Emergency disconnect");
}

static void close_voice_input_dialog() {
    if (g_voice_input_overlay) {
        lv_obj_del(g_voice_input_overlay);
        g_voice_input_overlay = nullptr;
    }
    g_voice_input_ta = nullptr;
}

static void voice_input_cancel_cb(lv_event_t* e) {
    (void)e;
    close_voice_input_dialog();
}

static void voice_input_apply_cb(lv_event_t* e) {
    (void)e;
    if (!g_voice_input_ta || !chat_input) {
        close_voice_input_dialog();
        return;
    }
    const char* text = lv_textarea_get_text(g_voice_input_ta);
    if (text && text[0]) {
        const char* current = lv_textarea_get_text(chat_input);
        if (current && current[0]) lv_textarea_add_text(chat_input, "\n");
        lv_textarea_add_text(chat_input, text);
        lv_obj_send_event(chat_input, LV_EVENT_VALUE_CHANGED, nullptr);
        ui_log("[Voice] Transcript inserted into chat input");
    }
    close_voice_input_dialog();
}

static bool voice_try_system_stt(std::string& transcript, std::string& err) {
    transcript.clear();
    err.clear();
    const char* cmd =
        "powershell -NoProfile -ExecutionPolicy Bypass -Command "
        "\"[Console]::OutputEncoding=[System.Text.UTF8Encoding]::UTF8; "
        "Add-Type -AssemblyName System.Speech; "
        "$r=New-Object System.Speech.Recognition.SpeechRecognitionEngine; "
        "$r.SetInputToDefaultAudioDevice(); "
        "$r.LoadGrammar((New-Object System.Speech.Recognition.DictationGrammar)); "
        "$res=$r.Recognize([TimeSpan]::FromSeconds(8)); "
        "if($res -and $res.Text){ Write-Output $res.Text } else { Write-Output '__EMPTY__' }\"";
    FILE* pipe = _popen(cmd, "r");
    if (!pipe) {
        err = "cannot_launch_stt_process";
        return false;
    }
    char buf[1024] = {0};
    while (fgets(buf, sizeof(buf), pipe)) transcript += buf;
    int rc = _pclose(pipe);
    while (!transcript.empty() && (transcript.back() == '\r' || transcript.back() == '\n' || transcript.back() == ' ' || transcript.back() == '\t')) {
        transcript.pop_back();
    }
    if (rc != 0) {
        err = "stt_command_failed";
        return false;
    }
    if (transcript.empty() || transcript == "__EMPTY__") {
        err = "no_speech_result";
        return false;
    }
    return true;
}

static void open_voice_manual_dialog() {
    if (g_voice_input_overlay) return;
    lv_obj_t* box = create_dialog(lv_screen_active(),
                                  g_lang == Lang::CN ? "语音输入（转文字）" : "Voice Input (to text)",
                                  0, 0, &g_voice_input_overlay);
    if (!box) return;
    lv_obj_t* hint = lv_label_create(box);
    lv_label_set_text(hint, g_lang == Lang::CN
        ? "将语音识别结果粘贴到这里，点击“插入输入框”后会写入 Chat 输入区。"
        : "Paste your speech-to-text result here, then insert it into chat input.");
    lv_obj_set_style_text_color(hint, g_colors->text_dim, 0);
    lv_obj_set_style_text_font(hint, CJK_FONT_SMALL, 0);
    lv_label_set_long_mode(hint, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(hint, LV_PCT(100));

    g_voice_input_ta = lv_textarea_create(box);
    lv_textarea_set_one_line(g_voice_input_ta, false);
    lv_textarea_set_placeholder_text(g_voice_input_ta, g_lang == Lang::CN ? "语音转文字结果..." : "Voice transcript...");
    lv_obj_set_size(g_voice_input_ta, LV_PCT(100), SCALE(120));
    lv_obj_set_style_bg_color(g_voice_input_ta, g_colors->surface, 0);
    lv_obj_set_style_text_color(g_voice_input_ta, g_colors->text, 0);

    lv_obj_t* row = lv_obj_create(box);
    lv_obj_set_size(row, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(row, 0, 0);
    lv_obj_set_style_pad_all(row, 0, 0);
    lv_obj_set_style_pad_gap(row, 8, 0);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(row, LV_FLEX_ALIGN_END, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* btn_cancel = aw_btn_create(row, g_lang == Lang::CN ? "取消" : "Cancel", BTN_SECONDARY, SCALE(120), SCALE(34));
    lv_obj_add_event_cb(btn_cancel, voice_input_cancel_cb, LV_EVENT_CLICKED, nullptr);
    lv_obj_t* btn_apply = aw_btn_create(row, g_lang == Lang::CN ? "插入输入框" : "Insert", BTN_PRIMARY, SCALE(170), SCALE(34));
    lv_obj_add_event_cb(btn_apply, voice_input_apply_cb, LV_EVENT_CLICKED, nullptr);
}

static DWORD WINAPI voice_stt_thread_proc(LPVOID arg) {
    int token = (int)(INT_PTR)arg;
    std::string transcript;
    std::string err;
    bool ok = voice_try_system_stt(transcript, err);
    LONG active = InterlockedCompareExchange(&g_voice_stt_active_token, 0, 0);
    if (token != (int)active) return 0;
    g_voice_stt_ok = ok;
    snprintf(g_voice_stt_text, sizeof(g_voice_stt_text), "%s", ok ? transcript.c_str() : "");
    snprintf(g_voice_stt_err, sizeof(g_voice_stt_err), "%s", ok ? "" : err.c_str());
    InterlockedExchange(&g_voice_stt_done, 1);
    InterlockedExchange(&g_voice_stt_running, 0);
    return 0;
}

static void voice_stt_poll_cb(lv_timer_t* t) {
    (void)t;
    if (InterlockedCompareExchange(&g_voice_stt_running, 0, 0) &&
        GetTickCount() - g_voice_stt_start_tick > 12000) {
        InterlockedExchange(&g_voice_stt_running, 0);
        InterlockedIncrement(&g_voice_stt_active_token); /* invalidate stale thread result */
        if (g_voice_stt_thread) {
            CloseHandle(g_voice_stt_thread);
            g_voice_stt_thread = nullptr;
        }
        if (g_voice_stt_poll_timer) {
            lv_timer_del(g_voice_stt_poll_timer);
            g_voice_stt_poll_timer = nullptr;
        }
        ui_log("[Voice] System STT timeout, fallback to manual dialog");
        open_voice_manual_dialog();
        return;
    }
    if (!InterlockedCompareExchange(&g_voice_stt_done, 0, 1)) return;
    if (g_voice_stt_poll_timer) {
        lv_timer_del(g_voice_stt_poll_timer);
        g_voice_stt_poll_timer = nullptr;
    }
    if (g_voice_stt_thread) {
        CloseHandle(g_voice_stt_thread);
        g_voice_stt_thread = nullptr;
    }
    if (g_voice_stt_ok && g_voice_stt_text[0]) {
        if (chat_input) {
            const char* current = lv_textarea_get_text(chat_input);
            if (current && current[0]) lv_textarea_add_text(chat_input, "\n");
            lv_textarea_add_text(chat_input, g_voice_stt_text);
            lv_obj_send_event(chat_input, LV_EVENT_VALUE_CHANGED, nullptr);
        }
        ui_log("[Voice] System STT success, transcript inserted");
    } else {
        ui_log("[Voice] System STT unavailable (%s), fallback to manual transcript dialog", g_voice_stt_err[0] ? g_voice_stt_err : "unknown");
        open_voice_manual_dialog();
    }
}

static void apply_mode_switch_visuals() {
    if (!mode_panel_chat || !mode_panel_work) return;

    if (g_ui_nav_module != UI_NAV_BOT) {
        lv_obj_add_flag(mode_panel_chat, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(mode_panel_work, LV_OBJ_FLAG_HIDDEN);
        if (ctrl_bar) lv_obj_add_flag(ctrl_bar, LV_OBJ_FLAG_HIDDEN);
        update_main_title();
        return;
    }

    if (g_ui_mode == UI_MODE_CHAT) {
        lv_obj_clear_flag(mode_panel_chat, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(mode_panel_work, LV_OBJ_FLAG_HIDDEN);
        if (ctrl_bar) lv_obj_clear_flag(ctrl_bar, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_add_flag(mode_panel_chat, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(mode_panel_work, LV_OBJ_FLAG_HIDDEN);
        if (ctrl_bar) lv_obj_add_flag(ctrl_bar, LV_OBJ_FLAG_HIDDEN);
    }

    auto paint_btn = [](lv_obj_t* btn, bool selected) {
        if (!btn) return;
        const ThemeColors* c = g_colors ? g_colors : &THEME_DARK;
        lv_obj_set_style_bg_color(btn, selected ? c->btn_action : c->btn_secondary, 0);
        if (selected) apply_btn_gradient(btn);
        lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, 0);
    };
    paint_btn(mode_btn_chat, g_ui_mode == UI_MODE_CHAT);
    paint_btn(mode_btn_work, g_ui_mode == UI_MODE_WORK);
    paint_btn(top_mode_chat, g_ui_mode == UI_MODE_CHAT);
    paint_btn(top_mode_work, g_ui_mode == UI_MODE_WORK);

    /* Update nav_session toggle icon */
    if (nav_session) {
        const ThemeColors* nc = g_colors ? g_colors : &THEME_DARK;
        lv_obj_t* lbl = lv_obj_get_child(nav_session, 0);
        if (lbl && lv_obj_check_type(lbl, &lv_label_class)) {
            lv_label_set_text(lbl, (g_ui_mode == UI_MODE_CHAT) ? "\xE2\x9E\xA1" : "\xE2\x96\xB6");
            lv_obj_set_style_text_color(lbl, (g_ui_mode == UI_MODE_WORK) ? nc->accent : nc->text_dim, 0);
        }
    }

    update_main_title();
}

static void update_main_title() {
    if (!title_label) return;
    const I18n* s = &STR_TITLE;
    if (g_ui_nav_module == UI_NAV_BOT) {
        s = (g_ui_mode == UI_MODE_CHAT) ? &STR_TITLE_BOT_CHAT : &STR_TITLE_BOT_WORK;
    } else if (g_ui_nav_module == UI_NAV_TASKS) {
        s = &STR_TITLE_TASKS;
    } else if (g_ui_nav_module == UI_NAV_FILES) {
        s = &STR_TITLE_FILES;
    }
    lv_label_set_text(title_label, tr(*s));
}

static void refresh_tasks_module_data(bool run_pending) {
    if (!module_tasks_state) return;
    SessionManager& sm = session_mgr();
    bool refreshed = sm.refresh();
    auto sessions = sm.active_sessions();
    int visible_tasks = g_task_count;
    int active_sessions = (int)sessions.size();
    int cron_sessions = 0;
    for (const auto& s : sessions) if (s.isCron) cron_sessions++;

    if (run_pending) {
        lv_label_set_text_fmt(module_tasks_state,
            "%s", tr(I18n{"State: session cache synced", "状态：会话缓存已同步"}));
        app_refresh_status();
        ui_log("[Tasks] Session sync requested (ok=%d, visible=%d, sessions=%d)", refreshed ? 1 : 0, visible_tasks, active_sessions);
    } else {
        lv_label_set_text_fmt(module_tasks_state,
            "%s", tr(I18n{"State: queue refreshed", "状态：队列已刷新"}));
        ui_log("[Tasks] Queue refreshed (ok=%d, visible=%d, sessions=%d)", refreshed ? 1 : 0, visible_tasks, active_sessions);
    }

    if (!module_tasks_view) return;
    char buf[2048] = {0};
    snprintf(buf, sizeof(buf),
             "Visible task widgets: %d\n"
             "Active sessions: %d\n"
             "Cron sessions: %d\n\n",
             visible_tasks, active_sessions, cron_sessions);
    for (int i = 0; i < g_task_count && i < 8; i++) {
        const char* name = g_tasks[i].name[0] ? g_tasks[i].name : "(unnamed)";
        const char* status = g_tasks[i].status[0] ? g_tasks[i].status : "unknown";
        char line[220] = {0};
        snprintf(line, sizeof(line), "%d. %s [%s]\n", i + 1, name, status);
        strncat_s(buf, sizeof(buf), line, _TRUNCATE);
    }
    if (g_task_count == 0) {
        strncat_s(buf, sizeof(buf), tr(I18n{"No task widgets yet.", "当前无任务条目。"}), _TRUNCATE);
    }
    lv_textarea_set_text(module_tasks_view, buf);
}

static void refresh_files_module_data(bool scan_workspace) {
    if (!module_files_state || !module_files_view) return;
    namespace fs = std::filesystem;
    std::string ws_root = workspace_get_root();
    WorkspaceHealth wh = workspace_check_health();
    int ws_files = 0;
    int ws_dirs = 0;
    int asset_files = 0;
    int asset_fonts = 0;
    int asset_icons = 0;
    int asset_sounds = 0;
    int matched_total = 0;
    std::vector<std::string> filtered_entries;
    filtered_entries.reserve(32);

    std::error_code ec;
    if (scan_workspace && !ws_root.empty() && fs::exists(ws_root, ec)) {
        for (fs::recursive_directory_iterator it(ws_root, fs::directory_options::skip_permission_denied, ec), end;
             it != end && ws_files < 2000; it.increment(ec)) {
            if (ec) break;
            if (it->is_regular_file(ec)) ws_files++;
            else if (it->is_directory(ec)) ws_dirs++;
        }
    }

    const std::string assets_dir = "assets";
    std::string search = g_module_files_search;
    std::transform(search.begin(), search.end(), search.begin(), ::tolower);

    auto has_suffix = [](const std::string& text, const char* suffix) {
        size_t n = strlen(suffix);
        if (text.size() < n) return false;
        return text.compare(text.size() - n, n, suffix) == 0;
    };

    auto pass_ext_filter = [&](const std::string& lower) {
        switch (g_module_file_ext_filter) {
            case 1: return has_suffix(lower, ".png");
            case 2: return has_suffix(lower, ".svg");
            case 3: return has_suffix(lower, ".ttf") || has_suffix(lower, ".otf");
            case 4: return has_suffix(lower, ".wav") || has_suffix(lower, ".mp3") || has_suffix(lower, ".ogg");
            case 5: return has_suffix(lower, ".json");
            case 6: return has_suffix(lower, ".md");
            default: return true;
        }
    };

    if (fs::exists(assets_dir, ec)) {
        for (fs::recursive_directory_iterator it(assets_dir, fs::directory_options::skip_permission_denied, ec), end;
             it != end && asset_files < 1000; it.increment(ec)) {
            if (ec) break;
            if (it->is_regular_file(ec)) {
                asset_files++;
                std::string p = it->path().generic_string();
                std::string lower = p;
                std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
                bool is_font = lower.find("/fonts/") != std::string::npos;
                bool is_icon = lower.find("/icons/") != std::string::npos;
                bool is_sound = lower.find("/sounds/") != std::string::npos;
                if (is_font) asset_fonts++;
                if (is_icon) asset_icons++;
                if (is_sound) asset_sounds++;

                bool pass = (g_module_file_filter == MODULE_FILE_ALL)
                    || (g_module_file_filter == MODULE_FILE_FONTS && is_font)
                    || (g_module_file_filter == MODULE_FILE_ICONS && is_icon)
                    || (g_module_file_filter == MODULE_FILE_SOUNDS && is_sound);
                if (pass && !search.empty() && lower.find(search) == std::string::npos) pass = false;
                if (pass && !pass_ext_filter(lower)) pass = false;
                if (pass) {
                    matched_total++;
                    if (filtered_entries.size() < 20) filtered_entries.push_back(p);
                }
            }
        }
    }

    auto paint_filter_btn = [](lv_obj_t* btn, bool selected) {
        if (!btn) return;
        const ThemeColors* c = g_colors ? g_colors : &THEME_DARK;
        lv_obj_set_style_bg_color(btn, selected ? c->btn_action : c->btn_secondary, 0);
        lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, 0);
    };
    paint_filter_btn(module_files_filter_all, g_module_file_filter == MODULE_FILE_ALL);
    paint_filter_btn(module_files_filter_fonts, g_module_file_filter == MODULE_FILE_FONTS);
    paint_filter_btn(module_files_filter_icons, g_module_file_filter == MODULE_FILE_ICONS);
    paint_filter_btn(module_files_filter_sounds, g_module_file_filter == MODULE_FILE_SOUNDS);

    lv_label_set_text(module_files_state,
        scan_workspace
            ? tr(I18n{"State: workspace scanned", "状态：工作区已扫描"})
            : tr(I18n{"State: quick asset summary", "状态：资源快速摘要"}));

    char buf[2048] = {0};
    snprintf(buf, sizeof(buf),
             "Workspace root:\n%s\n\n"
             "Exists: %s\n"
             "Writable: %s\n"
             "Workspace files: %d\n"
             "Workspace dirs: %d\n"
             "Assets files: %d\n"
             "  - fonts: %d\n"
             "  - icons: %d\n"
             "  - sounds: %d\n"
             "Matched: %d\n\n",
             ws_root.empty() ? "(empty)" : ws_root.c_str(),
             wh.exists ? "yes" : "no",
             wh.writable ? "yes" : "no",
             ws_files, ws_dirs, asset_files, asset_fonts, asset_icons, asset_sounds, matched_total);

    const char* filter_name = "all";
    if (g_module_file_filter == MODULE_FILE_FONTS) filter_name = "fonts";
    else if (g_module_file_filter == MODULE_FILE_ICONS) filter_name = "icons";
    else if (g_module_file_filter == MODULE_FILE_SOUNDS) filter_name = "sounds";
    const char* ext_name = "any";
    if (g_module_file_ext_filter == 1) ext_name = ".png";
    else if (g_module_file_ext_filter == 2) ext_name = ".svg";
    else if (g_module_file_ext_filter == 3) ext_name = "font";
    else if (g_module_file_ext_filter == 4) ext_name = "audio";
    else if (g_module_file_ext_filter == 5) ext_name = ".json";
    else if (g_module_file_ext_filter == 6) ext_name = ".md";
    char head[160] = {0};
    snprintf(head, sizeof(head), "Filter: %s | Ext: %s | Search: %s\n",
             filter_name, ext_name,
             search.empty() ? "(none)" : search.c_str());
    strncat_s(buf, sizeof(buf), head, _TRUNCATE);
    if (filtered_entries.empty()) {
        strncat_s(buf, sizeof(buf), tr(I18n{"(no entries)", "（无匹配条目）"}), _TRUNCATE);
    } else {
        for (size_t i = 0; i < filtered_entries.size(); ++i) {
            char line[220] = {0};
            snprintf(line, sizeof(line), "%zu. %s\n", i + 1, filtered_entries[i].c_str());
            strncat_s(buf, sizeof(buf), line, _TRUNCATE);
        }
    }
    lv_textarea_set_text(module_files_view, buf);
    ui_log("[Resources] Summary refreshed (scan=%d, files=%d, dirs=%d, assets=%d)",
           scan_workspace ? 1 : 0, ws_files, ws_dirs, asset_files);
}

static void run_tasks_template(const char* template_name, const char* prompt) {
    if (!mode_ta_work_prompt || !prompt || !prompt[0]) {
        if (module_tasks_state) lv_label_set_text(module_tasks_state, tr(I18n{"State: template unavailable", "状态：模板不可用"}));
        return;
    }
    if (is_streaming_now()) {
        if (module_tasks_state) lv_label_set_text(module_tasks_state, tr(I18n{"State: wait current response", "状态：请等待当前响应完成"}));
        ui_toast_warn(g_lang == Lang::CN ? "请等待当前响应完成" : "Wait for current response");
        return;
    }

    lv_textarea_set_text(mode_ta_work_prompt, prompt);
    snprintf(g_work_last_prompt, sizeof(g_work_last_prompt), "%s", prompt);
    work_send_cb(nullptr);

    if (module_tasks_state) {
        char state[256] = {0};
        snprintf(state, sizeof(state), "%s: %s",
                 tr(I18n{"State: template executed", "状态：模板已执行"}),
                 template_name ? template_name : "task");
        lv_label_set_text(module_tasks_state, state);
    }
    ui_log("[Tasks] Template executed: %s", template_name ? template_name : "task");

    g_ui_nav_module = UI_NAV_BOT;
    g_ui_mode = UI_MODE_WORK;
    apply_nav_module_visuals();
}

static void apply_nav_module_visuals() {
    auto paint_nav_btn = [](lv_obj_t* btn, bool selected) {
        if (!btn) return;
        const ThemeColors* c = g_colors ? g_colors : &THEME_DARK;
        lv_obj_set_style_bg_opa(btn, selected ? LV_OPA_20 : LV_OPA_TRANSP, 0);
        lv_obj_set_style_bg_color(btn, selected ? c->accent : c->text_hint, 0);
        lv_obj_t* lbl = lv_obj_get_child(btn, 0);
        if (lbl && lv_obj_check_type(lbl, &lv_label_class)) {
            lv_obj_set_style_text_color(lbl, selected ? c->accent : c->text_dim, 0);
        }
    };

    paint_nav_btn(nav_btn_bot, g_ui_nav_module == UI_NAV_BOT);
    paint_nav_btn(nav_btn_tasks, g_ui_nav_module == UI_NAV_TASKS);
    paint_nav_btn(nav_btn_files, g_ui_nav_module == UI_NAV_FILES);

    if (g_ui_nav_module == UI_NAV_BOT) {
        if (left_panel) lv_obj_clear_flag(left_panel, LV_OBJ_FLAG_HIDDEN);
        if (right_panel) lv_obj_clear_flag(right_panel, LV_OBJ_FLAG_HIDDEN);
        if (module_placeholder) lv_obj_add_flag(module_placeholder, LV_OBJ_FLAG_HIDDEN);
    } else {
        if (left_panel) lv_obj_add_flag(left_panel, LV_OBJ_FLAG_HIDDEN);
        if (right_panel) lv_obj_clear_flag(right_panel, LV_OBJ_FLAG_HIDDEN);
        if (module_placeholder) {
            const char* title = (g_ui_nav_module == UI_NAV_TASKS)
                ? tr(I18n{"Tasks Center", "任务中心"})
                : tr(I18n{"Resources Center", "资源中心"});
            const char* desc = (g_ui_nav_module == UI_NAV_TASKS)
                ? tr(I18n{"Task scheduling, queue and execution shortcuts.", "任务调度、队列与执行快捷入口。"})
                : tr(I18n{"Workspace, files and skill assets shortcuts.", "工作区、文件与技能资源快捷入口。"});
            if (module_placeholder_title) lv_label_set_text(module_placeholder_title, title);
            if (module_placeholder_desc) lv_label_set_text(module_placeholder_desc, desc);
            if (module_tasks_panel) {
                if (g_ui_nav_module == UI_NAV_TASKS) lv_obj_clear_flag(module_tasks_panel, LV_OBJ_FLAG_HIDDEN);
                else lv_obj_add_flag(module_tasks_panel, LV_OBJ_FLAG_HIDDEN);
            }
            if (module_files_panel) {
                if (g_ui_nav_module == UI_NAV_FILES) lv_obj_clear_flag(module_files_panel, LV_OBJ_FLAG_HIDDEN);
                else lv_obj_add_flag(module_files_panel, LV_OBJ_FLAG_HIDDEN);
            }
            lv_obj_clear_flag(module_placeholder, LV_OBJ_FLAG_HIDDEN);
        }
    }

    apply_mode_switch_visuals();
    relayout_panels();
}

static void mode_chat_cb(lv_event_t* e) {
    (void)e;
    if (g_ui_nav_module != UI_NAV_BOT) return;
    g_ui_mode = UI_MODE_CHAT;
    apply_mode_switch_visuals();
    relayout_panels();
}
static void mode_voice_cb(lv_event_t* e) {
    (void)e;
    if (InterlockedCompareExchange(&g_voice_stt_running, 0, 0)) {
        ui_log("[Voice] STT capture is still running, please wait");
        return;
    }
    ui_log("[Voice] Starting system STT capture asynchronously...");
    g_voice_stt_done = 0;
    g_voice_stt_ok = false;
    g_voice_stt_text[0] = '\0';
    g_voice_stt_err[0] = '\0';
    InterlockedExchange(&g_voice_stt_running, 1);
    LONG token = InterlockedIncrement(&g_voice_stt_active_token);
    g_voice_stt_start_tick = GetTickCount();
    g_voice_stt_thread = CreateThread(nullptr, 0, voice_stt_thread_proc, (LPVOID)(INT_PTR)token, 0, nullptr);
    if (!g_voice_stt_thread) {
        InterlockedExchange(&g_voice_stt_running, 0);
        ui_log("[Voice] Failed to start STT thread, fallback to manual dialog");
        open_voice_manual_dialog();
        return;
    }
    if (g_voice_stt_poll_timer) lv_timer_del(g_voice_stt_poll_timer);
    g_voice_stt_poll_timer = lv_timer_create(voice_stt_poll_cb, 160, nullptr);
}
static void mode_work_cb(lv_event_t* e) {
    (void)e;
    if (g_ui_nav_module != UI_NAV_BOT) return;
    g_ui_mode = UI_MODE_WORK;
    apply_mode_switch_visuals();
    relayout_panels();
}
static void mode_chat_work_toggle_cb(lv_event_t* e) {
    (void)e;
    g_ui_mode = (g_ui_mode == UI_MODE_WORK) ? UI_MODE_CHAT : UI_MODE_WORK;
    apply_mode_switch_visuals();
    relayout_panels();
}

static lv_obj_t* g_disclaimer_modal = nullptr;
static lv_obj_t* g_exit_dialog_modal = nullptr;
static lv_obj_t* g_about_dialog = nullptr;
static bool g_ui_ready = false;

/* Called from main.cpp when OS triggers maximize (double-click title bar, Win+Up, etc.) */
void ui_on_window_maximized() {
    g_maximized = true;
    if (lbl_maximize) lv_label_set_text(lbl_maximize, "\xE2\xA7\xA9");  /* ⧉ */
    /* Recalculate layout for new window size */
    ui_relayout_all();
}

/* Called from main.cpp when OS triggers restore (double-click title bar, Win+Down, etc.) */
void ui_on_window_restored() {
    g_maximized = false;
    if (lbl_maximize) lv_label_set_text(lbl_maximize, "\xE2\x96\xA1");  /* □ */
    /* Recalculate layout for new window size */
    ui_relayout_all();
}

/* ── Window drag ── */
static void title_drag_cb(lv_event_t* e) {
    lv_event_code_t code = lv_event_get_code(e);
    /* Ignore bubbled events from window control buttons. */
    if (lv_event_get_target_obj(e) != title_bar) return;
    if (code == LV_EVENT_PRESSING) {
        /* Don't drag when maximized */
        if (g_maximized) return;
        /* Use Win32 native title bar drag — no LVGL event conflict with buttons */
        SDL_Window* sdlWin = app_get_window();
        if (sdlWin) {
            SDL_SysWMinfo wmInfo;
            SDL_VERSION(&wmInfo.version);
            if (SDL_GetWindowWMInfo(sdlWin, &wmInfo)) {
                HWND hwnd = wmInfo.info.win.window;
                ReleaseCapture();
                SendMessage(hwnd, WM_NCLBUTTONDOWN, HTCAPTION, 0);
            }
        }
    }
}

/* ═══ Full window relayout — called on resize/maximize/DPI change ═══ */
void ui_relayout_all() {
    SDL_Window* win = app_get_window();
    if (!win) return;

    int new_w, new_h;
    SDL_GetWindowSize(win, &new_w, &new_h);
    if (new_w < SCALE(WIN_MIN_W)) new_w = SCALE(WIN_MIN_W);
    if (new_h < SCALE(WIN_MIN_H)) new_h = SCALE(WIN_MIN_H);

    WIN_W = new_w;
    WIN_H = new_h;
    PANEL_H = WIN_H - TITLE_H - FOOTER_H - SCALE(24);
    if (PANEL_H < SCALE(300)) PANEL_H = SCALE(300);

    LOG_D("UI", "Relayout: window=%dx%d, left=%d, right=%d, panel_h=%d",
           WIN_W, WIN_H, LEFT_PANEL_W, RIGHT_PANEL_W, PANEL_H);

    /* Update divider line width */
    lv_obj_t* scr = lv_screen_active();
    if (scr) {
        /* Resize first child (divider) if it exists */
        uint32_t cc = lv_obj_get_child_count(scr);
        for (uint32_t i = 0; i < cc; i++) {
            lv_obj_t* ch = lv_obj_get_child(scr, (int32_t)i);
            if (ch && lv_obj_get_height(ch) == 1 && lv_obj_get_y(ch) == TITLE_H) {
                lv_obj_set_width(ch, WIN_W);
                break;
            }
        }
    }

    /* Update title bar width */
    if (title_bar) {
        lv_obj_set_width(title_bar, WIN_W);
    }

    /* Update window control button positions */
    if (btn_minimize && btn_maximize && btn_close && title_bar) {
        int wc_btn_size = std::max(TITLE_H * WC_BTN_H_PCT / 100, SCALE(28));
        int wc_btn_h = wc_btn_size;
        int wc_btn_gap = 6;
        int wc_btn_margin = 10;
        int wc_btn_y = (TITLE_H - wc_btn_h) / 2;
        int wc_base_x = WIN_W - wc_btn_size * 3 - wc_btn_gap * 2 - wc_btn_margin;
        lv_obj_set_pos(btn_minimize, wc_base_x, wc_btn_y);
        lv_obj_set_pos(btn_maximize, wc_base_x + wc_btn_size + wc_btn_gap, wc_btn_y);
        lv_obj_set_pos(btn_close, wc_base_x + (wc_btn_size + wc_btn_gap) * 2, wc_btn_y);

        int mode_w = std::max(WIN_W * MODE_BAR_W_PCT / 100, MODE_BAR_W_MIN);
        int side_w = std::max(WIN_W * SIDE_BTN_W_PCT / 100, SIDE_BTN_W_MIN);
        int top_gap = SCALE(6);
        int top_y = (TITLE_H - wc_btn_h) / 2;
        int right_x = wc_base_x - top_gap;
        if (top_btn_settings) {
            right_x -= side_w;
            lv_obj_set_size(top_btn_settings, side_w, wc_btn_h);
            lv_obj_set_pos(top_btn_settings, right_x, top_y);
        }
        if (mode_bar) {
            right_x -= (mode_w + top_gap);
            lv_obj_set_size(mode_bar, mode_w, wc_btn_h);
            lv_obj_set_pos(mode_bar, right_x, top_y);
        }
    }

    /* Update panels (left, right, splitter, chat, input, buttons) */
    relayout_panels();

    /* Update dialog sizes */
    if (g_exit_dialog_modal) {
        lv_obj_set_size(g_exit_dialog_modal, WIN_W, WIN_H);
        lv_obj_t* box = lv_obj_get_child(g_exit_dialog_modal, 0);
        if (box) lv_obj_set_pos(box, (WIN_W - lv_obj_get_width(box)) / 2, (WIN_H - lv_obj_get_height(box)) / 2);
    }
    if (g_about_dialog) {
        lv_obj_set_size(g_about_dialog, WIN_W, WIN_H);
    }

    /* Update footer position */
    /* Footer is aligned to bottom - LVGL handles this automatically via LV_ALIGN_BOTTOM_MID */

    /* Resize settings panel if visible */
    extern void ui_settings_relayout();
    ui_settings_relayout();

    lv_obj_invalidate(scr);
    LOG_D("UI", "Relayout complete");
}

/* ── Splitter relayout: update all panels and children ── */
static void relayout_panels() {
    if (!left_panel || !right_panel || !splitter) return;
    bool bot_module = (g_ui_nav_module == UI_NAV_BOT);

    /* Recompute from percentages */
    TITLE_H   = std::max(WIN_H * TITLE_H_PCT / 100, SCALE(TITLE_H_MIN));
    PANEL_TOP = TITLE_H + SCALE(8);
    PANEL_H   = WIN_H - PANEL_TOP - FOOTER_H - SCALE(16);

    int nav_w   = std::max(WIN_W * NAV_W_PCT / 100, SCALE(NAV_W_MIN));
    int avail_w = WIN_W - nav_w;
    PANEL_GAP   = std::max(WIN_W * SAFE_PAD_PCT / 100, SCALE(GAP_BASE));
    SPLITTER_W  = std::max((int)(WIN_W * SPLITTER_W_PCT / 100), SCALE(SPLITTER_W_MIN));

    if (bot_module) {
        LEFT_PANEL_W  = std::max(avail_w * LEFT_PANEL_PCT / 100, SCALE(LEFT_PANEL_MIN_W));
        RIGHT_PANEL_W = avail_w - LEFT_PANEL_W - PANEL_GAP;  /* no splitter — removed v2.2.11 */
        if (RIGHT_PANEL_W < SCALE(RIGHT_PANEL_MIN_W)) {
            RIGHT_PANEL_W = SCALE(RIGHT_PANEL_MIN_W);
            LEFT_PANEL_W = avail_w - RIGHT_PANEL_W - PANEL_GAP;
        }
    } else {
        LEFT_PANEL_W = 0;
        RIGHT_PANEL_W = std::max(SCALE(RIGHT_PANEL_MIN_W), avail_w - SCALE(8));
    }

    /* Move & resize left nav */
    if (left_nav) {
        lv_obj_set_size(left_nav, nav_w, PANEL_H);
        lv_obj_set_pos(left_nav, 0, PANEL_TOP);
    }

    /* Move & resize left panel */
    int panel_x = nav_w + SCALE(8);
    if (bot_module) {
        lv_obj_set_size(left_panel, LEFT_PANEL_W, PANEL_H);
        lv_obj_set_pos(left_panel, panel_x, PANEL_TOP);
    }

    /* Move & resize right panel (no splitter — removed in v2.2.11) */
    int right_x = bot_module ? (panel_x + LEFT_PANEL_W + PANEL_GAP) : panel_x;
    lv_obj_set_size(right_panel, RIGHT_PANEL_W, PANEL_H);
    lv_obj_set_pos(right_panel, right_x, PANEL_TOP);

    /* Resize ctrl_bar to follow right_panel width */
    if (ctrl_bar) {
        lv_obj_set_size(ctrl_bar, RIGHT_PANEL_W - CHAT_GAP * 2, SCALE(CTRL_BAR_H));
    }

    int content_w = mode_content_w();
    int content_h = mode_content_h();
    int content_y = CHAT_GAP + SCALE(CTRL_BAR_H);
    if (!bot_module) {
        content_h = PANEL_H - CHAT_GAP * 2;
        content_y = CHAT_GAP;
    }

    if (mode_panel_chat) {
        lv_obj_set_size(mode_panel_chat, content_w, content_h);
        lv_obj_set_pos(mode_panel_chat, CHAT_GAP, content_y);
    }
    if (mode_panel_work) {
        lv_obj_set_size(mode_panel_work, content_w, content_h);
        lv_obj_set_pos(mode_panel_work, CHAT_GAP, content_y);
    }
    if (module_placeholder) {
        lv_obj_set_size(module_placeholder, content_w, content_h);
        lv_obj_set_pos(module_placeholder, CHAT_GAP, content_y);
        int module_card_w = std::min(content_w - SCALE(40), SCALE(760));
        if (module_placeholder_desc) lv_obj_set_width(module_placeholder_desc, std::max(SCALE(260), module_card_w));
        if (module_tasks_panel) lv_obj_set_width(module_tasks_panel, std::max(SCALE(260), module_card_w));
        if (module_files_panel) lv_obj_set_width(module_files_panel, std::max(SCALE(260), module_card_w));
        int module_view_w = std::max(SCALE(220), module_card_w - SCALE(40));
        if (module_tasks_view) lv_obj_set_width(module_tasks_view, module_view_w);
        if (module_files_view) lv_obj_set_width(module_files_view, module_view_w);
    }
    if (mode_dd_control) lv_obj_set_width(mode_dd_control, std::min(content_w - 16, SCALE(360)));
    if (mode_dd_llm) lv_obj_set_width(mode_dd_llm, std::min(content_w - 16, SCALE(360)));
    if (mode_dd_ai_mode) lv_obj_set_width(mode_dd_ai_mode, std::min(content_w - 16, SCALE(360)));
    if (mode_lbl_work_hint) lv_obj_set_width(mode_lbl_work_hint, content_w - 16);
    if (mode_lbl_gemma_recommend) lv_obj_set_width(mode_lbl_gemma_recommend, content_w - 16);
    int work_chat_w = g_work_chat_collapsed ? SCALE(40) : (content_w * 38 / 100);
    if (g_c2_dual_result) work_chat_w = std::max(SCALE(260), content_w * 42 / 100);
    int work_primary_w = content_w - (g_c2_dual_result ? (work_chat_w + SCALE(20)) : SCALE(16));
    work_primary_w = std::max(work_primary_w, SCALE(280));
    int profile_cap = g_c2_dual_result ? SCALE(760) : SCALE(520);
    int profile_w = std::min(work_primary_w, profile_cap);

    if (mode_panel_work) {
        lv_obj_set_style_pad_right(mode_panel_work, g_c2_dual_result ? (work_chat_w + SCALE(12)) : 0, 0);
        uint32_t child_cnt = lv_obj_get_child_count(mode_panel_work);
        for (uint32_t i = 0; i < child_cnt; ++i) {
            lv_obj_t* ch = lv_obj_get_child(mode_panel_work, i);
            if (!ch || ch == mode_work_chat_panel) continue;
            lv_obj_set_width(ch, std::max(SCALE(180), profile_w + SCALE(24)));
        }
    }

    if (mode_ta_user_name) lv_obj_set_width(mode_ta_user_name, profile_w);
    if (mode_ta_user_role) lv_obj_set_width(mode_ta_user_role, profile_w);
    if (mode_ta_ai_name) lv_obj_set_width(mode_ta_ai_name, profile_w);
    if (mode_ta_ai_role) lv_obj_set_width(mode_ta_ai_role, profile_w);
    if (mode_ta_ai_persona) lv_obj_set_width(mode_ta_ai_persona, profile_w);
    if (mode_ta_ai_skills) lv_obj_set_width(mode_ta_ai_skills, profile_w);
    if (mode_ta_work_prompt) lv_obj_set_width(mode_ta_work_prompt, profile_w);
    if (mode_work_chat_panel) {
        lv_obj_set_width(mode_work_chat_panel, work_chat_w);
        if (g_c2_dual_result) lv_obj_set_height(mode_work_chat_panel, std::max(SCALE(220), content_h - SCALE(16)));
        else lv_obj_set_height(mode_work_chat_panel, g_work_chat_collapsed ? SCALE(40) : SCALE(220));
        lv_obj_set_pos(mode_work_chat_panel, std::max(SCALE(8), content_w - work_chat_w - SCALE(8)), SCALE(8));
    }
    if (mode_ta_work_chat_feed) lv_obj_set_width(mode_ta_work_chat_feed, std::max(SCALE(16), work_chat_w - SCALE(24)));
    if (mode_ta_work_chat_input) lv_obj_set_width(mode_ta_work_chat_input, std::max(SCALE(16), work_chat_w - SCALE(24)));
    if (mode_sec_step_stream) lv_obj_set_width(mode_sec_step_stream, profile_w + SCALE(24));
    if (mode_work_step_stream) {
        lv_obj_set_width(mode_work_step_stream, profile_w);
        lv_obj_set_height(mode_work_step_stream, SCALE(g_work_step_stream_h));
    }
    if (mode_work_splitter) lv_obj_set_width(mode_work_splitter, profile_w);
    if (mode_sec_work_console) lv_obj_set_width(mode_sec_work_console, profile_w + SCALE(24));
    if (mode_work_output_wrap) {
        lv_obj_set_width(mode_work_output_wrap, profile_w);
        lv_obj_set_height(mode_work_output_wrap, SCALE(g_work_output_h));
    }
    if (mode_dd_user_avatar) lv_obj_set_width(mode_dd_user_avatar, profile_w);
    if (mode_dd_ai_avatar) lv_obj_set_width(mode_dd_ai_avatar, profile_w);
    if (mode_remote_warning_bar) lv_obj_set_width(mode_remote_warning_bar, std::min(content_w - 16, SCALE(560)) - 24);
    if (mode_lbl_remote_state) lv_obj_set_width(mode_lbl_remote_state, content_w - 16);

    /* Re-layout chat mode children */
    int input_h = chat_input ? (int)lv_obj_get_height(chat_input) : 36;
    if (mode_trace_chat_panel) {
        int trace_h = chat_trace_panel_h(content_w);
        lv_obj_set_size(mode_trace_chat_panel, content_w - CHAT_GAP * 2, trace_h);
        lv_obj_set_pos(mode_trace_chat_panel, CHAT_GAP, CHAT_GAP);
    }
    int chat_y = chat_top_y_with_trace(content_w);
    int chat_h = content_h - chat_y - input_h - CHAT_GAP;
    int input_y = content_h - input_h - CHAT_GAP;

    /* Use global widget pointers for relayout */
    if (chat_cont) {
        int32_t saved_scroll = lv_obj_get_scroll_y(chat_cont);
        lv_obj_set_size(chat_cont, content_w - CHAT_GAP * 2, chat_h);
        lv_obj_update_layout(chat_cont);
        int32_t max_s = lv_obj_get_scroll_bottom(chat_cont);
        int32_t restore = saved_scroll;
        if (restore > max_s) restore = max_s;
        if (max_s > 0) lv_obj_scroll_to_y(chat_cont, restore, LV_ANIM_OFF);
    }
    if (chat_display) lv_obj_set_width(chat_display, content_w - CHAT_GAP * 3);
    if (chat_input) {
        lv_obj_set_size(chat_input, content_w - CHAT_GAP * 2, input_h);
        lv_obj_set_pos(chat_input, CHAT_GAP, input_y);
    }
    /* Reposition action buttons: send / upload / voice */
    layout_chat_action_buttons(content_w, input_y, input_h);
    /* Reposition search button: left of upload button */
    if (g_search_btn) {
        int btn_size = SCALE(CHAT_ACTION_BTN_BASE);
        int btn_margin = SCALE(CHAT_ACTION_BTN_MARGIN);
        int btn_gap = SCALE(CHAT_ACTION_BTN_GAP);
        int base_x = content_w - CHAT_GAP - btn_size - btn_margin;
        if (btn_upload_widget) base_x -= (btn_size + btn_gap);
        if (btn_voice_widget) base_x -= (btn_size + btn_gap);
        if (btn_work_widget) base_x -= (btn_size + btn_gap);
        lv_obj_set_pos(g_search_btn, base_x - btn_size - btn_gap, input_y + input_h - btn_size - btn_margin);
    }
    /* mode_dd_chat_ai_mode is now in ctrl_bar, no need to reposition here */
    if (mode_lbl_chat_status) {
        lv_obj_set_pos(mode_lbl_chat_status, CHAT_GAP + SCALE(96), input_y - SCALE(16));
    }

    /* Update left panel children widths + x positions */
    if (lp_separator) { lv_obj_set_x(lp_separator, GAP); }
    if (lp_status_row) { lv_obj_set_width(lp_status_row, LEFT_PANEL_W - GAP); lv_obj_set_x(lp_status_row, GAP); }
    /* status_label width now managed by flex container */
    if (lp_separator) { lv_obj_set_width(lp_separator, LEFT_PANEL_W - GAP * 2); lv_obj_set_x(lp_separator, GAP); }
    if (lp_task_title) { lv_obj_set_width(lp_task_title, LEFT_PANEL_W - GAP * 5); lv_obj_set_x(lp_task_title, GAP); }
    if (lp_task_count) { lv_obj_set_x(lp_task_count, GAP + lv_obj_get_width(lp_task_title) + 4); }
    if (task_empty_label) { lv_obj_set_width(task_empty_label, LEFT_PANEL_W - GAP * 4); lv_obj_set_x(task_empty_label, GAP); }
    if (lp_progress_panel) {
        lv_obj_set_width(lp_progress_panel, LEFT_PANEL_W - GAP * 2);
        lv_obj_align(lp_progress_panel, LV_ALIGN_BOTTOM_MID, 0, -SCALE(30));
    }

    /* Resize + reposition task item widgets */
    for (int i = 0; i < g_task_count; i++) {
        if (g_tasks[i].widget) {
            lv_obj_set_width(g_tasks[i].widget, LEFT_PANEL_W - GAP * 2);
            lv_obj_set_x(g_tasks[i].widget, GAP);
        }
    }

    lv_obj_invalidate(lv_screen_active());
}

/* ── Splitter drag handler ── */
/* ── Splitter cursor helpers ───────────────────────────────── */
static SDL_Cursor* g_cursor_resize = nullptr;
static SDL_Cursor* g_cursor_default = nullptr;
static bool g_splitter_hover = false;

static void splitter_cursor_init() {
    g_cursor_resize = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZEWE);
    g_cursor_default = SDL_GetDefaultCursor();
}

/* Timer: check if mouse is over splitter, update cursor */
static void splitter_hover_timer_cb(lv_timer_t* t) {
    (void)t;
    if (!splitter) return;
    int mx, my;
    SDL_GetMouseState(&mx, &my);
    lv_area_t a;
    lv_obj_get_coords(splitter, &a);
    bool over = (mx >= a.x1 && mx <= a.x2 && my >= a.y1 && my <= a.y2);
    if (over != g_splitter_hover) {
        g_splitter_hover = over;
        SDL_SetCursor(over ? g_cursor_resize : g_cursor_default);
    }
}

static void splitter_drag_cb(lv_event_t* e) {
    lv_event_code_t code = lv_event_get_code(e);
    const ThemeColors* c = g_colors ? g_colors : &THEME_DARK;
    static int drag_start_x = 0;
    static bool is_dragging = false;

    if (code == LV_EVENT_PRESSED) {
        is_dragging = true;
        lv_indev_t* indev = lv_indev_get_act();
        if (indev) {
            lv_point_t p;
            lv_indev_get_point(indev, &p);
            drag_start_x = p.x;
        }
        /* Highlight + resize cursor on press */
        lv_obj_set_style_bg_color(splitter, c->accent_secondary, 0);
        if (g_cursor_resize) SDL_SetCursor(g_cursor_resize);
    } else if (code == LV_EVENT_PRESSING && is_dragging) {
        lv_indev_t* indev = lv_indev_get_act();
        if (!indev) return;
        lv_point_t p;
        lv_indev_get_point(indev, &p);
        int dx = p.x - drag_start_x;
        drag_start_x = p.x;

        LEFT_PANEL_W += dx;
        relayout_panels();
    } else if (code == LV_EVENT_RELEASED || code == LV_EVENT_LEAVE) {
        is_dragging = false;
        /* Reset splitter color + cursor */
        const ThemeColors* c = g_colors;
        lv_obj_set_style_bg_color(splitter, c->border, 0);
        if (!g_splitter_hover && g_cursor_default) SDL_SetCursor(g_cursor_default);
        ui_log("[Splitter] Left panel width: %d, Right: %d", LEFT_PANEL_W, RIGHT_PANEL_W);
    }
}

static const char* status_text(ClawStatus s) {
    switch (s) {
        case ClawStatus::NotInstalled: return tr(STR_NOTINST);
        case ClawStatus::Detected:     return tr(STR_IDLE);
        case ClawStatus::Ready:        return tr(STR_READY);
        case ClawStatus::Busy:         return tr(STR_BUSY);
        case ClawStatus::Checking:     return tr(STR_CHECKING);
        case ClawStatus::Error:
            /* Show specific failure reason if available */
            if (!g_status_reason.empty()) return g_status_reason.c_str();
            return tr(STR_ERROR);
        default:                       return tr(STR_UNKNOWN);
    }
}

static lv_color_t status_color(ClawStatus s) {
    switch (s) {
        case ClawStatus::Ready:    return g_colors->success;     /* Green */
        case ClawStatus::Busy:     return g_colors->warning;   /* Yellow */
        case ClawStatus::Checking: return g_colors->warning;   /* Yellow */
        case ClawStatus::Error:    return g_colors->danger;    /* Red */
        case ClawStatus::Detected: return g_colors->warning;   /* Yellow */
        default:                   return g_colors->text_dim;   /* White / dim */
    }
}

static void auto_refresh_cb(lv_timer_t* timer) {
    (void)timer;
    proactive_tick();
    ui_log("[Timer] Auto-refreshing...");
    app_refresh_status();
}

/* Buttons moved to tray menu — callbacks removed */

/* P2-27: 自动刷新频率配置（可配置：15s / 30s / 60s） */
int g_refresh_interval_ms = HEALTH_CHECK_DEFAULT_MS;
lv_timer_t* g_refresh_timer = nullptr;

void update_refresh_interval(int new_ms) {
    g_refresh_interval_ms = new_ms;
    if (g_refresh_timer) {
        lv_timer_set_period(g_refresh_timer, new_ms);
    }
    /* Update hint text */
    char hint_text[128];
    snprintf(hint_text, sizeof(hint_text), "Auto-refresh: %ds", new_ms / 1000);
    ui_log("[Settings] Refresh interval changed to %ds", new_ms / 1000);
}

/* ═══════════════════════════════════════════════════════════════
 *  Label text selection — 全应用文字选中+复制
 * ═══════════════════════════════════════════════════════════════ */
#include "../../thirdparty/lvgl/src/core/lv_obj_tree.h"
#include "../../thirdparty/lvgl/src/misc/lv_timer_private.h"
#include "../../thirdparty/lvgl/src/widgets/label/lv_label_private.h"
extern const lv_obj_class_t lv_label_class;
extern const lv_obj_class_t lv_button_class;

/* Forward declarations for clipboard functions (defined later) */
void clipboard_copy_to_win(const char* text);
static char* clipboard_paste_from_win();

/* Currently selected label (for Ctrl+C) */
static lv_obj_t* g_selected_label = nullptr;
/* Drag state: whether mouse moved during press (to distinguish click vs drag) */
static bool g_label_dragging = false;
static lv_point_t g_label_press_pos = {0, 0};

/* Check if a label is inside a button (direct or indirect child) */
static bool is_label_in_button(lv_obj_t* lbl) {
    lv_obj_t* parent = lv_obj_get_parent(lbl);
    while (parent) {
        if (lv_obj_check_type(parent, &lv_button_class)) return true;
        parent = lv_obj_get_parent(parent);
    }
    return false;
}

/* Get char index at mouse position (relative to label) */
static uint32_t label_char_at(lv_obj_t* lbl, lv_point_t* pos) {
    lv_point_t p = {pos->x, pos->y};
    return lv_label_get_letter_on(lbl, &p, false);
}

static void label_select_cb(lv_event_t* e) {
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t* lbl = (lv_obj_t*)lv_event_get_target(e);
    lv_label_t* label_priv = (lv_label_t*)lbl;

    if (code == LV_EVENT_PRESSED) {
        /* Clear previous label's selection */
        if (g_selected_label && g_selected_label != lbl) {
            lv_label_t* prev = (lv_label_t*)g_selected_label;
            prev->sel_start = LV_LABEL_TEXT_SELECTION_OFF;
            prev->sel_end = LV_LABEL_TEXT_SELECTION_OFF;
            lv_obj_invalidate(g_selected_label);
        }
        g_selected_label = lbl;
        g_label_dragging = false;

        /* Record press position */
        lv_indev_t* indev = lv_indev_get_act();
        if (indev) lv_indev_get_point(indev, &g_label_press_pos);

        /* Convert to label-local coordinates */
        lv_area_t lbl_area;
        lv_obj_get_coords(lbl, &lbl_area);
        lv_point_t local = {g_label_press_pos.x - lbl_area.x1, g_label_press_pos.y - lbl_area.y1};

        /* Set selection start at press position */
        uint32_t ch = label_char_at(lbl, &local);
        label_priv->sel_start = ch;
        label_priv->sel_end = ch;
        lv_obj_invalidate(lbl);
    }
    else if (code == LV_EVENT_PRESSING) {
        /* Update selection end while dragging */
        lv_indev_t* indev = lv_indev_get_act();
        if (!indev) return;
        lv_point_t cur;
        lv_indev_get_point(indev, &cur);

        /* Detect drag: moved more than 3px from press point */
        int dx = cur.x - g_label_press_pos.x;
        int dy = cur.y - g_label_press_pos.y;
        if (dx * dx + dy * dy > 9) g_label_dragging = true;

        if (g_label_dragging) {
            lv_area_t lbl_area;
            lv_obj_get_coords(lbl, &lbl_area);
            lv_point_t local = {cur.x - lbl_area.x1, cur.y - lbl_area.y1};
            uint32_t ch = label_char_at(lbl, &local);
            label_priv->sel_end = ch;
            lv_obj_invalidate(lbl);
        }
    }
    else if (code == LV_EVENT_CLICKED) {
        /* Track this label for Ctrl+C, regardless of whether drag occurred */
        g_selected_label = lbl;
        g_label_dragging = false;
    }
    else if (code == LV_EVENT_DEFOCUSED) {
        /* Clear selection on defocus */
        label_priv->sel_start = LV_LABEL_TEXT_SELECTION_OFF;
        label_priv->sel_end = LV_LABEL_TEXT_SELECTION_OFF;
        if (g_selected_label == lbl) g_selected_label = nullptr;
        g_label_dragging = false;
        lv_obj_invalidate(lbl);
    }
}

/* Enable text selection on a single label (drag-to-select + click-to-select-all) */
void make_label_selectable(lv_obj_t* lbl) {
    if (!lbl) return;
    const ThemeColors* c = g_colors ? g_colors : &THEME_DARK;
    /* Skip button labels — they should only respond to clicks, not text selection */
    if (is_label_in_button(lbl)) return;
    lv_obj_add_flag(lbl, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(lbl, label_select_cb, LV_EVENT_PRESSED, nullptr);
    lv_obj_add_event_cb(lbl, label_select_cb, LV_EVENT_PRESSING, nullptr);
    lv_obj_add_event_cb(lbl, label_select_cb, LV_EVENT_CLICKED, nullptr);
    lv_obj_add_event_cb(lbl, label_select_cb, LV_EVENT_DEFOCUSED, nullptr);
    /* Selection highlight color — same light blue as chat input */
    lv_obj_set_style_bg_color(lbl, c->accent_secondary, LV_PART_SELECTED);
    lv_obj_set_style_bg_opa(lbl, LV_OPA_COVER, LV_PART_SELECTED);
    lv_obj_set_style_text_color(lbl, c->text_inverse, LV_PART_SELECTED);
}

/* Recursively make all labels in a widget tree selectable */
static void make_labels_selectable_recursive(lv_obj_t* parent) {
    if (!parent) return;
    uint32_t cnt = lv_obj_get_child_count(parent);
    for (uint32_t i = 0; i < cnt; i++) {
        lv_obj_t* child = lv_obj_get_child(parent, (int32_t)i);
        if (!child) continue;
        if (lv_obj_check_type(child, &lv_label_class)) {
            make_label_selectable(child);
        }
        make_labels_selectable_recursive(child);
    }
}

/* ═══════════════════════════════════════════════════════════════
 *  Global wheel scroll — 根据鼠标位置分发滚轮事件
 * ═══════════════════════════════════════════════════════════════ */
/* Global wheel diff — written by SDL event watch (main.cpp), read by ui_process_wheel_scroll */
int g_wheel_diff = 0;

/* Called from main loop to apply wheel scrolling */
void ui_process_wheel_scroll() {
    if (g_wheel_diff == 0) return;
    int diff = g_wheel_diff;
    g_wheel_diff = 0;

    /* Get mouse position directly from SDL — more reliable than LVGL indev state
     * which only updates on click/move, not on wheel-only events. */
    int mx, my;
    SDL_GetMouseState(&mx, &my);
    lv_point_t pos = { (int32_t)mx, (int32_t)my };

    /* Find the object under the mouse cursor */
    lv_obj_t* screen = lv_screen_active();
    if (!screen) return;

    lv_obj_t* obj = lv_indev_search_obj(screen, &pos);
    if (!obj) return;

    /* Walk up to find if cursor is over a scrollable target */
    lv_obj_t* scroll_obj = NULL;
    lv_obj_t* cur = obj;
    while (cur && cur != screen) {
        if (cur == chat_input || cur == chat_cont) {
            scroll_obj = cur;
            break;
        }
        /* Settings / list: scroll container with actual scrollable content.
         * LVGL v9 sets SCROLLABLE on all objects by default, so we also check
         * that the content actually overflows (scroll_top + scroll_bottom > 0). */
        if (lv_obj_has_flag(cur, LV_OBJ_FLAG_SCROLLABLE) &&
            (lv_obj_get_scroll_dir(cur) & LV_DIR_VER) &&
            (lv_obj_get_scroll_top(cur) + lv_obj_get_scroll_bottom(cur) > 0)) {
            scroll_obj = cur;
            break;
        }
        cur = lv_obj_get_parent(cur);
    }
    if (!scroll_obj) return; /* Not over a scrollable object — ignore wheel */

    /* Scroll: wheel-up = content up (show older messages above) */
    int32_t scroll_amount = (int32_t)(diff * SCROLL_SPEED_FACTOR);
    lv_obj_scroll_by(scroll_obj, 0, scroll_amount, LV_ANIM_OFF);
}

/* Global Ctrl+C: copy selected label text to clipboard */
void global_ctrl_c_copy() {
    if (!g_selected_label) return;
    lv_label_t* lbl = (lv_label_t*)g_selected_label;
    if (lbl->sel_start == LV_LABEL_TEXT_SELECTION_OFF || lbl->sel_end == LV_LABEL_TEXT_SELECTION_OFF) return;
    if (lbl->sel_start == lbl->sel_end) return;

    const char* text = lv_label_get_text(g_selected_label);
    if (!text) return;

    uint32_t s = lbl->sel_start < lbl->sel_end ? lbl->sel_start : lbl->sel_end;
    uint32_t e = lbl->sel_start < lbl->sel_end ? lbl->sel_end : lbl->sel_start;
    uint32_t text_len = (uint32_t)strlen(text);
    if (s >= text_len || e > text_len) return;

    char* selected = (char*)malloc(e - s + 1);
    if (selected) {
        memcpy(selected, text + s, e - s);
        selected[e - s] = '\0';
        clipboard_copy_to_win(selected);
        ui_log("[Select] Copied %u chars", e - s);
        free(selected);
    }
}

/* ── Clipboard shortcuts for chat_input (Ctrl+C / Ctrl+V / Ctrl+X) ── */
#include "../../thirdparty/lvgl/src/widgets/textarea/lv_textarea_private.h"

void clipboard_copy_to_win(const char* text) {
    if (!text || !text[0]) return;
    /* UTF-8 → UTF-16 for CF_UNICODETEXT */
    int wlen = MultiByteToWideChar(CP_UTF8, 0, text, -1, nullptr, 0);
    if (wlen <= 0) return;
    HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, wlen * sizeof(WCHAR));
    if (!hMem) return;
    MultiByteToWideChar(CP_UTF8, 0, text, -1, (WCHAR*)GlobalLock(hMem), wlen);
    GlobalUnlock(hMem);
    if (OpenClipboard(nullptr)) {
        EmptyClipboard();
        SetClipboardData(CF_UNICODETEXT, hMem);
        CloseClipboard();
    } else {
        GlobalFree(hMem);
    }
}

static char* clipboard_paste_from_win() {
    if (!IsClipboardFormatAvailable(CF_UNICODETEXT)) return nullptr;
    if (!OpenClipboard(nullptr)) return nullptr;
    HGLOBAL hMem = GetClipboardData(CF_UNICODETEXT);
    if (!hMem) { CloseClipboard(); return nullptr; }
    WCHAR* wtext = (WCHAR*)GlobalLock(hMem);
    if (!wtext) { CloseClipboard(); return nullptr; }
    /* UTF-16 → UTF-8 */
    int len = WideCharToMultiByte(CP_UTF8, 0, wtext, -1, nullptr, 0, nullptr, nullptr);
    char* text = (char*)malloc(len);
    if (text) WideCharToMultiByte(CP_UTF8, 0, wtext, -1, text, len, nullptr, nullptr);
    GlobalUnlock(hMem);
    CloseClipboard();
    return text;
}


/* ═══════════════════════════════════════════════════════════════
 *  Right-click context menu for chat_input
 * ═══════════════════════════════════════════════════════════════ */
static lv_obj_t* g_ctx_menu = nullptr;

static void ctx_menu_item_cb(lv_event_t* e) {
    lv_obj_t* btn = (lv_obj_t*)lv_event_get_target(e);
    const char* action = (const char*)lv_event_get_user_data(e);
    lv_textarea_t* ta_priv = (lv_textarea_t*)chat_input;

    if (strcmp(action, "copy") == 0) {
        if (ta_priv->sel_start != ta_priv->sel_end) {
            const char* full_text = lv_textarea_get_text(chat_input);
            uint32_t s = ta_priv->sel_start < ta_priv->sel_end ? ta_priv->sel_start : ta_priv->sel_end;
            uint32_t e2 = ta_priv->sel_start < ta_priv->sel_end ? ta_priv->sel_end : ta_priv->sel_start;
            uint32_t text_len = (uint32_t)strlen(full_text);
            if (s < text_len && e2 <= text_len) {
                char* selected = (char*)malloc(e2 - s + 1);
                if (selected) {
                    memcpy(selected, full_text + s, e2 - s);
                    selected[e2 - s] = '\0';
                    clipboard_copy_to_win(selected);
                    ui_log("[CtxMenu] Copied %u chars", e2 - s);
                    free(selected);
                }
            }
        }
    }
    else if (strcmp(action, "paste") == 0) {
        char* text = clipboard_paste_from_win();
        if (text) {
            if (ta_priv->sel_start != ta_priv->sel_end) {
                uint32_t s = ta_priv->sel_start < ta_priv->sel_end ? ta_priv->sel_start : ta_priv->sel_end;
                uint32_t e2 = ta_priv->sel_start < ta_priv->sel_end ? ta_priv->sel_end : ta_priv->sel_start;
                lv_textarea_set_cursor_pos(chat_input, (int32_t)e2);
                for (uint32_t i = 0; i < e2 - s; i++) lv_textarea_delete_char(chat_input);
            }
            lv_textarea_add_text(chat_input, text);
            ui_log("[CtxMenu] Pasted %u chars", (uint32_t)strlen(text));
            free(text);
        }
    }
    else if (strcmp(action, "cut") == 0) {
        if (ta_priv->sel_start != ta_priv->sel_end) {
            const char* full_text = lv_textarea_get_text(chat_input);
            uint32_t s = ta_priv->sel_start < ta_priv->sel_end ? ta_priv->sel_start : ta_priv->sel_end;
            uint32_t e2 = ta_priv->sel_start < ta_priv->sel_end ? ta_priv->sel_end : ta_priv->sel_start;
            uint32_t text_len = (uint32_t)strlen(full_text);
            if (s < text_len && e2 <= text_len) {
                char* selected = (char*)malloc(e2 - s + 1);
                if (selected) {
                    memcpy(selected, full_text + s, e2 - s);
                    selected[e2 - s] = '\0';
                    clipboard_copy_to_win(selected);
                    lv_textarea_set_cursor_pos(chat_input, (int32_t)e2);
                    for (uint32_t i = 0; i < e2 - s; i++) lv_textarea_delete_char(chat_input);
                    ui_log("[CtxMenu] Cut %u chars", e2 - s);
                    free(selected);
                }
            }
        }
    }
    else if (strcmp(action, "selectall") == 0) {
        const char* text = lv_textarea_get_text(chat_input);
        if (text) {
            ta_priv->sel_start = 0;
            ta_priv->sel_end = (uint32_t)strlen(text);
            lv_obj_invalidate(chat_input);
            ui_log("[CtxMenu] Select all");
        }
    }

    /* Close menu */
    if (g_ctx_menu) { lv_obj_del(g_ctx_menu); g_ctx_menu = nullptr; }
}

static void ctx_menu_bg_cb(lv_event_t* e) {
    /* Click outside menu closes it */
    if (g_ctx_menu) { lv_obj_del(g_ctx_menu); g_ctx_menu = nullptr; }
}

static void chat_input_right_click_cb(lv_event_t* e) {
    lv_event_code_t code = lv_event_get_code(e);
    if (code != LV_EVENT_LONG_PRESSED) return;

    /* Get mouse position */
    lv_indev_t* indev = lv_indev_get_act();
    if (!indev) return;
    lv_point_t p;
    lv_indev_get_point(indev, &p);

    /* Close previous menu if any */
    if (g_ctx_menu) { lv_obj_del(g_ctx_menu); g_ctx_menu = nullptr; }

    /* Create menu container */
    g_ctx_menu = lv_obj_create(lv_scr_act());
    lv_obj_set_size(g_ctx_menu, 160, LV_SIZE_CONTENT);
    lv_obj_set_pos(g_ctx_menu, p.x, p.y);
    lv_obj_set_style_bg_color(g_ctx_menu, g_colors->raised, 0);
    lv_obj_set_style_bg_opa(g_ctx_menu, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(g_ctx_menu, 1, 0);
    lv_obj_set_style_border_color(g_ctx_menu, g_colors->border_strong, 0);
    lv_obj_set_style_radius(g_ctx_menu, SCALE(g_colors->radius_sm), 0);
    lv_obj_set_style_pad_all(g_ctx_menu, 4, 0);
    lv_obj_clear_flag(g_ctx_menu, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_flex_flow(g_ctx_menu, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_gap(g_ctx_menu, 2, 0);

    /* Menu items */
    struct { const char* label; const char* action; } items[] = {
        {"Copy    Ctrl+C", "copy"},
        {"Cut     Ctrl+X", "cut"},
        {"Paste   Ctrl+V", "paste"},
        {"Select All  Ctrl+A", "selectall"},
    };

    for (int i = 0; i < 4; i++) {
        lv_obj_t* btn = lv_button_create(g_ctx_menu);
        lv_obj_set_width(btn, LV_PCT(100));
        lv_obj_set_height(btn, std::max(WIN_H * CTX_ITEM_H_PCT / 100, CTX_ITEM_H_MIN));
        lv_obj_set_style_bg_color(btn, g_colors->overlay, 0);
        lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, 0);
        lv_obj_set_style_radius(btn, SCALE(g_colors->radius_sm), 0);
        lv_obj_set_style_border_width(btn, 0, 0);
        lv_obj_set_style_pad_all(btn, 6, 0);
        lv_obj_set_style_text_color(btn, g_colors->text, 0);
        lv_obj_add_event_cb(btn, ctx_menu_item_cb, LV_EVENT_CLICKED, (void*)items[i].action);

        lv_obj_t* lbl = lv_label_create(btn);
        lv_label_set_text(lbl, items[i].label);
        lv_obj_set_style_text_font(lbl, FONT(12), 0);
        lv_obj_center(lbl);
    }

    /* Bring to front */
    lv_obj_move_foreground(g_ctx_menu);
}

/* ── Input box auto-resize callback ── */
static bool g_resize_in_progress = false;  /* re-entrancy guard */
static int g_input_line_count = 1;          /* track actual line count to avoid redundant resize */

static void chat_input_resize_cb(lv_event_t* e) {
    lv_event_code_t code = lv_event_get_code(e);
    if (code != LV_EVENT_VALUE_CHANGED && code != LV_EVENT_INSERT) return;
    if (g_resize_in_progress) return;
    g_resize_in_progress = true;

    lv_obj_t* ta = (lv_obj_t*)lv_event_get_target(e);
    if (!ta || !chat_input || !chat_cont || !right_panel || !mode_panel_chat) { g_resize_in_progress = false; return; }

    const char* text = lv_textarea_get_text(ta);
    if (!text) { g_resize_in_progress = false; return; }

    /* Measure actual text height including auto-wrap (not just \n) */
    const lv_font_t* font = lv_obj_get_style_text_font(ta, LV_PART_MAIN);
    int32_t ta_inner_w = lv_obj_get_content_width(ta);
    lv_point_t txt_sz;
    lv_text_get_size(&txt_sz, text, font, 0, 8, ta_inner_w, LV_TEXT_FLAG_BREAK_ALL);

    const int pad_ver = SCALE(24);
    const int min_h = SCALE(CHAT_INPUT_MIN_H_BASE);
    const int max_h = SCALE(CHAT_INPUT_MAX_H_BASE);
    int new_h = txt_sz.y + pad_ver;
    if (new_h < min_h) new_h = min_h;
    if (new_h > max_h) new_h = max_h;

    int cur_h = (int)lv_obj_get_height(chat_input);
    if (new_h != cur_h) {
        lv_obj_set_height(chat_input, new_h);

        if (new_h >= max_h) {
            lv_obj_add_flag(chat_input, LV_OBJ_FLAG_SCROLLABLE);
            /* Force scrollbar to show */
            lv_obj_set_scrollbar_mode(chat_input, LV_SCROLLBAR_MODE_AUTO);
        } else {
            lv_obj_clear_flag(chat_input, LV_OBJ_FLAG_SCROLLABLE);
            lv_obj_set_scrollbar_mode(chat_input, LV_SCROLLBAR_MODE_OFF);
        }

        int content_h = mode_content_h();
        int content_w = mode_content_w();
        int new_input_y = content_h - new_h - GAP;
        lv_obj_set_pos(chat_input, CHAT_GAP, new_input_y);

        layout_chat_action_buttons(content_w, new_input_y, new_h);

        int panel_h = content_h;
        int chat_y = chat_top_y_with_trace(content_w);
        int new_chat_h = panel_h - chat_y - new_h - GAP;
        if (new_chat_h < 50) new_chat_h = 50;
        int32_t saved_scroll = lv_obj_get_scroll_y(chat_cont);
        lv_obj_set_height(chat_cont, new_chat_h);
        lv_obj_update_layout(chat_cont);
        int32_t max_scroll = lv_obj_get_scroll_bottom(chat_cont);
        int32_t restore = saved_scroll;
        if (restore > max_scroll) restore = max_scroll;
        if (restore < 0) restore = 0;
        if (max_scroll > 0) lv_obj_scroll_to_y(chat_cont, restore, LV_ANIM_OFF);
    }

    g_resize_in_progress = false;
}

/* ── P2: Chat area ── */

/* P2-21: 消息气泡 - 用户消息靠右蓝色，AI消息靠左灰色 */

/* Helper: scroll chat container by given amount */
static void chat_scroll_by(int dy) {
    if (!chat_cont) return;
    lv_obj_invalidate(chat_cont);
    int32_t cur = lv_obj_get_scroll_y(chat_cont);
    int32_t max_scroll = lv_obj_get_scroll_bottom(chat_cont);
    int32_t new_y = cur + dy;
    if (new_y < 0) new_y = 0;
    if (new_y > max_scroll) new_y = max_scroll;
    lv_obj_scroll_to_y(chat_cont, new_y, LV_ANIM_OFF);
}

/* Block drag-scroll but allow wheel-scroll: cancel SCROLL_BEGIN from drag */
static void chat_scroll_begin_cb(lv_event_t* e) {
    /* Let wheel scroll through (crown mode sets pointer.diff) */
    lv_indev_t* indev = lv_indev_get_act();
    if (indev && lv_indev_get_type(indev) == LV_INDEV_TYPE_POINTER) {
        /* This is a drag scroll - block it */
        lv_event_stop_processing(e);
    }
}

/* Block all scroll events from any source (including auto-scroll-to-focus) */
/* REMOVED: not needed — SCROLL_ON_FOCUS cleared prevents auto-scroll */

/* Focus management: mouse over chat → chat gets focus (wheel scrolls history);
   mouse leaves → input gets focus (typing resumes) */
static void chat_focus_cb(lv_event_t* e) {
    lv_event_code_t code = lv_event_get_code(e);
    lv_group_t* grp = lv_group_get_default();
    if (!grp) return;
    if (code == LV_EVENT_FOCUSED) {
        /* Mouse entered chat area — wheel will scroll chat */
    } else if (code == LV_EVENT_DEFOCUSED) {
        /* Mouse left chat area — restore focus to input for typing */
        if (chat_input) lv_group_focus_obj(chat_input);
    }
}

/* Helper: scroll chat container to bottom (show newest messages).
 * Only scrolls if user is already at/near bottom — prevents fighting
 * with manual scroll during streaming. */
static void chat_scroll_to_bottom() {
    if (!chat_cont) return;
    uint32_t child_cnt = lv_obj_get_child_cnt(chat_cont);
    if (child_cnt == 0) return;
    /* Check if user is already near bottom (within 40px tolerance) */
    int32_t scroll_y = lv_obj_get_scroll_y(chat_cont);
    int32_t max_y = lv_obj_get_scroll_bottom(chat_cont);
    if (max_y > 0 && (max_y - scroll_y) > 40) return; /* user scrolled up — don't fight */
    lv_obj_t* last = lv_obj_get_child(chat_cont, child_cnt - 1);
    if (last) {
        lv_obj_scroll_to_view(last, LV_ANIM_OFF);
    }
}

/* Always scroll to bottom (for initial messages / first message after send) */
static void chat_force_scroll_bottom() {
    if (!chat_cont) return;
    uint32_t child_cnt = lv_obj_get_child_cnt(chat_cont);
    if (child_cnt == 0) return;
    lv_obj_t* last = lv_obj_get_child(chat_cont, child_cnt - 1);
    if (last) {
        lv_obj_scroll_to_view(last, LV_ANIM_OFF);
    }
}

static void chat_add_user_bubble(const char* text) {
    if (!chat_cont) return;
    const ThemeColors* c = g_colors ? g_colors : &THEME_DARK;

    char ts[32];
    SYSTEMTIME st;
    GetLocalTime(&st);
    snprintf(ts, sizeof(ts), "[%04d-%02d-%02d %02d:%02d:%02d] [User]",
             st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);

    /*
     * 用户消息 (QQ风格: 气泡靠右, 贴合文字宽度):
     *
     * row_wrap (ROW, width=100%, FLEX_ALIGN_END → 内容靠右)
     *   └── inner (COLUMN, SIZE_CONTENT)
     *         ├── meta (ROW, 右对齐: timestamp [User] [🧄])
     *         └── lbl  (蓝色气泡, 贴合文字, max_width限制)
     */
    lv_obj_t* row_wrap = lv_obj_create(chat_cont);
    lv_obj_set_width(row_wrap, LV_PCT(100));
    lv_obj_set_height(row_wrap, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(row_wrap, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(row_wrap, 0, 0);
    lv_obj_set_style_pad_all(row_wrap, 0, 0);
    lv_obj_set_style_radius(row_wrap, 0, 0);
    lv_obj_set_style_margin_hor(row_wrap, CHAT_MSG_MARGIN, 0);
    lv_obj_clear_flag(row_wrap, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_flex_flow(row_wrap, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(row_wrap, LV_FLEX_ALIGN_END, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_END);
    lv_obj_set_style_pad_gap(row_wrap, 0, 0);

    /* Inner column: meta + bubble */
    lv_obj_t* inner = lv_obj_create(row_wrap);
    lv_obj_set_width(inner, LV_PCT(70));
    lv_obj_set_height(inner, LV_SIZE_CONTENT);

    lv_obj_set_style_bg_opa(inner, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(inner, 0, 0);
    lv_obj_set_style_pad_all(inner, 0, 0);
    lv_obj_set_style_radius(inner, 0, 0);
    lv_obj_clear_flag(inner, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_flex_flow(inner, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(inner, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_END, LV_FLEX_ALIGN_END);
    lv_obj_set_style_pad_gap(inner, 4, 0);

    /* Meta row: [timestamp] [User] [🧄 avatar] */
    lv_obj_t* meta = lv_obj_create(inner);
    lv_obj_set_size(meta, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(meta, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(meta, 0, 0);
    lv_obj_set_style_pad_all(meta, 0, 0);
    lv_obj_set_style_radius(meta, 0, 0);
    lv_obj_clear_flag(meta, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_flex_flow(meta, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(meta, LV_FLEX_ALIGN_END, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_gap(meta, 6, 0);

    lv_obj_t* ts_lbl = lv_label_create(meta);
    lv_label_set_text(ts_lbl, ts);
    lv_obj_set_style_text_color(ts_lbl, c->text_hint, 0);
    lv_obj_set_style_text_font(ts_lbl, FONT(10), 0);

    lv_obj_t* user_lbl = lv_label_create(meta);
    lv_label_set_text(user_lbl, profile_user_name());
    lv_obj_set_style_text_color(user_lbl, c->text_hint, 0);
    lv_obj_set_style_text_font(user_lbl, FONT(10), 0);

    lv_obj_t* avatar = lv_image_create(meta);
    lv_image_set_src(avatar, profile_user_avatar_src());
    lv_obj_set_size(avatar, SCALE(AVATAR_USER_SIZE), SCALE(AVATAR_USER_SIZE));
    lv_image_set_scale(avatar, 256);
    lv_obj_clear_flag(avatar, LV_OBJ_FLAG_SCROLLABLE);

    /* Text label (蓝色气泡, 短消息贴合文字, 长消息70%换行) */
    lv_obj_t* lbl = lv_label_create(inner);
    lv_label_set_text(lbl, text);
    lv_label_set_long_mode(lbl, LV_LABEL_LONG_WRAP);

    /* Measure text: short → small bubble, long → 70% wrap */
    lv_point_t txt_sz;
    lv_text_get_size(&txt_sz, text, CJK_FONT_CHAT, 0, 0, LV_COORD_MAX, LV_TEXT_FLAG_NONE);
    lv_obj_update_layout(chat_cont);
    int32_t cont_w = lv_obj_get_content_width(chat_cont);
    int32_t max_bubble_w = cont_w * 70 / 100;
    int32_t text_w = txt_sz.x + 20; /* + pad_hor*2 */
    LOG_D("Chat", "User bubble: text=%dx%d cont_w=%d max_w=%d", txt_sz.x, txt_sz.y, cont_w, max_bubble_w);

    if (text_w < max_bubble_w) {
        /* Short text: shrink inner & label to fit */
        lv_obj_set_width(lbl, LV_SIZE_CONTENT);
        /* inner width = max(text_w, meta_row_w) */
        lv_obj_update_layout(meta);
        int32_t meta_w = lv_obj_get_width(meta);
        int32_t inner_w = (text_w > meta_w) ? text_w : meta_w;
        if (inner_w > max_bubble_w) inner_w = max_bubble_w;
        lv_obj_set_width(inner, inner_w);
    } else {
        /* Long text: fill 70% and wrap */
        lv_obj_set_width(lbl, LV_PCT(100));
        lv_obj_set_width(inner, max_bubble_w);
    }

    lv_obj_set_style_text_align(lbl, LV_TEXT_ALIGN_LEFT, 0);
    lv_obj_set_style_text_color(lbl, c->text_inverse, 0);
    lv_obj_set_style_text_font(lbl, CJK_FONT_CHAT, 0);
    lv_obj_set_style_pad_hor(lbl, 10, 0);
    lv_obj_set_style_pad_ver(lbl, 4, 0);
    lv_obj_set_style_bg_color(lbl, c->btn_action, 0);
    lv_obj_set_style_bg_opa(lbl, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(lbl, SCALE(g_colors->radius_xl), 0);
    lv_obj_set_style_border_width(lbl, 0, 0);
    lv_obj_clear_flag(lbl, LV_OBJ_FLAG_SCROLLABLE);
    make_label_selectable(lbl);  /* Enable mouse text selection + Ctrl+C */

    /* Save to in-memory chat_history */
    char entry[512];
    snprintf(entry, sizeof(entry), "%s %s", ts, text);
    size_t hlen = strlen(chat_history);
    size_t elen = strlen(entry);
    if (hlen + elen < sizeof(chat_history) - 1) {
        strcat(chat_history, entry);
        strcat(chat_history, "\n");
    }

    /* Persist to disk */
    char ts_short[32];
    snprintf(ts_short, sizeof(ts_short), "%04d-%02d-%02d %02d:%02d:%02d",
             st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);
    persist_chat_message("user", text, ts_short);

    chat_force_scroll_bottom();
    update_chat_empty_state();
}
#include "markdown.h"

/* P2-19: AI streaming state */
/* ═══════════════════════════════════════════════════════════════
 *  Real AI Chat via OpenClaw Gateway /v1/chat/completions
 * ═══════════════════════════════════════════════════════════════ */
#include <windows.h>
#include <process.h>
#include <fstream>

/* Forward declarations (defined later in this file) */
static void chat_add_ai_bubble(const char* text);

static lv_obj_t* g_stream_bubble = nullptr;
static lv_obj_t* g_stream_label = nullptr;
static char g_stream_buffer[16384] = {0};  /* 16KB — doubled for long AI responses */
static CRITICAL_SECTION g_stream_cs;
static bool g_stream_cs_ready = false;
static volatile LONG g_stream_new_data = 0;   /* atomic flag: new data available */
volatile LONG g_stream_done = 0;        /* atomic flag: stream finished */
static lv_timer_t* g_stream_timer = nullptr;
static bool is_streaming_now() { return g_streaming; }
static HANDLE g_stream_thread = nullptr;
static DWORD g_stream_start_tick = 0;          /* when stream started (GetTickCount) */
static DWORD g_stream_last_data_tick = 0;      /* last time new data arrived */

struct WorkStepEvent {
    char action[64];
    char detail[1024];
    bool write_op;
};
static CRITICAL_SECTION g_work_step_cs;
static bool g_work_step_cs_ready = false;
static WorkStepEvent g_work_step_queue[32];
static int g_work_step_q_head = 0;
static int g_work_step_q_tail = 0;

static void stream_buf_init_once() {
    if (!g_stream_cs_ready) {
        InitializeCriticalSection(&g_stream_cs);
        g_stream_cs_ready = true;
    }
    if (!g_work_step_cs_ready) {
        InitializeCriticalSection(&g_work_step_cs);
        g_work_step_cs_ready = true;
    }
}

static void work_step_enqueue(const char* action, const char* detail, bool write_op) {
    stream_buf_init_once();
    EnterCriticalSection(&g_work_step_cs);
    int next = (g_work_step_q_tail + 1) % (int)(sizeof(g_work_step_queue) / sizeof(g_work_step_queue[0]));
    if (next == g_work_step_q_head) {
        g_work_step_q_head = (g_work_step_q_head + 1) % (int)(sizeof(g_work_step_queue) / sizeof(g_work_step_queue[0]));
    }
    WorkStepEvent& ev = g_work_step_queue[g_work_step_q_tail];
    snprintf(ev.action, sizeof(ev.action), "%s", action ? action : "step");
    snprintf(ev.detail, sizeof(ev.detail), "%s", detail ? detail : "");
    ev.write_op = write_op;
    g_work_step_q_tail = next;
    LeaveCriticalSection(&g_work_step_cs);
}

static bool work_step_dequeue(WorkStepEvent& out) {
    stream_buf_init_once();
    EnterCriticalSection(&g_work_step_cs);
    if (g_work_step_q_head == g_work_step_q_tail) {
        LeaveCriticalSection(&g_work_step_cs);
        return false;
    }
    out = g_work_step_queue[g_work_step_q_head];
    g_work_step_q_head = (g_work_step_q_head + 1) % (int)(sizeof(g_work_step_queue) / sizeof(g_work_step_queue[0]));
    LeaveCriticalSection(&g_work_step_cs);
    return true;
}

static void stream_buf_clear() {
    stream_buf_init_once();
    EnterCriticalSection(&g_stream_cs);
    g_stream_buffer[0] = '\0';
    LeaveCriticalSection(&g_stream_cs);
}

static void stream_buf_append_text(const char* value) {
    if (!value || !value[0]) return;
    stream_buf_init_once();
    EnterCriticalSection(&g_stream_cs);
    LONG buf_len = (LONG)strlen(g_stream_buffer);
    LONG vi = (LONG)strlen(value);
    LONG space = (LONG)sizeof(g_stream_buffer) - buf_len - 1;
    if (space > 0 && vi <= space) {
        strcat(g_stream_buffer, value);
    } else if (space > 0) {
        int safe_len = space;
        while (safe_len > 0 && (value[safe_len] & 0xC0) == 0x80) safe_len--;
        if (safe_len > 0) {
            memcpy(g_stream_buffer + buf_len, value, safe_len);
            g_stream_buffer[buf_len + safe_len] = '\0';
        }
    }
    LeaveCriticalSection(&g_stream_cs);
}

static void stream_buf_copy(char* out, size_t out_size) {
    if (!out || out_size == 0) return;
    stream_buf_init_once();
    EnterCriticalSection(&g_stream_cs);
    strncpy(out, g_stream_buffer, out_size - 1);
    out[out_size - 1] = '\0';
    LeaveCriticalSection(&g_stream_cs);
}

/* Read Gateway auth token from openclaw.json */
static std::string read_gateway_token() {
    /* Try common config paths */
    const char* paths[] = {
        nullptr,  /* will be set from APPDATA */
        "Z:\\root\\.openclaw\\openclaw.json",  /* test path */
    };
    std::string config_path;

    /* Build APPDATA path */
    char appdata_path[512] = {0};
    const char* appdata = std::getenv("APPDATA");
    if (appdata) {
        const char* userprofile = std::getenv("USERPROFILE");
        if (userprofile) snprintf(appdata_path, sizeof(appdata_path), "%s\\.openclaw\\openclaw.json", userprofile);
    }

    /* Try APPDATA first, then test path */
    for (int i = 0; i < 2; i++) {
        const char* p = (i == 0) ? appdata_path : paths[1];
        if (!p || !p[0]) continue;
        std::ifstream f(p);
        if (f.is_open()) {
            config_path = p;
            break;
        }
    }

    if (config_path.empty()) return "";

    /* Read file and find token */
    std::ifstream f(config_path);
    std::string content((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
    f.close();

    /* Find "token": "..." under gateway.auth */
    size_t auth_pos = content.find("\"auth\"");
    if (auth_pos == std::string::npos) return "";
    /* Find the "token" KEY (not value). It's followed by : and a quoted value.
     * Skip past "mode": "token" which has "token" as a value, not a key. */
    size_t search_from = auth_pos;
    size_t token_key_pos = std::string::npos;
    while (true) {
        size_t pos = content.find("\"token\"", search_from);
        if (pos == std::string::npos) break;
        /* Check if this is a key: next non-whitespace char after "token" should be : */
        size_t after = pos + 7;
        while (after < content.size() && (content[after] == ' ' || content[after] == '\t' || content[after] == '\n' || content[after] == '\r')) after++;
        if (after < content.size() && content[after] == ':') {
            token_key_pos = pos;
            break;
        }
        /* This "token" is a value (e.g. "mode": "token"), skip it */
        search_from = pos + 7;
    }
    if (token_key_pos == std::string::npos) return "";
    /* Find the value: first " after the : */
    size_t colon = content.find(':', token_key_pos + 7);
    if (colon == std::string::npos) return "";
    size_t token_start = content.find('"', colon + 1);
    if (token_start == std::string::npos) return "";
    token_start++;  /* skip opening quote */
    size_t token_end = content.find('"', token_start);
    if (token_end == std::string::npos) return "";

    std::string token = content.substr(token_start, token_end - token_start);
    LOG_D("Chat", "Read gateway token: %zu chars from %s", token.size(), config_path.c_str());
    return token;
}

/* Read Gateway port from openclaw.json (default 18789) */
static int read_gateway_port() {
    const char* userprofile = std::getenv("USERPROFILE");
    if (!userprofile) return 18789;

    char path[512];
    snprintf(path, sizeof(path), "%s\\.openclaw\\openclaw.json", userprofile);
    std::ifstream f(path);
    if (!f.is_open()) return 18789;

    std::string content((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
    f.close();

    size_t pos = content.find("\"port\"");
    if (pos == std::string::npos) return 18789;
    pos = content.find(':', pos);
    if (pos == std::string::npos) return 18789;
    pos++;
    while (pos < content.size() && (content[pos] == ' ' || content[pos] == '\t')) pos++;
    int port = 0;
    while (pos < content.size() && content[pos] >= '0' && content[pos] <= '9') {
        port = port * 10 + (content[pos] - '0');
        pos++;
    }
    return port > 0 ? port : 18789;
}

/* SSE stream callback — called from background thread for each HTTP chunk */
static void sse_chunk_cb(const char* chunk, void* ctx) {
    (void)ctx;
    if (!chunk || !chunk[0]) return;
    g_stream_last_data_tick = GetTickCount(); /* watchdog: track activity */

    /* Parse SSE lines: "data: {json}\n" */
    const char* p = chunk;
    while (p && *p) {
        const char* data_start = strstr(p, "data: ");
        if (!data_start) break;
        data_start += 6;

        const char* line_end = strstr(data_start, "\n");
        if (!line_end) line_end = data_start + strlen(data_start);

        /* Check for [DONE] */
        if (line_end - data_start >= 6 && strncmp(data_start, "[DONE]", 6) == 0) {
            InterlockedExchange(&g_stream_done, 1);
            return;
        }

        if (line_end > data_start && strstr(data_start, "\"type\":\"ask_feedback\"")) {
            char evt_json[768] = {0};
            int n = (int)std::min((ptrdiff_t)sizeof(evt_json) - 1, line_end - data_start);
            memcpy(evt_json, data_start, n);
            evt_json[n] = '\0';
            char reason[256] = {0};
            char suggestion[256] = {0};
            json_extract_string(evt_json, "reason", reason, sizeof(reason));
            json_extract_string(evt_json, "suggestion", suggestion, sizeof(suggestion));
            snprintf(g_ask_feedback_reason, sizeof(g_ask_feedback_reason), "%s", reason[0] ? reason : "LLM requested confirmation");
            snprintf(g_ask_feedback_suggestion, sizeof(g_ask_feedback_suggestion), "%s", suggestion[0] ? suggestion : "Please confirm next action");
            g_ask_feedback_options[0] = '\0';
            const char* op = strstr(evt_json, "\"options\":[");
            if (op) {
                op = strchr(op, '[');
                if (op) op++;
                int count = 0;
                while (op && *op && *op != ']' && count < 4) {
                    while (*op && *op != '"' && *op != ']') op++;
                    if (*op == ']') break;
                    op++;
                    char one[80] = {0};
                    int oi = 0;
                    while (*op && *op != '"' && oi < (int)sizeof(one) - 1) one[oi++] = *op++;
                    if (one[0]) {
                        if (g_ask_feedback_options[0]) strncat(g_ask_feedback_options, "|", sizeof(g_ask_feedback_options) - strlen(g_ask_feedback_options) - 1);
                        strncat(g_ask_feedback_options, one, sizeof(g_ask_feedback_options) - strlen(g_ask_feedback_options) - 1);
                        count++;
                    }
                    if (*op) op++;
                }
            }
            InterlockedExchange(&g_ask_feedback_pending, 1);
        }

        if (line_end > data_start && strstr(data_start, "\"tool_calls\"")) {
            const char* np = strstr(data_start, "\"name\":\"");
            if (np) {
                np += 8;
                char name[48] = {0};
                int ni = 0;
                while (np < line_end && *np && *np != '"' && ni < (int)sizeof(name) - 1) name[ni++] = *np++;
                char detail[1024] = {0};
                snprintf(detail, sizeof(detail), "%.*s", (int)std::min((ptrdiff_t)900, line_end - data_start), data_start);
                const bool write_op = strstr(name, "write") != nullptr;
                const bool high_risk_op =
                    write_op ||
                    strstr(name, "delete") != nullptr ||
                    strstr(name, "exec") != nullptr ||
                    strstr(name, "shell") != nullptr ||
                    strstr(name, "send") != nullptr ||
                    strstr(name, "install") != nullptr ||
                    strstr(name, "network") != nullptr;
                char action[64] = {0};
                snprintf(action, sizeof(action), "tool:%s", name[0] ? name : "unknown");
                work_step_enqueue(action, detail, write_op);
                if (high_risk_op && g_ai_interaction_mode == AIMODE_ASK &&
                    InterlockedCompareExchange(&g_ask_feedback_pending, 0, 0) == 0) {
                    snprintf(g_ask_feedback_reason, sizeof(g_ask_feedback_reason),
                             "Tool call requests risky operation: %s", name[0] ? name : "tool");
                    snprintf(g_ask_feedback_suggestion, sizeof(g_ask_feedback_suggestion),
                             "Approve or reject this action before continuing");
                    snprintf(g_ask_feedback_options, sizeof(g_ask_feedback_options), "allow_once|deny|adjust_plan");
                    InterlockedExchange(&g_ask_feedback_pending, 1);
                }
            }
        }

        /* Extract content from JSON: "content":"..." */
        const char* cp = data_start;
        while (cp < line_end) {
            const char* content_key = strstr(cp, "\"content\"");
            if (!content_key || content_key >= line_end) break;
            content_key += 9;
            while (content_key < line_end && (*content_key == ' ' || *content_key == ':' || *content_key == '"')) content_key++;

            char value[2048] = {0};
            int vi = 0;
            while (content_key < line_end && *content_key != '"' && vi < 2000) {
                if (*content_key == '\\' && content_key + 1 < line_end) {
                    content_key++;
                    switch (*content_key) {
                        case 'n': value[vi++] = '\n'; content_key++; break;
                        case 't': value[vi++] = '\t'; content_key++; break;
                        case '"': value[vi++] = '"'; content_key++; break;
                        case '\\': value[vi++] = '\\'; content_key++; break;
                        case 'u': {
                            /* Decode \uXXXX → UTF-8 (handles surrogate pairs) */
                            content_key++;
                            if (content_key + 4 <= line_end) {
                                unsigned int codepoint = 0;
                                for (int h = 0; h < 4; h++) {
                                    char c = content_key[h];
                                    codepoint <<= 4;
                                    if (c >= '0' && c <= '9') codepoint |= (c - '0');
                                    else if (c >= 'a' && c <= 'f') codepoint |= (c - 'a' + 10);
                                    else if (c >= 'A' && c <= 'F') codepoint |= (c - 'A' + 10);
                                }
                                content_key += 4;
                                /* Check for surrogate pair: \uD8xx\uDCxx */
                                if (codepoint >= 0xD800 && codepoint <= 0xDBFF &&
                                    content_key + 1 < line_end && content_key[0] == '\\' && content_key[1] == 'u' &&
                                    content_key + 6 <= line_end) {
                                    unsigned int low = 0;
                                    for (int h = 0; h < 4; h++) {
                                        char c = content_key[2 + h];
                                        low <<= 4;
                                        if (c >= '0' && c <= '9') low |= (c - '0');
                                        else if (c >= 'a' && c <= 'f') low |= (c - 'a' + 10);
                                        else if (c >= 'A' && c <= 'F') low |= (c - 'A' + 10);
                                    }
                                    if (low >= 0xDC00 && low <= 0xDFFF) {
                                        codepoint = 0x10000 + ((codepoint - 0xD800) << 10) + (low - 0xDC00);
                                        content_key += 6;  /* skip \uXXXX (low surrogate) */
                                    }
                                }
                                /* UTF-8 encode — emoji → placeholder (LVGL can't render colored emoji) */
                                if (codepoint >= 0x1F300 && codepoint <= 0x1F9FF) {
                                    /* Emoji ranges → skip (no font support) */
                                } else if (codepoint < 0x80) {
                                    value[vi++] = (char)codepoint;
                                } else if (codepoint < 0x800) {
                                    value[vi++] = (char)(0xC0 | (codepoint >> 6));
                                    value[vi++] = (char)(0x80 | (codepoint & 0x3F));
                                } else if (codepoint < 0x10000) {
                                    value[vi++] = (char)(0xE0 | (codepoint >> 12));
                                    value[vi++] = (char)(0x80 | ((codepoint >> 6) & 0x3F));
                                    value[vi++] = (char)(0x80 | (codepoint & 0x3F));
                                } else {
                                    /* 4-byte UTF-8 for supplementary plane */
                                    value[vi++] = (char)(0xF0 | ((codepoint >> 18) & 0x07));
                                    value[vi++] = (char)(0x80 | ((codepoint >> 12) & 0x3F));
                                    value[vi++] = (char)(0x80 | ((codepoint >> 6) & 0x3F));
                                    value[vi++] = (char)(0x80 | (codepoint & 0x3F));
                                }
                            }
                            break;
                        }
                        default: value[vi++] = *content_key++; break;
                    }
                } else {
                    value[vi++] = *content_key++;
                }
            }
            value[vi] = '\0';

            if (vi > 0) {
                stream_buf_append_text(value);
                InterlockedExchange(&g_stream_new_data, 1);
            }
            cp = content_key;
        }
        p = line_end + 1;
    }
}

/* Background thread: call OpenClaw Gateway chat completions API */
static unsigned __stdcall chat_api_thread(void* arg) {
    char* user_msg = (char*)arg;
    if (!user_msg) { InterlockedExchange(&g_stream_done, 1); return 1; }

    /* Read Gateway connection info */
    std::string gw_token = read_gateway_token();
    int gw_port = read_gateway_port();

    /* Escape user message and system prompt for JSON (dynamic std::string) */
    std::string escaped_msg = json_escape(user_msg);
    std::string escaped_sys = json_escape(ai_mode_system_prompt());

    /* Decide route: gateway(openclaw:main) or local llama.cpp */
    char route_model[256] = {0};
    app_get_current_model(route_model, sizeof(route_model));
    if (!route_model[0] && g_selected_model[0]) {
        snprintf(route_model, sizeof(route_model), "%s", g_selected_model);
    }
    bool use_gemma_local = gemma_local_is_model(route_model);
    if (use_gemma_local) {
        char local_err[256] = {0};
        if (!gemma_local_server_start(route_model, local_err, sizeof(local_err))) {
            stream_buf_clear();
            stream_buf_append_text(local_err[0] ? local_err : "⚠️ Gemma local server start failed.");
            InterlockedExchange(&g_stream_new_data, 1);
            InterlockedExchange(&g_stream_done, 1);
            free(user_msg);
            return 1;
        }
    } else if (gw_token.empty()) {
        LOG_E("Chat", "Cannot read Gateway token from openclaw.json");
        ui_log("[Chat] Error: Cannot read Gateway auth token");
        InterlockedExchange(&g_stream_done, 1);
        free(user_msg);
        return 1;
    }

    /* Build request JSON — OpenAI-compatible format */
    const char* model_id = use_gemma_local ? route_model : "openclaw:main";
    std::string json_body = "{\"model\":\"" + std::string(model_id) +
        "\",\"messages\":[{\"role\":\"system\",\"content\":\"" + escaped_sys +
        "\"},{\"role\":\"user\",\"content\":\"" + escaped_msg +
        "\"}],\"stream\":true}";

    /* Build request URL */
    char url[256];
    if (use_gemma_local) snprintf(url, sizeof(url), "http://127.0.0.1:%d/v1/chat/completions", gemma_local_server_port());
    else snprintf(url, sizeof(url), "http://127.0.0.1:%d/v1/chat/completions", gw_port);

    LOG_I("Chat", "Calling %s route at %s", use_gemma_local ? "gemma-local" : "gateway", url);
    ui_log(use_gemma_local ? "[Chat] Sending to Gemma local..." : "[Chat] Sending to OpenClaw Gateway...");

    /* Initialize stream buffer */
    stream_buf_clear();
    InterlockedExchange(&g_stream_new_data, 0);
    InterlockedExchange(&g_stream_done, 0);

    /* Build auth header — http_post_stream adds "Bearer " prefix, pass raw token */
    char auth_header[512];
    snprintf(auth_header, sizeof(auth_header), "%s", use_gemma_local ? "" : gw_token.c_str());

    /* Call streaming API */
    DWORD tick_start = GetTickCount();
    int status = http_post_stream(url, json_body.c_str(), auth_header, sse_chunk_cb, nullptr, 60);
    DWORD tick_elapsed = GetTickCount() - tick_start;

    char stream_snapshot[16384] = {0};
    stream_buf_copy(stream_snapshot, sizeof(stream_snapshot));
    printf("[Chat] Gateway response: status=%d, buffer_len=%zu\n", status, strlen(stream_snapshot));

    /* Record result for failover health tracking (gateway route only) */
    if (!use_gemma_local) {
        char cur_model[256] = {0};
        app_get_current_model(cur_model, sizeof(cur_model));
        if (cur_model[0]) {
            failover_record_result(cur_model, (status == 200 && stream_snapshot[0] != '\0'), tick_elapsed);
        }
    }

    /* If HTTP failed or no data received, try failover */
    if (!use_gemma_local && (status != 200 || stream_snapshot[0] == '\0')) {
        /* Try failover: switch to a healthy backup model */
        if (failover_is_enabled()) {
            char cur_model[256] = {0};
            app_get_current_model(cur_model, sizeof(cur_model));
            const char* backup = failover_get_next_healthy(cur_model);
            if (backup) {
                ui_log("[Chat] Failover: switching to %s", backup);
                LOG_I("Chat", "Failover: switching from %s to %s", cur_model, backup);
                if (app_update_model_config(nullptr, backup)) {
                    Sleep(3000); /* wait for gateway restart */
                    stream_buf_clear();
                    InterlockedExchange(&g_stream_new_data, 0);
                    json_body = "{\"model\":\"openclaw:main\",\"messages\":[{\"role\":\"system\",\"content\":\"" + escaped_sys + "\"},{\"role\":\"user\",\"content\":\"" + escaped_msg + "\"}],\"stream\":true}";
                    status = http_post_stream(url, json_body.c_str(), auth_header, sse_chunk_cb, nullptr, 60);
                    DWORD fe = GetTickCount() - tick_start;
                    ui_log("[Chat] Failover result: HTTP %d (%lums)", status, fe);
                    stream_buf_copy(stream_snapshot, sizeof(stream_snapshot));
                    failover_record_result(backup, (status == 200 && stream_snapshot[0] != '\0'), fe);
                }
            }
        }

        if (status != 200 || stream_snapshot[0] == '\0') {
            if (status == 404) {
                /* Auto-fix: enable chat completions endpoint and retry */
                ui_log("[Chat] Endpoint 404, auto-enabling chatCompletions...");
                LOG_W("Chat", "chatCompletions endpoint returned 404, attempting auto-fix");
                if (app_enable_chat_endpoint()) {
                    Sleep(3000);
                    stream_buf_clear();
                    InterlockedExchange(&g_stream_new_data, 0);
                    status = http_post_stream(url, json_body.c_str(), auth_header, sse_chunk_cb, nullptr, 60);
                    stream_buf_copy(stream_snapshot, sizeof(stream_snapshot));
                }
            }
            if (status != 200 || stream_snapshot[0] == '\0') {
                if (status == 0) {
                    stream_buf_clear();
                    stream_buf_append_text("⚠️ Gateway 未响应，请确认 OpenClaw 正在运行。");
                } else if (status == 404) {
                    stream_buf_clear();
                    stream_buf_append_text("⚠️ Gateway HTTP API 未开启。\n请执行: openclaw config set gateway.http.endpoints.chatCompletions.enabled true");
                } else {
                    char err_msg[128] = {0};
                    snprintf(err_msg, sizeof(err_msg), "⚠️ Gateway 返回错误 (HTTP %d)", status);
                    stream_buf_clear();
                    stream_buf_append_text(err_msg);
                }
                InterlockedExchange(&g_stream_new_data, 1);
            }
        }
    }

    InterlockedExchange(&g_stream_done, 1);
    free(user_msg);
    return 0;
}

/* Main-thread timer: render new data from background thread into LVGL label */
static void stream_timer_cb(lv_timer_t* timer) {
    (void)timer;
    if (!g_stream_label || !g_streaming) return;

    /* Watchdog: if no data for 30s or total elapsed > 45s, force-finish */
    DWORD now = GetTickCount();
    DWORD idle_ms = now - g_stream_last_data_tick;
    DWORD total_ms = now - g_stream_start_tick;
    if ((idle_ms > 30000 || total_ms > 45000) && !g_stream_done) {
        LOG_W("Chat", "Stream timeout: idle=%lums total=%lums, force-finish", idle_ms, total_ms);
        ui_log("[Chat] Stream timeout (%.0fs), finishing...", total_ms / 1000.0);
        set_ai_next_step("Timeout, please retry");
        append_ai_script_log("[error] stream timeout");
        if (g_work_waiting_ai) {
            work_add_step_card("模型输出超时", "流式结果触发超时保护", true, false);
        }
        update_chat_status_label("Timeout", false);
        char stream_snapshot[16384] = {0};
        stream_buf_copy(stream_snapshot, sizeof(stream_snapshot));
        if (stream_snapshot[0] == '\0') {
            stream_buf_clear();
            stream_buf_append_text("⚠️ AI 回复超时，请重试。");
            InterlockedExchange(&g_stream_new_data, 1);
        }
        InterlockedExchange(&g_stream_done, 1);
    }

    if (InterlockedCompareExchange(&g_stream_new_data, 0, 1)) {
        char stream_snapshot[16384] = {0};
        stream_buf_copy(stream_snapshot, sizeof(stream_snapshot));
        render_markdown_to_label(g_stream_label, stream_snapshot, CJK_FONT_CHAT);
        chat_scroll_to_bottom();
    }

    WorkStepEvent sev{};
    while (work_step_dequeue(sev)) {
        work_add_step_card(sev.action, sev.detail, false, sev.write_op);
    }

    if (InterlockedCompareExchange(&g_ask_feedback_pending, 0, 1)) {
        char answer[256] = {0};
        bool ok = ask_mode_confirm_action(g_ask_feedback_reason, g_ask_feedback_suggestion, g_ask_feedback_options, answer, sizeof(answer));
        const char* normalized = answer;
        if (!normalized[0]) normalized = ok ? "allow" : "reject";
        else if (_stricmp(normalized, "approve_plan") == 0 || _stricmp(normalized, "plan_confirm") == 0) normalized = "plan_confirm";
        else if (_stricmp(normalized, "adjust_plan") == 0 || _stricmp(normalized, "plan_modify") == 0) normalized = "plan_modify";
        else if (_stricmp(normalized, "cancel") == 0 || _stricmp(normalized, "deny") == 0 || _stricmp(normalized, "reject") == 0) normalized = "reject";
        char escaped[320] = {0};
        int ei = 0;
        for (int i = 0; normalized[i] && ei < (int)sizeof(escaped) - 2; i++) {
            if (normalized[i] == '"' || normalized[i] == '\\') escaped[ei++] = '\\';
            escaped[ei++] = normalized[i];
        }
        escaped[ei] = '\0';
        snprintf(g_pending_feedback_payload, sizeof(g_pending_feedback_payload),
                 "{\"type\":\"feedback\",\"answer\":\"%s\"}", escaped[0] ? escaped : (ok ? "allow" : "reject"));
    }

    if (InterlockedCompareExchange(&g_stream_done, 0, 1)) {
        g_streaming = false;
        lv_timer_del(g_stream_timer);
        g_stream_timer = nullptr;
        update_send_button_state();
        DWORD total_ms = GetTickCount() - g_stream_start_tick;
        char stream_snapshot[16384] = {0};
        stream_buf_copy(stream_snapshot, sizeof(stream_snapshot));
        LOG_I("Chat", "Stream finished: total=%lums buf_len=%zu", total_ms, strlen(stream_snapshot));
        render_markdown_to_label(g_stream_label, stream_snapshot, CJK_FONT_CHAT);

        size_t hlen = strlen(chat_history);
        size_t slen = strlen(stream_snapshot);
        if (hlen + slen < sizeof(chat_history) - 1) {
            strcat(chat_history, stream_snapshot);
            strcat(chat_history, "\n");
        }

        if (g_work_waiting_ai) {
            work_append_md_block("AI", stream_snapshot);
            work_add_step_card("模型输出完成", "结果已写入 Output Area", true, false);
            g_work_waiting_ai = false;
            g_work_last_prompt[0] = '\0';
            c2_refresh_work_chat_state_label();
        }
        execute_ai_ui_action_if_any(stream_snapshot);
        if (mode_ta_work_chat_feed && stream_snapshot[0]) {
            lv_textarea_add_text(mode_ta_work_chat_feed, "\n[AI] ");
            lv_textarea_add_text(mode_ta_work_chat_feed, stream_snapshot);
        }

        if (g_stream_thread) {
            DWORD wait = WaitForSingleObject(g_stream_thread, 0);
            if (wait == WAIT_OBJECT_0) {
                CloseHandle(g_stream_thread);
                g_stream_thread = nullptr;
            }
        }

        chat_force_scroll_bottom();
        set_ai_next_step("Completed, waiting next input");
        append_ai_script_log("[done] stream completed");
        update_chat_status_label("Ready", false);
        if (g_pending_feedback_payload[0]) {
            std::string payload = g_pending_feedback_payload;
            g_pending_feedback_payload[0] = '\0';
            submit_prompt_to_chat(payload.c_str());
        }
    }
}

static void chat_start_stream(const char* user_message) {
    const ThemeColors* c = g_colors ? g_colors : &THEME_DARK;
    if (g_streaming) {
        /* FIX: If g_stream_done is set but timer hasn't consumed it yet, force cleanup */
        if (g_stream_done) {
            LOG_W("Chat", "Stale streaming state detected, forcing cleanup");
            g_streaming = false;
            if (g_stream_timer) {
                lv_timer_del(g_stream_timer);
                g_stream_timer = nullptr;
            }
            if (g_stream_thread) {
                /* Thread should have exited since g_stream_done=1 */
                DWORD wait = WaitForSingleObject(g_stream_thread, 0);
                if (wait == WAIT_TIMEOUT) {
                    LOG_W("Chat", "Stream thread still alive after done flag, keep handle for async cleanup");
                } else {
                    CloseHandle(g_stream_thread);
                    g_stream_thread = nullptr;
                }
            }
            /* Fall through to start new stream */
        } else {
            LOG_W("Chat", "Already streaming, ignoring new request");
            return;
        }
    }
    if (!chat_cont) {
        LOG_W("Chat", "chat_cont is null, cannot start stream");
        if (g_work_waiting_ai) {
            g_work_waiting_ai = false;
            c2_refresh_work_chat_state_label();
        }
        return;
    }
    LOG_I("Chat", "Starting stream: msg=%.60s%s", user_message,
          strlen(user_message) > 60 ? "..." : "");
    update_chat_status_label("Streaming...", true);

    /* Check if Gateway is reachable */
    {
        std::string gw_token = read_gateway_token();
        if (gw_token.empty()) {
            static const I18n S_NO_TOKEN = {
                "⚠️ 无法读取 Gateway 配置。请确认 OpenClaw 已安装并配置。",
                "⚠️ Cannot read Gateway config. Please ensure OpenClaw is installed."
            };
            chat_add_user_bubble(tr(S_NO_TOKEN));
            ui_log("[Chat] Cannot read Gateway token");
            update_chat_status_label("Gateway config missing", false);
            if (g_work_waiting_ai) {
                work_add_step_card("模型调用失败", "Gateway 配置缺失，任务未发出", true, false);
                g_work_waiting_ai = false;
                c2_refresh_work_chat_state_label();
            }
            return;
        }
    }
    if (g_work_waiting_ai) {
        work_add_step_card("调用模型", "向 Gateway 发送流式请求", false, false);
    }

    /* Create streaming bubble UI (same layout as before) */
    g_stream_bubble = lv_obj_create(chat_cont);
    lv_obj_set_width(g_stream_bubble, LV_PCT(100));
    lv_obj_set_height(g_stream_bubble, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(g_stream_bubble, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(g_stream_bubble, 0, 0);
    lv_obj_set_style_pad_all(g_stream_bubble, 0, 0);
    lv_obj_set_style_radius(g_stream_bubble, 0, 0);
    lv_obj_set_style_margin_hor(g_stream_bubble, CHAT_MSG_MARGIN, 0);
    lv_obj_clear_flag(g_stream_bubble, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_flex_flow(g_stream_bubble, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(g_stream_bubble, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);

    lv_obj_t* inner = lv_obj_create(g_stream_bubble);
    lv_obj_set_width(inner, LV_PCT(70));
    lv_obj_set_height(inner, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(inner, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(inner, 0, 0);
    lv_obj_set_style_pad_all(inner, 0, 0);
    lv_obj_set_style_radius(inner, 0, 0);
    lv_obj_clear_flag(inner, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_flex_flow(inner, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(inner, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_set_style_pad_gap(inner, 4, 0);

    /* Meta row: [🦞 avatar] [AI] [timestamp] */
    lv_obj_t* meta = lv_obj_create(inner);
    lv_obj_set_size(meta, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(meta, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(meta, 0, 0);
    lv_obj_set_style_pad_all(meta, 0, 0);
    lv_obj_clear_flag(meta, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_flex_flow(meta, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(meta, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_gap(meta, 6, 0);

    lv_obj_t* avatar = lv_image_create(meta);
    lv_image_set_src(avatar, profile_ai_avatar_src());
    lv_obj_set_size(avatar, SCALE(AVATAR_AI_SIZE), SCALE(AVATAR_AI_SIZE));
    lv_image_set_scale(avatar, 256);
    lv_obj_clear_flag(avatar, LV_OBJ_FLAG_SCROLLABLE);

    SYSTEMTIME st;
    GetLocalTime(&st);
    char meta_ts[64];
    snprintf(meta_ts, sizeof(meta_ts), "[%s] [%04d-%02d-%02d %02d:%02d:%02d]",
             profile_ai_name(),
             st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);

    lv_obj_t* ts_lbl = lv_label_create(meta);
    lv_label_set_text(ts_lbl, meta_ts);
    lv_obj_set_style_text_color(ts_lbl, c->text_dim, 0);
    lv_obj_set_style_text_font(ts_lbl, FONT(10), 0);

    /* Streaming text label */
    stream_buf_clear();
    g_stream_label = lv_label_create(inner);
    lv_obj_set_style_text_color(g_stream_label, c->text, 0);
    lv_obj_set_style_text_font(g_stream_label, CJK_FONT_CHAT, 0);
    lv_label_set_long_mode(g_stream_label, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(g_stream_label, LV_PCT(100));
    lv_obj_set_style_pad_hor(g_stream_label, 10, 0);
    lv_obj_set_style_pad_ver(g_stream_label, 6, 0);
    make_label_selectable(g_stream_label);  /* Enable mouse text selection + Ctrl+C */

    /* Show "thinking" indicator */
    lv_label_set_text(g_stream_label, "...");
    chat_force_scroll_bottom();

    g_streaming = true;
    update_send_button_state();
    g_stream_start_tick = GetTickCount();
    g_stream_last_data_tick = GetTickCount();
    InterlockedExchange(&g_stream_new_data, 0);
    InterlockedExchange(&g_stream_done, 0);

    /* Start main-thread timer to poll for new data */
    g_stream_timer = lv_timer_create(stream_timer_cb, 80, nullptr);

    /* Start background thread for API call */
    char* msg_copy = _strdup(user_message);
    g_stream_thread = (HANDLE)_beginthreadex(nullptr, 0, chat_api_thread, msg_copy, 0, nullptr);
    if (!g_stream_thread) {
        if (msg_copy) free(msg_copy);
        g_streaming = false;
        if (g_stream_timer) {
            lv_timer_del(g_stream_timer);
            g_stream_timer = nullptr;
        }
        update_send_button_state();
        update_chat_status_label("Stream start failed", false);
        if (g_work_waiting_ai) {
            work_add_step_card("模型调用失败", "流式线程启动失败", true, false);
            g_work_waiting_ai = false;
            c2_refresh_work_chat_state_label();
        }
        return;
    }

    set_ai_next_step("Calling Gateway /v1/chat/completions");
    append_ai_script_log("[request] POST /v1/chat/completions stream=true");
}

static void chat_add_ai_bubble(const char* text) {
    if (!chat_cont) return;
    const ThemeColors* c = g_colors ? g_colors : &THEME_DARK;

    /*
     * AI 消息 (QQ风格: 气泡靠左, 贴合文字宽度):
     *
     * row_wrap (ROW, width=100%, FLEX_ALIGN_START → 内容靠左)
     *   └── inner (COLUMN, SIZE_CONTENT)
     *         ├── meta (ROW: [🦞] [AI] [timestamp])
     *         └── lbl  (消息文字, 贴合文字, max_width限制)
     */
    lv_obj_t* row_wrap = lv_obj_create(chat_cont);
    lv_obj_set_width(row_wrap, LV_PCT(100));
    lv_obj_set_height(row_wrap, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(row_wrap, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(row_wrap, 0, 0);
    lv_obj_set_style_pad_all(row_wrap, 0, 0);
    lv_obj_set_style_radius(row_wrap, 0, 0);
    lv_obj_set_style_margin_hor(row_wrap, CHAT_MSG_MARGIN, 0);
    lv_obj_clear_flag(row_wrap, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_flex_flow(row_wrap, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(row_wrap, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);

    /* Inner column: meta + text */
    lv_obj_t* inner = lv_obj_create(row_wrap);
    lv_obj_set_width(inner, LV_PCT(70));
    lv_obj_set_height(inner, LV_SIZE_CONTENT);

    lv_obj_set_style_bg_opa(inner, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(inner, 0, 0);
    lv_obj_set_style_pad_all(inner, 0, 0);
    lv_obj_set_style_radius(inner, 0, 0);
    lv_obj_clear_flag(inner, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_flex_flow(inner, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(inner, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_set_style_pad_gap(inner, 4, 0);

    /* Meta row: [🦞 avatar] [AI] [timestamp] */
    lv_obj_t* meta = lv_obj_create(inner);
    lv_obj_set_size(meta, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(meta, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(meta, 0, 0);
    lv_obj_set_style_pad_all(meta, 0, 0);
    lv_obj_set_style_radius(meta, 0, 0);
    lv_obj_clear_flag(meta, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_flex_flow(meta, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(meta, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_gap(meta, 6, 0);

    lv_obj_t* avatar = lv_image_create(meta);
    lv_image_set_src(avatar, profile_ai_avatar_src());
    lv_obj_set_size(avatar, SCALE(AVATAR_AI_SIZE), SCALE(AVATAR_AI_SIZE));
    lv_image_set_scale(avatar, 256);
    lv_obj_clear_flag(avatar, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* ai_lbl = lv_label_create(meta);
    lv_label_set_text(ai_lbl, profile_ai_name());
    lv_obj_set_style_text_color(ai_lbl, c->text_hint, 0);
    lv_obj_set_style_text_font(ai_lbl, FONT(10), 0);

    /* Text label (短消息贴合文字, 长消息70%换行) */
    lv_obj_t* lbl = lv_label_create(inner);
    char entry[2048];
    snprintf(entry, sizeof(entry), "%s", text);

    render_markdown_to_label(lbl, entry, CJK_FONT_CHAT);
    lv_obj_set_style_text_color(lbl, c->text, 0);
    lv_label_set_long_mode(lbl, LV_LABEL_LONG_WRAP);

    /* Measure rendered text: short → small bubble, long → 70% wrap */
    const char* rendered = lv_label_get_text(lbl);
    lv_point_t ai_txt_sz;
    lv_text_get_size(&ai_txt_sz, rendered, CJK_FONT_CHAT, 0, 0, LV_COORD_MAX, LV_TEXT_FLAG_NONE);
    lv_obj_update_layout(chat_cont);
    int32_t ai_cont_w = lv_obj_get_content_width(chat_cont);
    int32_t ai_max_bubble = ai_cont_w * 70 / 100;
    int32_t ai_text_w = ai_txt_sz.x + 20;

    if (ai_text_w < ai_max_bubble) {
        lv_obj_set_width(lbl, LV_SIZE_CONTENT);
        lv_obj_update_layout(meta);
        int32_t ai_meta_w = lv_obj_get_width(meta);
        int32_t ai_inner_w = (ai_text_w > ai_meta_w) ? ai_text_w : ai_meta_w;
        if (ai_inner_w > ai_max_bubble) ai_inner_w = ai_max_bubble;
        lv_obj_set_width(inner, ai_inner_w);
    } else {
        lv_obj_set_width(lbl, LV_PCT(100));
        lv_obj_set_width(inner, ai_max_bubble);
    }

    lv_obj_set_style_pad_hor(lbl, 10, 0);
    lv_obj_set_style_pad_ver(lbl, 6, 0);
    lv_obj_set_style_bg_color(lbl, c->panel, 0);
    lv_obj_set_style_bg_opa(lbl, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(lbl, SCALE(g_colors->radius_xl), 0);
    lv_obj_set_style_border_width(lbl, 0, 0);
    make_label_selectable(lbl);  /* Enable mouse text selection + Ctrl+C */

    char history_entry[2048];
    char ts[32];
    SYSTEMTIME st;
    GetLocalTime(&st);
    snprintf(ts, sizeof(ts), "[%04d-%02d-%02d %02d:%02d:%02d]",
             st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);
    snprintf(history_entry, sizeof(history_entry), "[AI] %s %s", ts, text);
    size_t hlen = strlen(chat_history);
    size_t elen2 = strlen(history_entry);
    if (hlen + elen2 < sizeof(chat_history) - 1) {
        strcat(chat_history, history_entry);
        strcat(chat_history, "\n");
    }

    /* Persist to disk */
    char ts_short[32];
    snprintf(ts_short, sizeof(ts_short), "%04d-%02d-%02d %02d:%02d:%02d",
             st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);
    persist_chat_message("ai", text, ts_short);

    chat_force_scroll_bottom();
    update_chat_empty_state();
}

/* DEV: test message injection callback (function pointer, not lambda) */
static void test_inject_messages_cb(lv_timer_t* t) {
    /* Removed: fake demo messages. Real AI chat now via OpenRouter API. */
    lv_timer_del(t);
}

static void update_send_button_state();
static void work_boot_export_report(const std::vector<BootCheckResult>& results);
static void work_trace_refresh_cb(lv_event_t* e);
static void work_trace_toggle_sort_cb(lv_event_t* e);
static void work_trace_toggle_fail_filter_cb(lv_event_t* e);
static void work_retry_failed_attachments_cb(lv_event_t* e);
static void work_boot_open_report_cb(lv_event_t* e);
static void work_lan_start_host_cb(lv_event_t* e);
static void work_lan_connect_cb(lv_event_t* e);
static void work_lan_send_global_cb(lv_event_t* e);
static void work_lan_send_private_cb(lv_event_t* e);
static void work_lan_create_group_cb(lv_event_t* e);
static void work_lan_join_group_cb(lv_event_t* e);
static void work_lan_send_group_cb(lv_event_t* e);
static void work_lan_discover_cb(lv_event_t* e);
static void work_ftp_upload_cb(lv_event_t* e);
static void work_ftp_download_cb(lv_event_t* e);
static void work_ftp_cancel_cb(lv_event_t* e);
static void work_ftp_retry_cb(lv_event_t* e);
static void work_kb_add_source_cb(lv_event_t* e);
static void work_kb_import_dir_cb(lv_event_t* e);
static void work_kb_export_manifest_cb(lv_event_t* e);
static void work_kb_search_cb(lv_event_t* e);
static void work_cron_refresh_cb(lv_event_t* e);
static void work_cron_run_cb(lv_event_t* e);
static void work_cron_enable_cb(lv_event_t* e);
static void work_cron_disable_cb(lv_event_t* e);
static void work_cron_delete_cb(lv_event_t* e);
static void enqueue_attachment_card(const char* path, bool is_dir);

/* Reset input box to single-line height after sending */
static void chat_input_reset_height() {
    if (!chat_input) return;
    const int min_h = SCALE(CHAT_INPUT_MIN_H_BASE);
    int content_h = mode_content_h();
    int content_w = mode_content_w();
    int cur_h = (int)lv_obj_get_height(chat_input);
    if (cur_h > min_h) {
        lv_obj_set_height(chat_input, min_h);
        lv_obj_clear_flag(chat_input, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_scrollbar_mode(chat_input, LV_SCROLLBAR_MODE_OFF);
        int new_input_y = content_h - min_h - GAP;
        lv_obj_set_pos(chat_input, CHAT_GAP, new_input_y);
        layout_chat_action_buttons(content_w, new_input_y, min_h);
        int chat_y = chat_top_y_with_trace(content_w);
        int new_chat_h = content_h - chat_y - min_h - GAP;
        if (new_chat_h > 0) lv_obj_set_height(chat_cont, new_chat_h);
    }
}

static std::string build_plan_mode_prompt(const char* raw) {
    std::string user_task = raw ? raw : "";
    return
        "PLAN mode protocol (strict):\n"
        "1) First output one compact JSON block:\n"
        "{\"type\":\"plan\",\"goal\":\"...\",\"steps\":[\"...\"],\"risk\":\"...\",\"est_time\":\"...\"}\n"
        "2) Then output one compact JSON confirmation block:\n"
        "{\"type\":\"ask_feedback\",\"reason\":\"Confirm plan before execution\",\"suggestion\":\"Choose one option\",\"options\":[\"plan_confirm\",\"plan_modify\",\"cancel\"]}\n"
        "   (compat aliases accepted: approve_plan -> plan_confirm, adjust_plan -> plan_modify)\n"
        "3) Do not execute tools before approval.\n"
        "User task:\n" + user_task;
}

static void chat_send_cb(lv_event_t* e) {
    (void)e;
    if (!chat_input) return;
    if (is_streaming_now()) {
        ui_log("[Chat] AI is still replying, wait before sending next message");
        update_chat_status_label("Busy: AI replying", true);
        return;
    }
    const char* text = lv_textarea_get_text(chat_input);
    if (!text || !text[0]) return;
    if (strcmp(text, "ask_feedback") == 0 || strcmp(text, "/ask_feedback") == 0) {
        char answer[128] = {0};
        bool ok = ask_mode_confirm_action("Manual ask_feedback test", "Choose one option to continue", "allow_once,deny,adjust_plan", answer, sizeof(answer));
        ui_log("[AskMode] manual ask_feedback test: %s answer=%s", ok ? "allow" : "deny", answer[0] ? answer : "-");
        lv_textarea_set_text(chat_input, "");
        chat_input_reset_height();
        update_send_button_state();
        return;
    }
    if (g_c2_dual_result) c2_mirror_prompt_to_work(text);
    std::string prompt = text;
    if (g_ai_interaction_mode == AIMODE_PLAN) {
        prompt = build_plan_mode_prompt(text);
    }
    submit_prompt_to_chat(prompt.c_str());
    set_ai_next_step("Preparing request payload");
    append_ai_script_log("[input] user prompt submitted");

    /* Clear input */
    lv_textarea_set_text(chat_input, "");
    chat_input_reset_height();
    update_send_button_state();

    ui_log("[Chat] Message sent, streaming response...");
}

/* ── Send button callback (CI-01-2) ── */
static void chat_send_btn_cb(lv_event_t* e) {
    (void)e;
    if (!chat_input) return;
    if (is_streaming_now()) {
        ui_log("[Chat] AI is still replying, send button is temporarily disabled");
        update_chat_status_label("Busy: AI replying", true);
        return;
    }
    const char* text = lv_textarea_get_text(chat_input);
    if (!text || !text[0]) return;
    if (strcmp(text, "ask_feedback") == 0 || strcmp(text, "/ask_feedback") == 0) {
        char answer[128] = {0};
        bool ok = ask_mode_confirm_action("Manual ask_feedback test", "Choose one option to continue", "allow_once,deny,adjust_plan", answer, sizeof(answer));
        ui_log("[AskMode] manual ask_feedback test(button): %s answer=%s", ok ? "allow" : "deny", answer[0] ? answer : "-");
        lv_textarea_set_text(chat_input, "");
        chat_input_reset_height();
        update_send_button_state();
        return;
    }
    if (g_c2_dual_result) c2_mirror_prompt_to_work(text);
    std::string prompt = text;
    if (g_ai_interaction_mode == AIMODE_PLAN) {
        prompt = build_plan_mode_prompt(text);
    }
    submit_prompt_to_chat(prompt.c_str());
    set_ai_next_step("Preparing request payload");
    append_ai_script_log("[input] user prompt submitted (button)");

    lv_textarea_set_text(chat_input, "");
    chat_input_reset_height();
    update_send_button_state();

    ui_log("[Chat] Message sent via button, streaming response...");
}

static void work_send_cb(lv_event_t* e) {
    (void)e;
    if (!mode_ta_work_prompt) return;
    if (is_streaming_now()) {
        ui_log("[Work] Wait for current AI reply to finish");
        return;
    }
    const char* text = lv_textarea_get_text(mode_ta_work_prompt);
    if (!text || !text[0]) return;
    snprintf(g_work_last_prompt, sizeof(g_work_last_prompt), "%s", text);
    g_work_waiting_ai = true;
    work_append_md_block("User", text);
    work_add_step_card("接收任务", text, false, false);
    if (mode_ta_work_chat_feed) {
        lv_textarea_add_text(mode_ta_work_chat_feed, "\n[Work] ");
        lv_textarea_add_text(mode_ta_work_chat_feed, text);
    }
    c2_refresh_work_chat_state_label();
    set_ai_next_step("Work mode: executing task");
    append_ai_script_log("[work] prompt submitted");
    if (g_ai_interaction_mode == AIMODE_PLAN) {
        std::string plan_prompt = build_plan_mode_prompt(text);
        submit_prompt_to_chat(plan_prompt.c_str());
    } else {
        submit_prompt_to_chat(text);
    }
    lv_textarea_set_text(mode_ta_work_prompt, "");
}

static void chat_send_to_work_cb(lv_event_t* e) {
    (void)e;
    if (!chat_input) return;
    if (is_streaming_now()) {
        ui_log("[C2] Wait for current AI reply to finish");
        return;
    }
    const char* text = lv_textarea_get_text(chat_input);
    if (!text || !text[0]) return;

    c2_mirror_prompt_to_work(text);
    std::string prompt = text;
    if (g_ai_interaction_mode == AIMODE_PLAN) prompt = build_plan_mode_prompt(text);
    submit_prompt_to_chat(prompt.c_str());
    set_ai_next_step("Work mode: executing task");
    append_ai_script_log("[c2] prompt submitted from chat to work");

    lv_textarea_set_text(chat_input, "");
    chat_input_reset_height();
    update_send_button_state();

    g_ui_mode = UI_MODE_WORK;
    apply_mode_switch_visuals();
    relayout_panels();
    ui_log("[C2] Prompt routed to Work view");
}

static void work_chat_toggle_cb(lv_event_t* e) {
    (void)e;
    if (!mode_work_chat_panel || !mode_ta_work_chat_feed || !mode_ta_work_chat_input) return;
    if (g_c2_dual_result) {
        c2_commit_work_panel_policy(false);
        ui_log("[C2] Work chat collapse is locked while Dual is on");
        return;
    }
    g_work_chat_collapsed = !g_work_chat_collapsed;
    g_work_chat_collapsed_pref = g_work_chat_collapsed;
    c2_commit_work_panel_policy(true);
}

static void work_vertical_splitter_cb(lv_event_t* e) {
    static int start_y = 0;
    static int start_step_h = 160;
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_PRESSED) {
        lv_point_t p{};
        lv_indev_t* indev = lv_indev_get_act();
        if (!indev) return;
        lv_indev_get_point(indev, &p);
        start_y = p.y;
        start_step_h = g_work_step_stream_h;
        /* Highlight splitter on press */
        if (mode_work_splitter) lv_obj_set_style_bg_color(mode_work_splitter, g_colors->accent_secondary, 0);
        return;
    }
    if (code == LV_EVENT_RELEASED || code == LV_EVENT_LEAVE) {
        /* Reset splitter color on release */
        const ThemeColors* c = g_colors ? g_colors : &THEME_DARK;
        if (mode_work_splitter) lv_obj_set_style_bg_color(mode_work_splitter, c->surface, 0);
        return;
    }
    if (code != LV_EVENT_PRESSING) return;
    lv_indev_t* indev = lv_indev_get_act();
    if (!indev) return;
    lv_point_t p{};
    lv_indev_get_point(indev, &p);
    int delta = p.y - start_y;
    int content_h = mode_content_h();
    int min_h = SCALE(90);
    int max_h = std::max(min_h, content_h - SCALE(420));
    int step_h = std::clamp(start_step_h + delta, min_h, max_h);
    int out_h = std::max(min_h, g_work_step_stream_h + g_work_output_h - step_h);
    g_work_step_stream_h = step_h;
    g_work_output_h = out_h;
    if (mode_work_step_stream) lv_obj_set_height(mode_work_step_stream, SCALE(g_work_step_stream_h));
    if (mode_work_output_wrap) lv_obj_set_height(mode_work_output_wrap, SCALE(g_work_output_h));
}

static void work_chat_send_cb(lv_event_t* e) {
    (void)e;
    if (!mode_ta_work_chat_input || !mode_ta_work_chat_feed) return;
    const char* text = lv_textarea_get_text(mode_ta_work_chat_input);
    if (!text || !text[0]) return;
    lv_textarea_add_text(mode_ta_work_chat_feed, "\n[User] ");
    lv_textarea_add_text(mode_ta_work_chat_feed, text);
    submit_prompt_to_chat(text);
    lv_textarea_set_text(mode_ta_work_chat_input, "");
}

static std::string build_boot_check_detail_text(const std::vector<BootCheckResult>& results) {
    std::string text;
    text.reserve(2048);
    for (const auto& r : results) {
        const char* s = (r.status == BootCheckStatus::Ok) ? "✓" :
                        (r.status == BootCheckStatus::Warn) ? "⚠" : "✗";
        text += s;
        text += " ";
        text += r.check_name;
        text += "  ";
        text += r.message;
        if (r.fix_applied) text += "  [auto-fixed]";
        text += "\n";
    }
    if (text.empty()) text = "No boot check results.";
    return text;
}

static std::string build_boot_check_live_text(const std::vector<BootCheckResult>& done, int running_idx) {
    const char* kNames[9] = {
        "Node.js", "npm", "OpenClaw", "Gateway Port", "Config Directory",
        "Workspace", "Disk Space", "Network", "SDL2.dll"
    };
    std::string text;
    text.reserve(2200);
    for (int i = 0; i < 9; ++i) {
        text += "• ";
        text += kNames[i];
        text += "  ";
        if (i < (int)done.size()) {
            const auto& r = done[(size_t)i];
            if (r.status == BootCheckStatus::Ok) text += "✓ ";
            else if (r.status == BootCheckStatus::Warn) text += "⚠ ";
            else text += "✗ ";
            text += r.message;
            if (r.fix_applied) text += " [auto-fixed]";
        } else if (i == running_idx) {
            text += "Checking...";
        } else {
            text += "Waiting...";
        }
        text += "\n";
    }
    return text;
}

static BootCheckResult run_boot_check_by_index(int idx) {
    switch (idx) {
        case 0: return check_nodejs_version();
        case 1: return check_npm();
        case 2: return check_openclaw();
        case 3: return check_gateway_port();
        case 4: return check_config_dir();
        case 5: return check_workspace();
        case 6: return check_disk_space();
        case 7: return check_network();
        case 8: return check_sdl2dll();
        default: {
            BootCheckResult r;
            r.check_name = "Unknown";
            r.status = BootCheckStatus::Error;
            r.message = "Invalid check index";
            return r;
        }
    }
}

static void work_boot_check_cb(lv_event_t* e) {
    (void)e;
    if (g_boot_check_running.exchange(true)) {
        ui_log("[BootCheck] Health check is already running");
        return;
    }
    if (mode_boot_progress_panel) lv_obj_clear_flag(mode_boot_progress_panel, LV_OBJ_FLAG_HIDDEN);
    if (mode_boot_progress_task) lv_label_set_text(mode_boot_progress_task, "Task: Boot Check");
    if (mode_boot_progress_step) lv_label_set_text(mode_boot_progress_step, "Step: Starting...");
    if (mode_boot_progress_result) lv_label_set_text(mode_boot_progress_result, "");
    if (mode_boot_progress_bar) lv_bar_set_value(mode_boot_progress_bar, 5, LV_ANIM_OFF);
    if (mode_boot_progress_list) lv_textarea_set_text(mode_boot_progress_list, "Preparing checks...\n");
    std::thread([]() {
        TraceSpan span("boot_check_run_and_fix", "work_mode");
        ui_progress_begin("Boot Check", "Running environment checks", 5);
        BootCheckManager mgr;
        std::vector<BootCheckResult> results;
        results.reserve(9);
        const char* check_names[9] = {
            "Node.js", "npm", "OpenClaw", "Gateway Port", "Config Directory",
            "Workspace", "Disk Space", "Network", "SDL2.dll"
        };
        for (int i = 0; i < 9; ++i) {
            char step[96] = {0};
            snprintf(step, sizeof(step), "Checking %s (%d/9)", check_names[i], i + 1);
            int pct = 5 + (i * 80) / 9;
            ui_progress_update("Boot Check", step, pct);
            {
                std::lock_guard<std::mutex> lk(g_boot_detail_mtx);
                g_boot_detail_text = build_boot_check_live_text(results, i);
            }
            g_boot_detail_dirty.store(true);

            BootCheckResult r = run_boot_check_by_index(i);
            if (r.status != BootCheckStatus::Ok && r.auto_fix_available) {
                mgr.auto_fix(r);
            }
            results.push_back(r);

            {
                std::lock_guard<std::mutex> lk(g_boot_detail_mtx);
                g_boot_detail_text = build_boot_check_live_text(results, -1);
            }
            g_boot_detail_dirty.store(true);
        }
        work_boot_export_report(results);
        int ok = 0;
        int warn = 0;
        int err = 0;
        for (const auto& r : results) {
            if (r.status == BootCheckStatus::Ok) ok++;
            else if (r.status == BootCheckStatus::Warn) warn++;
            else err++;
        }
        char summary[160] = {0};
        snprintf(summary, sizeof(summary), "BootCheck: %d ok / %d warn / %d error", ok, warn, err);
        {
            std::lock_guard<std::mutex> lk(g_boot_detail_mtx);
            g_boot_detail_text = build_boot_check_detail_text(results);
        }
        g_boot_detail_dirty.store(true);
        ui_progress_update("Boot Check", summary, 85);
        ui_progress_finish("Boot Check", err == 0, summary);
        if (err > 0) span.set_outcome("warn");
        g_boot_check_running.store(false);
    }).detach();
}

static void work_boot_export_report(const std::vector<BootCheckResult>& results) {
    char appdata[MAX_PATH] = {0};
    if (SHGetFolderPathA(nullptr, CSIDL_APPDATA, nullptr, 0, appdata) != S_OK) return;
    std::string dir = std::string(appdata) + "\\AnyClaw_LVGL";
    std::error_code ec;
    std::filesystem::create_directories(dir, ec);
    std::string report = dir + "\\bootcheck-last.md";
    std::ofstream out(report, std::ios::trunc);
    if (!out.is_open()) return;
    SYSTEMTIME st{};
    GetLocalTime(&st);
    char time_buf[64] = {0};
    snprintf(time_buf, sizeof(time_buf), "%04d-%02d-%02d %02d:%02d:%02d",
             st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);
    out << "# BootCheck Report\n\n";
    out << "- Generated: " << time_buf << "\n";
    out << "- Summary: " << BootCheckManager::summarize(results) << "\n\n";
    out << "| Check | Status | Message | AutoFix |\n";
    out << "|---|---|---|---|\n";
    for (const auto& r : results) {
        const char* s = (r.status == BootCheckStatus::Ok) ? "OK" :
                        (r.status == BootCheckStatus::Warn) ? "WARN" : "ERROR";
        out << "| " << r.check_name << " | " << s << " | " << r.message << " | "
            << (r.auto_fix_available ? "yes" : "no") << " |\n";
    }
    out.close();
    ui_log("[BootCheck] Report exported: %s", report.c_str());
}

static void work_trace_refresh_cb(lv_event_t* e) {
    (void)e;
    if (!mode_ta_trace_recent) return;
    auto evs = TraceManager::instance().get_recent_events(24);
    if (g_trace_fail_only) {
        evs.erase(std::remove_if(evs.begin(), evs.end(), [](const TraceEvent& ev) {
            return ev.outcome == "ok";
        }), evs.end());
    }
    if (g_trace_sort_latency) {
        std::sort(evs.begin(), evs.end(), [](const TraceEvent& a, const TraceEvent& b) {
            return a.latency_ms > b.latency_ms;
        });
    }
    std::string text;
    text.reserve(2048);
    if (evs.empty()) {
        text = "No trace events yet.";
    } else {
        for (const auto& ev : evs) {
            text += ev.ts + " | " + ev.action + " | " + ev.outcome + " | "
                + std::to_string(ev.latency_ms) + "ms\n";
        }
    }
    lv_textarea_set_text(mode_ta_trace_recent, text.c_str());
}

static void work_trace_toggle_sort_cb(lv_event_t* e) {
    (void)e;
    g_trace_sort_latency = !g_trace_sort_latency;
    ui_log("[OBS] Trace sort mode: %s", g_trace_sort_latency ? "latency-desc" : "time-desc");
    work_trace_refresh_cb(nullptr);
}

static void work_trace_toggle_fail_filter_cb(lv_event_t* e) {
    (void)e;
    g_trace_fail_only = !g_trace_fail_only;
    ui_log("[OBS] Trace filter: %s", g_trace_fail_only ? "fail-only" : "all");
    work_trace_refresh_cb(nullptr);
}

static void work_retry_failed_attachments_cb(lv_event_t* e) {
    (void)e;
    if (g_attachment_failed_cache.empty()) {
        ui_log("[Share] No failed attachments to retry");
        return;
    }
    int queued = 0;
    std::vector<AttachmentQueueItem> failed = g_attachment_failed_cache;
    g_attachment_failed_cache.clear();
    for (const auto& it : failed) {
        if (!it.path.empty()) {
            enqueue_attachment_card(it.path.c_str(), it.is_dir);
            queued++;
        }
    }
    ui_log("[Share] Retry queued for %d failed attachments", queued);
}

static void work_boot_open_report_cb(lv_event_t* e) {
    (void)e;
    char appdata[MAX_PATH] = {0};
    if (SHGetFolderPathA(nullptr, CSIDL_APPDATA, nullptr, 0, appdata) != S_OK) return;
    std::string report = std::string(appdata) + "\\AnyClaw_LVGL\\bootcheck-last.md";
    if (!std::filesystem::exists(report)) {
        ui_log("[BootCheck] Report not found: run health check first");
        return;
    }
    if (mode_lbl_boot_report_hint) {
        lv_label_set_text_fmt(mode_lbl_boot_report_hint, "Report: %s", report.c_str());
    }
    HINSTANCE r = ShellExecuteA(nullptr, "open", report.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
    if ((INT_PTR)r <= 32) {
        ui_log("[BootCheck] Failed to open report: %s", report.c_str());
    }
}

static std::string ta_text(lv_obj_t* ta, const char* fallback = "") {
    if (!ta) return fallback ? fallback : "";
    const char* t = lv_textarea_get_text(ta);
    if (!t) return fallback ? fallback : "";
    return t;
}

static void lan_emit_ui(const LanChatEvent& ev) {
    if (ev.type == LanChatEventType::UserList && mode_ta_lan_users) {
        lv_textarea_set_text(mode_ta_lan_users, ev.text.c_str());
    }
    if (ev.type == LanChatEventType::GroupList && mode_ta_lan_groups) {
        lv_textarea_set_text(mode_ta_lan_groups, ev.text.c_str());
    }
    if (!mode_ta_lan_feed) return;
    char line[1024] = {0};
    const char* tag = "[LAN]";
    if (ev.type == LanChatEventType::PrivateMessage) tag = "[LAN-PM]";
    else if (ev.type == LanChatEventType::GroupMessage) tag = "[LAN-GRP]";
    else if (ev.type == LanChatEventType::System) tag = "[LAN-SYS]";
    snprintf(line, sizeof(line), "%s %s -> %s: %s\n", tag, ev.from.c_str(), ev.target.c_str(), ev.text.c_str());
    lv_textarea_add_text(mode_ta_lan_feed, line);
}

static void work_lan_start_host_cb(lv_event_t* e) {
    (void)e;
    std::string err;
    int port = atoi(ta_text(mode_ta_lan_port, "19999").c_str());
    if (port <= 0) port = 19999;
    g_lan_chat_client.set_event_callback(lan_emit_ui);
    if (g_lan_chat_client.start_host((unsigned short)port, err)) {
        ui_log("[LAN] Host started on 0.0.0.0:%d", port);
    } else {
        ui_log("[LAN] Host start failed: %s", err.c_str());
    }
}

static void work_lan_connect_cb(lv_event_t* e) {
    (void)e;
    std::string err;
    std::string host = ta_text(mode_ta_lan_host, "127.0.0.1");
    int port = atoi(ta_text(mode_ta_lan_port, "19999").c_str());
    std::string nick = ta_text(mode_ta_lan_nick, "user");
    if (port <= 0) port = 19999;
    g_lan_chat_client.set_event_callback(lan_emit_ui);
    if (g_lan_chat_client.connect_server(host, (unsigned short)port, nick, err)) {
        ui_log("[LAN] Connected to %s:%d as %s", host.c_str(), port, nick.c_str());
    } else {
        ui_log("[LAN] Connect failed: %s", err.c_str());
    }
}

static void work_lan_send_global_cb(lv_event_t* e) {
    (void)e;
    std::string msg = ta_text(mode_ta_lan_msg);
    if (msg.empty()) return;
    if (!g_lan_chat_client.send_global(msg)) ui_log("[LAN] Global send failed");
}

static void work_lan_send_private_cb(lv_event_t* e) {
    (void)e;
    std::string to = ta_text(mode_ta_lan_to);
    std::string msg = ta_text(mode_ta_lan_msg);
    if (to.empty() || msg.empty()) return;
    if (!g_lan_chat_client.send_private(to, msg)) ui_log("[LAN] Private send failed");
}

static void work_lan_create_group_cb(lv_event_t* e) {
    (void)e;
    std::string group = ta_text(mode_ta_lan_group);
    if (group.empty()) return;
    if (!g_lan_chat_client.create_group(group)) ui_log("[LAN] Create group failed");
}

static void work_lan_join_group_cb(lv_event_t* e) {
    (void)e;
    std::string group = ta_text(mode_ta_lan_group);
    if (group.empty()) return;
    if (!g_lan_chat_client.join_group(group)) ui_log("[LAN] Join group failed");
}

static void work_lan_send_group_cb(lv_event_t* e) {
    (void)e;
    std::string group = ta_text(mode_ta_lan_group);
    std::string msg = ta_text(mode_ta_lan_msg);
    if (group.empty() || msg.empty()) return;
    if (!g_lan_chat_client.send_group(group, msg)) ui_log("[LAN] Group send failed");
}

static void work_lan_discover_cb(lv_event_t* e) {
    (void)e;
    int port = atoi(ta_text(mode_ta_lan_port, "19999").c_str());
    if (port <= 0) port = 19999;
    auto hosts = g_lan_chat_client.discover_hosts((unsigned short)(port + 1), 1200);
    if (hosts.empty()) {
        ui_log("[LAN] No host discovered on UDP port %d", port + 1);
        return;
    }
    std::string merged;
    for (size_t i = 0; i < hosts.size(); ++i) {
        if (i) merged += ", ";
        merged += hosts[i];
    }
    ui_log("[LAN] Discovered hosts: %s", merged.c_str());
}

static void work_cron_refresh_cb(lv_event_t* e) {
    (void)e;
    if (!mode_ta_cron_list) return;
    char out[8192] = {0};
    if (app_cron_list(out, sizeof(out))) {
        lv_textarea_set_text(mode_ta_cron_list, out[0] ? out : "(empty)");
        ui_log("[Cron] Refreshed cron list");
    } else {
        lv_textarea_set_text(mode_ta_cron_list, out[0] ? out : "cron list failed");
        ui_log("[Cron] Refresh failed: %s", out);
    }
}

static void work_cron_run_cb(lv_event_t* e) {
    (void)e;
    std::string id = ta_text(mode_ta_cron_id);
    if (id.empty()) { ui_log("[Cron] run requires cron id"); return; }
    char out[2048] = {0};
    bool ok = app_cron_run(id.c_str(), out, sizeof(out));
    ui_log("[Cron] run %s: %s", id.c_str(), ok ? "ok" : "fail");
    if (out[0]) ui_log("[Cron] %s", out);
    work_cron_refresh_cb(nullptr);
}

static void work_cron_enable_cb(lv_event_t* e) {
    (void)e;
    std::string id = ta_text(mode_ta_cron_id);
    if (id.empty()) { ui_log("[Cron] enable requires cron id"); return; }
    char out[2048] = {0};
    bool ok = app_cron_enable(id.c_str(), true, out, sizeof(out));
    ui_log("[Cron] enable %s: %s", id.c_str(), ok ? "ok" : "fail");
    if (out[0]) ui_log("[Cron] %s", out);
    work_cron_refresh_cb(nullptr);
}

static void work_cron_disable_cb(lv_event_t* e) {
    (void)e;
    std::string id = ta_text(mode_ta_cron_id);
    if (id.empty()) { ui_log("[Cron] disable requires cron id"); return; }
    char out[2048] = {0};
    bool ok = app_cron_enable(id.c_str(), false, out, sizeof(out));
    ui_log("[Cron] disable %s: %s", id.c_str(), ok ? "ok" : "fail");
    if (out[0]) ui_log("[Cron] %s", out);
    work_cron_refresh_cb(nullptr);
}

static void work_cron_delete_cb(lv_event_t* e) {
    (void)e;
    std::string id = ta_text(mode_ta_cron_id);
    if (id.empty()) { ui_log("[Cron] delete requires cron id"); return; }
    if (!ask_mode_confirm_action("Delete cron task", "Confirm delete selected cron id")) return;
    char out[2048] = {0};
    bool ok = app_cron_delete(id.c_str(), out, sizeof(out));
    ui_log("[Cron] delete %s: %s", id.c_str(), ok ? "ok" : "fail");
    if (out[0]) ui_log("[Cron] %s", out);
    work_cron_refresh_cb(nullptr);
}

static void run_ftp_transfer_config(const FtpTransferConfig& cfg) {
    if (g_ftp_running.exchange(true)) {
        ui_log("[FTP] Transfer already running");
        return;
    }
    if (mode_btn_ftp_upload) lv_obj_add_state(mode_btn_ftp_upload, LV_STATE_DISABLED);
    if (mode_btn_ftp_download) lv_obj_add_state(mode_btn_ftp_download, LV_STATE_DISABLED);
    if (mode_btn_ftp_cancel) lv_obj_clear_state(mode_btn_ftp_cancel, LV_STATE_DISABLED);
    if (mode_btn_ftp_retry) lv_obj_add_state(mode_btn_ftp_retry, LV_STATE_DISABLED);
    g_ftp_cancel.store(false);
    {
        std::lock_guard<std::mutex> lk(g_ftp_last_mtx);
        g_ftp_last_cfg = cfg;
        g_ftp_has_last = true;
    }
    std::thread([cfg]() {
        ui_progress_begin("FTP Transfer", cfg.upload ? "Starting upload" : "Starting download", 3);
        std::string err;
        bool ok = ftp_transfer_file(cfg, g_ftp_cancel, err, [](int pct, const char* step) {
            ui_progress_update("FTP Transfer", step ? step : "Running", pct);
        });
        if (ok) {
            ui_progress_finish("FTP Transfer", true, "FTP transfer done");
            ui_log("[FTP] %s success: %s", cfg.upload ? "Upload" : "Download", cfg.remote_path.c_str());
        } else {
            ui_progress_finish("FTP Transfer", false, err.c_str());
            ui_log("[FTP] %s failed: %s", cfg.upload ? "Upload" : "Download", err.c_str());
        }
        g_ftp_running.store(false);
        if (mode_btn_ftp_upload) lv_obj_clear_state(mode_btn_ftp_upload, LV_STATE_DISABLED);
        if (mode_btn_ftp_download) lv_obj_clear_state(mode_btn_ftp_download, LV_STATE_DISABLED);
        if (mode_btn_ftp_cancel) lv_obj_add_state(mode_btn_ftp_cancel, LV_STATE_DISABLED);
        if (mode_btn_ftp_retry) {
            if (ok) lv_obj_add_state(mode_btn_ftp_retry, LV_STATE_DISABLED);
            else lv_obj_clear_state(mode_btn_ftp_retry, LV_STATE_DISABLED);
        }
    }).detach();
}

static void run_ftp_transfer(bool upload) {
    FtpTransferConfig cfg;
    cfg.host = ta_text(mode_ta_ftp_host);
    cfg.port = atoi(ta_text(mode_ta_ftp_port, "21").c_str());
    cfg.username = ta_text(mode_ta_ftp_user);
    cfg.password = ta_text(mode_ta_ftp_pass);
    cfg.remote_path = ta_text(mode_ta_ftp_remote);
    cfg.local_path = ta_text(mode_ta_ftp_local);
    cfg.upload = upload;
    cfg.use_ftps = mode_sw_ftp_ftps && lv_obj_has_state(mode_sw_ftp_ftps, LV_STATE_CHECKED);
    cfg.recursive_upload = upload && mode_sw_ftp_recursive && lv_obj_has_state(mode_sw_ftp_recursive, LV_STATE_CHECKED);
    cfg.resume_download = !upload;
    run_ftp_transfer_config(cfg);
}

static void work_ftp_upload_cb(lv_event_t* e) {
    (void)e;
    if (!ask_mode_confirm_action("FTP upload will access remote endpoint and transfer files", "Review host/path and then continue")) return;
    run_ftp_transfer(true);
}

static void work_ftp_download_cb(lv_event_t* e) {
    (void)e;
    if (!ask_mode_confirm_action("FTP download will write files to local disk", "Confirm destination path before continue")) return;
    run_ftp_transfer(false);
}

static void work_ftp_cancel_cb(lv_event_t* e) {
    (void)e;
    g_ftp_cancel.store(true);
}

static void work_ftp_retry_cb(lv_event_t* e) {
    (void)e;
    FtpTransferConfig cfg;
    {
        std::lock_guard<std::mutex> lk(g_ftp_last_mtx);
        if (!g_ftp_has_last) {
            ui_log("[FTP] No previous transfer to retry");
            return;
        }
        cfg = g_ftp_last_cfg;
    }
    ui_log("[FTP] Retrying last %s task", cfg.upload ? "upload" : "download");
    run_ftp_transfer_config(cfg);
}

static void work_kb_add_source_cb(lv_event_t* e) {
    (void)e;
    if (!ask_mode_confirm_action("Add KB source file into local index", "Proceed with selected file")) return;
    std::string p = ta_text(mode_ta_kb_source);
    std::string err;
    if (KbStore::instance().add_source_file(p.c_str(), err)) {
        ui_log("[KB] Added source: %s (total=%d)", p.c_str(), KbStore::instance().doc_count());
    } else {
        ui_log("[KB] Add source failed: %s", err.c_str());
    }
}

static void work_kb_import_dir_cb(lv_event_t* e) {
    (void)e;
    if (!ask_mode_confirm_action("Import KB directory recursively", "Proceed and index markdown/text documents")) return;
    std::string dir = ta_text(mode_ta_kb_source);
    int added = 0;
    std::string err;
    if (KbStore::instance().add_source_dir(dir.c_str(), &added, err)) {
        ui_log("[KB] Imported directory: %s (added=%d, docs=%d, sources=%d)",
               dir.c_str(), added, KbStore::instance().doc_count(), KbStore::instance().source_count());
    } else {
        ui_log("[KB] Import directory failed: %s", err.c_str());
    }
}

static void work_kb_export_manifest_cb(lv_event_t* e) {
    (void)e;
    if (!ask_mode_confirm_action("Export KB manifest to local path", "Proceed and write kb_export_manifest.md")) return;
    std::string out_dir = ta_text(mode_ta_kb_source);
    std::string err;
    if (KbStore::instance().export_manifest(out_dir.c_str(), err)) {
        ui_log("[KB] Exported manifest to: %s\\kb_export_manifest.md", out_dir.c_str());
    } else {
        ui_log("[KB] Export manifest failed: %s", err.c_str());
    }
}

static void work_kb_search_cb(lv_event_t* e) {
    (void)e;
    if (!mode_ta_kb_result) return;
    std::string kw = ta_text(mode_ta_kb_keyword);
    auto hits = KbStore::instance().search_keyword(kw.c_str(), 8);
    std::string out;
    if (hits.empty()) out = "No hits.";
    else {
        out += "Docs=" + std::to_string(KbStore::instance().doc_count()) +
               ", Sources=" + std::to_string(KbStore::instance().source_count()) + "\n\n";
        for (size_t i = 0; i < hits.size(); ++i) {
            out += "[" + std::to_string(i + 1) + "] " + hits[i].path + "\n";
        }
        out += "\n--- Context Snippet ---\n";
        out += KbStore::instance().build_context_snippet(kw.c_str(), 3, 1400);
    }
    lv_textarea_set_text(mode_ta_kb_result, out.c_str());
}

/* Update send button visual state based on input content */
static void update_send_button_state() {
    if (!btn_send_widget || !chat_input) return;
    const ThemeColors* c = g_colors ? g_colors : &THEME_DARK;
    const char* text = lv_textarea_get_text(chat_input);
    bool empty = (!text || !text[0]);
    bool disabled = g_streaming || empty;
    if (disabled) {
        lv_obj_set_style_bg_color(btn_send_widget, c->btn_secondary, 0);
        lv_obj_set_style_opa(btn_send_widget, LV_OPA_50, 0);
        lv_obj_add_state(btn_send_widget, LV_STATE_DISABLED);
    } else {
        lv_obj_set_style_bg_color(btn_send_widget, c->btn_action, 0);
        apply_btn_gradient(btn_send_widget);
        lv_obj_set_style_opa(btn_send_widget, LV_OPA_COVER, 0);
        lv_obj_clear_state(btn_send_widget, LV_STATE_DISABLED);
    }
    if (btn_upload_widget) {
        if (g_streaming) lv_obj_add_state(btn_upload_widget, LV_STATE_DISABLED);
        else lv_obj_clear_state(btn_upload_widget, LV_STATE_DISABLED);
        lv_obj_set_style_opa(btn_upload_widget, g_streaming ? LV_OPA_40 : LV_OPA_COVER, 0);
    }
    if (btn_voice_widget) {
        if (g_streaming) lv_obj_add_state(btn_voice_widget, LV_STATE_DISABLED);
        else lv_obj_clear_state(btn_voice_widget, LV_STATE_DISABLED);
        lv_obj_set_style_opa(btn_voice_widget, g_streaming ? LV_OPA_40 : LV_OPA_COVER, 0);
    }
    if (btn_work_widget) {
        if (g_streaming) lv_obj_add_state(btn_work_widget, LV_STATE_DISABLED);
        else lv_obj_clear_state(btn_work_widget, LV_STATE_DISABLED);
        lv_obj_set_style_opa(btn_work_widget, g_streaming ? LV_OPA_40 : LV_OPA_COVER, 0);
    }
}

/* Update send button on input text change */
static void chat_input_value_changed_for_btn_cb(lv_event_t* e) {
    (void)e;
    update_send_button_state();
}

static void chat_input_cb(lv_event_t* e) {
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_READY) {
        chat_send_cb(e);
        lv_event_stop_processing(e);
    }
    if (code == LV_EVENT_KEY) {
        uint32_t key = lv_event_get_key(e);
        if (key == LV_KEY_ENTER) {
            /* Check if Shift is held (Shift+Enter = newline) */
            bool shift_held = false;
            bool ctrl_held = false;
            SDL_Keymod mod = SDL_GetModState();
            if (mod & (KMOD_LSHIFT | KMOD_RSHIFT)) shift_held = true;
            if (mod & (KMOD_LCTRL | KMOD_RCTRL)) ctrl_held = true;

            if (ctrl_held && !shift_held) {
                chat_send_to_work_cb(e);
                lv_event_stop_processing(e);
                return;
            }

            if (!shift_held) {
                /* Plain Enter → send message, stop textarea from inserting newline */
                chat_send_cb(e);
                lv_event_stop_processing(e);
            }
            /* Shift+Enter → let textarea handle as newline */
        }
    }
}

static void work_prompt_input_cb(lv_event_t* e) {
    if (lv_event_get_code(e) != LV_EVENT_KEY) return;
    uint32_t key = lv_event_get_key(e);
    if (key != LV_KEY_ENTER) return;

    SDL_Keymod mod = SDL_GetModState();
    bool shift_held = (mod & (KMOD_LSHIFT | KMOD_RSHIFT)) != 0;
    if (shift_held) return;

    work_send_cb(e);
    lv_event_stop_processing(e);
}

/* ── P2-3: Task list management ── */

/* Add a task to the left panel task list */
/* ── Session abort callback ── */
static void session_abort_cb(lv_event_t* e) {
    lv_obj_t* btn = lv_event_get_target_obj(e);
    /* Find matching task item */
    for (int i = 0; i < g_task_count; i++) {
        if (g_tasks[i].abort_btn == btn && g_tasks[i].session_key[0]) {
            LOG_I("Session", "User abort: %s", g_tasks[i].session_key);
            lv_label_set_text(lv_obj_get_child(btn, 0), "...");
            lv_obj_add_state(btn, LV_STATE_DISABLED);

            /* Execute abort in background (blocking ~1.5s, but UI shows loading) */
            bool ok = app_abort_session(g_tasks[i].session_key);
            if (ok) {
                ui_log("[Session] Aborted: %.60s", g_tasks[i].session_key);
                ui_toast_warn(g_lang == Lang::CN ? "会话已终止" : "Session aborted");
            } else {
                ui_log("[Session] Abort failed: %.60s", g_tasks[i].session_key);
                ui_toast_error(g_lang == Lang::CN ? "终止失败" : "Abort failed");
                lv_label_set_text(lv_obj_get_child(btn, 0), "✕");
                lv_obj_clear_state(btn, LV_STATE_DISABLED);
            }
            break;
        }
    }
}

static void task_add(const char* name, const char* status, const char* detail = nullptr) {
    if (!left_panel || g_task_count >= MAX_TASK_WIDGETS) return;
    const ThemeColors* c = g_colors ? g_colors : &THEME_DARK;
    if (task_empty_label) {
        lv_obj_add_flag(task_empty_label, LV_OBJ_FLAG_HIDDEN);
    }

    TaskItem& t = g_tasks[g_task_count];
    snprintf(t.name, sizeof(t.name), "%s", name);
    snprintf(t.status, sizeof(t.status), "%s", status);
    snprintf(t.detail, sizeof(t.detail), "%s", detail ? detail : "");
    t.session_key[0] = '\0';
    t.tooltip = nullptr;
    t.abort_btn = nullptr;

    /* Task container row */
    t.widget = lv_obj_create(left_panel);
    lv_obj_set_size(t.widget, LEFT_PANEL_W - GAP * 2, SCALE(32));
    lv_obj_set_pos(t.widget, GAP, g_task_next_y);
    lv_obj_set_style_bg_color(t.widget, c->panel, 0);
    lv_obj_set_style_bg_opa(t.widget, LV_OPA_60, 0);
    lv_obj_set_style_border_width(t.widget, 0, 0);
    lv_obj_set_style_radius(t.widget, SCALE(g_colors->radius_sm), 0);
    lv_obj_set_style_pad_all(t.widget, 4, 0);
    lv_obj_clear_flag(t.widget, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_flex_flow(t.widget, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(t.widget, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_gap(t.widget, 8, 0);

    /* Status indicator dot */
    lv_obj_t* dot = lv_obj_create(t.widget);
    lv_obj_set_size(dot, LED_DOT_SIZE, LED_DOT_SIZE);
    lv_obj_set_style_radius(dot, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_border_width(dot, 0, 0);
    lv_obj_set_style_pad_all(dot, 0, 0);
    lv_obj_clear_flag(dot, LV_OBJ_FLAG_SCROLLABLE);
    /* Status indicator dot — 四色统一：白(Idle)/绿(Running)/红(Error)/黄(Checking) */
    if (strcmp(status, "Running") == 0 || strcmp(status, "运行中") == 0) {
        lv_obj_set_style_bg_color(dot, c->success, 0);
    } else if (strcmp(status, "Error") == 0 || strcmp(status, "异常") == 0) {
        lv_obj_set_style_bg_color(dot, c->danger, 0);
    } else if (strcmp(status, "Checking") == 0 || strcmp(status, "检测中") == 0) {
        lv_obj_set_style_bg_color(dot, c->warning, 0);
    } else {
        lv_obj_set_style_bg_color(dot, c->text_dim, 0); /* White / Idle */
    }

    /* Task name — fill remaining space via flex_grow */
    t.label = lv_label_create(t.widget);
    lv_label_set_text(t.label, name);
    lv_obj_set_style_text_color(t.label, c->text_dim, 0);
    lv_obj_set_style_text_font(t.label, CJK_FONT_SMALL, 0);
    lv_label_set_long_mode(t.label, LV_LABEL_LONG_MODE_DOTS);
    lv_obj_set_flex_grow(t.label, 1);

    /* TASK-054: Register hover event for detail popup */
    lv_obj_add_event_cb(t.widget, task_hover_cb, LV_EVENT_HOVER_OVER, nullptr);
    lv_obj_add_event_cb(t.widget, task_hover_cb, LV_EVENT_HOVER_LEAVE, nullptr);

    g_task_count++;
    g_task_next_y += 32 + 12; /* height + rg */
    ui_log("[Task] Added: %s [%s]", name, status);
}

/* Add a task item with an abort/reset button */
static void task_add_with_abort(const char* name, const char* status, const char* detail,
                                 const char* session_key) {
    const ThemeColors* c = g_colors ? g_colors : &THEME_DARK;
    task_add(name, status, detail);
    if (g_task_count < 1) return;

    TaskItem& t = g_tasks[g_task_count - 1];
    if (session_key && session_key[0]) {
        snprintf(t.session_key, sizeof(t.session_key), "%s", session_key);

        /* Create abort button (small, right-aligned) */
        t.abort_btn = lv_btn_create(t.widget);
        lv_obj_set_size(t.abort_btn, SCALE(28), SCALE(22));
        lv_obj_set_style_bg_color(t.abort_btn, c->btn_close, 0);
        lv_obj_set_style_bg_color(t.abort_btn, c->danger, LV_STATE_PRESSED);
        lv_obj_set_style_bg_color(t.abort_btn, c->disabled_bg, LV_STATE_DISABLED);
        lv_obj_set_style_radius(t.abort_btn, SCALE(g_colors->radius_sm), 0);
        lv_obj_set_style_border_width(t.abort_btn, 0, 0);
        lv_obj_set_style_pad_all(t.abort_btn, 0, 0);
        lv_obj_clear_flag(t.abort_btn, LV_OBJ_FLAG_SCROLLABLE);

        lv_obj_t* abort_lbl = lv_label_create(t.abort_btn);
        lv_label_set_text(abort_lbl, LV_SYMBOL_CLOSE);
        lv_obj_set_style_text_color(abort_lbl, c->text_inverse, 0);
        lv_obj_set_style_text_font(abort_lbl, &lv_font_montserrat_14, 0);
        lv_obj_center(abort_lbl);

        lv_obj_add_event_cb(t.abort_btn, session_abort_cb, LV_EVENT_CLICKED, nullptr);
    }
}

/* Remove all tasks from list */
static void task_clear() {
    task_destroy_all_tooltips(); /* TASK-054: destroy tooltips before widgets */
    for (int i = 0; i < g_task_count; i++) {
        if (g_tasks[i].widget) {
            lv_obj_del(g_tasks[i].widget);
            g_tasks[i].widget = nullptr;
            g_tasks[i].label = nullptr;
            g_tasks[i].status_lbl = nullptr;
        }
    }
    g_task_count = 0;
    /* Reset to empty label position */
    if (task_empty_label) {
        lv_obj_clear_flag(task_empty_label, LV_OBJ_FLAG_HIDDEN);
        g_task_next_y = lv_obj_get_y(task_empty_label);
    }
}

/* Update task list based on current status */
static void update_task_list(ClawStatus status) {
    static const I18n S_READY    = {"Ready", "就绪"};
    static const I18n S_RUNNING  = {"Running", "运行中"};
    static const I18n S_BUSY     = {"Busy", "处理中"};
    static const I18n S_CHECKING = {"Checking", "检测中"};
    static const I18n S_DETECTED = {"Detected", "已检测"};
    static const I18n S_MONITOR  = {"Health Monitor", "健康监控"};
    static const I18n S_SCHED    = {"Scheduler", "调度器"};
    static const I18n S_AGENT    = {"Agent", "Agent"};
    static const I18n S_ERROR    = {"Error", "异常"};
    static const I18n S_SESSION  = {"Session", "会话"};

    /* Clear previous tasks */
    task_clear();

    /* Get active session data from SessionManager */
    SessionManager& sm = session_mgr();
    auto active_sessions = sm.active_sessions();
    int session_count = (int)active_sessions.size();
    int cron_active_count = 0;
    for (const auto& s : active_sessions) {
        if (s.isCron) cron_active_count++;
    }

    /* Update count badge */
    if (lp_task_count) {
        char count_str[16];
        snprintf(count_str, sizeof(count_str), "(%d)", session_count);
        lv_label_set_text(lp_task_count, count_str);
    }

    /* Build gateway detail string for hover popup */
    char gw_detail[256] = {0};
    {
        int gw_port = read_gateway_port();
        char gw_model[256] = {0};
        app_get_current_model(gw_model, sizeof(gw_model));
        snprintf(gw_detail, sizeof(gw_detail), "Port: %d\nModel: %s",
                 gw_port, gw_model[0] ? gw_model : "N/A");
    }

    switch (status) {
        case ClawStatus::Ready:
            task_add("Gateway Service", tr(S_READY), gw_detail);
            task_add(tr(S_MONITOR), tr(S_RUNNING), "Interval: 30s\nChecks: Gateway + Sessions");
            {
                char cron_detail[160] = {0};
                snprintf(cron_detail, sizeof(cron_detail),
                         "Active sessions: %d\nActive cron sessions: %d",
                         session_count, cron_active_count);
                task_add(tr(S_SCHED), tr(S_RUNNING), cron_detail);
            }
            break;
        case ClawStatus::Busy:
            task_add("Gateway Service", tr(S_READY), gw_detail);
            {
                char cron_detail[160] = {0};
                snprintf(cron_detail, sizeof(cron_detail),
                         "Active sessions: %d\nActive cron sessions: %d",
                         session_count, cron_active_count);
                task_add(tr(S_SCHED), tr(S_RUNNING), cron_detail);
            }
            /* Show each active session with abort button */
            if (session_count > 0) {
                for (size_t i = 0; i < active_sessions.size() && (int)i < MAX_TASK_WIDGETS - 3; i++) {
                    const auto& s = active_sessions[i];

                    /* Format: "channel · agentId (age)" */
                    char task_name[128];
                    snprintf(task_name, sizeof(task_name), "%s · %s (%ds)",
                             s.display_channel(), s.display_name(), s.age_seconds());

                    /* Build session detail for hover */
                    char session_detail[256];
                    snprintf(session_detail, sizeof(session_detail),
                             "Channel: %s\nAgent: %s\nActive: %ds\nKey: %.60s",
                             s.display_channel(), s.agentId.c_str(),
                             s.age_seconds(), s.key.c_str());

                    /* Show abort button for active sessions */
                    task_add_with_abort(task_name, tr(S_BUSY), session_detail, s.key.c_str());
                }
            } else {
                task_add(tr(S_AGENT), tr(S_BUSY), "Gateway processing request");
            }
            break;
        case ClawStatus::Checking:
            task_add("Gateway Service", tr(S_CHECKING), "Checking Gateway status...");
            break;
        case ClawStatus::Detected:
            task_add("Gateway Service", tr(S_DETECTED), gw_detail);
            break;
        case ClawStatus::Error: {
            const char* reason = g_status_reason.empty()
                ? tr(S_ERROR)
                : g_status_reason.c_str();
            char err_detail[256];
            snprintf(err_detail, sizeof(err_detail), "%s\nReason: %s",
                     gw_detail, reason);
            task_add("Gateway Service", reason, err_detail);
            break;
        }
        default:
            break;
    }
}

static void lang_cn_cb(lv_event_t* e) {
    (void)e;
    if (g_lang != Lang::CN) { g_lang = Lang::CN; update_ui_language(); ui_log("[Lang] Chinese mode"); }
}
static void lang_en_cb(lv_event_t* e) {
    (void)e;
    if (g_lang != Lang::EN) { g_lang = Lang::EN; update_ui_language(); ui_log("[Lang] English mode"); }
}
/* P2-01: CN/EN toggle button - single language per PRD 2.1 */
static void lang_toggle_cb(lv_event_t* e) {
    (void)e;
    /* English only mode - toggle does nothing useful, just show EN */
    if (g_lang_toggle_label) {
        lv_label_set_text(g_lang_toggle_label, "EN");
        lv_obj_center(g_lang_toggle_label);
    }
    ui_log("[Lang] English mode (bilingual disabled)");
}

void app_refresh_status() {
    ClawStatus status = ClawStatus::Checking;
    (void)health_try_get_last_status(&status);

    /* Update status text (small dim, next to LED) */
    if (status_label) {
        lv_label_set_text(status_label, status_text(status));
        lv_obj_set_style_text_color(status_label, g_colors->text_dim, 0);
    }
    /* Single LED: green=Ready, yellow=Busy, red=Error */
    lv_led_set_color(led_ok, status_color(status));
    lv_led_on(led_ok);

    /* P2: Update dynamic task list */
    update_task_list(status);

    lv_obj_invalidate(lv_screen_active());
}

void update_ui_language() {
    static const I18n S_SEND  = {"Send", "发送"};

    /* Title bar */
    update_main_title();

    /* Left panel */
    static const I18n S_GW_STATUS = {"Gateway Status", "Gateway 状态"};
    static const I18n S_TASK_LIST = {"Task List", "任务列表"};
    static const I18n S_NO_TASKS  = {"No tasks yet", "暂无任务"};
    static const I18n S_AUTOREF   = {"Auto-refresh: 30s", "自动刷新: 30s"};

    if (lp_panel_title) lv_label_set_text(lp_panel_title, tr(S_GW_STATUS));
    if (lp_task_title) lv_label_set_text(lp_task_title, tr(S_TASK_LIST));
    if (task_empty_label) lv_label_set_text(task_empty_label, tr(S_NO_TASKS));
    if (lp_hint) lv_label_set_text(lp_hint, tr(S_AUTOREF));

    /* Send button */
    if (btn_send_widget) {
        lv_obj_t* lbl = lv_obj_get_child(btn_send_widget, 0);
        if (lbl) lv_label_set_text(lbl, tr(S_SEND));
    }

    /* Chat input placeholder */
    if (chat_input) {
        lv_textarea_set_placeholder_text(chat_input,
            g_lang == Lang::CN
                ? "输入消息... (Shift+Enter 换行, Ctrl+Enter 任务)"
                : "Type message... (Shift+Enter newline, Ctrl+Enter task)");
    }
    if (mode_ta_work_prompt) {
        lv_textarea_set_placeholder_text(mode_ta_work_prompt,
            g_lang == Lang::CN
                ? "输入任务... (Enter 发送, Shift+Enter 换行)"
                : "Type task... (Enter send, Shift+Enter newline)");
    }
    c2_refresh_dual_view_button();
    c2_refresh_work_chat_state_label();

    /* Refresh status (re-renders status text + task list in new language) */
    app_refresh_status();
}

/* ── Tooltip overlay for truncated labels ── */
static lv_obj_t* g_tooltip_overlay = nullptr;
static lv_timer_t* g_label_hover_timer = nullptr;
static lv_obj_t* g_pending_label_hover = nullptr;

/* Cancel pending label hover timer */
static void cancel_label_hover_timer(void) {
    if (g_label_hover_timer) {
        lv_timer_del(g_label_hover_timer);
        g_label_hover_timer = nullptr;
    }
    g_pending_label_hover = nullptr;
}

/* Timer callback: show label tooltip after debounce */
static void on_label_hover_timer(lv_timer_t* t) {
    if (g_label_hover_timer == t) g_label_hover_timer = nullptr;
    lv_timer_del(t); /* one-shot debounce timer */

    if (!g_pending_label_hover || !g_colors) return;
    if (!lv_obj_is_valid(g_pending_label_hover)) {
        g_pending_label_hover = nullptr;
        return;
    }

    /* Check if text is actually truncated */
    const char* txt = lv_label_get_text(g_pending_label_hover);
    if (!txt || !txt[0]) { g_pending_label_hover = nullptr; return; }

    lv_coord_t label_w = lv_obj_get_width(g_pending_label_hover);
    const lv_font_t* font = lv_obj_get_style_text_font(g_pending_label_hover, LV_PART_MAIN);
    lv_point_t txt_size;
    lv_text_get_size(&txt_size, txt, font, 0, 0, LV_COORD_MAX, LV_TEXT_FLAG_NONE);
    if (txt_size.x <= label_w) { g_pending_label_hover = nullptr; return; }

    /* Create tooltip overlay if not exists */
    if (!g_tooltip_overlay) {
        g_tooltip_overlay = lv_label_create(lv_scr_act());
        lv_obj_set_style_bg_color(g_tooltip_overlay, g_colors->raised, 0);
        lv_obj_set_style_bg_opa(g_tooltip_overlay, LV_OPA_COVER, 0);
        lv_obj_set_style_text_color(g_tooltip_overlay, g_colors->text, 0);
        lv_obj_set_style_text_font(g_tooltip_overlay, CJK_FONT_SMALL, 0);
        lv_obj_set_style_radius(g_tooltip_overlay, SCALE(g_colors->radius_sm), 0);
        lv_obj_set_style_pad_all(g_tooltip_overlay, 4, 0);
        lv_obj_set_style_border_width(g_tooltip_overlay, 1, 0);
        lv_obj_set_style_border_color(g_tooltip_overlay, g_colors->border_strong, 0);
    }
    lv_label_set_text(g_tooltip_overlay, txt);
    lv_obj_clear_flag(g_tooltip_overlay, LV_OBJ_FLAG_HIDDEN);

    lv_area_t area;
    lv_obj_get_coords(g_pending_label_hover, &area);
    lv_obj_set_pos(g_tooltip_overlay, area.x1, area.y2 + 2);
    lv_obj_move_foreground(g_tooltip_overlay);
    g_pending_label_hover = nullptr;
}

/* Show full text on hover */
static void label_hover_cb(lv_event_t* e) {
    lv_obj_t* lbl = (lv_obj_t*)lv_event_get_target(e);
    lv_event_code_t code = lv_event_get_code(e);

    if (code == LV_EVENT_HOVER_OVER) {
        /* Debounce: 80ms delay before showing tooltip */
        cancel_label_hover_timer();
        g_pending_label_hover = lbl;
        g_label_hover_timer = lv_timer_create(on_label_hover_timer, 80, nullptr);
        lv_timer_set_repeat_count(g_label_hover_timer, 1);
    } else if (code == LV_EVENT_HOVER_LEAVE) {
        cancel_label_hover_timer();
        if (g_tooltip_overlay) {
            lv_obj_add_flag(g_tooltip_overlay, LV_OBJ_FLAG_HIDDEN);
        }
    } else if (code == LV_EVENT_DELETE) {
        if (g_pending_label_hover == lbl) {
            cancel_label_hover_timer();
        }
        if (g_tooltip_overlay) {
            lv_obj_add_flag(g_tooltip_overlay, LV_OBJ_FLAG_HIDDEN);
        }
    }
}

/* Helper: add hover tooltip to label */
static void add_label_hover_tooltip(lv_obj_t* lbl) {
    lv_obj_add_event_cb(lbl, label_hover_cb, LV_EVENT_HOVER_OVER, nullptr);
    lv_obj_add_event_cb(lbl, label_hover_cb, LV_EVENT_HOVER_LEAVE, nullptr);
    lv_obj_add_event_cb(lbl, label_hover_cb, LV_EVENT_DELETE, nullptr);
}

/*
 * ═══ Three-Zone Row Layout ═══
 * Creates a flex row with LEFT / CENTER / RIGHT zones.
 * Pass nullptr for any zone you don't need.
 *
 *  ┌──────────────────────────────────────────────┐
 *  │ [LEFT item]  [CENTER item]    [RIGHT item]   │
 *  └──────────────────────────────────────────────┘
 *
 * CENTER uses flex_grow=1 with centered text.
 * Row follows LP_ROW_* macros for spacing/borders.
 */
static lv_obj_t* create_three_zone_row(lv_obj_t* parent, int y, int row_w) {
    lv_obj_t* row = lv_obj_create(parent);
    lv_obj_set_size(row, row_w, SCALE(LP_ROW_H));
    lv_obj_set_pos(row, SCALE(GAP_BASE), y);
    lv_obj_set_style_bg_opa(row, LP_ROW_BG_OPA, 0);
    lv_obj_set_style_border_width(row, LP_ROW_BORDER_W, 0);
    lv_obj_set_style_pad_all(row, LP_ROW_PAD_ALL, 0);
    lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(row, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_gap(row, SCALE(LP_ROW_GAP), 0);
    return row;
}

/* Add an item to a three-zone row. zone: 0=left, 1=center, 2=right */
static lv_obj_t* row_add_label(lv_obj_t* row, const char* text, lv_color_t color, int zone) {
    lv_obj_t* lbl = lv_label_create(row);
    lv_label_set_text(lbl, text);
    lv_obj_set_style_text_color(lbl, color, 0);
    lv_obj_set_style_text_font(lbl, CJK_FONT, 0);
    if (zone == 1) {
        /* Center zone: fill remaining space, center text */
        lv_obj_set_width(lbl, LV_PCT(100));
        lv_obj_set_style_text_align(lbl, LV_TEXT_ALIGN_CENTER, 0);
    } else {
        lv_label_set_long_mode(lbl, LV_LABEL_LONG_MODE_DOTS);
    }
    return lbl;
}

/* Add an LVGL object (LED, button, etc.) to a row zone */
static void row_add_obj(lv_obj_t* row, lv_obj_t* obj, int zone) {
    (void)zone; /* SPACE_BETWEEN handles positioning */
    /* obj already created as child of row by caller */
}

/* ── Helper: create styled label with optional tooltip ── */
static lv_obj_t* create_styled_label(lv_obj_t* parent, const char* text, lv_color_t color, int x, int y, int w) {
    lv_obj_t* lbl = lv_label_create(parent);
    lv_label_set_text(lbl, text);
    lv_obj_set_style_text_color(lbl, color, 0);
    lv_obj_set_style_text_font(lbl, CJK_FONT, 0);
    if (w > 0) {
        lv_obj_set_width(lbl, w);
        lv_label_set_long_mode(lbl, LV_LABEL_LONG_MODE_DOTS);
        /* Add hover tooltip for truncated labels */
        add_label_hover_tooltip(lbl);
    }
    lv_obj_set_pos(lbl, x, y);
    return lbl;
}

/* ═══ Dialog drag — click-hold on dialog box to move it ═══ */

static void dialog_drag_cb(lv_event_t* e) {
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t* box = (lv_obj_t*)lv_event_get_target(e);

    static lv_obj_t* dragging_box = nullptr;
    static lv_point_t drag_start = {0, 0};
    static lv_point_t box_start = {0, 0};

    if (code == LV_EVENT_PRESSED) {
        lv_indev_t* indev = lv_indev_get_act();
        if (!indev) return;
        lv_indev_get_point(indev, &drag_start);
        box_start.x = lv_obj_get_x(box);
        box_start.y = lv_obj_get_y(box);
        dragging_box = box;
    } else if (code == LV_EVENT_PRESSING && dragging_box == box) {
        lv_indev_t* indev = lv_indev_get_act();
        if (!indev) return;
        lv_point_t p;
        lv_indev_get_point(indev, &p);
        int dx = p.x - drag_start.x;
        int dy = p.y - drag_start.y;
        lv_obj_set_pos(box, box_start.x + dx, box_start.y + dy);
    } else if (code == LV_EVENT_RELEASED || code == LV_EVENT_LEAVE) {
        dragging_box = nullptr;
    }
}

/*
 * ═══ Unified Dialog Factory ═══
 * Creates a consistent dialog: overlay + centered box + title + close button.
 *
 *  ┌─ overlay (full screen, dark bg) ──────────────┐
 *  │  ┌─ box ────────────────────────────────────┐  │
 *  │  │  [title label]                    [✕]    │  │
 *  │  │  ─────────────────────────────────────── │  │
 *  │  │                                          │  │
 *  │  │  ← content area (caller fills this) →   │  │
 *  │  │                                          │  │
 *  │  │  [Cancel]              [OK]              │  │
 *  │  └──────────────────────────────────────────┘  │
 *  └────────────────────────────────────────────────┘
 *
 * @param parent     Parent object (usually lv_scr_act())
 * @param title      Dialog title text
 * @param w          Box width (0 = default 520)
 * @param h          Box height (0 = LV_SIZE_CONTENT)
 * @param out_overlay Returns the overlay pointer (for closing)
 * @return           The box container (caller adds content + buttons)
 */
static lv_obj_t* create_dialog(lv_obj_t* parent, const char* title, int w, int h, lv_obj_t** out_overlay) {
    const ThemeColors* c = g_colors;
    if (w <= 0) w = WIN_W * DIALOG_W_PCT / 100;

    /* Full-screen overlay */
    lv_obj_t* overlay = lv_obj_create(parent);
    lv_obj_set_size(overlay, WIN_W, WIN_H);
    lv_obj_set_pos(overlay, 0, 0);
    lv_obj_set_style_bg_color(overlay, c->shadow_lg, 0);
    lv_obj_set_style_bg_opa(overlay, c->mask_opacity, 0);
    lv_obj_set_style_border_width(overlay, 0, 0);
    lv_obj_set_style_pad_all(overlay, 0, 0);
    lv_obj_clear_flag(overlay, LV_OBJ_FLAG_SCROLLABLE);

    /* Centered box */
    lv_obj_t* box = lv_obj_create(overlay);
    if (h > 0) {
        lv_obj_set_size(box, w, h);
    } else {
        lv_obj_set_size(box, w, LV_SIZE_CONTENT);
    }
    /* Use absolute position instead of alignment to avoid drag conflict */
    lv_obj_set_pos(box, (WIN_W - w) / 2, WIN_H / 4);
    lv_obj_set_style_bg_color(box, c->panel, 0);
    lv_obj_set_style_border_color(box, c->border, 0);
    lv_obj_set_style_border_width(box, 1, 0);
    lv_obj_set_style_radius(box, SCALE(g_colors->radius_lg), 0);
    lv_obj_set_style_pad_all(box, SCALE(16), 0);
    lv_obj_set_flex_flow(box, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(box, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_set_style_pad_gap(box, SCALE(10), 0);
    lv_obj_clear_flag(box, LV_OBJ_FLAG_SCROLLABLE);

    /* Draggable */
    lv_obj_add_event_cb(box, dialog_drag_cb, LV_EVENT_PRESSED, nullptr);
    lv_obj_add_event_cb(box, dialog_drag_cb, LV_EVENT_PRESSING, nullptr);
    lv_obj_add_event_cb(box, dialog_drag_cb, LV_EVENT_RELEASED, nullptr);

    /* Title row: only title (no top-right close button) */
    lv_obj_t* title_row = lv_obj_create(box);
    lv_obj_set_size(title_row, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(title_row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(title_row, 0, 0);
    lv_obj_set_style_pad_all(title_row, 0, 0);
    lv_obj_clear_flag(title_row, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_flex_flow(title_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(title_row, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    lv_obj_t* lbl_title = lv_label_create(title_row);
    lv_label_set_text(lbl_title, title);
    lv_obj_set_style_text_color(lbl_title, c->accent, 0);
    lv_obj_set_style_text_font(lbl_title, CJK_FONT, 0);

    if (out_overlay) *out_overlay = overlay;
    return box;
}

/* Helper: add [Cancel] [OK] button row to a dialog box */
static void add_dialog_buttons(lv_obj_t* box, lv_event_cb_t ok_cb, lv_event_cb_t cancel_cb, void* user_data) {
    const ThemeColors* c = g_colors;

    lv_obj_t* btn_row = lv_obj_create(box);
    lv_obj_set_size(btn_row, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(btn_row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(btn_row, 0, 0);
    lv_obj_set_style_pad_all(btn_row, 0, 0);
    lv_obj_clear_flag(btn_row, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_flex_flow(btn_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(btn_row, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    /* Cancel (left) */
    lv_obj_t* btn_cancel = lv_button_create(btn_row);
    lv_obj_set_size(btn_cancel, SCALE(80), SCALE(36));
    lv_obj_set_style_bg_color(btn_cancel, c->btn_secondary, 0);
    lv_obj_set_style_radius(btn_cancel, SCALE(g_colors->radius_sm), 0);
    lv_obj_t* lbl_c = lv_label_create(btn_cancel);
    lv_label_set_text(lbl_c, "Cancel");
    lv_obj_set_style_text_font(lbl_c, CJK_FONT, 0);
    lv_obj_set_style_text_color(lbl_c, c->text_inverse, 0);
    lv_obj_center(lbl_c);
    if (cancel_cb) {
        lv_obj_add_event_cb(btn_cancel, cancel_cb, LV_EVENT_CLICKED, user_data);
    } else {
        /* Default: close overlay */
        lv_obj_add_event_cb(btn_cancel, [](lv_event_t* e) {
            lv_obj_t* ov = lv_obj_get_parent(lv_obj_get_parent(lv_obj_get_parent(lv_event_get_target_obj(e))));
            lv_obj_del(ov);
        }, LV_EVENT_CLICKED, nullptr);
    }

    /* OK (right) */
    lv_obj_t* btn_ok = lv_button_create(btn_row);
    lv_obj_set_size(btn_ok, SCALE(80), SCALE(36));
    lv_obj_set_style_bg_color(btn_ok, c->btn_action, 0);
    apply_btn_gradient(btn_ok);
    lv_obj_set_style_radius(btn_ok, SCALE(g_colors->radius_sm), 0);
    lv_obj_t* lbl_o = lv_label_create(btn_ok);
    lv_label_set_text(lbl_o, "OK");
    lv_obj_set_style_text_font(lbl_o, CJK_FONT, 0);
    lv_obj_set_style_text_color(lbl_o, c->text_inverse, 0);
    lv_obj_center(lbl_o);
    if (ok_cb) {
        lv_obj_add_event_cb(btn_ok, ok_cb, LV_EVENT_CLICKED, user_data);
    }
}

struct PermDialogCtx {
    volatile int* decision; /* 0=pending 1=deny 2=allow_once 3=allow_persist */
    lv_obj_t* overlay;
};

static void perm_dialog_deny_cb(lv_event_t* e) {
    PermDialogCtx* ctx = (PermDialogCtx*)lv_event_get_user_data(e);
    if (!ctx || !ctx->decision) return;
    *ctx->decision = 1;
}

static void perm_dialog_allow_once_cb(lv_event_t* e) {
    PermDialogCtx* ctx = (PermDialogCtx*)lv_event_get_user_data(e);
    if (!ctx || !ctx->decision) return;
    *ctx->decision = 2;
}

static void perm_dialog_allow_persist_cb(lv_event_t* e) {
    PermDialogCtx* ctx = (PermDialogCtx*)lv_event_get_user_data(e);
    if (!ctx || !ctx->decision) return;
    *ctx->decision = 3;
}

int ui_permission_confirm(const char* perm_key, const char* target) {
    if (!g_ui_ready || g_ui_thread_id == 0 || GetCurrentThreadId() != g_ui_thread_id) {
        return -1;
    }

    {
        char msg[768];
        snprintf(msg, sizeof(msg),
                 "Agent requests command permission.\n\nPermission: %s\nTarget: %s\n\nYes = allow once\nNo = deny\nCancel = allow and remember",
                 perm_key ? perm_key : "exec_shell",
                 target ? target : "(unknown)");
        int r = MessageBoxA(nullptr, msg, "AnyClaw Permission", MB_ICONQUESTION | MB_YESNOCANCEL | MB_TOPMOST | MB_SETFOREGROUND);
        if (r == IDYES) return 1;
        if (r == IDCANCEL) return 2;
        return 0;
    }

    static bool in_dialog = false;
    if (in_dialog) return 0;
    in_dialog = true;

    lv_obj_t* scr = lv_screen_active();
    if (!scr) {
        in_dialog = false;
        return -1;
    }

    lv_obj_t* overlay = nullptr;
    lv_obj_t* box = create_dialog(scr, "AnyClaw 权限确认", 0, 0, &overlay);
    if (!box || !overlay) {
        in_dialog = false;
        return -1;
    }

    const ThemeColors* c = g_colors;
    lv_obj_t* lbl_intro = lv_label_create(box);
    lv_label_set_text(lbl_intro, "Agent 请求执行命令：");
    lv_obj_set_style_text_color(lbl_intro, c->text, 0);
    lv_obj_set_style_text_font(lbl_intro, CJK_FONT_SMALL, 0);
    lv_obj_set_width(lbl_intro, LV_PCT(100));

    lv_obj_t* cmd_box = lv_obj_create(box);
    lv_obj_set_size(cmd_box, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_style_bg_color(cmd_box, c->surface, 0);
    lv_obj_set_style_border_color(cmd_box, c->border, 0);
    lv_obj_set_style_border_width(cmd_box, 1, 0);
    lv_obj_set_style_radius(cmd_box, SCALE(g_colors->radius_lg), 0);
    lv_obj_set_style_pad_all(cmd_box, SCALE(10), 0);
    lv_obj_set_flex_flow(cmd_box, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(cmd_box, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_clear_flag(cmd_box, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* lbl_cmd = lv_label_create(cmd_box);
    lv_label_set_text(lbl_cmd, target ? target : "(unknown)");
    lv_obj_set_style_text_color(lbl_cmd, c->text, 0);
    lv_obj_set_style_text_font(lbl_cmd, CJK_FONT_SMALL, 0);
    lv_label_set_long_mode(lbl_cmd, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(lbl_cmd, LV_PCT(100));

    lv_obj_t* lbl_perm = lv_label_create(box);
    lv_label_set_text_fmt(lbl_perm, "权限项: %s", perm_key ? perm_key : "exec_shell");
    lv_obj_set_style_text_color(lbl_perm, c->text_dim, 0);
    lv_obj_set_style_text_font(lbl_perm, CJK_FONT_SMALL, 0);
    lv_obj_set_width(lbl_perm, LV_PCT(100));

    lv_obj_t* btn_row = lv_obj_create(box);
    lv_obj_set_size(btn_row, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(btn_row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(btn_row, 0, 0);
    lv_obj_set_style_pad_all(btn_row, 0, 0);
    lv_obj_set_style_pad_gap(btn_row, SCALE(8), 0);
    lv_obj_set_flex_flow(btn_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(btn_row, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag(btn_row, LV_OBJ_FLAG_SCROLLABLE);

    volatile int decision = 0;
    PermDialogCtx ctx = {&decision, overlay};

    auto add_btn = [&](const char* text, lv_color_t bg, lv_event_cb_t cb) -> lv_obj_t* {
        lv_obj_t* b = lv_button_create(btn_row);
        lv_obj_set_height(b, SCALE(38));
        lv_obj_set_flex_grow(b, 1);
        lv_obj_set_style_bg_color(b, bg, 0);
        if (lv_color_eq(bg, c->btn_action)) apply_btn_gradient(b);
        lv_obj_set_style_radius(b, SCALE(g_colors->radius_md), 0);
        lv_obj_set_style_border_width(b, 0, 0);
        lv_obj_t* t = lv_label_create(b);
        lv_label_set_text(t, text);
        lv_obj_set_style_text_color(t, lv_color_white(), 0);
        lv_obj_set_style_text_font(t, CJK_FONT_SMALL, 0);
        lv_obj_center(t);
        lv_obj_add_event_cb(b, cb, LV_EVENT_CLICKED, &ctx);
        return b;
    };

    lv_obj_t* btn_deny = add_btn("拒绝", c->btn_close, perm_dialog_deny_cb);
    lv_obj_t* btn_once = add_btn("此次授权", c->btn_action, perm_dialog_allow_once_cb);
    lv_obj_t* btn_perm = add_btn("永久授权", c->btn_add, perm_dialog_allow_persist_cb);
    (void)btn_deny;
    (void)btn_perm;
    lv_obj_add_state(btn_once, LV_STATE_FOCUSED);

    lv_obj_t* lbl_timeout = lv_label_create(box);
    lv_label_set_text(lbl_timeout, "⏱ 60秒超时自动拒绝");
    lv_obj_set_style_text_color(lbl_timeout, c->text_dim, 0);
    lv_obj_set_style_text_font(lbl_timeout, CJK_FONT_SMALL, 0);
    lv_obj_set_width(lbl_timeout, LV_PCT(100));

    lv_obj_move_foreground(overlay);

    uint32_t start = GetTickCount();
    while (decision == 0) {
        lv_timer_handler();
        Sleep(16);
        if (GetTickCount() - start > 60000) {
            decision = 1;
            break;
        }
    }

    if (overlay) lv_obj_del(overlay);
    in_dialog = false;
    if (decision == 2) return 1;
    if (decision == 3) return 2;
    return 0;
}

/* ═══ Legal Disclaimer (first launch) ═══ */
static bool g_disclaimer_accepted = false;

static std::string get_disclaimer_path() {
    const char* appdata = std::getenv("APPDATA");
    if (appdata) {
        std::string dir = std::string(appdata) + "\\AnyClaw_LVGL";
        std::filesystem::create_directories(dir);
        return dir + "\\accepted.json";
    }
    return "accepted.json";
}

static bool is_disclaimer_accepted() {
    std::ifstream f(get_disclaimer_path());
    return f.good();
}

static void save_disclaimer_accepted() {
    std::ofstream f(get_disclaimer_path());
    if (f.is_open()) {
        f << "{\n  \"disclaimer\": true,\n  \"timestamp\": \"2026-04-01\"\n}\n";
        f.close();
    }
}

static void disclaimer_ok_cb(lv_event_t* e) {
    lv_obj_t* checkbox = (lv_obj_t*)lv_event_get_user_data(e);
    if (checkbox && lv_obj_has_state(checkbox, LV_STATE_CHECKED)) {
        g_disclaimer_accepted = true;
        save_disclaimer_accepted();
        /* Hide disclaimer panel */
        lv_obj_t* panel = (lv_obj_t*)lv_event_get_target(e);
        panel = lv_obj_get_parent(lv_obj_get_parent(panel)); /* go up to modal */
        if (panel) {
            lv_obj_add_flag(panel, LV_OBJ_FLAG_HIDDEN);
            lv_obj_del(panel);
        }
        ui_log("[Disclaimer] Accepted by user");
    }
}

static void show_disclaimer(lv_obj_t* parent) {
    const ThemeColors* c = g_colors;
    if (is_disclaimer_accepted()) {
        g_disclaimer_accepted = true;
        return;
    }

    /* Create modal overlay */
    lv_obj_t* modal = lv_obj_create(parent);
    lv_obj_set_size(modal, WIN_W, WIN_H);
    lv_obj_set_pos(modal, 0, 0);
    lv_obj_set_style_bg_color(modal, c->bg, 0);
    lv_obj_set_style_bg_opa(modal, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(modal, 0, 0);
    lv_obj_set_style_radius(modal, 0, 0);
    lv_obj_set_style_pad_all(modal, 0, 0);
    lv_obj_clear_flag(modal, LV_OBJ_FLAG_SCROLLABLE);

    /* Disclaimer box — 41% × 50% of window */
    lv_obj_t* box = lv_obj_create(modal);
    lv_obj_set_size(box, WIN_W * DIALOG_W_PCT / 100, WIN_H * DIALOG_H_PCT / 100);
    lv_obj_set_pos(box, (WIN_W - 600) / 2, (WIN_H - 400) / 2);
    lv_obj_set_style_bg_color(box, c->panel, 0);
    lv_obj_set_style_bg_opa(box, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(box, 1, 0);
    lv_obj_set_style_border_color(box, c->border_strong, 0);
    lv_obj_set_style_radius(box, SCALE(g_colors->radius_lg), 0);
    lv_obj_set_style_pad_all(box, 20, 0);
    lv_obj_set_flex_flow(box, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_gap(box, 12, 0);
    lv_obj_clear_flag(box, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_event_cb(box, dialog_drag_cb, LV_EVENT_PRESSED, nullptr);
    lv_obj_add_event_cb(box, dialog_drag_cb, LV_EVENT_PRESSING, nullptr);
    lv_obj_add_event_cb(box, dialog_drag_cb, LV_EVENT_RELEASED, nullptr);

    /* Title */
    lv_obj_t* lbl_title = lv_label_create(box);
    lv_label_set_text(lbl_title, "Usage Disclaimer");
    lv_obj_set_style_text_color(lbl_title, c->accent_secondary, 0);
    lv_obj_set_style_text_font(lbl_title, CJK_FONT, 0);

    /* Content */
    lv_obj_t* lbl_content = lv_label_create(box);
    lv_label_set_text(lbl_content,
        "AnyClaw is an open-source desktop assistant.\n"
        "By using this software, you agree that:\n"
        "1. Skill downloads are third-party content\n"
        "2. The developer is not liable for data loss\n"
        "3. API keys are stored locally at your risk\n"
        "4. No warranty is provided\n\n"
        "Please read the full license at GitHub.");
    lv_obj_set_style_text_color(lbl_content, c->text, 0);
    lv_obj_set_style_text_font(lbl_content, CJK_FONT, 0);
    lv_label_set_long_mode(lbl_content, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(lbl_content, LV_PCT(100));

    /* Spacer */
    lv_obj_t* spacer = lv_obj_create(box);
    lv_obj_set_size(spacer, LV_PCT(100), 1);
    lv_obj_set_style_bg_color(spacer, c->border, 0);
    lv_obj_set_style_border_width(spacer, 0, 0);
    lv_obj_clear_flag(spacer, LV_OBJ_FLAG_SCROLLABLE);

    /* Checkbox */
    lv_obj_t* cb = lv_checkbox_create(box);
    lv_checkbox_set_text(cb, "I have read and agree");
    lv_obj_set_style_text_color(cb, c->text, 0);
    lv_obj_set_style_text_font(cb, CJK_FONT, 0);
    lv_obj_set_width(cb, LV_PCT(100));

    /* OK button */
    lv_obj_t* btn_ok = lv_button_create(box);
    lv_obj_set_width(btn_ok, LV_PCT(100));
    lv_obj_set_height(btn_ok, 40);
    lv_obj_set_style_bg_color(btn_ok, c->accent, 0);
    lv_obj_set_style_bg_color(btn_ok, c->accent_hover, LV_STATE_PRESSED);
    lv_obj_set_style_radius(btn_ok, SCALE(g_colors->radius_md), 0);
    lv_obj_add_event_cb(btn_ok, disclaimer_ok_cb, LV_EVENT_CLICKED, cb);
    lv_obj_t* lbl_ok = lv_label_create(btn_ok);
    lv_label_set_text(lbl_ok, "Accept");
    lv_obj_set_style_text_font(lbl_ok, CJK_FONT, 0);
    lv_obj_center(lbl_ok);

    ui_log("[Disclaimer] Showing legal disclaimer");
}

/* ═══ Startup error modal (e.g. "Already Running") ═══ */

static lv_obj_t* g_startup_error_modal = nullptr;

static void startup_error_ok_cb(lv_event_t* e) {
    (void)e;
    /* If this is a non-fatal error (e.g. Node.js warning), just close the popup */
    if (!g_startup_error_title.empty()) {
        if (g_startup_error_modal) {
            lv_obj_del(g_startup_error_modal);
            g_startup_error_modal = nullptr;
        }
        return;
    }
    PostQuitMessage(0);
}

static void show_startup_error(lv_obj_t* parent) {
    const ThemeColors* c = g_colors;
    /* FIX Bug 8: Lock before reading cross-thread strings */
    std::string local_title, local_msg;
    {
        std::lock_guard<std::mutex> lk(g_startup_error_mtx);
        local_title = g_startup_error_title;
        local_msg = g_startup_error;
    }
    if (local_msg.empty()) return;

    /* Full-screen dark overlay */
    lv_obj_t* modal = lv_obj_create(parent);
    lv_obj_set_size(modal, WIN_W, WIN_H);
    lv_obj_set_pos(modal, 0, 0);
    lv_obj_set_style_bg_color(modal, c->bg, 0);
    lv_obj_set_style_bg_opa(modal, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(modal, 0, 0);
    lv_obj_set_style_radius(modal, 0, 0);
    lv_obj_set_style_pad_all(modal, 0, 0);
    lv_obj_clear_flag(modal, LV_OBJ_FLAG_SCROLLABLE);
    g_startup_error_modal = modal;

    /* Error box — 34% × 35% of window */
    int err_w = WIN_W * 34 / 100;
    int err_h = WIN_H * 35 / 100;
    lv_obj_t* box = lv_obj_create(modal);
    lv_obj_set_size(box, err_w, err_h);
    lv_obj_set_pos(box, (WIN_W - err_w) / 2, (WIN_H - err_h) / 2);
    lv_obj_set_style_bg_color(box, c->panel, 0);
    lv_obj_set_style_bg_opa(box, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(box, 1, 0);
    lv_obj_set_style_border_color(box, c->border_strong, 0);
    lv_obj_set_style_radius(box, SCALE(g_colors->radius_lg), 0);
    lv_obj_set_style_pad_all(box, 20, 0);
    lv_obj_set_flex_flow(box, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_gap(box, 12, 0);
    lv_obj_clear_flag(box, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_event_cb(box, dialog_drag_cb, LV_EVENT_PRESSED, nullptr);
    lv_obj_add_event_cb(box, dialog_drag_cb, LV_EVENT_PRESSING, nullptr);
    lv_obj_add_event_cb(box, dialog_drag_cb, LV_EVENT_RELEASED, nullptr);

    /* Title - use custom title if set, yellow for warnings */
    lv_obj_t* lbl_title = lv_label_create(box);
    bool is_warning = !local_title.empty();
    const char* title_text = is_warning ? local_title.c_str() : "AnyClaw - Already Running";
    lv_label_set_text(lbl_title, title_text);
    lv_obj_set_style_text_color(lbl_title,
        is_warning ? c->warning : c->accent_secondary, 0);
    lv_obj_set_style_text_font(lbl_title, CJK_FONT, 0);

    /* Message */
    lv_obj_t* lbl_msg = lv_label_create(box);
    lv_label_set_text(lbl_msg, local_msg.c_str());
    lv_obj_set_style_text_color(lbl_msg, c->text, 0);
    lv_obj_set_style_text_font(lbl_msg, CJK_FONT, 0);
    lv_label_set_long_mode(lbl_msg, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(lbl_msg, LV_PCT(100));

    /* Spacer to push button down */
    lv_obj_t* spacer = lv_obj_create(box);
    lv_obj_set_size(spacer, LV_PCT(100), 1);
    lv_obj_set_style_bg_opa(spacer, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(spacer, 0, 0);
    lv_obj_clear_flag(spacer, LV_OBJ_FLAG_SCROLLABLE);

    /* OK button */
    lv_obj_t* btn_ok = lv_button_create(box);
    lv_obj_set_width(btn_ok, LV_PCT(100));
    lv_obj_set_height(btn_ok, 40);
    lv_obj_set_style_bg_color(btn_ok, c->accent, 0);
    lv_obj_set_style_bg_color(btn_ok, c->accent_hover, LV_STATE_PRESSED);
    lv_obj_set_style_radius(btn_ok, SCALE(g_colors->radius_md), 0);
    lv_obj_add_event_cb(btn_ok, startup_error_ok_cb, LV_EVENT_CLICKED, nullptr);
    lv_obj_t* lbl_ok = lv_label_create(btn_ok);
    lv_label_set_text(lbl_ok, "OK");
    lv_obj_set_style_text_font(lbl_ok, CJK_FONT, 0);
    lv_obj_center(lbl_ok);

    ui_log("[Startup] Showing error: %s", local_msg.c_str());
}

/* ═══ Exit Confirmation Dialog (LVGL modal) ═══ */

/* g_exit_dialog_modal declared as forward declaration above */

static void exit_dialog_destroy() {
    if (g_exit_dialog_modal) {
        lv_obj_del(g_exit_dialog_modal);
        g_exit_dialog_modal = nullptr;
    }
}

static void exit_dialog_confirm_cb(lv_event_t* e) {
    (void)e;
    exit_dialog_destroy();
    tray_request_quit();
}

static void exit_dialog_cancel_cb(lv_event_t* e) {
    (void)e;
    exit_dialog_destroy();
}

void ui_show_exit_dialog() {
    if (g_exit_dialog_modal) return;

    static const I18n S_EXIT_TITLE = {"Exit Confirmation", "退出确认"};
    static const I18n S_EXIT_MSG = {
        "Are you sure you want to exit AnyClaw?\n\nExiting will also stop the OpenClaw Gateway service.",
        "确定要退出 AnyClaw 吗？\n\n退出后将同时停止 OpenClaw Gateway 服务。"
    };
    static const I18n S_EXIT_BTN = {"Exit", "退出"};
    static const I18n S_CANCEL = {"Cancel", "取消"};

    lv_obj_t* box = create_dialog(lv_screen_active(), tr(S_EXIT_TITLE), 0, 0, &g_exit_dialog_modal);

    lv_obj_t* lbl_msg = lv_label_create(box);
    lv_label_set_text(lbl_msg, tr(S_EXIT_MSG));
    lv_obj_set_style_text_color(lbl_msg, g_colors->text, 0);
    lv_obj_set_style_text_font(lbl_msg, CJK_FONT, 0);
    lv_label_set_long_mode(lbl_msg, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(lbl_msg, LV_PCT(100));

    lv_obj_t* btn_row = lv_obj_create(box);
    lv_obj_set_size(btn_row, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(btn_row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(btn_row, 0, 0);
    lv_obj_set_style_pad_all(btn_row, 0, 0);
    lv_obj_clear_flag(btn_row, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_flex_flow(btn_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(btn_row, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    lv_obj_t* btn_exit = lv_button_create(btn_row);
    lv_obj_set_size(btn_exit, SCALE(80), SCALE(36));
    lv_obj_set_style_bg_color(btn_exit, g_colors->btn_close, 0);
    lv_obj_set_style_radius(btn_exit, SCALE(g_colors->radius_sm), 0);
    lv_obj_add_event_cb(btn_exit, exit_dialog_confirm_cb, LV_EVENT_CLICKED, nullptr);
    lv_obj_t* lbl_exit = lv_label_create(btn_exit);
    lv_label_set_text(lbl_exit, tr(S_EXIT_BTN));
    lv_obj_set_style_text_font(lbl_exit, CJK_FONT, 0);
    lv_obj_set_style_text_color(lbl_exit, g_colors->text_inverse, 0);
    lv_obj_center(lbl_exit);

    lv_obj_t* btn_cancel = lv_button_create(btn_row);
    lv_obj_set_size(btn_cancel, SCALE(80), SCALE(36));
    lv_obj_set_style_bg_color(btn_cancel, g_colors->btn_secondary, 0);
    lv_obj_set_style_radius(btn_cancel, SCALE(g_colors->radius_sm), 0);
    lv_obj_add_event_cb(btn_cancel, exit_dialog_cancel_cb, LV_EVENT_CLICKED, nullptr);
    lv_obj_t* lbl_cancel = lv_label_create(btn_cancel);
    lv_label_set_text(lbl_cancel, tr(S_CANCEL));
    lv_obj_set_style_text_font(lbl_cancel, CJK_FONT, 0);
    lv_obj_set_style_text_color(lbl_cancel, g_colors->text_inverse, 0);
    lv_obj_center(lbl_cancel);

    ui_log("[Exit] Showing exit confirmation dialog");
}


void ui_close_exit_dialog() {
    exit_dialog_destroy();
}

/* ═══ About dialog (LVGL) ═══ */
/* g_about_dialog declared as forward declaration above */

static void about_dialog_close_cb(lv_event_t* e) {
    (void)e;
    if (g_about_dialog) {
        lv_obj_del(g_about_dialog);
        g_about_dialog = nullptr;
    }
}

void ui_show_about_dialog() {
    if (g_about_dialog) return;
    const ThemeColors* c = g_colors;

    static const I18n S_ABOUT_TITLE = {"About", "关于"};
    static const I18n S_VERSION = {"Version v2.2.11", "版本 v2.2.11"};
    static const I18n S_SLOGAN = {"Your OpenClaw Desktop Manager", "龙虾要吃蒜蓉的 🧄🦞"};
    static const I18n S_CLOSE = {"Close", "关闭"};

    lv_obj_t* box = create_dialog(lv_screen_active(), tr(S_ABOUT_TITLE), 0, 0, &g_about_dialog);

    /* Brand title */
    lv_obj_t* lbl_title = lv_label_create(box);
    lv_label_set_text(lbl_title, "AnyClaw LVGL");
    lv_obj_set_style_text_color(lbl_title, c->accent_secondary, 0);
    lv_obj_set_style_text_font(lbl_title, CJK_FONT, 0);

    /* Version */
    lv_obj_t* lbl_ver = lv_label_create(box);
    lv_label_set_text(lbl_ver, tr(S_VERSION));
    lv_obj_set_style_text_color(lbl_ver, c->text, 0);
    lv_obj_set_style_text_font(lbl_ver, CJK_FONT, 0);

    /* Slogan */
    lv_obj_t* lbl_slogan = lv_label_create(box);
    lv_label_set_text(lbl_slogan, tr(S_SLOGAN));
    lv_obj_set_style_text_color(lbl_slogan, c->text_dim, 0);
    lv_obj_set_style_text_font(lbl_slogan, CJK_FONT, 0);
    lv_label_set_long_mode(lbl_slogan, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(lbl_slogan, LV_PCT(100));

    /* Separator */
    lv_obj_t* sep = lv_obj_create(box);
    lv_obj_set_size(sep, LV_PCT(100), 1);
    lv_obj_set_style_bg_color(sep, c->border, 0);
    lv_obj_set_style_bg_opa(sep, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(sep, 0, 0);
    lv_obj_set_style_pad_all(sep, 0, 0);
    lv_obj_clear_flag(sep, LV_OBJ_FLAG_SCROLLABLE);

    /* Tech stack */
    lv_obj_t* lbl_tech = lv_label_create(box);
    lv_label_set_text(lbl_tech, "LVGL 9.6 + SDL2  |  C++17 / Win32 API\nWindows 10/11 x64");
    lv_obj_set_style_text_color(lbl_tech, c->text_dim, 0);
    lv_obj_set_style_text_font(lbl_tech, CJK_FONT, 0);
    lv_label_set_long_mode(lbl_tech, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(lbl_tech, LV_PCT(100));

    /* Links */
    lv_obj_t* lbl_links = lv_label_create(box);
    lv_label_set_text(lbl_links, "github.com/IwanFlag/AnyClaw_LVGL");
    lv_obj_set_style_text_color(lbl_links, c->accent_secondary, 0);
    lv_obj_set_style_text_font(lbl_links, FONT(12), 0);

    /* Close button */
    lv_obj_t* btn_close = lv_button_create(box);
    lv_obj_set_size(btn_close, LV_PCT(100), SCALE(36));
    lv_obj_set_style_bg_color(btn_close, c->accent_secondary, 0);
    lv_obj_set_style_bg_color(btn_close, c->info, LV_STATE_PRESSED);
    lv_obj_set_style_radius(btn_close, SCALE(g_colors->radius_md), 0);
    lv_obj_add_event_cb(btn_close, about_dialog_close_cb, LV_EVENT_CLICKED, nullptr);
    lv_obj_t* lbl_close = lv_label_create(btn_close);
    lv_label_set_text(lbl_close, tr(S_CLOSE));
    lv_obj_set_style_text_font(lbl_close, CJK_FONT, 0);
    lv_obj_center(lbl_close);

    ui_log("[About] Showing about dialog");
}

/* ═══ License Activation Dialog ═══ */
static lv_obj_t* g_license_dialog = nullptr;
static lv_obj_t* g_license_input = nullptr;
static lv_obj_t* g_license_status = nullptr;

static void license_activate_cb(lv_event_t* e) {
    (void)e;
    const ThemeColors* c = g_colors ? g_colors : &THEME_DARK;
    if (!g_license_input) return;
    const char* key = lv_textarea_get_text(g_license_input);
    if (!key || !key[0]) {
        lv_label_set_text(g_license_status, "Please enter a license key");
        lv_obj_set_style_text_color(g_license_status, c->danger, 0);
        return;
    }

    int seq = 0, hours = 0;
    const char* error = nullptr;
    if (license_activate(key, &seq, &hours, &error)) {
        char msg[128];
        snprintf(msg, sizeof(msg), "Activated! %dh (%.1f days) remaining", hours, hours / 24.0);
        lv_label_set_text(g_license_status, msg);
        lv_obj_set_style_text_color(g_license_status, c->success, 0);

        /* Close dialog after 1.5s */
        lv_timer_create([](lv_timer_t* t) {
            if (g_license_dialog) { lv_obj_del(g_license_dialog); g_license_dialog = nullptr; }
            lv_timer_del(t);
        }, 1500, nullptr);
    } else {
        lv_label_set_text(g_license_status, error ? error : "Activation failed");
        lv_obj_set_style_text_color(g_license_status, c->danger, 0);
    }
}

static void license_dialog_close_cb(lv_event_t* e) {
    (void)e;
    /* Close button — allow closing even if expired (user can still use the app briefly) */
    if (g_license_dialog) { lv_obj_del(g_license_dialog); g_license_dialog = nullptr; }
}

void ui_show_license_dialog() {
    if (g_license_dialog) return;
    const ThemeColors* c = g_colors;

    lv_obj_t* box = create_dialog(lv_screen_active(), "License Activation", 0, 0, &g_license_dialog);

    /* Title */
    lv_obj_t* lbl_title = lv_label_create(box);
    lv_label_set_text(lbl_title, "Trial Period Ended");
    lv_obj_set_style_text_color(lbl_title, c->danger, 0);
    lv_obj_set_style_text_font(lbl_title, CJK_FONT, 0);

    /* Description */
    lv_obj_t* lbl_desc = lv_label_create(box);
    lv_label_set_text(lbl_desc, "Enter a license key to continue using AnyClaw.\nKeys are available from the developer.");
    lv_obj_set_style_text_color(lbl_desc, c->text, 0);
    lv_obj_set_style_text_font(lbl_desc, CJK_FONT, 0);
    lv_label_set_long_mode(lbl_desc, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(lbl_desc, LV_PCT(100));

    /* Machine id for HW license generation */
    char machine_id[64] = {0};
    if (!license_get_machine_id(machine_id, sizeof(machine_id))) {
        snprintf(machine_id, sizeof(machine_id), "Unavailable");
    }
    char machine_info[160];
    snprintf(machine_info, sizeof(machine_info), "Machine ID (MAC): %s", machine_id);
    lv_obj_t* lbl_machine = lv_label_create(box);
    lv_label_set_text(lbl_machine, machine_info);
    lv_obj_set_style_text_color(lbl_machine, c->text_dim, 0);
    lv_obj_set_style_text_font(lbl_machine, FONT(12), 0);

    /* Key input */
    g_license_input = lv_textarea_create(box);
    lv_textarea_set_one_line(g_license_input, true);
    lv_textarea_set_placeholder_text(g_license_input, "HW-<MAC12><SIGN12> / TM-<EXP8><SIGN12>");
    lv_obj_set_width(g_license_input, LV_PCT(100));
    lv_obj_set_style_bg_color(g_license_input, c->surface, 0);
    lv_obj_set_style_border_color(g_license_input, c->border, 0);
    lv_obj_set_style_text_font(g_license_input, FONT(14), 0);
    lv_obj_set_style_radius(g_license_input, SCALE(g_colors->radius_md), 0);

    /* Status label */
    g_license_status = lv_label_create(box);
    lv_label_set_text(g_license_status, "");
    lv_obj_set_style_text_font(g_license_status, FONT(12), 0);

    /* Buttons row */
    lv_obj_t* btn_row = lv_obj_create(box);
    lv_obj_set_size(btn_row, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(btn_row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(btn_row, 0, 0);
    lv_obj_set_style_pad_all(btn_row, 0, 0);
    lv_obj_clear_flag(btn_row, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_flex_flow(btn_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(btn_row, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_gap(btn_row, SCALE(10), 0);

    /* Activate button */
    lv_obj_t* btn_activate = lv_button_create(btn_row);
    lv_obj_set_size(btn_activate, LV_PCT(50), SCALE(36));
    lv_obj_set_style_bg_color(btn_activate, c->success, 0);
    lv_obj_set_style_bg_color(btn_activate, c->success, LV_STATE_PRESSED);
    lv_obj_set_style_radius(btn_activate, SCALE(g_colors->radius_md), 0);
    lv_obj_add_event_cb(btn_activate, license_activate_cb, LV_EVENT_CLICKED, nullptr);
    lv_obj_t* lbl_act = lv_label_create(btn_activate);
    lv_label_set_text(lbl_act, "Activate");
    lv_obj_set_style_text_font(lbl_act, CJK_FONT, 0);
    lv_obj_center(lbl_act);

    /* Close button */
    lv_obj_t* btn_close = lv_button_create(btn_row);
    lv_obj_set_size(btn_close, LV_PCT(40), SCALE(36));
    lv_obj_set_style_bg_color(btn_close, c->btn_secondary, 0);
    lv_obj_set_style_radius(btn_close, SCALE(g_colors->radius_md), 0);
    lv_obj_add_event_cb(btn_close, license_dialog_close_cb, LV_EVENT_CLICKED, nullptr);
    lv_obj_t* lbl_close = lv_label_create(btn_close);
    lv_label_set_text(lbl_close, "Close");
    lv_obj_set_style_text_font(lbl_close, CJK_FONT, 0);
    lv_obj_center(lbl_close);

    ui_log("[License] Showing activation dialog");
}

/* ═══ Apply theme colors to all UI elements ═══ */
void apply_theme_to_all() {
    const ThemeColors* c = g_colors;
    lv_obj_t* scr = lv_screen_active();

    /* Screen background */
    lv_obj_set_style_bg_color(scr, c->bg, 0);

    /* Title bar - stored as user data, iterate children */
    if (title_bar) {
        lv_obj_set_style_bg_color(title_bar, c->panel, 0);
        lv_obj_set_style_border_color(title_bar, c->border, 0);
    }

    /* Find left panel (child 3 of screen typically) */
    lv_obj_t* child;
    uint32_t i = 0;
    while ((child = lv_obj_get_child(scr, i)) != nullptr) {
        /* Skip title bar (child 0) and divider (child 1) */
        if (i == 0 || i == 1) { i++; continue; }

        /* Check if this is a panel (has bg color close to old panel) */
        /* Apply panel theme to all main panels */
        lv_color_t bg = lv_obj_get_style_bg_color(child, LV_PART_MAIN);
        lv_opa_t opa = lv_obj_get_style_bg_opa(child, LV_PART_MAIN);
        if (opa == LV_OPA_COVER) {
            /* Check if it's a panel by size */
            int w = lv_obj_get_width(child);
            int h = lv_obj_get_height(child);
            if (h == PANEL_H || h == WIN_H) {
                /* Left or right panel, or settings overlay */
                if (w == WIN_W) {
                    /* Settings overlay */
                    lv_obj_set_style_bg_color(child, c->bg, 0);
                } else {
                    lv_obj_set_style_bg_color(child, c->panel, 0);
                    lv_obj_set_style_border_color(child, c->border, 0);
                }
            }
        }
        i++;
    }

    /* Status label colors */
    if (status_label) lv_obj_set_style_text_color(status_label, c->text_dim, 0);
    if (lp_panel_title) lv_obj_set_style_text_color(lp_panel_title, c->accent, 0);
    if (lp_progress_panel) {
        lv_obj_set_style_bg_color(lp_progress_panel, c->surface, 0);
        lv_obj_set_style_border_color(lp_progress_panel, c->border, 0);
    }
    if (lp_progress_title) lv_obj_set_style_text_color(lp_progress_title, c->text, 0);
    if (lp_progress_step) lv_obj_set_style_text_color(lp_progress_step, c->text_dim, 0);
    if (lp_progress_bar) {
        lv_obj_set_style_bg_color(lp_progress_bar, c->panel, LV_PART_MAIN);
        lv_obj_set_style_bg_color(lp_progress_bar, c->btn_action, LV_PART_INDICATOR);
    }
    // version_label, port_label removed from main panel

    /* Chat area */
    if (chat_cont) {
        lv_obj_set_style_bg_opa(chat_cont, LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_width(chat_cont, 0, 0);
    }
    if (chat_input) {
        lv_obj_set_style_bg_color(chat_input, c->surface, 0);
        lv_obj_set_style_text_color(chat_input, c->text, 0);
        lv_obj_set_style_border_color(chat_input, c->border, 0);
        lv_obj_set_style_text_color(chat_input, c->text, LV_PART_SELECTED);
    }
    /* Send button theme */
    if (btn_send_widget) {
        const char* text = chat_input ? lv_textarea_get_text(chat_input) : nullptr;
        bool empty = (!text || !text[0]);
        if (empty) {
            lv_obj_set_style_bg_color(btn_send_widget, c->btn_secondary, 0);
        } else {
            lv_obj_set_style_bg_color(btn_send_widget, c->btn_action, 0);
            apply_btn_gradient(btn_send_widget);
        }
    }

    /* Splitter */
    if (splitter) {
        lv_obj_set_style_bg_color(splitter, c->border, 0);
    }

    /* Title bar text */
    if (title_label) {
        lv_obj_set_style_text_color(title_label, c->accent, 0);
    }

    /* Divider line below title bar */
    lv_obj_t* div_child = lv_obj_get_child(scr, 1);
    if (div_child) {
        lv_obj_set_style_bg_color(div_child, c->border, 0);
    }

    /* LED colors */
    /* LED color managed by status, not theme */

    /* Task list items - update text color for existing tasks */
    for (int i = 0; i < g_task_count; i++) {
        if (g_tasks[i].label) {
            lv_obj_set_style_text_color(g_tasks[i].label, c->text, 0);
        }
        if (g_tasks[i].status_lbl) {
            lv_obj_set_style_text_color(g_tasks[i].status_lbl, c->text_dim, 0);
        }
        if (g_tasks[i].widget) {
            lv_obj_set_style_bg_color(g_tasks[i].widget, c->surface, 0);
        }
    }
    if (task_empty_label) {
        lv_obj_set_style_text_color(task_empty_label, c->text_dim, 0);
    }

    /* Buttons removed — actions moved to tray menu */

    /* Upload button */
    if (btn_upload_widget) {
        lv_obj_set_style_bg_color(btn_upload_widget, c->btn_secondary, 0);
    }
    if (btn_voice_widget) {
        lv_obj_set_style_bg_color(btn_voice_widget, c->btn_secondary, 0);
    }
    if (btn_work_widget) {
        lv_obj_set_style_bg_color(btn_work_widget, c->btn_secondary, 0);
    }
    /* Search button */
    if (g_search_btn) {
        lv_obj_set_style_bg_color(g_search_btn, c->btn_secondary, 0);
    }
    /* Search bar */
    if (g_search_bar) {
        lv_obj_set_style_bg_color(g_search_bar, c->surface, 0);
        lv_obj_set_style_border_color(g_search_bar, c->btn_action, 0);
    }
    if (mode_remote_warning_bar) {
        lv_obj_set_style_bg_color(mode_remote_warning_bar, c->btn_close, 0);
    }

    /* Theme the settings panel if it exists */
    extern void ui_settings_apply_theme();
    ui_settings_apply_theme();

    /* Walk ALL children recursively to update fonts/colors */
    /* Chat bubbles — update bg colors */
    if (chat_cont) {
        uint32_t ci = 0;
        lv_obj_t* child;
        while ((child = lv_obj_get_child(chat_cont, ci)) != nullptr) {
            /* Each child is a row_wrap (100% wide) */
            lv_obj_t* inner = lv_obj_get_child(child, 0);
            if (inner) {
                /* Check if it's a user bubble (blue) or AI bubble (gray) */
                lv_color_t bg = lv_obj_get_style_bg_color(inner, LV_PART_MAIN);
                if (bg.blue > 180 && bg.red < 100) {
                    /* User bubble (blue) - keep blue */
                } else if (bg.red > 40 && bg.green > 40 && bg.blue > 50 && bg.red < 80) {
                    /* AI bubble (gray) - update to current theme */
                    lv_obj_set_style_bg_color(inner, c->surface, 0);
                }
            }
            ci++;
        }
    }

    /* Recursively theme left panel children */
    if (left_panel) {
        uint32_t li = 0;
        lv_obj_t* lchild;
        while ((lchild = lv_obj_get_child(left_panel, li)) != nullptr) {
            /* Update label text colors */
            const lv_obj_class_t* cls = lv_obj_get_class(lchild);
            if (cls == &lv_label_class) {
                lv_obj_set_style_text_color(lchild, c->text, 0);
            }
            li++;
        }
    }

    /* Refresh tray icon/menu colors */
    tray_refresh_theme();

    /* Update animation config for new theme */
    anim_theme_transition();

    /* Reload sounds for new theme */
    sound_reload_theme();

    lv_obj_invalidate(scr);
}

/* ═══ Upload menu callbacks ═══ */
struct UploadMenuCtx { lv_obj_t* menu; lv_obj_t* btn; void* hover_timer; };
static UploadMenuCtx g_upload_ctx = {nullptr, nullptr, nullptr};

static void upload_menu_hide_cb(lv_event_t* e) {
    UploadMenuCtx* ctx = (UploadMenuCtx*)lv_event_get_user_data(e);
    if (ctx && ctx->menu) lv_obj_add_flag(ctx->menu, LV_OBJ_FLAG_HIDDEN);
    if (ctx && ctx->hover_timer) { lv_timer_del((lv_timer_t*)ctx->hover_timer); ctx->hover_timer = nullptr; }
}

static void upload_hover_check_cb(lv_timer_t* t) {
    UploadMenuCtx* ctx = (UploadMenuCtx*)t->user_data;
    if (!ctx || !ctx->menu || !ctx->btn) { if (ctx) ctx->hover_timer = nullptr; return; }
    if (lv_obj_has_flag(ctx->menu, LV_OBJ_FLAG_HIDDEN)) { ctx->hover_timer = nullptr; lv_timer_del(t); return; }
    int mx, my;
    SDL_GetMouseState(&mx, &my);
    lv_area_t btn_a, menu_a;
    lv_obj_get_coords(ctx->btn, &btn_a);
    lv_obj_get_coords(ctx->menu, &menu_a);
    bool over_btn = (mx >= btn_a.x1 && mx <= btn_a.x2 && my >= btn_a.y1 && my <= btn_a.y2);
    bool over_menu = (mx >= menu_a.x1 && mx <= menu_a.x2 && my >= menu_a.y1 && my <= menu_a.y2);
    if (!over_btn && !over_menu) {
        lv_obj_add_flag(ctx->menu, LV_OBJ_FLAG_HIDDEN);
        ctx->hover_timer = nullptr;
        lv_timer_del(t);
    }
}

static void upload_menu_show_cb(lv_event_t* e) {
    UploadMenuCtx* ctx = (UploadMenuCtx*)lv_event_get_user_data(e);
    if (!ctx || !ctx->menu || !ctx->btn) return;
    /* Position menu: above button */
    lv_obj_update_layout(ctx->menu);
    lv_area_t btn_area;
    lv_obj_get_coords(ctx->btn, &btn_area);
    int menu_w = lv_obj_get_width(ctx->menu);
    int menu_h = lv_obj_get_height(ctx->menu);
    lv_obj_set_pos(ctx->menu, btn_area.x1 - menu_w + btn_area.x2 - btn_area.x1, btn_area.y1 - menu_h - 4);
    lv_obj_clear_flag(ctx->menu, LV_OBJ_FLAG_HIDDEN);
    lv_obj_move_foreground(ctx->menu);
    /* Start hover check timer */
    if (ctx->hover_timer) { lv_timer_del((lv_timer_t*)ctx->hover_timer); }
    ctx->hover_timer = lv_timer_create(upload_hover_check_cb, 80, ctx);
}

static bool is_image_path(const char* path) {
    if (!path || !path[0]) return false;
    const char* dot = strrchr(path, '.');
    if (!dot) return false;
    std::string ext(dot);
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    return ext == ".png" || ext == ".jpg" || ext == ".jpeg" || ext == ".bmp" || ext == ".gif" || ext == ".webp";
}

static const char* path_basename(const char* path) {
    if (!path) return "";
    const char* p1 = strrchr(path, '\\');
    const char* p2 = strrchr(path, '/');
    const char* p = p1 ? p1 : p2;
    if (p1 && p2) p = (p1 > p2) ? p1 : p2;
    return p ? (p + 1) : path;
}

static std::string format_bytes_u64(uint64_t n) {
    char buf[64] = {0};
    if (n < 1024ULL) { snprintf(buf, sizeof(buf), "%llu B", (unsigned long long)n); return buf; }
    if (n < 1024ULL * 1024ULL) { snprintf(buf, sizeof(buf), "%.1f KB", (double)n / 1024.0); return buf; }
    if (n < 1024ULL * 1024ULL * 1024ULL) { snprintf(buf, sizeof(buf), "%.1f MB", (double)n / 1024.0 / 1024.0); return buf; }
    snprintf(buf, sizeof(buf), "%.2f GB", (double)n / 1024.0 / 1024.0 / 1024.0);
    return buf;
}

static std::string attachment_detail_text(const char* path, bool is_dir) {
    if (!path || !path[0]) return "";
    std::error_code ec;
    if (is_dir) {
        size_t child_count = 0;
        for (const auto& entry : std::filesystem::directory_iterator(path, ec)) {
            (void)entry;
            if (!ec) child_count++;
            if (child_count >= 9999) break;
        }
        if (ec) return "Directory (unreadable)";
        char buf[64] = {0};
        snprintf(buf, sizeof(buf), "Directory, %zu entries", child_count);
        return buf;
    }
    auto sz = std::filesystem::file_size(path, ec);
    if (ec) return "File (size unavailable)";
    return std::string("File, ") + format_bytes_u64((uint64_t)sz);
}

static void open_local_path_cb(lv_event_t* e) {
    const char* path = (const char*)lv_event_get_user_data(e);
    if (!path || !path[0]) return;
    HINSTANCE r = ShellExecuteA(nullptr, "open", path, nullptr, nullptr, SW_SHOWNORMAL);
    if ((INT_PTR)r <= 32) {
        LOG_W("SHARE", "Open path failed: %s (code=%Id)", path, (INT_PTR)r);
    }
}

static void attachment_queue_timer_cb(lv_timer_t* t);

static void free_path_userdata_cb(lv_event_t* e) {
    char* path = (char*)lv_event_get_user_data(e);
    if (path) free(path);
}

static void free_retry_ctx_cb(lv_event_t* e) {
    AttachmentRetryCtx* ctx = (AttachmentRetryCtx*)lv_event_get_user_data(e);
    if (!ctx) return;
    if (ctx->path) free(ctx->path);
    delete ctx;
}

static void attachment_retry_click_cb(lv_event_t* e) {
    const ThemeColors* c = g_colors ? g_colors : &THEME_DARK;
    AttachmentRetryCtx* ctx = (AttachmentRetryCtx*)lv_event_get_user_data(e);
    if (!ctx || !ctx->status_lbl || !ctx->path || !ctx->path[0]) return;
    for (const auto& it : g_attachment_queue) {
        if (!it.done && it.status_lbl == ctx->status_lbl) return;
    }
    lv_label_set_text(ctx->status_lbl, "Queue: pending (retry)");
    lv_obj_set_style_text_color(ctx->status_lbl, c->warning, 0);
    AttachmentQueueItem item;
    item.path = ctx->path;
    item.is_dir = ctx->is_dir;
    item.status_lbl = ctx->status_lbl;
    item.done = false;
    g_attachment_queue.push_back(item);
    if (!g_attachment_queue_timer) {
        g_attachment_queue_timer = lv_timer_create(attachment_queue_timer_cb, 280, nullptr);
    }
}

static lv_obj_t* chat_add_attachment_card(const char* path, bool is_dir) {
    if (!chat_cont || !path || !path[0]) return nullptr;
    const ThemeColors* c = g_colors ? g_colors : &THEME_DARK;

    lv_obj_t* row = lv_obj_create(chat_cont);
    lv_obj_set_width(row, LV_PCT(100));
    lv_obj_set_height(row, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(row, 0, 0);
    lv_obj_set_style_pad_all(row, 0, 0);
    lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW_REVERSE);
    lv_obj_set_flex_align(row, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);

    lv_obj_t* card = lv_button_create(row);
    lv_obj_set_style_bg_color(card, c->panel, 0);
    lv_obj_set_style_bg_color(card, c->surface, LV_STATE_PRESSED);
    lv_obj_set_style_radius(card, SCALE(g_colors->radius_lg), 0);
    lv_obj_set_style_border_width(card, 1, 0);
    lv_obj_set_style_border_color(card, c->accent, 0);
    lv_obj_set_style_pad_all(card, 10, 0);
    lv_obj_set_style_pad_gap(card, 6, 0);
    lv_obj_set_size(card, LV_PCT(82), LV_SIZE_CONTENT);
    lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_flex_flow(card, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(card, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);

    const bool image_file = (!is_dir && is_image_path(path));
    const char* tag = is_dir ? "[DIR]" : (image_file ? "[IMG]" : "[FILE]");
    const char* base = path_basename(path);
    lv_obj_t* title = lv_label_create(card);
    lv_label_set_text_fmt(title, "%s %s", tag, (base && base[0]) ? base : path);
    lv_label_set_long_mode(title, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(title, LV_PCT(100));
    lv_obj_set_style_text_color(title, c->text, 0);
    lv_obj_set_style_text_font(title, CJK_FONT_SMALL, 0);

    lv_obj_t* subtitle = lv_label_create(card);
    std::string detail = attachment_detail_text(path, is_dir);
    lv_label_set_text_fmt(subtitle, "%s\n%s", detail.c_str(), path);
    lv_label_set_long_mode(subtitle, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(subtitle, LV_PCT(100));
    lv_obj_set_style_text_color(subtitle, c->text_dim, 0);
    lv_obj_set_style_text_font(subtitle, FONT(10), 0);

    if (image_file) {
        lv_obj_t* preview = lv_image_create(card);
        if (std::filesystem::exists(path)) {
            lv_image_set_src(preview, path);
            lv_obj_set_size(preview, SCALE(96), SCALE(96));
            lv_obj_set_style_radius(preview, SCALE(g_colors->radius_sm), 0);
            lv_obj_t* preview_hint = lv_label_create(card);
            lv_label_set_text(preview_hint, "Image preview (click to open original)");
            lv_obj_set_style_text_color(preview_hint, c->accent_secondary, 0);
            lv_obj_set_style_text_font(preview_hint, CJK_FONT_SMALL, 0);
        } else {
            lv_obj_delete(preview);
            lv_obj_t* preview_hint = lv_label_create(card);
            lv_label_set_text(preview_hint, "Image preview unavailable");
            lv_obj_set_style_text_color(preview_hint, c->warning, 0);
            lv_obj_set_style_text_font(preview_hint, CJK_FONT_SMALL, 0);
        }
    }

    lv_obj_t* open_tip = lv_label_create(card);
    lv_label_set_text(open_tip, "Click to open");
    lv_obj_set_style_text_color(open_tip, c->accent, 0);
    lv_obj_set_style_text_font(open_tip, CJK_FONT_SMALL, 0);

    lv_obj_t* status_tip = lv_label_create(card);
    lv_label_set_text(status_tip, "Queue: pending");
    lv_obj_set_style_text_color(status_tip, c->warning, 0);
    lv_obj_set_style_text_font(status_tip, CJK_FONT_SMALL, 0);
    lv_obj_add_flag(status_tip, LV_OBJ_FLAG_CLICKABLE);

    char* path_copy = _strdup(path);
    lv_obj_add_event_cb(card, open_local_path_cb, LV_EVENT_CLICKED, path_copy);
    lv_obj_add_event_cb(card, free_path_userdata_cb, LV_EVENT_DELETE, path_copy);
    AttachmentRetryCtx* retry_ctx = new AttachmentRetryCtx();
    retry_ctx->path = _strdup(path);
    retry_ctx->is_dir = is_dir;
    retry_ctx->status_lbl = status_tip;
    lv_obj_add_event_cb(status_tip, attachment_retry_click_cb, LV_EVENT_CLICKED, retry_ctx);
    lv_obj_add_event_cb(status_tip, free_retry_ctx_cb, LV_EVENT_DELETE, retry_ctx);
    return status_tip;
}

static void attachment_queue_timer_cb(lv_timer_t* t) {
    (void)t;
    const ThemeColors* c = g_colors ? g_colors : &THEME_DARK;
    bool has_pending = false;
    const int total = (int)g_attachment_queue.size();
    for (auto& it : g_attachment_queue) {
        if (it.done) continue;
        has_pending = true;
        DWORD t0 = GetTickCount();
        bool ok = false;
        std::error_code ec;
        if (it.is_dir) ok = std::filesystem::is_directory(it.path, ec);
        else ok = std::filesystem::is_regular_file(it.path, ec);
        DWORD elapsed = GetTickCount() - t0;
        TraceManager::instance().record_event(
            TraceManager::instance().start_trace(),
            "attachment_queue_send",
            it.path,
            (int)elapsed,
            ok ? "ok" : "fail");
        if (it.status_lbl) {
            if (ok) {
                lv_label_set_text(it.status_lbl, "Queue: sent");
                lv_obj_set_style_text_color(it.status_lbl, c->success, 0);
                g_sent_attachments.push_back({it.path, it.is_dir});
            } else {
                lv_label_set_text(it.status_lbl, "Queue: failed (click to retry)");
                lv_obj_set_style_text_color(it.status_lbl, c->danger, 0);
                g_attachment_failed_cache.push_back(it);
            }
        }
        it.done = true;
        int done_count = 0;
        for (const auto& jt : g_attachment_queue) if (jt.done) done_count++;
        int progress = (total > 0) ? ((done_count * 100) / total) : 100;
        char step[96] = {0};
        snprintf(step, sizeof(step), "Attachment queue %d/%d", done_count, total);
        ui_progress_update("Attachment Queue", step, progress);
        break; /* process one item per tick */
    }
    if (!has_pending) {
        if (g_attachment_queue_timer) {
            lv_timer_del(g_attachment_queue_timer);
            g_attachment_queue_timer = nullptr;
        }
        if (g_attachment_failed_cache.empty()) {
            ui_progress_finish("Attachment Queue", true, "Attachment queue done");
        } else {
            char msg[96] = {0};
            snprintf(msg, sizeof(msg), "Attachment queue done (%d failed)", (int)g_attachment_failed_cache.size());
            ui_progress_finish("Attachment Queue", false, msg);
            ui_log("[Share] %d attachments failed. Use \"Retry Failed Attachments\" in Work mode.", (int)g_attachment_failed_cache.size());
        }
        g_attachment_queue.clear();
    }
}

static void enqueue_attachment_card(const char* path, bool is_dir) {
    bool was_empty = g_attachment_queue.empty();
    lv_obj_t* status_lbl = chat_add_attachment_card(path, is_dir);
    AttachmentQueueItem item;
    item.path = path ? path : "";
    item.is_dir = is_dir;
    item.status_lbl = status_lbl;
    item.done = false;
    g_attachment_queue.push_back(item);
    if (was_empty) {
        ui_progress_begin("Attachment Queue", "Queued first attachment", 0);
    }
    if (!g_attachment_queue_timer) {
        g_attachment_queue_timer = lv_timer_create(attachment_queue_timer_cb, 280, nullptr);
    }
}

/* Upload menu click callbacks */
static void upload_file_click_cb(lv_event_t* e) {
    upload_menu_hide_cb(e);
    if (!ask_mode_confirm_action("Select and send local files as attachments", "Proceed to open file chooser")) return;
    /* Win32 file open dialog */
    OPENFILENAMEA ofn;
    char file_buf[8192] = {0};
    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.lpstrFile = file_buf;
    ofn.nMaxFile = (DWORD)sizeof(file_buf);
    ofn.lpstrFilter = "All Files\0*.*\0";
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_ALLOWMULTISELECT | OFN_EXPLORER;
    if (GetOpenFileNameA(&ofn)) {
        const char* dir_or_file = file_buf;
        const char* next = dir_or_file + strlen(dir_or_file) + 1;
        if (!next[0]) {
            /* Single file selected (full path returned). */
            ui_log("[Upload] Selected file: %s", dir_or_file);
            enqueue_attachment_card(dir_or_file, false);
        } else {
            /* Multi-select: first token is directory, followed by file names. */
            std::string base_dir = dir_or_file;
            int count = 0;
            while (next[0]) {
                std::string full = base_dir + "\\" + next;
                ui_log("[Upload] Selected file: %s", full.c_str());
                enqueue_attachment_card(full.c_str(), false);
                count++;
                next += strlen(next) + 1;
            }
            ui_log("[Upload] Batch select count=%d", count);
        }
        chat_force_scroll_bottom();
    }
}
static void upload_dir_click_cb(lv_event_t* e) {
    upload_menu_hide_cb(e);
    if (!ask_mode_confirm_action("Select and send local directory as attachment", "Proceed to open directory chooser")) return;
    /* Win32 folder browse dialog */
    BROWSEINFOA bi = {0};
    bi.lpszTitle = "Select Directory";
    bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;
    LPITEMIDLIST pidl = SHBrowseForFolderA(&bi);
    if (pidl) {
        char dir_buf[MAX_PATH];
        if (SHGetPathFromIDListA(pidl, dir_buf)) {
            ui_log("[Upload] Selected dir: %s", dir_buf);
            enqueue_attachment_card(dir_buf, true);
            chat_force_scroll_bottom();
        }
        CoTaskMemFree(pidl);
    }
}

/* ═══ Create title bar LAST so it's above all panels (z-order fix) ═══ */
static void create_title_bar(lv_obj_t* scr) {
    const ThemeColors* c = g_colors;

    title_bar = lv_obj_create(scr);
    lv_obj_set_size(title_bar, lv_obj_get_width(scr), TITLE_H);
    lv_obj_set_pos(title_bar, 0, 0);
    lv_obj_set_style_bg_color(title_bar, c->panel, 0);
    lv_obj_set_style_bg_opa(title_bar, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(title_bar, 0, 0);
    lv_obj_set_style_radius(title_bar, 0, 0);
    lv_obj_set_style_pad_all(title_bar, 0, 0);
    lv_obj_clear_flag(title_bar, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_event_cb(title_bar, title_drag_cb, LV_EVENT_PRESSING, nullptr);

    /* Title bar icon (32x32) */
    lv_obj_t* title_icon = lv_image_create(title_bar);
    lv_image_set_src(title_icon, "A:assets/garlic_48.png");
    lv_image_set_scale(title_icon, 256); /* 100% */
    lv_obj_align(title_icon, LV_ALIGN_LEFT_MID, 8, 0);
    lv_obj_add_flag(title_icon, LV_OBJ_FLAG_EVENT_BUBBLE); /* Pass events to title_bar for drag */

    title_label = lv_label_create(title_bar);
    lv_label_set_text(title_label, tr_title(STR_TITLE));
    lv_obj_set_style_text_color(title_label, c->text, 0);
    lv_obj_set_style_text_font(title_label, CJK_FONT, 0);
    lv_obj_align(title_label, LV_ALIGN_LEFT_MID, 48, 0);

    /* EN language toggle button removed - default language is English */

    /* ═══ Window control buttons (top-right of title bar) ═══ */
    {
        int wc_btn_size = SCALE(28);  /* Smaller square buttons */
        int wc_btn_h = wc_btn_size;
        int wc_btn_gap = 6;
        int wc_btn_margin = 10;  /* Gap from panel border */
        int wc_btn_y = (TITLE_H - wc_btn_h) / 2;
        int wc_base_x = WIN_W - wc_btn_size * 3 - wc_btn_gap * 2 - wc_btn_margin;
        /* Clamp: ensure buttons fit within title bar */
        {
            int title_w = (int)lv_display_get_horizontal_resolution(NULL);
            if (title_w > 0 && wc_base_x + wc_btn_size * 3 + wc_btn_gap * 2 + wc_btn_margin > title_w) {
                wc_base_x = title_w - wc_btn_size * 3 - wc_btn_gap * 2 - wc_btn_margin;
            }
        }

        int top_y = wc_btn_y;

        /* Status LED in title bar */
        int led_sz = TITLE_H * TITLE_LED_PCT / 100;
        led_ok = lv_led_create(title_bar);
        lv_obj_set_size(led_ok, led_sz, led_sz);
        lv_led_set_color(led_ok, g_colors->warning);
        lv_led_off(led_ok);
        int led_x = wc_base_x - SCALE(6) - led_sz;
        lv_obj_set_pos(led_ok, led_x, top_y + (wc_btn_h - led_sz) / 2);

        /* Model name (short) */
        {
            char gw_model_buf[128] = {0};
            app_get_current_model(gw_model_buf, sizeof(gw_model_buf));
            if (gw_model_buf[0]) {
                const char* short_name = gw_model_buf;
                const char* last_slash = strrchr(gw_model_buf, '/');
                if (last_slash && strlen(last_slash + 1) > 3) short_name = last_slash + 1;
                else {
                    const char* last_dot = strrchr(gw_model_buf, '.');
                    if (last_dot && strlen(last_dot + 1) > 3) short_name = last_dot + 1;
                }
                int model_w = WIN_W * TITLE_MODEL_W_PCT / 100;
                lv_obj_t* title_model = lv_label_create(title_bar);
                lv_label_set_text(title_model, short_name);
                lv_obj_set_style_text_color(title_model, g_colors->text_dim, 0);
                lv_obj_set_style_text_font(title_model, FONT(10), 0);
                int model_x = led_x - SCALE(6) - model_w;
                lv_obj_set_pos(title_model, model_x, top_y + 4);
                lv_obj_set_width(title_model, model_w - SCALE(8));
                lv_label_set_long_mode(title_model, LV_LABEL_LONG_MODE_DOTS);
            }
        }

        /* Minimize button - 灰色 */
        btn_minimize = lv_button_create(title_bar);
        lv_obj_set_size(btn_minimize, wc_btn_size, wc_btn_h);
        lv_obj_set_pos(btn_minimize, wc_base_x, wc_btn_y);
        lv_obj_set_style_bg_color(btn_minimize, c->btn_secondary, 0);
        lv_obj_set_style_bg_opa(btn_minimize, LV_OPA_COVER, 0);
        lv_obj_set_style_radius(btn_minimize, SCALE(g_colors->radius_sm), 0);
        lv_obj_set_style_border_width(btn_minimize, 1, 0);
        lv_obj_set_style_border_color(btn_minimize, c->border, 0);
        lv_obj_clear_flag(btn_minimize, LV_OBJ_FLAG_CLICK_FOCUSABLE);
        lv_obj_clear_flag(btn_minimize, LV_OBJ_FLAG_EVENT_BUBBLE);
        lv_obj_set_ext_click_area(btn_minimize, SCALE(6));
        /* Stop event propagation so title bar drag handler doesn't swallow clicks */
        lv_obj_add_event_cb(btn_minimize, [](lv_event_t* e){ lv_event_stop_processing(e); }, LV_EVENT_PRESSED, nullptr);
        lv_obj_add_event_cb(btn_minimize, [](lv_event_t* e){ lv_event_stop_processing(e); }, LV_EVENT_CLICKED, nullptr);
        lv_obj_add_event_cb(btn_minimize, btn_minimize_cb, LV_EVENT_CLICKED, nullptr);
        lv_obj_t* lbl_min = lv_label_create(btn_minimize);
        lv_label_set_text(lbl_min, "-");
        lv_obj_set_style_text_font(lbl_min, CJK_FONT, 0);
        lv_obj_center(lbl_min);

        /* Maximize/Restore button - 蓝色 */
        btn_maximize = lv_button_create(title_bar);
        lv_obj_set_size(btn_maximize, wc_btn_size, wc_btn_h);
        lv_obj_set_pos(btn_maximize, wc_base_x + wc_btn_size + wc_btn_gap, wc_btn_y);
        lv_obj_set_style_bg_color(btn_maximize, c->btn_secondary, 0);
        lv_obj_set_style_bg_opa(btn_maximize, LV_OPA_COVER, 0);
        lv_obj_set_style_radius(btn_maximize, SCALE(g_colors->radius_sm), 0);
        lv_obj_set_style_border_width(btn_maximize, 1, 0);
        lv_obj_set_style_border_color(btn_maximize, c->border, 0);
        lv_obj_clear_flag(btn_maximize, LV_OBJ_FLAG_CLICK_FOCUSABLE);
        lv_obj_clear_flag(btn_maximize, LV_OBJ_FLAG_EVENT_BUBBLE);
        lv_obj_set_ext_click_area(btn_maximize, SCALE(6));
        lv_obj_add_event_cb(btn_maximize, [](lv_event_t* e){ lv_event_stop_processing(e); }, LV_EVENT_PRESSED, nullptr);
        lv_obj_add_event_cb(btn_maximize, [](lv_event_t* e){ lv_event_stop_processing(e); }, LV_EVENT_CLICKED, nullptr);
        lv_obj_add_event_cb(btn_maximize, btn_maximize_cb, LV_EVENT_CLICKED, nullptr);
        lbl_maximize = lv_label_create(btn_maximize);
        lv_label_set_text(lbl_maximize, "[]");
        lv_obj_set_style_text_font(lbl_maximize, CJK_FONT, 0);
        lv_obj_center(lbl_maximize);

        /* Close button - 红色 */
        btn_close = lv_button_create(title_bar);
        lv_obj_set_size(btn_close, wc_btn_size, wc_btn_h);
        lv_obj_set_pos(btn_close, wc_base_x + (wc_btn_size + wc_btn_gap) * 2, wc_btn_y);
        lv_obj_set_style_bg_color(btn_close, c->btn_close, 0);
        lv_obj_set_style_bg_opa(btn_close, LV_OPA_COVER, 0);
        lv_obj_set_style_radius(btn_close, SCALE(g_colors->radius_sm), 0);
        lv_obj_set_style_border_width(btn_close, 1, 0);
        lv_obj_set_style_border_color(btn_close, c->border, 0);
        lv_obj_clear_flag(btn_close, LV_OBJ_FLAG_CLICK_FOCUSABLE);
        lv_obj_clear_flag(btn_close, LV_OBJ_FLAG_EVENT_BUBBLE);
        lv_obj_set_ext_click_area(btn_close, SCALE(6));
        lv_obj_add_event_cb(btn_close, [](lv_event_t* e){ lv_event_stop_processing(e); }, LV_EVENT_PRESSED, nullptr);
        lv_obj_add_event_cb(btn_close, [](lv_event_t* e){ lv_event_stop_processing(e); }, LV_EVENT_CLICKED, nullptr);
        lv_obj_add_event_cb(btn_close, btn_close_cb, LV_EVENT_CLICKED, nullptr);
        lv_obj_t* lbl_cls = lv_label_create(btn_close);
        lv_label_set_text(lbl_cls, "X");
        lv_obj_set_style_text_font(lbl_cls, CJK_FONT, 0);
        lv_obj_center(lbl_cls);

        printf("[UI] Window controls at x=%d, y=%d (title bar)\n", wc_base_x, wc_btn_y);
    }

    ui_log("[UI] Title bar created (z-order: top, width: %d)", WIN_W - 14);
}

/* ═══════════════════════════════════════════════════════════════
 *  Setup Wizard — First-launch Phase A 4-step onboarding
 * ═══════════════════════════════════════════════════════════════ */

#define WIZARD_STEPS 4
static lv_obj_t* g_wizard_modal = nullptr;
static lv_obj_t* g_wizard_box = nullptr;
static lv_obj_t* g_wizard_step_label = nullptr;
static lv_obj_t* g_wizard_title = nullptr;
static lv_obj_t* g_wizard_content = nullptr;
static lv_obj_t* g_wizard_btn_prev = nullptr;
static lv_obj_t* g_wizard_btn_next = nullptr;
static lv_obj_t* g_wizard_btn_close = nullptr;
static int g_wizard_step = 0;

/* Wizard selections */
static int g_wizard_lang_sel = (g_lang == Lang::CN) ? 0 : 1;  /* 0=CN, 1=EN, matches system lang */
static int g_wizard_openclaw_ok = 0;     /* detection result */
static bool g_wizard_hermes_ok = false;
static bool g_wizard_hermes_healthy = false;
static bool g_wizard_claude_ok = false;
static bool g_wizard_claude_healthy = false;
static char g_wizard_hermes_ver[64] = {0};
static char g_wizard_claude_ver[64] = {0};
static bool g_wizard_oc_installed_now = false; /* OC was installed during this wizard session */
static bool g_wizard_session_finished = false;
static char g_wizard_api_key[256] = {0};
static int g_wizard_model_sel = 0;
static char g_wizard_model_name[128] = {0};  /* Stored model name string (survives dropdown deletion) */
static char g_wizard_nickname[128] = {0};
static int g_wizard_tz_sel = 4;          /* timezone index, default Asia/Shanghai */
static bool g_wiz_im_tg_connected = false;
static bool g_wiz_im_discord_connected = false;
static bool g_wiz_im_whatsapp_connected = false;
static char g_wiz_im_tg_token[256] = {0};
static char g_wiz_im_discord_token[256] = {0};
static lv_obj_t* g_wiz_im_tg_status = nullptr;
static lv_obj_t* g_wiz_im_discord_status = nullptr;
static lv_obj_t* g_wiz_im_whatsapp_status = nullptr;
static lv_obj_t* g_wiz_im_token_overlay = nullptr;
static lv_obj_t* g_wiz_im_token_ta = nullptr;
static char* g_wiz_im_active_token = nullptr;
static size_t g_wiz_im_active_token_cap = 0;
static bool* g_wiz_im_active_connected = nullptr;

/* Widget refs per step */
static lv_obj_t* g_wiz_btn_cn = nullptr;
static lv_obj_t* g_wiz_btn_en = nullptr;
static lv_obj_t* g_wiz_detect_lbl = nullptr;
static lv_obj_t* g_wiz_api_ta = nullptr;
static lv_obj_t* g_wiz_model_dd = nullptr;
static lv_obj_t* g_wiz_leader_sw = nullptr;
static lv_obj_t* g_wiz_runtime_dd = nullptr;
static lv_obj_t* g_wiz_nick_ta = nullptr;
static lv_obj_t* g_wiz_tz_dd = nullptr;
static lv_obj_t* g_wiz_summary_lbl = nullptr;
static lv_obj_t* g_wiz_btn_install_net = nullptr;
static lv_obj_t* g_wiz_btn_install_local = nullptr;
static lv_obj_t* g_wiz_btn_cancel_install = nullptr;
static lv_obj_t* g_wiz_install_active_label = nullptr;
static lv_timer_t* g_wiz_install_poll_timer = nullptr;
static lv_timer_t* g_wiz_detect_poll_timer = nullptr;
static std::atomic<bool> g_wiz_install_running(false);
static std::atomic<bool> g_wiz_install_result_ready(false);
static std::atomic<bool> g_wiz_detect_running(false);
static std::atomic<bool> g_wiz_detect_result_ready(false);
static bool g_wiz_install_result_ok = false;
static char g_wiz_install_result_msg[512] = {0};
static EnvCheckResult g_wiz_detect_env{};
static bool g_wiz_detect_net_ok = false;
static lv_obj_t* g_wiz_install_progress_panel = nullptr;
static lv_obj_t* g_wiz_install_progress_task = nullptr;
static lv_obj_t* g_wiz_install_progress_step = nullptr;
static lv_obj_t* g_wiz_install_progress_result = nullptr;
static lv_obj_t* g_wiz_install_progress_bar = nullptr;
static lv_obj_t* g_wiz_install_progress_source = nullptr;
static lv_obj_t* g_wiz_install_progress_speed = nullptr;
static lv_obj_t* g_wiz_install_progress_eta = nullptr;
static lv_obj_t* g_wiz_install_progress_cancel_btn = nullptr;
/* Offline package download state */
static std::atomic<bool> g_wiz_dl_running(false);
static lv_obj_t* g_wiz_dl_node_btn = nullptr;
static lv_obj_t* g_wiz_dl_oc_btn = nullptr;

/* Step indicator bar widgets */
static lv_obj_t* g_wizard_step_bar = nullptr;
static lv_obj_t* g_wizard_step_dots[WIZARD_STEPS] = {nullptr};
static lv_obj_t* g_wizard_step_labels[WIZARD_STEPS] = {nullptr};
static lv_obj_t* g_wizard_step_lines[WIZARD_STEPS - 1] = {nullptr};

static const I18n wizard_step_short_names[WIZARD_STEPS] = {
    {"A-Language", "A-语言"},
    {"A-Env", "A-环境"},
    {"A-Model", "A-模型"},
    {"A-Complete", "A-完成"}
};

static void wizard_update_step_bar(int current_step);

static void wizard_progress_mirror_from_event(const PendingProgressEvent& ev, int pct) {
    if (!g_wizard_modal || !g_wiz_install_progress_panel || !g_wiz_install_progress_bar) return;
    bool is_setup_task = (strstr(ev.task, "Setup") || strstr(ev.task, "Node.js"));
    bool is_gemma_task = strstr(ev.task, "Gemma") != nullptr;
    if (!is_setup_task && !is_gemma_task) return;
    if ((is_setup_task && g_wizard_step != 1) || (is_gemma_task && g_wizard_step != 3)) return;
    lv_obj_clear_flag(g_wiz_install_progress_panel, LV_OBJ_FLAG_HIDDEN);
    if (g_wiz_install_progress_task) {
        lv_label_set_text_fmt(g_wiz_install_progress_task, "Task: %s", ev.task[0] ? ev.task : "Setup");
    }
    if (g_wiz_install_progress_step) {
        lv_label_set_text_fmt(g_wiz_install_progress_step, "Step: %s", ev.step[0] ? ev.step : "Running...");
    }
    lv_bar_set_value(g_wiz_install_progress_bar, pct, LV_ANIM_ON);
    if (g_wiz_install_progress_result) {
        if (ev.type == 1) {
            lv_label_set_text(g_wiz_install_progress_result, ev.result[0] ? ev.result : (ev.ok ? "Done" : "Failed"));
            lv_obj_set_style_text_color(g_wiz_install_progress_result, ev.ok ? g_colors->success : g_colors->danger, 0);
        } else {
            lv_label_set_text(g_wiz_install_progress_result, "");
        }
    }
    if (is_gemma_task) {
        if (g_wiz_gemma_progress_start_ms == 0 || pct <= 5) g_wiz_gemma_progress_start_ms = lv_tick_get();
        const char* src = "Source: Auto fallback";
        if (strstr(ev.step, "mirror") || strstr(ev.step, "Mirror")) src = "Source: Mirror";
        else if (strstr(ev.step, "ModelScope")) src = "Source: ModelScope";
        else if (strstr(ev.step, "HuggingFace")) src = "Source: HuggingFace";
        else if (strstr(ev.step, "official")) src = "Source: Official";
        if (g_wiz_install_progress_source) lv_label_set_text(g_wiz_install_progress_source, src);

        if (g_wiz_install_progress_speed || g_wiz_install_progress_eta) {
            uint32_t now = lv_tick_get();
            float elapsed_s = (now - g_wiz_gemma_progress_start_ms) / 1000.0f;
            float rate = (elapsed_s > 0.1f) ? ((float)pct / elapsed_s) : 0.0f;
            char speed_buf[96] = {0};
            char eta_buf[96] = {0};
            if (rate > 0.01f && pct > 0 && pct < 100) {
                float eta = (100.0f - (float)pct) / rate;
                snprintf(speed_buf, sizeof(speed_buf), "Speed: %.1f%%/s", rate);
                snprintf(eta_buf, sizeof(eta_buf), "ETA: %.0fs", eta);
            } else {
                snprintf(speed_buf, sizeof(speed_buf), "Speed: estimating...");
                snprintf(eta_buf, sizeof(eta_buf), "ETA: calculating...");
            }
            if (g_wiz_install_progress_speed) lv_label_set_text(g_wiz_install_progress_speed, speed_buf);
            if (g_wiz_install_progress_eta) lv_label_set_text(g_wiz_install_progress_eta, eta_buf);
        }
        if (g_wiz_install_progress_cancel_btn) {
            if (ev.type == 1) lv_obj_add_state(g_wiz_install_progress_cancel_btn, LV_STATE_DISABLED);
            else lv_obj_clear_state(g_wiz_install_progress_cancel_btn, LV_STATE_DISABLED);
        }
    }
}

/* Config: check wizard_completed */
static bool is_wizard_completed() {
    std::ifstream f(get_config_path());
    if (!f.is_open()) return false;
    std::string content((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
    f.close();
    /* FIX: Use precise substring to avoid false positives from other "true" values */
    return content.find("\"wizard_completed\": true") != std::string::npos;
}

static void save_wizard_completed() {
    g_wizard_completed = true;
    /* FIX Bug 1+2: Delegate to save_theme_config() which already serializes
     * wizard_completed correctly. No more fragile manual JSON editing. */
    save_theme_config();
}

/* Step title strings */
static const I18n wizard_step_titles[WIZARD_STEPS] = {
    {"Phase A · Language", "阶段A · 语言"},                         /* Step 0 */
    {"Phase A · OpenClaw Detection", "阶段A · OpenClaw 检测"},       /* Step 1 */
    {"Phase A · Model & API Key", "阶段A · 模型 & API 密钥"},        /* Step 2 */
    {"Phase A · Complete", "阶段A · 完成"}                          /* Step 3 */
};

/* Wizard text strings — bilingual */
static const I18n W_LANG_HINT    = {"Please select your language:", "请选择你的语言："};
static const I18n W_DETECT_HINT  = {"Detecting OpenClaw installation...", "正在检测 OpenClaw 安装..."};
static const I18n W_RUNNING      = {"Running", "运行中"};
static const I18n W_DETECTED     = {"Detected", "已检测"};
static const I18n W_ERROR        = {"Error", "错误"};
static const I18n W_NOTFOUND     = {"Not Found", "未找到"};
static const I18n W_VERSION      = {"Version:", "版本："};
static const I18n W_GW_PORT      = {"Gateway Port:", "Gateway 端口："};
static const I18n W_MODEL_HINT   = {"Select a model:", "选择一个模型："};
static const I18n W_API_HINT     = {"Enter your API key (required):", "输入你的 API 密钥（必填）："};
static const I18n W_API_GET_OR   = {"Get your key at openrouter.ai/settings/keys", "在 openrouter.ai/settings/keys 获取密钥"};
static const I18n W_API_GET_XM   = {"Get your key at api.xiaomimimo.com", "在 api.xiaomimimo.com 获取密钥"};
static const I18n W_PROVIDER_IS  = {"Provider:", "Provider:"};
static const I18n W_NODE_OK      = {"Node.js %s ✓", "Node.js %s ✓"};
static const I18n W_NODE_OLD     = {"Node.js %s (too old, need >= v22.14)", "Node.js %s（版本过低，需 >= v22.14）"};
static const I18n W_NODE_FAIL    = {"Node.js not found", "未找到 Node.js"};
static const I18n W_NODE_DL      = {"Download:", "下载地址："};
static const I18n W_NPM_OK       = {"npm %s ✓", "npm %s ✓"};
static const I18n W_NPM_FAIL     = {"npm not found", "未找到 npm"};
static const I18n W_NET_OK       = {"Network: OK ✓", "网络：正常 ✓"};
static const I18n W_NET_FAIL     = {"Network: Unreachable", "网络不可达"};
static const I18n W_OC_OK        = {"OpenClaw %s ✓", "OpenClaw %s ✓"};
static const I18n W_OC_NOT_INST  = {"OpenClaw not installed", "未安装 OpenClaw"};
static const I18n W_GW_OK        = {"Gateway: running ✓", "Gateway: 运行中 ✓"};
static const I18n W_GW_DOWN      = {"Gateway: not running", "Gateway: 未运行"};
static const I18n W_GW_INIT_BTN  = {"Initialize Gateway", "初始化 Gateway"};
static const I18n W_GW_INITING   = {"Initializing gateway...", "正在初始化 Gateway..."};
static const I18n W_GW_INIT_OK   = {"Gateway initialized ✓", "Gateway 初始化完成 ✓"};
static const I18n W_GW_INIT_FAIL = {"Gateway init failed", "Gateway 初始化失败"};
static const I18n W_INSTALL_NET  = {"Download from network (recommended)", "从网络下载（推荐）"};
static const I18n W_INSTALL_LOCAL= {"Use bundled package (offline)", "使用本地包（离线）"};
static const I18n W_INSTALLING   = {"Installing... (please wait)", "正在安装...（请稍候）"};
static const I18n W_INSTALLED    = {"Installed! Re-checking...", "安装完成！重新检测..."};
static const I18n W_INSTALL_FAIL = {"Install failed", "安装失败"};
static const I18n W_CHOOSE_INST  = {"Choose installation method:", "选择安装方式："};
static const I18n W_INIT_GATEWAY = {"Initializing gateway...", "正在初始化 Gateway..."};
static const I18n W_VERIFYING    = {"Verifying installation...", "正在验证安装..."};
static const I18n W_SETUP_OK     = {"Setup complete! OpenClaw is ready.", "安装完成！OpenClaw 已就绪。"};
static const I18n W_SETUP_FAIL   = {"Setup failed. Check logs for details.", "安装失败，请查看日志。"};
static const I18n W_LEADER_HINT  = {"Choose whether to enable Leader mode and preferred runtime.", "请选择是否开启 Leader 模式及默认运行时。"};
static const I18n W_LEADER_NOTE  = {"Skip keeps single-agent mode (OpenClaw only).", "Skip 将降级为单 Agent 模式（仅 OpenClaw）。"};
static const I18n W_AGENT_HINT   = {"Optional: install and check Hermes / Claude runtimes.", "可选：安装并检测 Hermes / Claude 运行时。"};
static const I18n W_LEADER_SKIP  = {"Skip Leader (single-agent)", "跳过 Leader（单 Agent）"};
static const I18n W_NICK_HINT    = {"Enter your nickname:", "输入你的昵称："};
static const I18n W_TZ_HINT      = {"Timezone:", "时区："};
static const I18n W_SUMMARY      = {"Configuration Summary:", "配置摘要："};
static const I18n W_LANG_LABEL   = {"Language:", "语言："};
static const I18n W_STATUS_LABEL = {"Status:", "状态："};
static const I18n W_MODEL_LABEL  = {"Model:", "模型："};
static const I18n W_NAME_LABEL   = {"Name:", "名称："};
static const I18n W_STEP_FMT     = {"Step %d/%d", "第 %d/%d 步"};
static const I18n W_GET_STARTED  = {"Get Started", "开始使用"};
static const I18n W_NEXT         = {"Next >", "下一步 >"};
static const I18n W_BACK         = {"< Back", "< 上一步"};
static const I18n W_CN           = {"中文", "中文"};
static const I18n W_EN           = {"English", "English"};
static const I18n W_ANONYMOUS    = {"(anonymous)", "（匿名）"};
static const I18n W_NOTSET       = {"(not set)", "（未设置）"};
static const I18n W_OC_LABEL     = {"OpenClaw:", "OpenClaw："};
static const I18n W_APIK_LABEL   = {"API Key:", "API 密钥："};
static const I18n W_NICK_PH      = {"Your nickname...", "你的昵称..."};
static const I18n W_NONE         = {"(none)", "（无）"};
static const I18n W_FINAL_HINT   = {
    "Final step: review your setup and enter AnyClaw. Advanced features are optional and can be configured later in C3.",
    "最后一步：确认当前配置并进入 AnyClaw。进阶能力是可选项，可稍后在 C3 中配置。"
};
static const I18n W_STAGEB_HINT  = {
    "Stage B recommendations (non-blocking): Local Models, IM Connect, Leader/Runtime management.",
    "阶段B 推荐（非阻断）：本地模型、IM 连接、Leader/Runtime 管理。"
};

static void wizard_update_step();
static void wizard_close_cb(lv_event_t* e);
static void wizard_set_next_enabled(bool enabled);
static void wizard_gemma_skip_cb(lv_event_t* e);
static void wizard_leader_skip_cb(lv_event_t* e);
static void wizard_install_hermes_cb(lv_event_t* e);
static void wizard_install_claude_cb(lv_event_t* e);
static void wizard_build_step_im();

/* Navigate to a specific step */
static void wizard_go_step(int step) {
    if (step < 0 || step >= WIZARD_STEPS) return;
    g_wizard_step = step;
    wizard_update_step();
}

static void wizard_set_next_enabled(bool enabled) {
    if (!g_wizard_btn_next) return;
    if (enabled) {
        lv_obj_clear_state(g_wizard_btn_next, LV_STATE_DISABLED);
        lv_obj_set_style_bg_opa(g_wizard_btn_next, LV_OPA_COVER, 0);
        if (g_wizard_step == WIZARD_STEPS - 1) {
            lv_obj_set_style_bg_color(g_wizard_btn_next, g_colors->success, 0);
        } else {
            lv_obj_set_style_bg_color(g_wizard_btn_next, g_colors->accent_secondary, 0);
        }
    } else {
        lv_obj_add_state(g_wizard_btn_next, LV_STATE_DISABLED);
        lv_obj_set_style_bg_color(g_wizard_btn_next, g_colors->disabled_bg, 0);
        lv_obj_set_style_bg_opa(g_wizard_btn_next, LV_OPA_50, 0);
    }
}

/* ── Step 0: Language ── */
static void wiz_lang_cn_cb(lv_event_t* e) {
    (void)e;
    g_wizard_lang_sel = 0;
    g_lang = Lang::CN;
    if (g_wiz_btn_cn) lv_obj_set_style_bg_color(g_wiz_btn_cn, g_colors->accent_secondary, 0);
    if (g_wiz_btn_en) lv_obj_set_style_bg_color(g_wiz_btn_en, g_colors->disabled_bg, 0);
}
static void wiz_lang_en_cb(lv_event_t* e) {
    (void)e;
    g_wizard_lang_sel = 1;
    g_lang = Lang::EN;
    if (g_wiz_btn_en) lv_obj_set_style_bg_color(g_wiz_btn_en, g_colors->accent_secondary, 0);
    if (g_wiz_btn_cn) lv_obj_set_style_bg_color(g_wiz_btn_cn, g_colors->disabled_bg, 0);
}

static void wizard_build_step_lang() {
    static const I18n S_HINT = {"Please select your language:", "请选择你的语言："};
    lv_obj_t* hint = lv_label_create(g_wizard_content);
    lv_label_set_text(hint, tr(S_HINT));
    lv_obj_set_style_text_color(hint, g_colors->text_dim, 0);
    lv_obj_set_style_text_font(hint, CJK_FONT, 0);

    lv_obj_t* row = lv_obj_create(g_wizard_content);
    lv_obj_set_size(row, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(row, 0, 0);
    lv_obj_set_style_pad_all(row, 0, 0);
    lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_gap(row, 16, 0);
    lv_obj_set_flex_align(row, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    g_wiz_btn_cn = lv_button_create(row);
    lv_obj_set_size(g_wiz_btn_cn, SCALE(140), SCALE(44));
    lv_obj_set_style_bg_color(g_wiz_btn_cn, (g_wizard_lang_sel == 0) ? g_colors->accent_secondary : g_colors->disabled_bg, 0);
    lv_obj_set_style_radius(g_wiz_btn_cn, SCALE(g_colors->radius_md), 0);
    lv_obj_add_event_cb(g_wiz_btn_cn, wiz_lang_cn_cb, LV_EVENT_CLICKED, nullptr);
    lv_obj_t* lbl_cn = lv_label_create(g_wiz_btn_cn);
    lv_label_set_text(lbl_cn, "中文");
    lv_obj_set_style_text_font(lbl_cn, CJK_FONT, 0);
    lv_obj_center(lbl_cn);

    g_wiz_btn_en = lv_button_create(row);
    lv_obj_set_size(g_wiz_btn_en, SCALE(140), SCALE(44));
    lv_obj_set_style_bg_color(g_wiz_btn_en, (g_wizard_lang_sel == 1) ? g_colors->accent_secondary : g_colors->disabled_bg, 0);
    lv_obj_set_style_radius(g_wiz_btn_en, SCALE(g_colors->radius_md), 0);
    lv_obj_add_event_cb(g_wiz_btn_en, wiz_lang_en_cb, LV_EVENT_CLICKED, nullptr);
    lv_obj_t* lbl_en = lv_label_create(g_wiz_btn_en);
    lv_label_set_text(lbl_en, "English");
    lv_obj_set_style_text_font(lbl_en, CJK_FONT, 0);
    lv_obj_center(lbl_en);
}

/* ── Step 1: OpenClaw Detection ── */
/* ── Helper: add a LED + label status row ── */
static lv_obj_t* wizard_add_status_row(lv_obj_t* parent, const char* text, lv_color_t color) {
    lv_obj_t* row = lv_obj_create(parent);
    lv_obj_set_size(row, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(row, 0, 0);
    lv_obj_set_style_pad_all(row, 0, 0);
    lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_gap(row, 8, 0);
    lv_obj_set_flex_align(row, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    lv_obj_t* led = lv_led_create(row);
    lv_obj_set_size(led, 12, 12);
    lv_led_set_color(led, color);
    lv_led_on(led);

    lv_obj_t* lbl = lv_label_create(row);
    lv_label_set_text(lbl, text);
    lv_obj_set_style_text_color(lbl, color, 0);
    lv_obj_set_style_text_font(lbl, CJK_FONT, 0);
    return lbl;
}

/* Install button callback — mode: "network" or "local" (async + cancelable) */
static void wiz_install_poll_cb(lv_timer_t* t) {
    (void)t;
    /* FIX Bug 3: Only process if wizard is still on the detect step.
     * User might have navigated away or closed the wizard. */
    if (!g_wizard_modal || g_wizard_step != 1) {
        g_wiz_install_result_ready.store(false);
        g_wiz_install_running.store(false);
        return;
    }
    if (!g_wiz_install_result_ready.load()) return;

    g_wiz_install_result_ready.store(false);
    if (g_wiz_btn_install_net) {
        lv_obj_clear_state(g_wiz_btn_install_net, LV_STATE_DISABLED);
        lv_obj_set_style_bg_opa(g_wiz_btn_install_net, LV_OPA_COVER, 0);
    }
    if (g_wiz_btn_install_local) {
        lv_obj_clear_state(g_wiz_btn_install_local, LV_STATE_DISABLED);
        lv_obj_set_style_bg_opa(g_wiz_btn_install_local, LV_OPA_COVER, 0);
    }
    if (g_wiz_btn_cancel_install) {
        lv_obj_clear_state(g_wiz_btn_cancel_install, LV_STATE_DISABLED);
        lv_obj_add_flag(g_wiz_btn_cancel_install, LV_OBJ_FLAG_HIDDEN);
    }

    if (g_wiz_install_active_label) {
        if (g_wiz_install_result_ok) {
            lv_label_set_text(g_wiz_install_active_label, tr(W_INSTALLED));
            lv_obj_set_style_text_color(g_wiz_install_active_label, g_colors->success, 0);
        } else {
            lv_label_set_text(g_wiz_install_active_label, g_wiz_install_result_msg[0] ? g_wiz_install_result_msg : tr(W_INSTALL_FAIL));
            lv_obj_set_style_text_color(g_wiz_install_active_label, g_colors->danger, 0);
        }
    }
    if (g_wiz_install_result_ok) {
        wizard_set_next_enabled(true);
        wizard_go_step(g_wizard_step);
    }
    /* FIX P3: Clean up timer — install finished (success or cancel), no more polling needed */
    if (g_wiz_install_poll_timer) {
        lv_timer_del(g_wiz_install_poll_timer);
        g_wiz_install_poll_timer = nullptr;
    }
}

static void wiz_detect_poll_cb(lv_timer_t* t) {
    (void)t;
    if (!g_wizard_modal || g_wizard_step != 1) return;
    if (!g_wiz_detect_result_ready.load()) return;
    g_wiz_detect_result_ready.store(false);
    wizard_go_step(1);
}

/* ── Gateway init (async, for users with OC already installed) ── */
static void wiz_gw_init_cb(lv_event_t* e) {
    (void)e;
    if (g_wiz_install_running.load()) return;

    /* Disable the init button */
    lv_obj_t* btn = (lv_obj_t*)lv_event_get_target(e);
    lv_obj_add_state(btn, LV_STATE_DISABLED);
    lv_obj_t* lbl = lv_obj_get_child(btn, 0);
    if (lbl) lv_label_set_text(lbl, tr(W_GW_INITING));

    g_wiz_install_running.store(true);
    wizard_set_next_enabled(false);

    std::thread([btn]() {
        char output[512] = {0};
        bool ok = app_init_openclaw(output, sizeof(output));

        /* Re-enable button and update label */
        lv_obj_clear_state(btn, LV_STATE_DISABLED);
        lv_obj_t* lbl = lv_obj_get_child(btn, 0);

        if (ok) {
            if (lbl) lv_label_set_text(lbl, tr(W_GW_INIT_OK));
            lv_obj_set_style_bg_color(btn, g_colors->success, 0);
            wizard_set_next_enabled(true);
        } else {
            if (lbl) lv_label_set_text(lbl, output[0] ? output : tr(W_GW_INIT_FAIL));
            lv_obj_set_style_text_color(lbl, g_colors->danger, 0);
        }

        g_wiz_install_running.store(false);
    }).detach();
}

static void wiz_cancel_install_cb(lv_event_t* e) {
    lv_obj_t* btn = (lv_obj_t*)lv_event_get_target(e);
    if (g_wiz_install_running.load()) {
        app_request_setup_cancel();
        lv_obj_add_state(btn, LV_STATE_DISABLED);
        lv_obj_t* lbl = lv_obj_get_child(btn, 0);
        if (lbl) lv_label_set_text(lbl, "取消中...");
        return;
    }
    wizard_close_cb(nullptr);
}

/* ═══ Offline Package Download (async via PowerShell) ═══ */

static std::string get_download_dir() {
    const char* userprofile = std::getenv("USERPROFILE");
    if (userprofile) {
        std::string dir = std::string(userprofile) + "\\Downloads\\AnyClaw";
        std::filesystem::create_directories(dir);
        return dir;
    }
    return ".\\downloads";
}

/* Run a system command and capture output (similar to boot_run_cmd but available here) */
static bool wiz_run_cmd(const char* cmd, std::string& output, DWORD timeout_ms = 60000) {
    SECURITY_ATTRIBUTES sa{};
    sa.nLength = sizeof(sa);
    sa.bInheritHandle = TRUE;
    HANDLE hRead = nullptr, hWrite = nullptr;
    if (!CreatePipe(&hRead, &hWrite, &sa, 0)) return false;
    SetHandleInformation(hRead, HANDLE_FLAG_INHERIT, 0);
    STARTUPINFOA si{};
    si.cb = sizeof(si);
    si.hStdOutput = hWrite;
    si.hStdError  = hWrite;
    si.dwFlags |= STARTF_USESTDHANDLES;
    PROCESS_INFORMATION pi{};
    std::string full = std::string("cmd /C \"") + cmd + "\"";
    std::vector<char> buf(full.begin(), full.end());
    buf.push_back('\0');
    if (!CreateProcessA(nullptr, buf.data(), nullptr, nullptr, TRUE,
                        CREATE_NO_WINDOW, nullptr, nullptr, &si, &pi)) {
        CloseHandle(hRead); CloseHandle(hWrite); return false;
    }
    CloseHandle(hWrite);
    output.clear();
    char readBuf[4096];
    DWORD bytesRead = 0;
    while (ReadFile(hRead, readBuf, sizeof(readBuf), &bytesRead, nullptr) && bytesRead > 0) {
        output.append(readBuf, bytesRead);
    }
    WaitForSingleObject(pi.hProcess, timeout_ms);
    DWORD exitCode = 1;
    GetExitCodeProcess(pi.hProcess, &exitCode);
    CloseHandle(pi.hProcess); CloseHandle(pi.hThread); CloseHandle(hRead);
    while (!output.empty() && (output.back() == '\r' || output.back() == '\n' || output.back() == ' '))
        output.pop_back();
    return exitCode == 0;
}

/* Async download: downloads a URL to a local file via PowerShell Invoke-WebRequest */
static void wiz_download_async(const char* url, const char* dest_path,
                                const char* task_name, const char* btn_label_key) {
    if (g_wiz_dl_running.load()) {
        ui_log("[DL] Download already running");
        return;
    }
    g_wiz_dl_running.store(true);

    /* Disable both download buttons */
    if (g_wiz_dl_node_btn) lv_obj_add_state(g_wiz_dl_node_btn, LV_STATE_DISABLED);
    if (g_wiz_dl_oc_btn) lv_obj_add_state(g_wiz_dl_oc_btn, LV_STATE_DISABLED);

    ui_progress_begin(task_name, "Starting download...", 0);
    ui_log("[DL] Downloading: %s -> %s", url, dest_path);

    std::string url_copy(url);
    std::string dest_copy(dest_path);
    std::string task_copy(task_name);

    std::thread([url_copy, dest_copy, task_copy]() {
        /* Build PowerShell download command */
        char cmd[2048];
        snprintf(cmd, sizeof(cmd),
            "powershell -NoProfile -Command \""
            "$ProgressPreference='SilentlyContinue';"
            "try { Invoke-WebRequest -Uri '%s' -OutFile '%s' -UseBasicParsing -TimeoutSec 120; "
            "if(Test-Path '%s'){ Write-Output 'OK' } else { Write-Output 'FAIL' } "
            "} catch { Write-Output $_.Exception.Message }\"",
            url_copy.c_str(), dest_copy.c_str(), dest_copy.c_str());

        ui_progress_update(task_copy.c_str(), "Downloading...", 10);
        std::string output;
        bool ok = wiz_run_cmd(cmd, output, 180000); /* 3min timeout */

        bool file_ok = ok && (output.find("OK") != std::string::npos);

        if (file_ok) {
            /* Verify file exists and has size > 0 */
            std::error_code ec;
            auto sz = std::filesystem::file_size(dest_copy, ec);
            file_ok = !ec && sz > 0;
            if (file_ok) {
                char size_str[64];
                if (sz > 1024*1024) snprintf(size_str, sizeof(size_str), "%.1f MB", sz / (1024.0*1024.0));
                else snprintf(size_str, sizeof(size_str), "%.0f KB", sz / 1024.0);
                ui_progress_finish(task_copy.c_str(), true, size_str);
                ui_log("[DL] Download complete: %s (%s)", dest_copy.c_str(), size_str);
            } else {
                ui_progress_finish(task_copy.c_str(), false, "File empty or missing");
                ui_log("[DL] Download file missing: %s", dest_copy.c_str());
            }
        } else {
            ui_progress_finish(task_copy.c_str(), false, output.substr(0, 120).c_str());
            ui_log("[DL] Download failed: %s", output.c_str());
        }

        g_wiz_dl_running.store(false);
        /* Re-enable buttons */
        if (g_wiz_dl_node_btn) lv_obj_clear_state(g_wiz_dl_node_btn, LV_STATE_DISABLED);
        if (g_wiz_dl_oc_btn) lv_obj_clear_state(g_wiz_dl_oc_btn, LV_STATE_DISABLED);
    }).detach();
}

static void wiz_dl_node_cb(lv_event_t* e) {
    (void)e;
    std::string dl_dir = get_download_dir();
    std::string dest = dl_dir + "\\node-v22.14.0-x64.msi";
    /* Node.js LTS v22.14.0 x64 MSI (official) */
    wiz_download_async(
        "https://nodejs.org/dist/v22.14.0/node-v22.14.0-x64.msi",
        dest.c_str(),
        "Node.js LTS Download",
        "Download Node.js"
    );
    /* Also offer to open folder when done */
    ui_log("[DL] Node.js will be saved to: %s", dest.c_str());
    ui_log("[DL] After download, run the .msi to install, then restart AnyClaw.");
}

static void wiz_dl_oc_cb(lv_event_t* e) {
    (void)e;
    std::string dl_dir = get_download_dir();
    std::string dest = dl_dir + "\\openclaw-latest.tgz";
    /* OpenClaw latest from npm registry */
    wiz_download_async(
        "https://registry.npmjs.org/openclaw/-/openclaw-latest.tgz",
        dest.c_str(),
        "OpenClaw Download",
        "Download OpenClaw"
    );
    ui_log("[DL] OpenClaw will be saved to: %s", dest.c_str());
    ui_log("[DL] After download, install via: npm install -g \"%s\"", dest.c_str());
}

static void wiz_dl_open_folder_cb(lv_event_t* e) {
    (void)e;
    std::string dl_dir = get_download_dir();
    ShellExecuteA(nullptr, "open", dl_dir.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
}

static void wiz_install_oc_cb(lv_event_t* e) {
    if (g_wiz_install_running.load()) return;
    const char* mode = (const char*)lv_event_get_user_data(e);
    if (!mode || !mode[0]) mode = "network";

    lv_obj_t* btn = (lv_obj_t*)lv_event_get_target(e);
    g_wiz_install_active_label = lv_obj_get_child(btn, 0);

    if (g_wiz_btn_install_net) {
        lv_obj_add_state(g_wiz_btn_install_net, LV_STATE_DISABLED);
        lv_obj_set_style_bg_opa(g_wiz_btn_install_net, LV_OPA_50, 0);
    }
    if (g_wiz_btn_install_local) {
        lv_obj_add_state(g_wiz_btn_install_local, LV_STATE_DISABLED);
        lv_obj_set_style_bg_opa(g_wiz_btn_install_local, LV_OPA_50, 0);
    }
    if (g_wiz_btn_cancel_install) {
        lv_obj_clear_flag(g_wiz_btn_cancel_install, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_state(g_wiz_btn_cancel_install, LV_STATE_DISABLED);
        lv_obj_t* c = lv_obj_get_child(g_wiz_btn_cancel_install, 0);
        if (c) lv_label_set_text(c, "退出安装");
    }
    if (g_wiz_install_active_label) lv_label_set_text(g_wiz_install_active_label, tr(W_INSTALLING));
    lv_refr_now(NULL);

    app_reset_setup_cancel();
    g_wiz_install_running.store(true);
    wizard_set_next_enabled(false);
    g_wiz_install_result_ready.store(false);
    g_wiz_install_result_ok = false;
    g_wiz_install_result_msg[0] = '\0';

    std::string mode_copy(mode);
    std::thread([mode_copy]() {
        char output[1024] = {0};
        bool ok = app_install_openclaw_ex(output, sizeof(output), mode_copy.c_str());

        if (!ok && mode_copy == "network" && !app_is_setup_cancelled()) {
            ui_log("[Setup] Network failed, auto-fallback to bundled...");
            ok = app_install_openclaw_ex(output, sizeof(output), "local");
        }

        if (ok && !app_is_setup_cancelled()) {
            ui_progress_update("OpenClaw Setup", "Initializing gateway", 88);
            char init_out[512] = {0};
            bool gw_ok = app_init_openclaw(init_out, sizeof(init_out));
            if (gw_ok) {
                g_wizard_oc_installed_now = true;
                ui_progress_finish("OpenClaw Setup", true, "OpenClaw ready");
            } else {
                /* FIX: Don't overwrite install success — report gateway issue separately */
                ui_progress_finish("OpenClaw Setup", false, init_out);
                char combined[1024] = {0};
                snprintf(combined, sizeof(combined), "Installed OK. Gateway init failed: %s", init_out);
                snprintf(output, sizeof(output), "%s", combined);
            }
            ok = gw_ok;
        } else if (app_is_setup_cancelled()) {
            snprintf(output, sizeof(output), "已取消安装");
            ui_progress_finish("OpenClaw Setup", false, output);
        }

        g_wiz_install_result_ok = ok;
        snprintf(g_wiz_install_result_msg, sizeof(g_wiz_install_result_msg), "%s", output[0] ? output : (ok ? "OK" : "Install failed"));
        g_wiz_install_running.store(false);
        g_wiz_install_result_ready.store(true);
    }).detach();

    if (!g_wiz_install_poll_timer) g_wiz_install_poll_timer = lv_timer_create(wiz_install_poll_cb, 120, nullptr);
}

/* ── Step 1: Detection (Node.js → npm → Network → OpenClaw → Gateway) ── */
static void wizard_build_step_detect() {
    if (!g_wiz_detect_result_ready.load()) {
        lv_obj_t* hint = lv_label_create(g_wizard_content);
        lv_label_set_text(hint, tr(W_DETECT_HINT));
        lv_obj_set_style_text_color(hint, g_colors->text_dim, 0);
        lv_obj_set_style_text_font(hint, CJK_FONT, 0);

        wizard_add_status_row(g_wizard_content, tr(W_RUNNING), g_colors->warning);
        wizard_set_next_enabled(false);

        if (!g_wiz_detect_running.load()) {
            g_wiz_detect_running.store(true);
            std::thread([]() {
                EnvCheckResult env = app_check_environment();
                char net_resp[128] = {0};
                int net_code = http_get("https://openrouter.ai/api/v1/models", net_resp, sizeof(net_resp), 2);
                bool net_ok = (net_code > 0 && net_code < 500);
                bool gw_ok = false;
                if (env.openclaw_ok) {
                    char gw_resp[128] = {0};
                    int gw_code = http_get("http://127.0.0.1:18789/health", gw_resp, sizeof(gw_resp), 2);
                    gw_ok = (gw_code > 0 && gw_code < 500);
                }
                env.gateway_ok = gw_ok;
                g_wiz_detect_env = env;
                g_wiz_detect_net_ok = net_ok;
                g_wiz_detect_running.store(false);
                g_wiz_detect_result_ready.store(true);
            }).detach();
        }

        if (!g_wiz_detect_poll_timer) {
            g_wiz_detect_poll_timer = lv_timer_create(wiz_detect_poll_cb, 120, nullptr);
        }
        return;
    }

    EnvCheckResult env = g_wiz_detect_env;
    bool net_ok = g_wiz_detect_net_ok;

    lv_obj_t* hint = lv_label_create(g_wizard_content);
    lv_label_set_text(hint, tr(W_DETECT_HINT));
    lv_obj_set_style_text_color(hint, g_colors->text_dim, 0);
    lv_obj_set_style_text_font(hint, CJK_FONT, 0);

    /* ═══ 1. Full environment check ═══ */
    g_wizard_openclaw_ok = env.openclaw_ok ? 1 : 0;
    g_wizard_hermes_ok = env.hermes_ok;
    g_wizard_hermes_healthy = env.hermes_healthy;
    g_wizard_claude_ok = env.claude_ok;
    g_wizard_claude_healthy = env.claude_healthy;
    snprintf(g_wizard_hermes_ver, sizeof(g_wizard_hermes_ver), "%s", env.hermes_ver);
    snprintf(g_wizard_claude_ver, sizeof(g_wizard_claude_ver), "%s", env.claude_ver);

    /* Node.js */
    if (env.node_ok && env.node_version_ok) {
        char buf[128];
        snprintf(buf, sizeof(buf), tr(W_NODE_OK), env.node_ver);
        wizard_add_status_row(g_wizard_content, buf, g_colors->success);
    } else if (env.node_ok && !env.node_version_ok) {
        char buf[128];
        snprintf(buf, sizeof(buf), tr(W_NODE_OLD), env.node_ver);
        wizard_add_status_row(g_wizard_content, buf, g_colors->warning);
    } else {
        wizard_add_status_row(g_wizard_content, tr(W_NODE_FAIL), g_colors->danger);
    }

    /* npm */
    if (env.npm_ok) {
        char buf[128];
        snprintf(buf, sizeof(buf), tr(W_NPM_OK), env.npm_ver);
        wizard_add_status_row(g_wizard_content, buf, g_colors->success);
    } else if (env.node_ok) {
        wizard_add_status_row(g_wizard_content, tr(W_NPM_FAIL), g_colors->danger);
    }

    /* ═══ 2. Network check ═══ */
    wizard_add_status_row(g_wizard_content,
        net_ok ? tr(W_NET_OK) : tr(W_NET_FAIL),
        net_ok ? g_colors->success : g_colors->danger);

    /* ═══ 3. OpenClaw ═══ */
    if (env.openclaw_ok) {
        char buf[128];
        snprintf(buf, sizeof(buf), tr(W_OC_OK), env.openclaw_ver);
        wizard_add_status_row(g_wizard_content, buf, g_colors->success);

        /* ═══ 3b. Gateway health (only if OC is installed) ═══ */
        bool gw_ok = env.gateway_ok;

        wizard_add_status_row(g_wizard_content,
            gw_ok ? tr(W_GW_OK) : tr(W_GW_DOWN),
            gw_ok ? g_colors->success : g_colors->warning);

        if (!gw_ok && net_ok) {
            lv_obj_t* gw_btn = lv_button_create(g_wizard_content);
            lv_obj_set_size(gw_btn, SCALE(220), SCALE(38));
            lv_obj_set_style_bg_color(gw_btn, g_colors->accent_secondary, 0);
            lv_obj_set_style_radius(gw_btn, SCALE(g_colors->radius_md), 0);
            lv_obj_t* gw_lbl = lv_label_create(gw_btn);
            lv_label_set_text(gw_lbl, tr(W_GW_INIT_BTN));
            lv_obj_set_style_text_font(gw_lbl, CJK_FONT, 0);
            lv_obj_center(gw_lbl);
            lv_obj_add_event_cb(gw_btn, wiz_gw_init_cb, LV_EVENT_CLICKED, nullptr);
        }
    } else {
        wizard_add_status_row(g_wizard_content, tr(W_OC_NOT_INST), g_colors->danger);
    }

    /* ═══ Node.js missing: offline download + links ═══ */
    if (!env.node_ok || !env.node_version_ok || !env.npm_ok) {
        lv_obj_t* dl_label = lv_label_create(g_wizard_content);
        lv_label_set_text(dl_label, tr(W_NODE_DL));
        lv_obj_set_style_text_color(dl_label, g_colors->text_dim, 0);
        lv_obj_set_style_text_font(dl_label, CJK_FONT, 0);

        /* One-click download button (Node.js LTS x64 MSI) */
        lv_obj_t* dl_row = lv_obj_create(g_wizard_content);
        lv_obj_set_size(dl_row, LV_PCT(100), LV_SIZE_CONTENT);
        lv_obj_set_style_bg_opa(dl_row, LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_width(dl_row, 0, 0);
        lv_obj_set_style_pad_all(dl_row, 0, 0);
        lv_obj_set_style_pad_gap(dl_row, 10, 0);
        lv_obj_set_flex_flow(dl_row, LV_FLEX_FLOW_ROW);
        lv_obj_set_flex_align(dl_row, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        lv_obj_clear_flag(dl_row, LV_OBJ_FLAG_SCROLLABLE);

        g_wiz_dl_node_btn = lv_button_create(dl_row);
        lv_obj_set_size(g_wiz_dl_node_btn, SCALE(240), SCALE(38));
        lv_obj_set_style_bg_color(g_wiz_dl_node_btn, g_colors->info, 0); /* Purple */
        lv_obj_set_style_radius(g_wiz_dl_node_btn, SCALE(g_colors->radius_md), 0);
        lv_obj_t* dl_node_lbl = lv_label_create(g_wiz_dl_node_btn);
        lv_label_set_text(dl_node_lbl, "Download Node.js LTS (.msi)");
        lv_obj_set_style_text_font(dl_node_lbl, CJK_FONT_SMALL, 0);
        lv_obj_center(dl_node_lbl);
        lv_obj_add_event_cb(g_wiz_dl_node_btn, wiz_dl_node_cb, LV_EVENT_CLICKED, nullptr);

        lv_obj_t* open_dl_btn = lv_button_create(dl_row);
        lv_obj_set_size(open_dl_btn, SCALE(120), SCALE(38));
        lv_obj_set_style_bg_color(open_dl_btn, g_colors->btn_secondary, 0);
        lv_obj_set_style_radius(open_dl_btn, SCALE(g_colors->radius_md), 0);
        lv_obj_t* open_lbl = lv_label_create(open_dl_btn);
        lv_label_set_text(open_lbl, "Open Folder");
        lv_obj_set_style_text_font(open_lbl, CJK_FONT_SMALL, 0);
        lv_obj_center(open_lbl);
        lv_obj_add_event_cb(open_dl_btn, wiz_dl_open_folder_cb, LV_EVENT_CLICKED, nullptr);

        /* Show download URLs as well */
        const char* urls[] = {
            "https://nodejs.org/",
            "https://registry.npmmirror.com/-/binary/node/",
            nullptr
        };
        for (int i = 0; urls[i]; i++) {
            lv_obj_t* link = lv_label_create(g_wizard_content);
            lv_label_set_text(link, urls[i]);
            lv_obj_set_style_text_color(link, g_colors->accent_secondary, 0);
            lv_obj_set_style_text_font(link, CJK_FONT_SMALL, 0);
        }
    }

    /* ═══ OpenClaw missing: always expose install entry (auto handles Node/npm bootstrapping). ═══ */
    if (!env.openclaw_ok) {
        lv_obj_t* choose_lbl = lv_label_create(g_wizard_content);
        lv_label_set_text(choose_lbl, tr(W_CHOOSE_INST));
        lv_obj_set_style_text_color(choose_lbl, g_colors->text_dim, 0);
        lv_obj_set_style_text_font(choose_lbl, CJK_FONT, 0);

        g_wiz_btn_install_net = nullptr;
        g_wiz_btn_install_local = nullptr;
        g_wiz_install_active_label = nullptr;

        /* Row: two buttons side by side */
        lv_obj_t* btn_row = lv_obj_create(g_wizard_content);
        lv_obj_set_size(btn_row, LV_PCT(100), LV_SIZE_CONTENT);
        lv_obj_set_style_bg_opa(btn_row, LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_width(btn_row, 0, 0);
        lv_obj_set_style_pad_all(btn_row, 0, 0);
        lv_obj_clear_flag(btn_row, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_flex_flow(btn_row, LV_FLEX_FLOW_ROW);
        lv_obj_set_style_pad_gap(btn_row, 10, 0);
        lv_obj_set_flex_align(btn_row, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

        /* Auto install button: includes Node/npm bootstrap + OpenClaw install chain. */
        lv_obj_t* btn_auto = lv_button_create(btn_row);
        lv_obj_set_size(btn_auto, LV_PCT(48), SCALE(44));
        lv_obj_set_style_bg_color(btn_auto, g_colors->success, 0);
        lv_obj_set_style_radius(btn_auto, SCALE(g_colors->radius_md), 0);
        lv_obj_t* lbl_auto = lv_label_create(btn_auto);
        lv_label_set_text(lbl_auto, "Auto Install");
        lv_obj_set_style_text_font(lbl_auto, CJK_FONT, 0);
        lv_obj_center(lbl_auto);
        g_wiz_btn_install_net = btn_auto;
        lv_obj_add_event_cb(btn_auto, wiz_install_oc_cb, LV_EVENT_CLICKED, (void*)"auto");

        if (!net_ok) {
            lv_obj_add_state(btn_auto, LV_STATE_DISABLED);
            lv_obj_set_style_bg_opa(btn_auto, LV_OPA_50, 0);
        }

        /* Local/bundled install button (blue) */
        lv_obj_t* btn_local = lv_button_create(btn_row);
        lv_obj_set_size(btn_local, LV_PCT(48), SCALE(44));
        lv_obj_set_style_bg_color(btn_local, g_colors->accent_secondary, 0);
        lv_obj_set_style_radius(btn_local, SCALE(g_colors->radius_md), 0);
        lv_obj_t* lbl = lv_label_create(btn_local);
        lv_label_set_text(lbl, tr(W_INSTALL_LOCAL));
        lv_obj_set_style_text_font(lbl, CJK_FONT, 0);
        lv_obj_center(lbl);

        g_wiz_btn_install_local = btn_local;
        lv_obj_add_event_cb(btn_local, wiz_install_oc_cb, LV_EVENT_CLICKED, (void*)"local");

        /* OpenClaw offline download row */
        lv_obj_t* oc_dl_row = lv_obj_create(g_wizard_content);
        lv_obj_set_size(oc_dl_row, LV_PCT(100), LV_SIZE_CONTENT);
        lv_obj_set_style_bg_opa(oc_dl_row, LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_width(oc_dl_row, 0, 0);
        lv_obj_set_style_pad_all(oc_dl_row, 0, 0);
        lv_obj_set_style_pad_gap(oc_dl_row, 10, 0);
        lv_obj_set_flex_flow(oc_dl_row, LV_FLEX_FLOW_ROW);
        lv_obj_set_flex_align(oc_dl_row, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        lv_obj_clear_flag(oc_dl_row, LV_OBJ_FLAG_SCROLLABLE);

        g_wiz_dl_oc_btn = lv_button_create(oc_dl_row);
        lv_obj_set_size(g_wiz_dl_oc_btn, SCALE(240), SCALE(38));
        lv_obj_set_style_bg_color(g_wiz_dl_oc_btn, g_colors->info, 0); /* Purple */
        lv_obj_set_style_radius(g_wiz_dl_oc_btn, SCALE(g_colors->radius_md), 0);
        lv_obj_t* dl_oc_lbl = lv_label_create(g_wiz_dl_oc_btn);
        lv_label_set_text(dl_oc_lbl, "Download OpenClaw (.tgz)");
        lv_obj_set_style_text_font(dl_oc_lbl, CJK_FONT_SMALL, 0);
        lv_obj_center(dl_oc_lbl);
        lv_obj_add_event_cb(g_wiz_dl_oc_btn, wiz_dl_oc_cb, LV_EVENT_CLICKED, nullptr);
        if (!net_ok) {
            lv_obj_add_state(g_wiz_dl_oc_btn, LV_STATE_DISABLED);
            lv_obj_set_style_bg_opa(g_wiz_dl_oc_btn, LV_OPA_50, 0);
        }

        lv_obj_t* open_oc_dl = lv_button_create(oc_dl_row);
        lv_obj_set_size(open_oc_dl, SCALE(120), SCALE(38));
        lv_obj_set_style_bg_color(open_oc_dl, g_colors->btn_secondary, 0);
        lv_obj_set_style_radius(open_oc_dl, SCALE(g_colors->radius_md), 0);
        lv_obj_t* open_oc_lbl = lv_label_create(open_oc_dl);
        lv_label_set_text(open_oc_lbl, "Open Folder");
        lv_obj_set_style_text_font(open_oc_lbl, CJK_FONT_SMALL, 0);
        lv_obj_center(open_oc_lbl);
        lv_obj_add_event_cb(open_oc_dl, wiz_dl_open_folder_cb, LV_EVENT_CLICKED, nullptr);

        lv_obj_t* btn_cancel = lv_button_create(g_wizard_content);
        lv_obj_set_size(btn_cancel, LV_PCT(100), SCALE(40));
        lv_obj_set_style_bg_color(btn_cancel, g_colors->btn_close, 0);
        lv_obj_set_style_radius(btn_cancel, SCALE(g_colors->radius_md), 0);
        lv_obj_t* lbl_cancel = lv_label_create(btn_cancel);
        lv_label_set_text(lbl_cancel, "退出安装");
        lv_obj_set_style_text_font(lbl_cancel, CJK_FONT, 0);
        lv_obj_center(lbl_cancel);
        lv_obj_add_event_cb(btn_cancel, wiz_cancel_install_cb, LV_EVENT_CLICKED, nullptr);
        g_wiz_btn_cancel_install = btn_cancel;
        lv_obj_add_flag(g_wiz_btn_cancel_install, LV_OBJ_FLAG_HIDDEN);

        g_wiz_install_progress_panel = lv_obj_create(g_wizard_content);
        lv_obj_set_size(g_wiz_install_progress_panel, LV_PCT(100), LV_SIZE_CONTENT);
        lv_obj_set_style_bg_color(g_wiz_install_progress_panel, g_colors->surface, 0);
        lv_obj_set_style_border_color(g_wiz_install_progress_panel, g_colors->border, 0);
        lv_obj_set_style_border_width(g_wiz_install_progress_panel, 1, 0);
        lv_obj_set_style_radius(g_wiz_install_progress_panel, SCALE(g_colors->radius_md), 0);
        lv_obj_set_style_pad_all(g_wiz_install_progress_panel, 8, 0);
        lv_obj_set_style_pad_gap(g_wiz_install_progress_panel, 6, 0);
        lv_obj_set_flex_flow(g_wiz_install_progress_panel, LV_FLEX_FLOW_COLUMN);
        lv_obj_set_flex_align(g_wiz_install_progress_panel, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
        lv_obj_clear_flag(g_wiz_install_progress_panel, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_add_flag(g_wiz_install_progress_panel, LV_OBJ_FLAG_HIDDEN);

        g_wiz_install_progress_task = lv_label_create(g_wiz_install_progress_panel);
        lv_label_set_text(g_wiz_install_progress_task, "Task: Setup");
        lv_obj_set_style_text_font(g_wiz_install_progress_task, CJK_FONT_SMALL, 0);
        lv_obj_set_style_text_color(g_wiz_install_progress_task, g_colors->accent, 0);

        g_wiz_install_progress_step = lv_label_create(g_wiz_install_progress_panel);
        lv_label_set_text(g_wiz_install_progress_step, "Step: Waiting...");
        lv_obj_set_style_text_font(g_wiz_install_progress_step, CJK_FONT_SMALL, 0);
        lv_obj_set_style_text_color(g_wiz_install_progress_step, g_colors->text_dim, 0);

        g_wiz_install_progress_bar = lv_bar_create(g_wiz_install_progress_panel);
        lv_obj_set_width(g_wiz_install_progress_bar, LV_PCT(100));
        lv_bar_set_range(g_wiz_install_progress_bar, 0, 100);
        lv_bar_set_value(g_wiz_install_progress_bar, 0, LV_ANIM_OFF);

        g_wiz_install_progress_result = lv_label_create(g_wiz_install_progress_panel);
        lv_label_set_text(g_wiz_install_progress_result, "");
        lv_obj_set_style_text_font(g_wiz_install_progress_result, CJK_FONT_SMALL, 0);
        lv_obj_set_style_text_color(g_wiz_install_progress_result, g_colors->text_dim, 0);
    }

    /* ═══ Block Next only if critical checks fail ═══
     * Network is NOT required — offline users should still be able to proceed
     * if Node.js + OpenClaw are already installed. */
    bool critical_ok = env.node_ok && env.node_version_ok && env.openclaw_ok && env.gateway_ok;
    wizard_set_next_enabled(critical_ok);

    if (!net_ok) {
        lv_obj_t* warn = lv_label_create(g_wizard_content);
        lv_label_set_text(warn, g_lang == Lang::CN
            ? "⚠ 网络不可达，安装/下载功能将不可用"
            : "⚠ Network unreachable — install/download features unavailable");
        lv_obj_set_style_text_color(warn, g_colors->warning, 0);
        lv_obj_set_style_text_font(warn, CJK_FONT_SMALL, 0);
    }
}

/* ── Step 2: Model + API Key ── */
static lv_obj_t* g_wiz_provider_hint = nullptr; /* Dynamic provider hint label */

/* Update provider hint when model dropdown changes */
static void wiz_model_change_cb(lv_event_t* e) {
    (void)e;
    if (!g_wiz_model_dd || !g_wiz_provider_hint) return;
    char sel[128];
    lv_dropdown_get_selected_str(g_wiz_model_dd, sel, sizeof(sel));
    char* free_tag = strstr(sel, " [free]");
    if (free_tag) *free_tag = '\0';
    snprintf(g_wizard_model_name, sizeof(g_wizard_model_name), "%s", sel); /* Save for summary */

    bool is_xiaomi = (strncmp(sel, "xiaomi/", 7) == 0);
    static char hint_buf[256];
    if (is_xiaomi) {
        snprintf(hint_buf, sizeof(hint_buf), "%s xiaomi  |  %s",
                 tr(W_PROVIDER_IS), tr(W_API_GET_XM));
    } else {
        snprintf(hint_buf, sizeof(hint_buf), "%s openrouter  |  %s",
                 tr(W_PROVIDER_IS), tr(W_API_GET_OR));
    }
    lv_label_set_text(g_wiz_provider_hint, hint_buf);
}

static void wizard_install_hermes_cb(lv_event_t* e) {
    (void)e;
    ui_log("[Wizard] Installing Hermes runtime...");
    std::thread([]() {
        bool ok = app_install_hermes("auto", nullptr, nullptr);
        if (ok) {
            ui_log("[Wizard] Hermes installed");
            ui_toast_success(g_lang == Lang::CN ? "Hermes 安装成功" : "Hermes installed");
        } else {
            ui_log("[Wizard] Hermes install failed");
            ui_toast_error(g_lang == Lang::CN ? "Hermes 安装失败" : "Hermes install failed");
        }
    }).detach();
}

static void wizard_install_claude_cb(lv_event_t* e) {
    (void)e;
    ui_log("[Wizard] Installing Claude runtime...");
    std::thread([]() {
        bool ok = app_install_claude_cli("auto", nullptr, nullptr);
        if (ok) {
            ui_log("[Wizard] Claude installed");
            ui_toast_success(g_lang == Lang::CN ? "Claude 安装成功" : "Claude installed");
        } else {
            ui_log("[Wizard] Claude install failed");
            ui_toast_error(g_lang == Lang::CN ? "Claude 安装失败" : "Claude install failed");
        }
    }).detach();
}

static void wizard_build_step_model_api() {
    /* ── Model Selection ── */
    lv_obj_t* hint_m = lv_label_create(g_wizard_content);
    lv_label_set_text(hint_m, tr(W_MODEL_HINT));
    lv_obj_set_style_text_color(hint_m, g_colors->text_dim, 0);
    lv_obj_set_style_text_font(hint_m, CJK_FONT, 0);

    g_wiz_model_dd = lv_dropdown_create(g_wizard_content);

    /* Build options string from model manager — mark free models */
    static char model_options[MODEL_MAX_COUNT * (MODEL_MAX_NAME_LEN + 10)];
    model_options[0] = '\0';
    for (int i = 0; i < model_get_count(); i++) {
        if (i > 0) strcat(model_options, "\n");
        const char* name = model_get_name(i);
        strcat(model_options, name);
        if (model_is_free(i)) strcat(model_options, " [free]");
    }
    lv_dropdown_set_options(g_wiz_model_dd, model_options);
    lv_dropdown_set_selected(g_wiz_model_dd, g_wizard_model_sel);
    lv_obj_set_width(g_wiz_model_dd, LV_PCT(100));
    lv_obj_set_style_bg_color(g_wiz_model_dd, g_colors->surface, 0);
    lv_obj_set_style_text_color(g_wiz_model_dd, g_colors->text, 0);
    lv_obj_set_style_border_color(g_wiz_model_dd, g_colors->border, 0);
    lv_obj_set_style_border_width(g_wiz_model_dd, 1, 0);
    lv_obj_set_style_radius(g_wiz_model_dd, SCALE(g_colors->radius_sm), 0);
    lv_obj_set_style_text_font(g_wiz_model_dd, CJK_FONT, 0);
    lv_obj_set_style_text_font(g_wiz_model_dd, FONT(14), LV_PART_INDICATOR);
    lv_obj_set_style_text_font(g_wiz_model_dd, CJK_FONT, LV_PART_ITEMS);
    lv_obj_set_style_text_color(g_wiz_model_dd, g_colors->text, LV_PART_ITEMS);
    lv_obj_set_style_bg_color(g_wiz_model_dd, g_colors->panel, LV_PART_ITEMS);

    /* Provider hint — updates dynamically when model changes */
    g_wiz_provider_hint = lv_label_create(g_wizard_content);
    lv_obj_set_style_text_color(g_wiz_provider_hint, g_colors->accent, 0);
    lv_obj_set_style_text_font(g_wiz_provider_hint, CJK_FONT_SMALL, 0);
    lv_obj_set_width(g_wiz_provider_hint, LV_PCT(100));
    lv_label_set_long_mode(g_wiz_provider_hint, LV_LABEL_LONG_WRAP);
    /* Trigger initial hint */
    lv_obj_add_event_cb(g_wiz_model_dd, wiz_model_change_cb, LV_EVENT_VALUE_CHANGED, nullptr);
    wiz_model_change_cb(nullptr); /* Set initial hint */

    /* ── Divider ── */
    lv_obj_t* div = lv_obj_create(g_wizard_content);
    lv_obj_set_size(div, LV_PCT(100), 1);
    lv_obj_set_style_bg_color(div, g_colors->divider, 0);
    lv_obj_set_style_border_width(div, 0, 0);
    lv_obj_clear_flag(div, LV_OBJ_FLAG_SCROLLABLE);

    /* ── API Key ── */
    lv_obj_t* hint_a = lv_label_create(g_wizard_content);
    lv_label_set_text(hint_a, tr(W_API_HINT));
    lv_obj_set_style_text_color(hint_a, g_colors->text_dim, 0);
    lv_obj_set_style_text_font(hint_a, CJK_FONT, 0);

    g_wiz_api_ta = lv_textarea_create(g_wizard_content);
    lv_textarea_set_one_line(g_wiz_api_ta, true);
    /* Show API key in plain text */

    lv_textarea_set_placeholder_text(g_wiz_api_ta, "sk-or-...");
    /* FIX Bug 4: Restore API key from saved state on step re-entry */
    if (g_wizard_api_key[0]) {
        lv_textarea_set_text(g_wiz_api_ta, g_wizard_api_key);
    }
    lv_obj_set_width(g_wiz_api_ta, LV_PCT(100));
    lv_obj_set_height(g_wiz_api_ta, SCALE(INPUT_H_BASE));
    lv_obj_set_style_bg_color(g_wiz_api_ta, g_colors->surface, 0);
    lv_obj_set_style_border_color(g_wiz_api_ta, g_colors->border, 0);
    lv_obj_set_style_border_width(g_wiz_api_ta, 1, 0);
    lv_obj_set_style_radius(g_wiz_api_ta, SCALE(g_colors->radius_sm), 0);
    lv_obj_set_style_text_color(g_wiz_api_ta, g_colors->text, 0);
    lv_obj_set_style_text_font(g_wiz_api_ta, CJK_FONT, 0);
    lv_obj_set_style_pad_all(g_wiz_api_ta, 8, 0);
    lv_group_add_obj(lv_group_get_default(), g_wiz_api_ta);

    /* ── Optional agent runtime install cards (Hermes / Claude) ── */
    lv_obj_t* div2 = lv_obj_create(g_wizard_content);
    lv_obj_set_size(div2, LV_PCT(100), 1);
    lv_obj_set_style_bg_color(div2, g_colors->divider, 0);
    lv_obj_set_style_border_width(div2, 0, 0);
    lv_obj_clear_flag(div2, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* hint_agent = lv_label_create(g_wizard_content);
    lv_label_set_text(hint_agent, tr(W_AGENT_HINT));
    lv_obj_set_style_text_color(hint_agent, g_colors->text_dim, 0);
    lv_obj_set_style_text_font(hint_agent, CJK_FONT_SMALL, 0);
    lv_label_set_long_mode(hint_agent, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(hint_agent, LV_PCT(100));

    auto add_runtime_card = [&](const char* title, bool installed, bool healthy, const char* version, lv_event_cb_t install_cb) {
        lv_obj_t* card = lv_obj_create(g_wizard_content);
        lv_obj_set_size(card, LV_PCT(100), LV_SIZE_CONTENT);
        lv_obj_set_style_bg_color(card, g_colors->surface, 0);
        lv_obj_set_style_border_color(card, g_colors->border, 0);
        lv_obj_set_style_border_width(card, 1, 0);
        lv_obj_set_style_radius(card, SCALE(g_colors->radius_md), 0);
        lv_obj_set_style_pad_all(card, 10, 0);
        lv_obj_set_style_pad_gap(card, 8, 0);
        lv_obj_set_flex_flow(card, LV_FLEX_FLOW_ROW);
        lv_obj_set_flex_align(card, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);

        lv_obj_t* left = lv_obj_create(card);
        lv_obj_set_size(left, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
        lv_obj_set_style_bg_opa(left, LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_width(left, 0, 0);
        lv_obj_set_style_pad_all(left, 0, 0);
        lv_obj_set_style_pad_gap(left, 4, 0);
        lv_obj_set_flex_flow(left, LV_FLEX_FLOW_COLUMN);
        lv_obj_clear_flag(left, LV_OBJ_FLAG_SCROLLABLE);

        lv_obj_t* t = lv_label_create(left);
        lv_label_set_text(t, title);
        lv_obj_set_style_text_color(t, g_colors->text, 0);
        lv_obj_set_style_text_font(t, CJK_FONT, 0);

        lv_obj_t* s = lv_label_create(left);
        char status[192];
        snprintf(status, sizeof(status), "%s%s%s",
            installed ? (g_lang == Lang::CN ? "已安装" : "Installed") : (g_lang == Lang::CN ? "未安装" : "Missing"),
            (installed && version && version[0]) ? "  v" : "",
            (installed && version && version[0]) ? version : "");
        lv_label_set_text(s, status);
        lv_obj_set_style_text_color(s, installed ? (healthy ? g_colors->success : g_colors->warning) : g_colors->warning, 0);
        lv_obj_set_style_text_font(s, CJK_FONT_SMALL, 0);

        lv_obj_t* btn = aw_btn_create(card,
            installed ? (g_lang == Lang::CN ? "重检" : "Recheck") : (g_lang == Lang::CN ? "安装" : "Install"),
            BTN_SECONDARY, SCALE(108), SCALE(32));
        lv_obj_add_event_cb(btn, install_cb, LV_EVENT_CLICKED, nullptr);
    };

    add_runtime_card("Hermes Agent", g_wizard_hermes_ok, g_wizard_hermes_healthy, g_wizard_hermes_ver, wizard_install_hermes_cb);
    add_runtime_card("Claude CLI", g_wizard_claude_ok, g_wizard_claude_healthy, g_wizard_claude_ver, wizard_install_claude_cb);
}

/* ── Step 3: Local Gemma Models (Optional) ── */
static void wizard_build_step_gemma() {
    lv_obj_t* gemma_hint = lv_label_create(g_wizard_content);
    lv_label_set_text(gemma_hint, g_lang == Lang::CN
        ? "可选：安装本地 Gemma 4 模型（需要较大磁盘空间）"
        : "Optional: Install local Gemma 4 models (requires disk space)");
    lv_obj_set_style_text_color(gemma_hint, g_colors->text_dim, 0);
    lv_obj_set_style_text_font(gemma_hint, CJK_FONT, 0);

    lv_obj_t* gemma_sw_row = lv_obj_create(g_wizard_content);
    lv_obj_set_size(gemma_sw_row, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(gemma_sw_row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(gemma_sw_row, 0, 0);
    lv_obj_set_style_pad_all(gemma_sw_row, 0, 0);
    lv_obj_set_style_pad_gap(gemma_sw_row, 8, 0);
    lv_obj_set_flex_flow(gemma_sw_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(gemma_sw_row, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag(gemma_sw_row, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_t* gemma_sw_label = lv_label_create(gemma_sw_row);
    lv_label_set_text(gemma_sw_label, g_lang == Lang::CN ? "启用 Gemma 本地安装" : "Enable Gemma local install");
    lv_obj_set_style_text_color(gemma_sw_label, g_colors->text, 0);
    lv_obj_set_style_text_font(gemma_sw_label, CJK_FONT_SMALL, 0);

    mode_sw_gemma_install = lv_switch_create(gemma_sw_row);
    if (g_gemma_install_opt_in) lv_obj_add_state(mode_sw_gemma_install, LV_STATE_CHECKED);
    lv_obj_add_event_cb(mode_sw_gemma_install, gemma_install_sw_cb, LV_EVENT_VALUE_CHANGED, nullptr);

    mode_cb_gemma_2b = lv_checkbox_create(g_wizard_content);
    lv_checkbox_set_text(mode_cb_gemma_2b, "Gemma 4 2B (download)");
    lv_obj_add_event_cb(mode_cb_gemma_2b, gemma_model_checkbox_cb, LV_EVENT_VALUE_CHANGED, nullptr);
    mode_cb_gemma_9b = lv_checkbox_create(g_wizard_content);
    lv_checkbox_set_text(mode_cb_gemma_9b, "Gemma 4 9B (download)");
    lv_obj_add_event_cb(mode_cb_gemma_9b, gemma_model_checkbox_cb, LV_EVENT_VALUE_CHANGED, nullptr);
    mode_cb_gemma_27b = lv_checkbox_create(g_wizard_content);
    lv_checkbox_set_text(mode_cb_gemma_27b, "Gemma 4 27B (download)");
    lv_obj_add_event_cb(mode_cb_gemma_27b, gemma_model_checkbox_cb, LV_EVENT_VALUE_CHANGED, nullptr);
    mode_lbl_gemma_recommend = lv_label_create(g_wizard_content);
    lv_obj_set_style_text_color(mode_lbl_gemma_recommend, g_colors->text_dim, 0);
    lv_obj_set_style_text_font(mode_lbl_gemma_recommend, CJK_FONT_SMALL, 0);
    lv_label_set_long_mode(mode_lbl_gemma_recommend, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(mode_lbl_gemma_recommend, LV_PCT(100));
    update_gemma_recommend_visuals();

    lv_obj_t* btn_start_gemma = lv_button_create(g_wizard_content);
    lv_obj_set_size(btn_start_gemma, LV_PCT(100), SCALE(36));
    lv_obj_set_style_bg_color(btn_start_gemma, g_colors->success, 0);
    lv_obj_set_style_radius(btn_start_gemma, SCALE(g_colors->radius_md), 0);
    lv_obj_t* lbl_start = lv_label_create(btn_start_gemma);
    lv_label_set_text(lbl_start, g_lang == Lang::CN ? "立即开始下载 Gemma" : "Start Gemma Download Now");
    lv_obj_set_style_text_font(lbl_start, CJK_FONT_SMALL, 0);
    lv_obj_center(lbl_start);
    lv_obj_add_event_cb(btn_start_gemma, [](lv_event_t* e) {
        (void)e;
        if (!g_gemma_install_opt_in || g_gemma_model_mask == 0) {
            ui_log("[Gemma] Please enable Gemma install and select at least one model");
            return;
        }
        start_gemma_install_async(g_gemma_model_mask);
    }, LV_EVENT_CLICKED, nullptr);

    g_wiz_install_progress_panel = lv_obj_create(g_wizard_content);
    lv_obj_set_size(g_wiz_install_progress_panel, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_style_bg_color(g_wiz_install_progress_panel, g_colors->surface, 0);
    lv_obj_set_style_border_color(g_wiz_install_progress_panel, g_colors->border, 0);
    lv_obj_set_style_border_width(g_wiz_install_progress_panel, 1, 0);
    lv_obj_set_style_radius(g_wiz_install_progress_panel, SCALE(g_colors->radius_md), 0);
    lv_obj_set_style_pad_all(g_wiz_install_progress_panel, 8, 0);
    lv_obj_set_style_pad_gap(g_wiz_install_progress_panel, 6, 0);
    lv_obj_set_flex_flow(g_wiz_install_progress_panel, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(g_wiz_install_progress_panel, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_clear_flag(g_wiz_install_progress_panel, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(g_wiz_install_progress_panel, LV_OBJ_FLAG_HIDDEN);

    g_wiz_install_progress_task = lv_label_create(g_wiz_install_progress_panel);
    lv_label_set_text(g_wiz_install_progress_task, "Task: Gemma Setup");
    lv_obj_set_style_text_font(g_wiz_install_progress_task, CJK_FONT_SMALL, 0);
    lv_obj_set_style_text_color(g_wiz_install_progress_task, g_colors->accent, 0);

    g_wiz_install_progress_step = lv_label_create(g_wiz_install_progress_panel);
    lv_label_set_text(g_wiz_install_progress_step, "Step: Waiting...");
    lv_obj_set_style_text_font(g_wiz_install_progress_step, CJK_FONT_SMALL, 0);
    lv_obj_set_style_text_color(g_wiz_install_progress_step, g_colors->text_dim, 0);

    g_wiz_install_progress_bar = lv_bar_create(g_wiz_install_progress_panel);
    lv_obj_set_width(g_wiz_install_progress_bar, LV_PCT(100));
    lv_bar_set_range(g_wiz_install_progress_bar, 0, 100);
    lv_bar_set_value(g_wiz_install_progress_bar, 0, LV_ANIM_OFF);

    g_wiz_install_progress_result = lv_label_create(g_wiz_install_progress_panel);
    lv_label_set_text(g_wiz_install_progress_result, "");
    lv_obj_set_style_text_font(g_wiz_install_progress_result, CJK_FONT_SMALL, 0);
    lv_obj_set_style_text_color(g_wiz_install_progress_result, g_colors->text_dim, 0);

    g_wiz_install_progress_source = lv_label_create(g_wiz_install_progress_panel);
    lv_label_set_text(g_wiz_install_progress_source, "Source: Auto fallback");
    lv_obj_set_style_text_font(g_wiz_install_progress_source, CJK_FONT_SMALL, 0);
    lv_obj_set_style_text_color(g_wiz_install_progress_source, g_colors->text_dim, 0);

    g_wiz_install_progress_speed = lv_label_create(g_wiz_install_progress_panel);
    lv_label_set_text(g_wiz_install_progress_speed, "Speed: estimating...");
    lv_obj_set_style_text_font(g_wiz_install_progress_speed, CJK_FONT_SMALL, 0);
    lv_obj_set_style_text_color(g_wiz_install_progress_speed, g_colors->text_dim, 0);

    g_wiz_install_progress_eta = lv_label_create(g_wiz_install_progress_panel);
    lv_label_set_text(g_wiz_install_progress_eta, "ETA: calculating...");
    lv_obj_set_style_text_font(g_wiz_install_progress_eta, CJK_FONT_SMALL, 0);
    lv_obj_set_style_text_color(g_wiz_install_progress_eta, g_colors->text_dim, 0);

    g_wiz_install_progress_cancel_btn = lv_button_create(g_wiz_install_progress_panel);
    lv_obj_set_size(g_wiz_install_progress_cancel_btn, SCALE(140), SCALE(32));
    lv_obj_set_style_bg_color(g_wiz_install_progress_cancel_btn, g_colors->btn_close, 0);
    lv_obj_set_style_radius(g_wiz_install_progress_cancel_btn, SCALE(g_colors->radius_md), 0);
    lv_obj_add_event_cb(g_wiz_install_progress_cancel_btn, [](lv_event_t* e) {
        (void)e;
        app_request_setup_cancel();
        ui_log("[Gemma] Cancel requested by user");
        if (g_wiz_install_progress_step) lv_label_set_text(g_wiz_install_progress_step, "Step: Cancel requested");
    }, LV_EVENT_CLICKED, nullptr);
    lv_obj_t* lbl_cancel_dl = lv_label_create(g_wiz_install_progress_cancel_btn);
    lv_label_set_text(lbl_cancel_dl, g_lang == Lang::CN ? "取消下载" : "Cancel Download");
    lv_obj_set_style_text_font(lbl_cancel_dl, CJK_FONT_SMALL, 0);
    lv_obj_center(lbl_cancel_dl);
    lv_obj_add_state(g_wiz_install_progress_cancel_btn, LV_STATE_DISABLED);

    /* Skip button — Gemma is optional, always allow Next */
    lv_obj_t* btn_skip_gemma = lv_button_create(g_wizard_content);
    lv_obj_set_size(btn_skip_gemma, LV_PCT(100), SCALE(36));
    lv_obj_set_style_bg_color(btn_skip_gemma, g_colors->accent_secondary, 0);
    lv_obj_set_style_radius(btn_skip_gemma, SCALE(g_colors->radius_md), 0);
    lv_obj_t* lbl_skip = lv_label_create(btn_skip_gemma);
    lv_label_set_text(lbl_skip, g_lang == Lang::CN ? "跳过（继续）" : "Skip (continue)");
    lv_obj_set_style_text_font(lbl_skip, CJK_FONT_SMALL, 0);
    lv_obj_center(lbl_skip);
    lv_obj_add_event_cb(btn_skip_gemma, wizard_gemma_skip_cb, LV_EVENT_CLICKED, nullptr);
}

static void wizard_gemma_skip_cb(lv_event_t* e) {
    (void)e;
    g_gemma_install_opt_in = false;
    g_gemma_model_mask = 0;
    update_gemma_recommend_visuals();
    ui_log("[Wizard] Gemma install skipped by user");
    wizard_go_step(g_wizard_step + 1);
}

static void wizard_im_refresh_status_labels() {
    auto set_status = [](lv_obj_t* lbl, bool ok) {
        if (!lbl) return;
        lv_label_set_text(lbl, ok ? "已连接" : "未配置");
        lv_obj_set_style_text_color(lbl, ok ? g_colors->success : g_colors->warning, 0);
    };
    set_status(g_wiz_im_tg_status, g_wiz_im_tg_connected);
    set_status(g_wiz_im_discord_status, g_wiz_im_discord_connected);
    set_status(g_wiz_im_whatsapp_status, g_wiz_im_whatsapp_connected);
}

static void wizard_im_token_close_dialog() {
    if (g_wiz_im_token_overlay) {
        lv_obj_del(g_wiz_im_token_overlay);
        g_wiz_im_token_overlay = nullptr;
    }
    g_wiz_im_token_ta = nullptr;
    g_wiz_im_active_token = nullptr;
    g_wiz_im_active_token_cap = 0;
    g_wiz_im_active_connected = nullptr;
}

static void wizard_im_token_cancel_cb(lv_event_t* e) {
    (void)e;
    wizard_im_token_close_dialog();
}

static void wizard_im_token_save_cb(lv_event_t* e) {
    (void)e;
    if (g_wiz_im_token_ta && g_wiz_im_active_token && g_wiz_im_active_token_cap > 0) {
        const char* text = lv_textarea_get_text(g_wiz_im_token_ta);
        snprintf(g_wiz_im_active_token, g_wiz_im_active_token_cap, "%s", text ? text : "");
        if (g_wiz_im_active_connected) *g_wiz_im_active_connected = (text && text[0]);
    }
    wizard_im_refresh_status_labels();
    wizard_im_token_close_dialog();
}

static void wizard_im_open_token_dialog(const char* title, char* token_buf, size_t token_cap, bool* connected_flag) {
    if (g_wiz_im_token_overlay) return;
    g_wiz_im_active_token = token_buf;
    g_wiz_im_active_token_cap = token_cap;
    g_wiz_im_active_connected = connected_flag;

    lv_obj_t* box = create_dialog(g_wizard_modal ? g_wizard_modal : lv_screen_active(),
                                  title, 0, 0, &g_wiz_im_token_overlay);
    if (!box) return;
    lv_obj_t* hint = lv_label_create(box);
    lv_label_set_text(hint, g_lang == Lang::CN ? "请输入 Bot Token，保存后标记为已连接。" : "Enter Bot token and save.");
    lv_obj_set_style_text_color(hint, g_colors->text_dim, 0);
    lv_obj_set_style_text_font(hint, CJK_FONT_SMALL, 0);
    lv_label_set_long_mode(hint, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(hint, LV_PCT(100));

    g_wiz_im_token_ta = lv_textarea_create(box);
    lv_textarea_set_one_line(g_wiz_im_token_ta, true);
    lv_textarea_set_placeholder_text(g_wiz_im_token_ta, "token...");
    lv_obj_set_size(g_wiz_im_token_ta, LV_PCT(100), SCALE(40));
    lv_obj_set_style_bg_color(g_wiz_im_token_ta, g_colors->surface, 0);
    lv_obj_set_style_text_color(g_wiz_im_token_ta, g_colors->text, 0);
    if (token_buf && token_buf[0]) lv_textarea_set_text(g_wiz_im_token_ta, token_buf);

    lv_obj_t* row = lv_obj_create(box);
    lv_obj_set_size(row, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(row, 0, 0);
    lv_obj_set_style_pad_all(row, 0, 0);
    lv_obj_set_style_pad_gap(row, 8, 0);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(row, LV_FLEX_ALIGN_END, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* btn_cancel = aw_btn_create(row, g_lang == Lang::CN ? "取消" : "Cancel", BTN_SECONDARY, SCALE(120), SCALE(34));
    lv_obj_add_event_cb(btn_cancel, wizard_im_token_cancel_cb, LV_EVENT_CLICKED, nullptr);
    lv_obj_t* btn_save = aw_btn_create(row, g_lang == Lang::CN ? "保存" : "Save", BTN_PRIMARY, SCALE(120), SCALE(34));
    lv_obj_add_event_cb(btn_save, wizard_im_token_save_cb, LV_EVENT_CLICKED, nullptr);
}

static void wizard_im_tg_config_cb(lv_event_t* e) {
    (void)e;
    wizard_im_open_token_dialog("Telegram Bot Token", g_wiz_im_tg_token, sizeof(g_wiz_im_tg_token), &g_wiz_im_tg_connected);
}

static void wizard_im_discord_config_cb(lv_event_t* e) {
    (void)e;
    wizard_im_open_token_dialog("Discord Bot Token", g_wiz_im_discord_token, sizeof(g_wiz_im_discord_token), &g_wiz_im_discord_connected);
}

static void wizard_im_whatsapp_bind_cb(lv_event_t* e) {
    (void)e;
    g_wiz_im_whatsapp_connected = true;
    wizard_im_refresh_status_labels();
    ui_log("[Wizard] WhatsApp QR binding marked as connected");
}

static void wizard_build_step_im() {
    lv_obj_t* hint = lv_label_create(g_wizard_content);
    lv_label_set_text(hint, g_lang == Lang::CN
        ? "可选：连接 IM 渠道，后续可通过手机远程指挥 AnyClaw。"
        : "Optional: connect IM channels for remote control.");
    lv_obj_set_style_text_color(hint, g_colors->text_dim, 0);
    lv_obj_set_style_text_font(hint, CJK_FONT, 0);

    auto add_channel_row = [&](const char* name, lv_obj_t** status_out, const char* btn_text, lv_event_cb_t cb) {
        lv_obj_t* row = lv_obj_create(g_wizard_content);
        lv_obj_set_size(row, LV_PCT(100), LV_SIZE_CONTENT);
        lv_obj_set_style_bg_color(row, g_colors->surface, 0);
        lv_obj_set_style_border_color(row, g_colors->border, 0);
        lv_obj_set_style_border_width(row, 1, 0);
        lv_obj_set_style_radius(row, SCALE(g_colors->radius_md), 0);
        lv_obj_set_style_pad_all(row, 8, 0);
        lv_obj_set_style_pad_gap(row, 8, 0);
        lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
        lv_obj_set_flex_align(row, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);

        lv_obj_t* left = lv_obj_create(row);
        lv_obj_set_size(left, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
        lv_obj_set_style_bg_opa(left, LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_width(left, 0, 0);
        lv_obj_set_style_pad_all(left, 0, 0);
        lv_obj_set_style_pad_gap(left, 4, 0);
        lv_obj_set_flex_flow(left, LV_FLEX_FLOW_COLUMN);
        lv_obj_set_flex_align(left, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        lv_obj_clear_flag(left, LV_OBJ_FLAG_SCROLLABLE);

        lv_obj_t* title = lv_label_create(left);
        lv_label_set_text(title, name);
        lv_obj_set_style_text_color(title, g_colors->text, 0);
        lv_obj_set_style_text_font(title, CJK_FONT, 0);

        *status_out = lv_label_create(left);
        lv_obj_set_style_text_font(*status_out, CJK_FONT_SMALL, 0);

        lv_obj_t* btn = aw_btn_create(row, btn_text, BTN_SECONDARY, SCALE(130), SCALE(32));
        lv_obj_add_event_cb(btn, cb, LV_EVENT_CLICKED, nullptr);
    };

    add_channel_row("Telegram", &g_wiz_im_tg_status, g_lang == Lang::CN ? "配置 Token" : "Configure", wizard_im_tg_config_cb);
    add_channel_row("Discord", &g_wiz_im_discord_status, g_lang == Lang::CN ? "配置 Token" : "Configure", wizard_im_discord_config_cb);
    add_channel_row("WhatsApp", &g_wiz_im_whatsapp_status, g_lang == Lang::CN ? "扫码绑定" : "Scan Bind", wizard_im_whatsapp_bind_cb);
    wizard_im_refresh_status_labels();

    lv_obj_t* skip_hint = lv_label_create(g_wizard_content);
    lv_label_set_text(skip_hint, g_lang == Lang::CN
        ? "可直接下一步（Skip），稍后可在设置中补充。"
        : "You can skip now and configure later in settings.");
    lv_obj_set_style_text_color(skip_hint, g_colors->text_dim, 0);
    lv_obj_set_style_text_font(skip_hint, CJK_FONT_SMALL, 0);
}

static void wizard_build_step_leader() {
    lv_obj_t* hint = lv_label_create(g_wizard_content);
    lv_label_set_text(hint, tr(W_LEADER_HINT));
    lv_obj_set_style_text_color(hint, g_colors->text_dim, 0);
    lv_obj_set_style_text_font(hint, CJK_FONT, 0);
    lv_label_set_long_mode(hint, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(hint, LV_PCT(100));

    lv_obj_t* box = lv_obj_create(g_wizard_content);
    lv_obj_set_size(box, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_style_bg_color(box, g_colors->surface, 0);
    lv_obj_set_style_border_width(box, 1, 0);
    lv_obj_set_style_border_color(box, g_colors->border, 0);
    lv_obj_set_style_radius(box, SCALE(g_colors->radius_md), 0);
    lv_obj_set_style_pad_all(box, 12, 0);
    lv_obj_set_flex_flow(box, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_gap(box, 8, 0);
    lv_obj_clear_flag(box, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* title = lv_label_create(box);
    lv_label_set_text(title, "OpenClaw Leader");
    lv_obj_set_style_text_color(title, g_colors->accent_secondary, 0);
    lv_obj_set_style_text_font(title, CJK_FONT, 0);

    lv_obj_t* desc = lv_label_create(box);
    lv_label_set_text(desc, tr(W_LEADER_NOTE));
    lv_obj_set_style_text_color(desc, g_colors->text, 0);
    lv_obj_set_style_text_font(desc, CJK_FONT, 0);
    lv_label_set_long_mode(desc, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(desc, LV_PCT(100));

    lv_obj_t* row = lv_obj_create(box);
    lv_obj_set_size(row, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(row, 0, 0);
    lv_obj_set_style_pad_all(row, 0, 0);
    lv_obj_set_style_pad_gap(row, 8, 0);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(row, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* sw_label = lv_label_create(row);
    lv_label_set_text(sw_label, g_lang == Lang::CN ? "启用 Leader 模式" : "Enable Leader mode");
    lv_obj_set_style_text_color(sw_label, g_colors->text, 0);
    lv_obj_set_style_text_font(sw_label, CJK_FONT_SMALL, 0);

    g_wiz_leader_sw = lv_switch_create(row);
    if (g_wizard_leader_mode) lv_obj_add_state(g_wiz_leader_sw, LV_STATE_CHECKED);
    lv_obj_add_event_cb(g_wiz_leader_sw, [](lv_event_t* e) {
        lv_obj_t* sw = (lv_obj_t*)lv_event_get_target(e);
        g_wizard_leader_mode = lv_obj_has_state(sw, LV_STATE_CHECKED);
    }, LV_EVENT_VALUE_CHANGED, nullptr);

    lv_obj_t* rt_label = lv_label_create(box);
    lv_label_set_text(rt_label, g_lang == Lang::CN ? "默认运行时" : "Default runtime");
    lv_obj_set_style_text_color(rt_label, g_colors->text_dim, 0);
    lv_obj_set_style_text_font(rt_label, CJK_FONT_SMALL, 0);

    g_wiz_runtime_dd = lv_dropdown_create(box);
    lv_dropdown_set_options(g_wiz_runtime_dd, "OpenClaw\nHermes\nClaude");
    lv_dropdown_set_selected(g_wiz_runtime_dd, (uint16_t)((int)g_wizard_active_runtime));
    lv_obj_set_width(g_wiz_runtime_dd, LV_PCT(100));
    lv_obj_set_style_bg_color(g_wiz_runtime_dd, g_colors->panel, 0);
    lv_obj_set_style_text_color(g_wiz_runtime_dd, g_colors->text, 0);
    lv_obj_set_style_border_color(g_wiz_runtime_dd, g_colors->border, 0);
    lv_obj_set_style_border_width(g_wiz_runtime_dd, 1, 0);
    lv_obj_set_style_radius(g_wiz_runtime_dd, SCALE(g_colors->radius_sm), 0);
    lv_obj_set_style_text_font(g_wiz_runtime_dd, CJK_FONT, 0);
    lv_obj_set_style_text_font(g_wiz_runtime_dd, FONT(14), LV_PART_INDICATOR);
    lv_obj_set_style_text_font(g_wiz_runtime_dd, CJK_FONT, LV_PART_ITEMS);
    lv_obj_set_style_text_color(g_wiz_runtime_dd, g_colors->text, LV_PART_ITEMS);
    lv_obj_set_style_bg_color(g_wiz_runtime_dd, g_colors->panel, LV_PART_ITEMS);
    lv_obj_add_event_cb(g_wiz_runtime_dd, [](lv_event_t* e) {
        lv_obj_t* dd = (lv_obj_t*)lv_event_get_target(e);
        uint16_t sel = lv_dropdown_get_selected(dd);
        if (sel <= 2) g_wizard_active_runtime = (Runtime)sel;
    }, LV_EVENT_VALUE_CHANGED, nullptr);

    lv_obj_t* btn_skip = lv_button_create(box);
    lv_obj_set_size(btn_skip, LV_PCT(100), SCALE(34));
    lv_obj_set_style_bg_color(btn_skip, g_colors->btn_secondary, 0);
    lv_obj_set_style_radius(btn_skip, SCALE(g_colors->radius_md), 0);
    lv_obj_add_event_cb(btn_skip, wizard_leader_skip_cb, LV_EVENT_CLICKED, nullptr);
    lv_obj_t* skip_lbl = lv_label_create(btn_skip);
    lv_label_set_text(skip_lbl, tr(W_LEADER_SKIP));
    lv_obj_set_style_text_font(skip_lbl, CJK_FONT_SMALL, 0);
    lv_obj_center(skip_lbl);
}

static void wizard_leader_skip_cb(lv_event_t* e) {
    (void)e;
    g_wizard_leader_mode = false;
    g_wizard_active_runtime = Runtime::OpenClaw;
    if (g_wiz_leader_sw) lv_obj_clear_state(g_wiz_leader_sw, LV_STATE_CHECKED);
    if (g_wiz_runtime_dd) lv_dropdown_set_selected(g_wiz_runtime_dd, (uint16_t)Runtime::OpenClaw);
    ui_log("[Wizard] Leader mode skipped, fallback to single-agent OpenClaw");
    wizard_go_step(g_wizard_step + 1);
}

/* ── Step 6: Profile & Confirm ── */
static const char* tz_options =
    "UTC-12\nUTC-11\nUTC-10\nUTC-9\nUTC-8 (PST)\nUTC-7 (MST)\n"
    "UTC-6 (CST-US)\nUTC-5 (EST)\nUTC-4\nUTC-3\nUTC-2\nUTC-1\n"
    "UTC+0\nUTC+1 (CET)\nUTC+2 (EET)\nUTC+3 (MSK)\nUTC+4\n"
    "UTC+5\nUTC+5:30 (IST)\nUTC+6\nUTC+7\nUTC+8 (Asia/Shanghai)\n"
    "UTC+9 (JST)\nUTC+10 (AEST)\nUTC+11\nUTC+12 (NZST)";

/* Build summary text from current wizard state */
static void wizard_build_summary_text(char* buf, int buf_size) {
    const char* lang_str = (g_wizard_lang_sel == 0) ? tr(W_CN) : tr(W_EN);
    const char* detect_str = g_wizard_openclaw_ok ? tr(W_DETECTED) : tr(W_NOTFOUND);
    const char* api_str = (g_wizard_api_key[0]) ? "****" : tr(W_NOTSET);
    const char* model_str = g_wizard_model_name[0] ? g_wizard_model_name : "N/A";
    const char* nick_str = g_wizard_nickname[0] ? g_wizard_nickname : tr(W_ANONYMOUS);
    const char* tg = g_wiz_im_tg_connected ? "on" : "off";
    const char* dc = g_wiz_im_discord_connected ? "on" : "off";
    const char* wa = g_wiz_im_whatsapp_connected ? "on" : "off";

    static char tz_str[64];
    if (g_wiz_tz_dd) {
        lv_dropdown_get_selected_str(g_wiz_tz_dd, tz_str, sizeof(tz_str));
    } else {
        snprintf(tz_str, sizeof(tz_str), "UTC+8 (Asia/Shanghai)");
    }

    const char* gemma_sw = g_gemma_install_opt_in ? "enabled" : "disabled";
    const char* gemma_models = gemma_mask_to_text(g_gemma_model_mask);
    const char* leader_mode = g_wizard_leader_mode ? "leader" : "single";
    const char* runtime_str = (g_wizard_active_runtime == Runtime::Hermes) ? "Hermes"
        : (g_wizard_active_runtime == Runtime::Claude) ? "Claude" : "OpenClaw";
    snprintf(buf, buf_size,
        "%s %s\n%s %s\n%s %s\n%s %s\n%s %s\n%s %s\nLeader Mode %s\nActive Runtime %s\nGemma Local %s\nGemma Models %s\nIM Telegram %s / Discord %s / WhatsApp %s",
        tr(W_LANG_LABEL), lang_str,
        tr(W_OC_LABEL), detect_str,
        tr(W_APIK_LABEL), api_str,
        tr(W_MODEL_LABEL), model_str,
        tr(W_NAME_LABEL), nick_str,
        tr(W_TZ_HINT), tz_str,
        leader_mode, runtime_str,
        gemma_sw, gemma_models,
        tg, dc, wa);
}

static void wizard_build_step_profile() {
    lv_obj_t* final_hint = lv_label_create(g_wizard_content);
    lv_label_set_text(final_hint, tr(W_FINAL_HINT));
    lv_label_set_long_mode(final_hint, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(final_hint, LV_PCT(100));
    lv_obj_set_style_text_color(final_hint, g_colors->accent_secondary, 0);
    lv_obj_set_style_text_font(final_hint, CJK_FONT_SMALL, 0);

    /* ── Nickname ── */
    lv_obj_t* hint = lv_label_create(g_wizard_content);
    lv_label_set_text(hint, tr(W_NICK_HINT));
    lv_obj_set_style_text_color(hint, g_colors->text_dim, 0);
    lv_obj_set_style_text_font(hint, CJK_FONT, 0);

    g_wiz_nick_ta = lv_textarea_create(g_wizard_content);
    lv_textarea_set_one_line(g_wiz_nick_ta, true);
    lv_textarea_set_max_length(g_wiz_nick_ta, 32);
    lv_textarea_set_placeholder_text(g_wiz_nick_ta, tr(W_NICK_PH));
    if (g_wizard_nickname[0]) {
        lv_textarea_set_text(g_wiz_nick_ta, g_wizard_nickname);
    }
    lv_obj_set_width(g_wiz_nick_ta, LV_PCT(100));
    lv_obj_set_height(g_wiz_nick_ta, 44);
    lv_obj_set_style_bg_color(g_wiz_nick_ta, g_colors->surface, 0);
    lv_obj_set_style_border_color(g_wiz_nick_ta, g_colors->border, 0);
    lv_obj_set_style_border_width(g_wiz_nick_ta, 1, 0);
    lv_obj_set_style_radius(g_wiz_nick_ta, SCALE(g_colors->radius_sm), 0);
    lv_obj_set_style_text_color(g_wiz_nick_ta, g_colors->text, 0);
    lv_obj_set_style_text_font(g_wiz_nick_ta, CJK_FONT, 0);
    lv_obj_set_style_pad_all(g_wiz_nick_ta, 8, 0);
    lv_group_add_obj(lv_group_get_default(), g_wiz_nick_ta);

    /* ── Timezone ── */
    lv_obj_t* tz_hint = lv_label_create(g_wizard_content);
    lv_label_set_text(tz_hint, tr(W_TZ_HINT));
    lv_obj_set_style_text_color(tz_hint, g_colors->text_dim, 0);
    lv_obj_set_style_text_font(tz_hint, CJK_FONT, 0);

    g_wiz_tz_dd = lv_dropdown_create(g_wizard_content);
    lv_dropdown_set_options(g_wiz_tz_dd, tz_options);
    lv_dropdown_set_selected(g_wiz_tz_dd, g_wizard_tz_sel);
    lv_obj_set_width(g_wiz_tz_dd, LV_PCT(100));
    lv_obj_set_style_bg_color(g_wiz_tz_dd, g_colors->surface, 0);
    lv_obj_set_style_text_color(g_wiz_tz_dd, g_colors->text, 0);
    lv_obj_set_style_border_color(g_wiz_tz_dd, g_colors->border, 0);
    lv_obj_set_style_border_width(g_wiz_tz_dd, 1, 0);
    lv_obj_set_style_radius(g_wiz_tz_dd, SCALE(g_colors->radius_sm), 0);
    lv_obj_set_style_text_font(g_wiz_tz_dd, CJK_FONT, 0);
    lv_obj_set_style_text_font(g_wiz_tz_dd, FONT(14), LV_PART_INDICATOR);
    lv_obj_set_style_text_font(g_wiz_tz_dd, CJK_FONT, LV_PART_ITEMS);
    lv_obj_set_style_text_color(g_wiz_tz_dd, g_colors->text, LV_PART_ITEMS);
    lv_obj_set_style_bg_color(g_wiz_tz_dd, g_colors->panel, LV_PART_ITEMS);

    /* ── Divider ── */
    lv_obj_t* div = lv_obj_create(g_wizard_content);
    lv_obj_set_size(div, LV_PCT(100), 1);
    lv_obj_set_style_bg_color(div, g_colors->divider, 0);
    lv_obj_set_style_border_width(div, 0, 0);
    lv_obj_clear_flag(div, LV_OBJ_FLAG_SCROLLABLE);

    /* ── Summary ── */
    lv_obj_t* summary_hint = lv_label_create(g_wizard_content);
    lv_label_set_text(summary_hint, tr(W_SUMMARY));
    lv_obj_set_style_text_color(summary_hint, g_colors->accent_secondary, 0);
    lv_obj_set_style_text_font(summary_hint, CJK_FONT, 0);

    lv_obj_t* box = lv_obj_create(g_wizard_content);
    lv_obj_set_size(box, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_style_bg_color(box, g_colors->surface, 0);
    lv_obj_set_style_border_width(box, 1, 0);
    lv_obj_set_style_border_color(box, g_colors->border, 0);
    lv_obj_set_style_radius(box, SCALE(g_colors->radius_md), 0);
    lv_obj_set_style_pad_all(box, 16, 0);
    lv_obj_set_flex_flow(box, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_gap(box, 8, 0);
    lv_obj_clear_flag(box, LV_OBJ_FLAG_SCROLLABLE);

    static char summary_buf[1280];
    wizard_build_summary_text(summary_buf, sizeof(summary_buf));

    g_wiz_summary_lbl = lv_label_create(box);
    lv_label_set_text(g_wiz_summary_lbl, summary_buf);
    lv_obj_set_style_text_color(g_wiz_summary_lbl, g_colors->text, 0);
    lv_obj_set_style_text_font(g_wiz_summary_lbl, CJK_FONT, 0);
    lv_label_set_long_mode(g_wiz_summary_lbl, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(g_wiz_summary_lbl, LV_PCT(100));

    lv_obj_t* stageb = lv_label_create(g_wizard_content);
    lv_label_set_text(stageb, tr(W_STAGEB_HINT));
    lv_label_set_long_mode(stageb, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(stageb, LV_PCT(100));
    lv_obj_set_style_text_color(stageb, g_colors->text_dim, 0);
    lv_obj_set_style_text_font(stageb, CJK_FONT_SMALL, 0);
}

/* Refresh summary text when returning to final step */
/* ── Build step content ── */
static void wizard_update_step_bar(int current_step) {
    if (!g_wizard_step_bar) return;
    for (int i = 0; i < WIZARD_STEPS; i++) {
        if (!g_wizard_step_dots[i]) continue;
        bool done = (i < current_step);
        bool active = (i == current_step);
        if (done) {
            /* Completed: accent filled + check icon */
            lv_obj_set_style_bg_color(g_wizard_step_dots[i], g_colors->accent, 0);
            lv_obj_set_style_bg_opa(g_wizard_step_dots[i], LV_OPA_COVER, 0);
            lv_obj_set_style_border_width(g_wizard_step_dots[i], 0, 0);
            lv_obj_set_size(g_wizard_step_dots[i], SCALE(12), SCALE(12));
            lv_obj_set_style_shadow_width(g_wizard_step_dots[i], 0, 0);
            /* Show check label */
            lv_obj_t* check = lv_obj_get_child(g_wizard_step_dots[i], 0);
            if (check && lv_obj_check_type(check, &lv_label_class)) {
                lv_obj_clear_flag(check, LV_OBJ_FLAG_HIDDEN);
            }
        } else if (active) {
            /* Current: accent filled + glow pulse */
            lv_obj_set_style_bg_color(g_wizard_step_dots[i], g_colors->accent, 0);
            lv_obj_set_style_bg_opa(g_wizard_step_dots[i], LV_OPA_COVER, 0);
            lv_obj_set_style_border_width(g_wizard_step_dots[i], 0, 0);
            lv_obj_set_size(g_wizard_step_dots[i], SCALE(14), SCALE(14));
            /* Glow shadow */
            lv_obj_set_style_shadow_width(g_wizard_step_dots[i], SCALE(8), 0);
            lv_obj_set_style_shadow_color(g_wizard_step_dots[i], g_colors->accent, 0);
            lv_obj_set_style_shadow_opa(g_wizard_step_dots[i], LV_OPA_40, 0);
            /* Hide check label */
            lv_obj_t* check = lv_obj_get_child(g_wizard_step_dots[i], 0);
            if (check && lv_obj_check_type(check, &lv_label_class)) {
                lv_obj_add_flag(check, LV_OBJ_FLAG_HIDDEN);
            }
            /* Start glow pulse animation */
            anim_pulse_opa(g_wizard_step_dots[i], 2000);
        } else {
            /* Future: outline only */
            lv_obj_set_style_bg_opa(g_wizard_step_dots[i], LV_OPA_TRANSP, 0);
            lv_obj_set_style_border_width(g_wizard_step_dots[i], 2, 0);
            lv_obj_set_style_border_color(g_wizard_step_dots[i], g_colors->border, 0);
            lv_obj_set_style_radius(g_wizard_step_dots[i], LV_RADIUS_CIRCLE, 0);
            lv_obj_set_size(g_wizard_step_dots[i], SCALE(12), SCALE(12));
            lv_obj_set_style_shadow_width(g_wizard_step_dots[i], 0, 0);
            /* Hide check label */
            lv_obj_t* check = lv_obj_get_child(g_wizard_step_dots[i], 0);
            if (check && lv_obj_check_type(check, &lv_label_class)) {
                lv_obj_add_flag(check, LV_OBJ_FLAG_HIDDEN);
            }
        }
        /* Update label color */
        if (g_wizard_step_labels[i]) {
            lv_obj_set_style_text_color(g_wizard_step_labels[i],
                active ? g_colors->accent : (done ? g_colors->text : g_colors->text_dim), 0);
        }
        /* Update connecting lines */
        if (i < WIZARD_STEPS - 1 && g_wizard_step_lines[i]) {
            lv_obj_set_style_bg_color(g_wizard_step_lines[i],
                done ? g_colors->accent : g_colors->border, 0);
        }
    }
}

static void wizard_update_step() {
    if (!g_wizard_content || !g_wizard_step_label || !g_wizard_title) return;

    /* Save inputs from current step before clearing */
    if (g_wizard_step == 2 && g_wiz_api_ta) {
        const char* t = lv_textarea_get_text(g_wiz_api_ta);
        if (t) snprintf(g_wizard_api_key, sizeof(g_wizard_api_key), "%s", t);
    }
    if (g_wizard_step == 2 && g_wiz_model_dd) {
        g_wizard_model_sel = lv_dropdown_get_selected(g_wiz_model_dd);
        lv_dropdown_get_selected_str(g_wiz_model_dd, g_wizard_model_name, sizeof(g_wizard_model_name));
        char* free_tag = strstr(g_wizard_model_name, " [free]");
        if (free_tag) *free_tag = '\0';
    }
    if (g_wizard_step == 3 && g_wiz_nick_ta) {
        const char* t = lv_textarea_get_text(g_wiz_nick_ta);
        if (t) snprintf(g_wizard_nickname, sizeof(g_wizard_nickname), "%s", t);
    }
    if (g_wizard_step == 3 && g_wiz_tz_dd) {
        g_wizard_tz_sel = lv_dropdown_get_selected(g_wiz_tz_dd);
    }
    if (g_wizard_step == 5 && g_wiz_leader_sw) {
        g_wizard_leader_mode = lv_obj_has_state(g_wiz_leader_sw, LV_STATE_CHECKED);
    }
    if (g_wizard_step == 5 && g_wiz_runtime_dd) {
        uint16_t sel = lv_dropdown_get_selected(g_wiz_runtime_dd);
        if (sel <= 2) g_wizard_active_runtime = (Runtime)sel;
    }

    /* Clear content area */
    lv_obj_clean(g_wizard_content);

    /* Reset step widget refs */
    g_wiz_btn_cn = nullptr;
    g_wiz_btn_en = nullptr;
    g_wiz_detect_lbl = nullptr;
    g_wiz_api_ta = nullptr;
    g_wiz_model_dd = nullptr;
    g_wiz_nick_ta = nullptr;
    g_wiz_tz_dd = nullptr;
    g_wiz_summary_lbl = nullptr;
    g_wiz_btn_install_net = nullptr;
    g_wiz_btn_install_local = nullptr;
    g_wiz_btn_cancel_install = nullptr;
    g_wiz_install_progress_panel = nullptr;
    g_wiz_install_progress_task = nullptr;
    g_wiz_install_progress_step = nullptr;
    g_wiz_install_progress_result = nullptr;
    g_wiz_install_progress_bar = nullptr;

    /* Update step indicator */
    char step_text[48];
    const char* phase_txt = (g_lang == Lang::CN) ? "阶段A" : "Phase A";
    snprintf(step_text, sizeof(step_text), "%s  ·  %d/%d", phase_txt, g_wizard_step + 1, WIZARD_STEPS);
    lv_label_set_text(g_wizard_step_label, step_text);

    /* Update step indicator bar */
    wizard_update_step_bar(g_wizard_step);

    /* Update title */
    lv_label_set_text(g_wizard_title, tr(wizard_step_titles[g_wizard_step]));

    /* Build step content — default-enable Next before build so build functions can override */
    wizard_set_next_enabled(true);
    switch (g_wizard_step) {
        case 0: wizard_build_step_lang(); break;
        case 1: wizard_build_step_detect(); break;       /* May override: wizard_set_next_enabled(critical_ok) */
        case 2: wizard_build_step_model_api(); break;    /* Model + API Key */
        case 3: wizard_build_step_profile(); break;      /* Final confirm */
    }

    /* Update buttons */
    if (g_wizard_step == 0) {
        lv_obj_add_flag(g_wizard_btn_prev, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_clear_flag(g_wizard_btn_prev, LV_OBJ_FLAG_HIDDEN);
    }

    if (g_wizard_step == 1 && g_wiz_install_running.load()) wizard_set_next_enabled(false);

    if (g_wizard_step == WIZARD_STEPS - 1) {
        /* Last step: "Get Started" button */
        lv_obj_t* lbl = lv_obj_get_child(g_wizard_btn_next, 0);
        if (lbl) lv_label_set_text(lbl, tr(W_GET_STARTED));
        lv_obj_set_style_bg_color(g_wizard_btn_next, g_colors->success, 0); /* Green */
    } else {
        lv_obj_t* lbl = lv_obj_get_child(g_wizard_btn_next, 0);
        if (lbl) lv_label_set_text(lbl, tr(W_NEXT));
        lv_obj_set_style_bg_color(g_wizard_btn_next, g_colors->accent_secondary, 0); /* Blue */
    }
}

/* ── Wizard navigation callbacks ── */
static void wizard_prev_cb(lv_event_t* e) {
    (void)e;
    if (g_wizard_step > 0) {
        wizard_go_step(g_wizard_step - 1);
    }
}

static void wizard_next_cb(lv_event_t* e) {
    (void)e;

    /* Save inputs from current step */
    if (g_wizard_step == 2 && g_wiz_api_ta) {
        const char* t = lv_textarea_get_text(g_wiz_api_ta);
        if (t) snprintf(g_wizard_api_key, sizeof(g_wizard_api_key), "%s", t);
    }
    if (g_wizard_step == 2 && g_wiz_model_dd) {
        g_wizard_model_sel = lv_dropdown_get_selected(g_wiz_model_dd);
        lv_dropdown_get_selected_str(g_wiz_model_dd, g_wizard_model_name, sizeof(g_wizard_model_name));
        char* free_tag = strstr(g_wizard_model_name, " [free]");
        if (free_tag) *free_tag = '\0';
    }
    if (g_wizard_step == 3 && g_wiz_nick_ta) {
        const char* t = lv_textarea_get_text(g_wiz_nick_ta);
        if (t) snprintf(g_wizard_nickname, sizeof(g_wizard_nickname), "%s", t);
    }
    if (g_wizard_step == 3 && g_wiz_tz_dd) {
        g_wizard_tz_sel = lv_dropdown_get_selected(g_wiz_tz_dd);
    }
    if (g_wizard_step == 5 && g_wiz_leader_sw) {
        g_wizard_leader_mode = lv_obj_has_state(g_wiz_leader_sw, LV_STATE_CHECKED);
    }
    if (g_wizard_step == 5 && g_wiz_runtime_dd) {
        uint16_t sel = lv_dropdown_get_selected(g_wiz_runtime_dd);
        if (sel <= 2) g_wizard_active_runtime = (Runtime)sel;
    }

    if (g_wizard_step < WIZARD_STEPS - 1) {
        wizard_go_step(g_wizard_step + 1);
    } else {
        /* P0: Warn if API key is empty before finishing */
        if (!g_wizard_api_key[0]) {
            /* Show native-style confirmation — user can still proceed */
            ui_log("[Wizard] Warning: API key is empty. Some features will not work.");
            /* Log warning but don't block — free models or local Gemma may suffice */
        }

        /* Last step: finish wizard */
        /* Apply language */
        g_lang = (g_wizard_lang_sel == 0) ? Lang::CN : Lang::EN;

        /* Apply model selection (save to config) */
        /* API key and nickname are already in g_wizard_api_key / g_wizard_nickname */

        /* Get selected timezone string */
        static char sel_tz[64];
        if (g_wiz_tz_dd) {
            lv_dropdown_get_selected_str(g_wiz_tz_dd, sel_tz, sizeof(sel_tz));
        } else {
            snprintf(sel_tz, sizeof(sel_tz), "UTC+8 (Asia/Shanghai)");
        }

        /* Write USER.md — workspace path for Agent consumption */
        {
            /* Test path (cross-compile env) + production path (%USERPROFILE%/.openclaw/workspace/) */
            std::string workspace_path;
            const char* userprofile = std::getenv("USERPROFILE");
            if (userprofile) {
                workspace_path = std::string(userprofile) + "\\.openclaw\\workspace\\USER.md";
                std::filesystem::create_directories(std::string(userprofile) + "\\.openclaw\\workspace");
            }

            const char* user_md_paths[] = {
                "Z:\\root\\.openclaw\\workspace\\USER.md",   /* test path */
                nullptr
            };
            /* Build USER.md content */
            std::string md_content;
            md_content += "# USER.md - About Your Human\n\n";
            md_content += std::string("- **Name:** ") + (g_wizard_nickname[0] ? g_wizard_nickname : "") + "\n";
            md_content += std::string("- **What to call them:** ") + (g_wizard_nickname[0] ? g_wizard_nickname : "") + "\n";
            md_content += "- **Pronouns:**\n";
            md_content += std::string("- **Timezone:** ") + sel_tz + "\n";
            md_content += "- **Notes:**\n\n";
            md_content += "## Context\n\n";
            md_content += "\n";

            /* Write to test path */
            for (int pi = 0; user_md_paths[pi]; pi++) {
                std::ofstream uf(user_md_paths[pi]);
                if (uf.is_open()) {
                    uf << md_content;
                    uf.close();
                    ui_log("[Wizard] USER.md written to %s", user_md_paths[pi]);
                }
            }
            /* Write to production path */
            if (!workspace_path.empty()) {
                std::ofstream uf(workspace_path);
                if (uf.is_open()) {
                    uf << md_content;
                    uf.close();
                    ui_log("[Wizard] USER.md written to %s", workspace_path.c_str());
                }
            }
        }

        /* Save all config */
        /* Sync wizard selections to global variables */
        if (g_wizard_api_key[0]) {
            snprintf(g_api_key, sizeof(g_api_key), "%s", g_wizard_api_key);
        }
        if (g_wiz_model_dd) {
            char model_buf[128];
            lv_dropdown_get_selected_str(g_wiz_model_dd, model_buf, sizeof(model_buf));
            char* free_tag = strstr(model_buf, " [free]");
            if (free_tag) *free_tag = '\0';
            snprintf(g_selected_model, sizeof(g_selected_model), "%s", model_buf);
        }
        save_wizard_completed();
        g_wizard_session_finished = true;
        save_theme_config();

        if (g_gemma_install_opt_in && g_gemma_model_mask != 0) {
            start_gemma_install_async(g_gemma_model_mask);
        }

        /* Sync model + API key to Gateway */
        if (g_selected_model[0]) {
            ui_log("[Wizard] Syncing model=%s to Gateway...", g_selected_model);
            app_update_model_config(
                g_api_key[0] ? g_api_key : nullptr,
                g_selected_model
            );
        }

        /* Persist active runtime according to leader-mode decision */
        if (!g_wizard_leader_mode) {
            g_wizard_active_runtime = Runtime::OpenClaw;
        }
        g_wizard_hermes_enabled = (g_wizard_active_runtime == Runtime::Hermes);
        app_set_active_runtime(g_wizard_active_runtime);

        ui_log("[Wizard] Setup completed. Lang=%s, Model=%d, Nick=%s, TZ=%s",
               g_wizard_lang_sel == 0 ? "CN" : "EN",
               g_wizard_model_sel,
               g_wizard_nickname[0] ? g_wizard_nickname : "(none)",
               sel_tz);

        /* Close wizard */
        if (g_wizard_modal) {
            lv_obj_del(g_wizard_modal);
            g_wizard_modal = nullptr;
        }

        /* Update UI language */
        update_ui_language();
    }
}

static void wizard_close_cb(lv_event_t* e) {
    (void)e;
    if (g_wiz_install_running.load()) {
        app_request_setup_cancel();
    }
    /* FIX: Clean up install poll timer to prevent leak + idle polling */
    if (g_wiz_install_poll_timer) {
        lv_timer_del(g_wiz_install_poll_timer);
        g_wiz_install_poll_timer = nullptr;
    }
    if (g_wiz_detect_poll_timer) {
        lv_timer_del(g_wiz_detect_poll_timer);
        g_wiz_detect_poll_timer = nullptr;
    }
    g_wiz_detect_running.store(false);
    g_wiz_detect_result_ready.store(false);
    if (g_wizard_modal) {
        lv_obj_del(g_wizard_modal);
        g_wizard_modal = nullptr;
    }

    /* If OpenClaw was installed during this wizard but user didn't finish → full uninstall */
    if (g_wizard_oc_installed_now && !g_wizard_session_finished) {
        printf("[Wizard] User cancelled after OC install → full uninstall\n");
        ui_log("[Wizard] Cancelling setup, removing OpenClaw...");
        char rm_output[512] = {0};
        app_uninstall_openclaw_full(rm_output, sizeof(rm_output), true);
        g_wizard_oc_installed_now = false;
    }
}

/* ── Main wizard UI ── */
void ui_show_setup_wizard_forced(void) { ui_show_setup_wizard(); }
void ui_show_setup_wizard() {
    if (g_wizard_modal) return;  /* Already showing */

    g_wizard_step = 0;
    g_wizard_oc_installed_now = false; /* Reset install tracking */
    g_wizard_session_finished = false;
    g_wiz_detect_running.store(false);
    g_wiz_detect_result_ready.store(false);
    /* FIX P2: Pre-fill wizard state from saved config */
    if (g_api_key[0] && !g_wizard_api_key[0]) {
        snprintf(g_wizard_api_key, sizeof(g_wizard_api_key), "%s", g_api_key);
    }
    if (g_selected_model[0] && !g_wizard_model_name[0]) {
        snprintf(g_wizard_model_name, sizeof(g_wizard_model_name), "%s", g_selected_model);
    }
    lv_obj_t* scr = lv_screen_active();

    /* Full-screen overlay */
    g_wizard_modal = lv_obj_create(scr);
    lv_obj_set_size(g_wizard_modal, WIN_W, WIN_H);
    lv_obj_set_pos(g_wizard_modal, 0, 0);
    lv_obj_set_style_bg_color(g_wizard_modal, g_colors->bg, 0);
    lv_obj_set_style_bg_opa(g_wizard_modal, LV_OPA_90, 0);
    lv_obj_set_style_border_width(g_wizard_modal, 0, 0);
    lv_obj_set_style_radius(g_wizard_modal, 0, 0);
    lv_obj_set_style_pad_all(g_wizard_modal, 0, 0);
    lv_obj_clear_flag(g_wizard_modal, LV_OBJ_FLAG_SCROLLABLE);

    /* Centered dialog box */
    int box_w = SCALE(560);
    int box_h = SCALE(480);
    g_wizard_box = lv_obj_create(g_wizard_modal);
    lv_obj_set_size(g_wizard_box, box_w, box_h);
    lv_obj_set_pos(g_wizard_box, (WIN_W - box_w) / 2, (WIN_H - box_h) / 2);
    lv_obj_set_style_bg_color(g_wizard_box, g_colors->panel, 0);
    lv_obj_set_style_bg_opa(g_wizard_box, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(g_wizard_box, 1, 0);
    lv_obj_set_style_border_color(g_wizard_box, g_colors->border, 0);
    lv_obj_set_style_radius(g_wizard_box, SCALE(g_colors->radius_lg), 0);
    lv_obj_set_style_pad_all(g_wizard_box, 0, 0);
    lv_obj_clear_flag(g_wizard_box, LV_OBJ_FLAG_SCROLLABLE);

    /* ── Header area ── */
    lv_obj_t* header = lv_obj_create(g_wizard_box);
    lv_obj_set_size(header, box_w, SCALE(56));
    lv_obj_set_pos(header, 0, 0);
    lv_obj_set_style_bg_color(header, g_colors->panel, 0);
    lv_obj_set_style_bg_opa(header, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(header, 0, 0);
    lv_obj_set_style_radius(header, SCALE(g_colors->radius_lg), 0);
    lv_obj_set_style_pad_all(header, 0, 0);
    lv_obj_clear_flag(header, LV_OBJ_FLAG_SCROLLABLE);

    /* Brand icon (top-left) */
    lv_obj_t* wiz_icon = lv_image_create(header);
    lv_image_set_src(wiz_icon, "A:assets/garlic_32.png");
    lv_obj_align(wiz_icon, LV_ALIGN_LEFT_MID, 16, 0);

    /* Title (after icon) */
    g_wizard_title = lv_label_create(header);
    lv_label_set_text(g_wizard_title, tr(wizard_step_titles[0]));
    lv_obj_set_style_text_color(g_wizard_title, g_colors->text, 0);
    lv_obj_set_style_text_font(g_wizard_title, CJK_FONT, 0);
    lv_obj_align(g_wizard_title, LV_ALIGN_LEFT_MID, SCALE(52), 0);

    /* Step counter (right of title) */
    g_wizard_step_label = lv_label_create(header);
    char step_text[48];
    snprintf(step_text, sizeof(step_text), "%s  ·  %d/%d", g_lang == Lang::CN ? "阶段A" : "Phase A", 1, WIZARD_STEPS);
    lv_label_set_text(g_wizard_step_label, step_text);
    lv_obj_set_style_text_color(g_wizard_step_label, g_colors->text_dim, 0);
    lv_obj_set_style_text_font(g_wizard_step_label, FONT(11), 0);
    lv_obj_align(g_wizard_step_label, LV_ALIGN_RIGHT_MID, -SCALE(110), 0);

    g_wizard_btn_close = lv_button_create(header);
    lv_obj_set_size(g_wizard_btn_close, SCALE(98), SCALE(32));
    lv_obj_set_style_bg_opa(g_wizard_btn_close, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(g_wizard_btn_close, 1, 0);
    lv_obj_set_style_border_color(g_wizard_btn_close, g_colors->border, 0);
    lv_obj_set_style_radius(g_wizard_btn_close, SCALE(g_colors->radius_md), 0);
    lv_obj_align(g_wizard_btn_close, LV_ALIGN_RIGHT_MID, -12, 0);
    lv_obj_add_event_cb(g_wizard_btn_close, wizard_close_cb, LV_EVENT_CLICKED, nullptr);
    lv_obj_t* close_lbl = lv_label_create(g_wizard_btn_close);
    lv_label_set_text(close_lbl, "退出向导");
    lv_obj_set_style_text_color(close_lbl, g_colors->text_dim, 0);
    lv_obj_set_style_text_font(close_lbl, CJK_FONT_SMALL, 0);
    lv_obj_center(close_lbl);

    /* Separator line under header */
    lv_obj_t* sep = lv_obj_create(g_wizard_box);
    lv_obj_set_size(sep, box_w, 1);
    lv_obj_set_pos(sep, 0, SCALE(56));
    lv_obj_set_style_bg_color(sep, g_colors->border, 0);
    lv_obj_set_style_bg_opa(sep, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(sep, 0, 0);
    lv_obj_set_style_pad_all(sep, 0, 0);
    lv_obj_clear_flag(sep, LV_OBJ_FLAG_SCROLLABLE);

    /* ── Step indicator bar ── */
    int step_bar_y = SCALE(56) + 1;
    int step_bar_h = SCALE(48);
    g_wizard_step_bar = lv_obj_create(g_wizard_box);
    lv_obj_set_size(g_wizard_step_bar, box_w, step_bar_h);
    lv_obj_set_pos(g_wizard_step_bar, 0, step_bar_y);
    lv_obj_set_style_bg_opa(g_wizard_step_bar, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(g_wizard_step_bar, 0, 0);
    lv_obj_set_style_pad_all(g_wizard_step_bar, 0, 0);
    lv_obj_clear_flag(g_wizard_step_bar, LV_OBJ_FLAG_SCROLLABLE);

    /* Calculate spacing: distribute dots evenly across the bar */
    int dot_size = SCALE(12);
    int bar_pad = SCALE(40);
    int usable_w = box_w - bar_pad * 2;
    int step_spacing = usable_w / (WIZARD_STEPS - 1);

    for (int i = 0; i < WIZARD_STEPS; i++) {
        int cx = bar_pad + i * step_spacing;

        /* Connecting line (before this dot, except first) */
        if (i > 0) {
            int line_x = bar_pad + (i - 1) * step_spacing + dot_size / 2 + 2;
            int line_w = step_spacing - dot_size - 4;
            if (line_w > 0) {
                g_wizard_step_lines[i - 1] = lv_obj_create(g_wizard_step_bar);
                lv_obj_set_size(g_wizard_step_lines[i - 1], line_w, 2);
                lv_obj_set_pos(g_wizard_step_lines[i - 1], line_x, SCALE(6));
                lv_obj_set_style_bg_color(g_wizard_step_lines[i - 1],
                    (i - 1 < g_wizard_step) ? g_colors->accent : g_colors->border, 0);
                lv_obj_set_style_bg_opa(g_wizard_step_lines[i - 1], LV_OPA_COVER, 0);
                lv_obj_set_style_border_width(g_wizard_step_lines[i - 1], 0, 0);
                lv_obj_set_style_radius(g_wizard_step_lines[i - 1], 1, 0);
                lv_obj_set_style_pad_all(g_wizard_step_lines[i - 1], 0, 0);
                lv_obj_clear_flag(g_wizard_step_lines[i - 1], LV_OBJ_FLAG_SCROLLABLE);
            }
        }

        /* Dot */
        g_wizard_step_dots[i] = lv_obj_create(g_wizard_step_bar);
        int this_dot = (i == g_wizard_step) ? SCALE(14) : dot_size;
        lv_obj_set_size(g_wizard_step_dots[i], this_dot, this_dot);
        lv_obj_set_pos(g_wizard_step_dots[i], cx - this_dot / 2, SCALE(2));
        lv_obj_set_style_radius(g_wizard_step_dots[i], LV_RADIUS_CIRCLE, 0);
        lv_obj_set_style_border_width(g_wizard_step_dots[i], 0, 0);
        lv_obj_set_style_pad_all(g_wizard_step_dots[i], 0, 0);
        lv_obj_clear_flag(g_wizard_step_dots[i], LV_OBJ_FLAG_SCROLLABLE);
        if (i <= g_wizard_step) {
            lv_obj_set_style_bg_color(g_wizard_step_dots[i], g_colors->accent, 0);
            lv_obj_set_style_bg_opa(g_wizard_step_dots[i], LV_OPA_COVER, 0);
        } else {
            lv_obj_set_style_bg_opa(g_wizard_step_dots[i], LV_OPA_TRANSP, 0);
            lv_obj_set_style_border_width(g_wizard_step_dots[i], 2, 0);
            lv_obj_set_style_border_color(g_wizard_step_dots[i], g_colors->border, 0);
        }
        /* Glow shadow for current step */
        if (i == g_wizard_step) {
            lv_obj_set_style_shadow_width(g_wizard_step_dots[i], SCALE(8), 0);
            lv_obj_set_style_shadow_color(g_wizard_step_dots[i], g_colors->accent, 0);
            lv_obj_set_style_shadow_opa(g_wizard_step_dots[i], LV_OPA_40, 0);
        }
        /* Check label inside completed dots */
        lv_obj_t* dot_check = lv_label_create(g_wizard_step_dots[i]);
        lv_label_set_text(dot_check, LV_SYMBOL_OK);
        lv_obj_set_style_text_font(dot_check, FONT(7), 0);
        lv_obj_set_style_text_color(dot_check, g_colors->text_inverse, 0);
        lv_obj_center(dot_check);
        if (i >= g_wizard_step) lv_obj_add_flag(dot_check, LV_OBJ_FLAG_HIDDEN);

        /* Label under dot */
        g_wizard_step_labels[i] = lv_label_create(g_wizard_step_bar);
        lv_label_set_text(g_wizard_step_labels[i], tr(wizard_step_short_names[i]));
        lv_obj_set_style_text_font(g_wizard_step_labels[i], FONT(10), 0);
        lv_obj_set_style_text_color(g_wizard_step_labels[i],
            (i == g_wizard_step) ? g_colors->accent : (i < g_wizard_step ? g_colors->text : g_colors->text_dim), 0);
        /* Position label centered under the dot */
        lv_obj_update_layout(g_wizard_step_labels[i]);
        int lbl_w = lv_obj_get_width(g_wizard_step_labels[i]);
        lv_obj_set_pos(g_wizard_step_labels[i], cx - lbl_w / 2, SCALE(20));
    }
    /* Start glow pulse on current step dot */
    if (g_wizard_step_dots[g_wizard_step]) {
        anim_pulse_opa(g_wizard_step_dots[g_wizard_step], 2000);
    }

    /* ── Content area (shifted down for step bar) ── */
    int content_y = step_bar_y + step_bar_h + 4;
    g_wizard_content = lv_obj_create(g_wizard_box);
    lv_obj_set_size(g_wizard_content, box_w - SCALE(24), box_h - content_y - SCALE(64));
    lv_obj_set_pos(g_wizard_content, SCALE(12), content_y);
    lv_obj_set_style_bg_opa(g_wizard_content, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(g_wizard_content, 0, 0);
    lv_obj_set_style_radius(g_wizard_content, 0, 0);
    lv_obj_set_style_pad_all(g_wizard_content, 0, 0);
    lv_obj_clear_flag(g_wizard_content, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_flex_flow(g_wizard_content, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(g_wizard_content, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_set_style_pad_gap(g_wizard_content, 12, 0);
    lv_obj_set_style_pad_column(g_wizard_content, SCALE(8), 0);

    /* ── Bottom button bar ── */
    lv_obj_t* btn_bar = lv_obj_create(g_wizard_box);
    lv_obj_set_size(btn_bar, box_w - 40, SCALE(44));
    lv_obj_set_pos(btn_bar, 20, box_h - SCALE(56));
    lv_obj_set_style_bg_opa(btn_bar, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(btn_bar, 0, 0);
    lv_obj_set_style_pad_all(btn_bar, 0, 0);
    lv_obj_clear_flag(btn_bar, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_flex_flow(btn_bar, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_gap(btn_bar, 12, 0);
    lv_obj_set_flex_align(btn_bar, LV_FLEX_ALIGN_END, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    /* Prev button */
    g_wizard_btn_prev = lv_button_create(btn_bar);
    lv_obj_set_size(g_wizard_btn_prev, SCALE(100), SCALE(40));
    lv_obj_set_style_bg_color(g_wizard_btn_prev, g_colors->disabled_bg, 0);
    lv_obj_set_style_bg_color(g_wizard_btn_prev, g_colors->btn_secondary, LV_STATE_PRESSED);
    lv_obj_set_style_radius(g_wizard_btn_prev, SCALE(g_colors->radius_md), 0);
    lv_obj_add_event_cb(g_wizard_btn_prev, wizard_prev_cb, LV_EVENT_CLICKED, nullptr);
    lv_obj_t* lbl_prev = lv_label_create(g_wizard_btn_prev);
    lv_label_set_text(lbl_prev, tr(W_BACK));
    lv_obj_set_style_text_font(lbl_prev, CJK_FONT, 0);
    lv_obj_center(lbl_prev);

    /* Next button */
    g_wizard_btn_next = lv_button_create(btn_bar);
    lv_obj_set_size(g_wizard_btn_next, SCALE(120), SCALE(40));
    lv_obj_set_style_bg_color(g_wizard_btn_next, g_colors->accent_secondary, 0);
    lv_obj_set_style_bg_color(g_wizard_btn_next, g_colors->accent_secondary, LV_STATE_PRESSED);
    lv_obj_set_style_radius(g_wizard_btn_next, SCALE(g_colors->radius_md), 0);
    lv_obj_add_event_cb(g_wizard_btn_next, wizard_next_cb, LV_EVENT_CLICKED, nullptr);
    lv_obj_t* lbl_next = lv_label_create(g_wizard_btn_next);
    lv_label_set_text(lbl_next, tr(W_NEXT));
    lv_obj_set_style_text_font(lbl_next, CJK_FONT, 0);
    lv_obj_center(lbl_next);

    /* Build first step content */
    wizard_update_step();

    lv_obj_move_foreground(g_wizard_modal);
    ui_log("[Wizard] Setup wizard opened");
}

static void start_wizard_gate_async_check() {
    if (g_wizard_gate_running.exchange(true)) return;
    g_wizard_gate_ready.store(false);
    std::thread([]() {
        EnvCheckResult env = app_check_environment();
        {
            std::lock_guard<std::mutex> lk(g_wizard_gate_mtx);
            g_wizard_gate_env = env;
        }
        g_wizard_gate_ready.store(true);
        g_wizard_gate_running.store(false);
    }).detach();
}

void app_ui_init() {
    g_ui_thread_id = GetCurrentThreadId();
    g_ui_ready = false;
    stream_buf_init_once();
    /* Load Windows system font (微软雅黑) for CJK text */
    init_system_font();

    /* Load saved config first (sets g_theme, g_lang, g_colors) */
    load_theme_config();
    init_theme_fonts(g_theme); /* Initialize per-theme font pointers */
    anim_init();  /* Initialize animation config for loaded theme */
    sound_init(); /* Initialize sound system */
    g_lang = Lang::EN;  /* Force English as default */

    /* DPI scale info */
    {
        int s = app_get_dpi_scale();
        if (s != 100)
            printf("[UI] DPI %d%%\n", s);
    }
    {
        SDL_Window* win = app_get_window();
        if (win) {
            SDL_GetWindowSize(win, &WIN_W, &WIN_H);
        } else {
            WIN_W = (int)lv_display_get_horizontal_resolution(NULL);
            WIN_H = (int)lv_display_get_vertical_resolution(NULL);
        }
    }
    if (WIN_W < SCALE(WIN_MIN_W)) WIN_W = SCALE(WIN_MIN_W);
    if (WIN_H < SCALE(WIN_MIN_H)) WIN_H = SCALE(WIN_MIN_H);

    /* ── 百分比布局计算 ── */
    TITLE_H     = std::max(WIN_H * TITLE_H_PCT / 100, SCALE(TITLE_H_MIN));
    FOOTER_H    = SCALE(30);
    MODE_BAR_H  = SCALE(36);
    PANEL_TOP   = TITLE_H + SCALE(8);
    PANEL_H     = WIN_H - PANEL_TOP - FOOTER_H - SCALE(16);

    int nav_w   = std::max(WIN_W * NAV_W_PCT / 100, SCALE(NAV_W_MIN));
    int avail_w = WIN_W - nav_w;
    PANEL_GAP   = std::max(WIN_W * SAFE_PAD_PCT / 100, SCALE(GAP_BASE));
    SPLITTER_W  = std::max((int)(WIN_W * SPLITTER_W_PCT / 100), SCALE(SPLITTER_W_MIN));

    LEFT_PANEL_W  = std::max(avail_w * LEFT_PANEL_PCT / 100, SCALE(LEFT_PANEL_MIN_W));
    RIGHT_PANEL_W = avail_w - LEFT_PANEL_W - PANEL_GAP;  /* no splitter — removed v2.2.11 */
    if (RIGHT_PANEL_W < SCALE(RIGHT_PANEL_MIN_W)) {
        RIGHT_PANEL_W = SCALE(RIGHT_PANEL_MIN_W);
        LEFT_PANEL_W = avail_w - RIGHT_PANEL_W - PANEL_GAP;
    }
    PANEL_H = WIN_H - TITLE_H - FOOTER_H - SCALE(24);
    if (PANEL_H < SCALE(300)) PANEL_H = SCALE(300);
    printf("[UI] Layout: window=%dx%d, left=%d, right=%d, panel_h=%d\n",
           WIN_W, WIN_H, LEFT_PANEL_W, RIGHT_PANEL_W, PANEL_H);

    const ThemeColors* c = g_colors;
    lv_obj_t* scr = lv_screen_active();
    lv_obj_set_style_bg_color(scr, c->bg, 0);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);
    /* Enable unified permission dialog as soon as base screen is available. */
    g_ui_ready = true;

    /* ═══ DIVIDER ═══ */
    lv_obj_t* div1 = lv_obj_create(scr);
    lv_obj_set_size(div1, WIN_W, 1);
    lv_obj_set_pos(div1, 0, TITLE_H);
    lv_obj_set_style_bg_color(div1, g_colors->divider, 0);
    lv_obj_set_style_bg_opa(div1, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(div1, 0, 0);
    lv_obj_set_style_pad_all(div1, 0, 0);
    lv_obj_clear_flag(div1, LV_OBJ_FLAG_SCROLLABLE);

    /* ═══ LEFT NAV: icon-only module navigation ═══ */
    int nav_scaled = std::max(WIN_W * NAV_W_PCT / 100, SCALE(NAV_W_MIN));
    int nav_btn_sz = nav_scaled * NAV_ICON_BTN_PCT / 100;
    int nav_gap = nav_scaled * NAV_ICON_GAP_PCT / 100;
    int nav_quick_h = PANEL_H * NAV_QUICK_AREA_H_PCT / 100;

    left_nav = lv_obj_create(scr);
    lv_obj_set_size(left_nav, nav_scaled, PANEL_H);
    lv_obj_set_pos(left_nav, 0, PANEL_TOP);
    lv_obj_set_style_bg_color(left_nav, c->panel, 0);
    lv_obj_set_style_bg_opa(left_nav, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(left_nav, 0, 0);
    lv_obj_set_style_radius(left_nav, 0, 0);
    lv_obj_set_style_pad_all(left_nav, 0, 0);
    lv_obj_clear_flag(left_nav, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_flex_flow(left_nav, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(left_nav, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    /* Top section: module icons */
    lv_obj_t* nav_top = lv_obj_create(left_nav);
    lv_obj_set_width(nav_top, LV_PCT(100));
    lv_obj_set_flex_grow(nav_top, 1);
    lv_obj_set_style_bg_opa(nav_top, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(nav_top, 0, 0);
    lv_obj_set_style_pad_all(nav_top, 0, 0);
    lv_obj_clear_flag(nav_top, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_flex_flow(nav_top, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(nav_top, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_gap(nav_top, nav_gap, 0);

    auto create_nav_btn = [&](const char* icon, const char* tip, bool active) -> lv_obj_t* {
        lv_obj_t* b = lv_button_create(nav_top);
        lv_obj_set_size(b, nav_btn_sz, nav_btn_sz);
        lv_obj_set_style_bg_color(b, active ? c->accent : c->text_hint, 0);
        lv_obj_set_style_bg_opa(b, active ? LV_OPA_20 : LV_OPA_TRANSP, 0);
        lv_obj_set_style_radius(b, NAV_ICON_BTN_RADIUS, 0);
        lv_obj_set_style_border_width(b, 0, 0);
        lv_obj_t* l = lv_label_create(b);
        lv_label_set_text(l, icon);
        lv_obj_set_style_text_font(l, CJK_FONT, 0);
        lv_obj_set_style_text_color(l, active ? c->accent : c->text_dim, 0);
        lv_obj_center(l);
        (void)tip;
        return b;
    };

    nav_btn_bot = create_nav_btn("\xF0\x9F\xA6\x9E", "Bot", true);       /* 🦞 Bot */
    nav_btn_tasks = create_nav_btn("\xE2\x9A\xA1", "Tasks", false);       /* ⚡ Tasks */
    nav_btn_files = create_nav_btn("\xF0\x9F\x93\x81", "Files", false);   /* 📁 Files */

    lv_obj_add_event_cb(nav_btn_bot, [](lv_event_t* e) {
        (void)e;
        if (g_ui_nav_module == UI_NAV_BOT) return;
        g_ui_nav_module = UI_NAV_BOT;
        apply_nav_module_visuals();
    }, LV_EVENT_CLICKED, nullptr);
    lv_obj_add_event_cb(nav_btn_tasks, [](lv_event_t* e) {
        (void)e;
        if (g_ui_nav_module == UI_NAV_TASKS) return;
        g_ui_nav_module = UI_NAV_TASKS;
        apply_nav_module_visuals();
    }, LV_EVENT_CLICKED, nullptr);
    lv_obj_add_event_cb(nav_btn_files, [](lv_event_t* e) {
        (void)e;
        if (g_ui_nav_module == UI_NAV_FILES) return;
        g_ui_nav_module = UI_NAV_FILES;
        apply_nav_module_visuals();
    }, LV_EVENT_CLICKED, nullptr);

    /* Bottom section: quick access */
    lv_obj_t* nav_bot = lv_obj_create(left_nav);
    lv_obj_set_width(nav_bot, LV_PCT(100));
    lv_obj_set_height(nav_bot, nav_quick_h);
    lv_obj_set_style_bg_opa(nav_bot, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(nav_bot, 0, 0);
    lv_obj_set_style_pad_all(nav_bot, 0, 0);
    lv_obj_clear_flag(nav_bot, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_flex_flow(nav_bot, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(nav_bot, LV_FLEX_ALIGN_END, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_gap(nav_bot, nav_gap, 0);

    /* ═══ BOTTOM NAV: Chat/Work toggle (replaces session icon) ═══ */
    nav_session = lv_btn_create(nav_bot);
    lv_obj_set_size(nav_session, nav_btn_sz, nav_btn_sz);
    lv_obj_set_style_bg_opa(nav_session, LV_OPA_TRANSP, 0);
    lv_obj_set_style_radius(nav_session, NAV_ICON_BTN_RADIUS, 0);
    lv_obj_set_style_border_width(nav_session, 0, 0);
    lv_obj_add_event_cb(nav_session, [](lv_event_t* e) {
        (void)e;
        if (g_ui_nav_module != UI_NAV_BOT) return;
        /* Toggle Chat ↔ Work mode */
        if (g_ui_mode == UI_MODE_CHAT) g_ui_mode = UI_MODE_WORK;
        else g_ui_mode = UI_MODE_CHAT;
        apply_mode_switch_visuals();
        relayout_panels();
        ui_log("[CTRL] Nav toggle -> %s", g_ui_mode == UI_MODE_CHAT ? "chat" : "work");
    }, LV_EVENT_CLICKED, nullptr);
    lv_obj_t* sl = lv_label_create(nav_session);
    lv_label_set_text(sl, (g_ui_mode == UI_MODE_CHAT) ? "\xE2\x9E\xA1" : "\xE2\x96\xB6"); /* ⇢ or ◂ */
    lv_obj_set_style_text_color(sl, c->accent, 0);
    lv_obj_center(sl);

    /* Settings */
    lv_obj_t* nav_settings = lv_button_create(nav_bot);
    lv_obj_set_size(nav_settings, nav_btn_sz, nav_btn_sz);
    lv_obj_set_style_bg_opa(nav_settings, LV_OPA_TRANSP, 0);
    lv_obj_set_style_radius(nav_settings, NAV_ICON_BTN_RADIUS, 0);
    lv_obj_set_style_border_width(nav_settings, 0, 0);
    lv_obj_add_event_cb(nav_settings, [](lv_event_t* e) {
        (void)e;
        if (ui_settings_is_open()) return;
        ui_settings_open();
    }, LV_EVENT_CLICKED, nullptr);
    lv_obj_t* sbl = lv_label_create(nav_settings);
    lv_label_set_text(sbl, "\xE2\x9A\x99"); /* ⚙ */
    lv_obj_set_style_text_color(sbl, c->text_dim, 0);
    lv_obj_center(sbl);

    /* ═══ LEFT PANEL: Status ═══ */
    lv_obj_t* pl = lv_obj_create(scr);
    left_panel = pl;  /* Store reference for splitter */
    lv_obj_set_size(pl, LEFT_PANEL_W, PANEL_H);
    lv_obj_set_pos(pl, nav_scaled + SCALE(8), PANEL_TOP);
    lv_obj_set_style_bg_color(pl, c->panel, 0);
    lv_obj_set_style_bg_opa(pl, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(pl, 1, 0);
    lv_obj_set_style_border_color(pl, c->border, 0);
    lv_obj_set_style_radius(pl, SCALE(g_colors->radius_lg), 0);
    lv_obj_set_style_pad_all(pl, 0, 0);
    lv_obj_clear_flag(pl, LV_OBJ_FLAG_SCROLLABLE);

    /* Status row: [Gateway Status] [● LED] [Running] */
    static const I18n S_GW_STATUS = {"Gateway Status", "Gateway 状态"};
    lp_status_row = lv_obj_create(pl);
    lv_obj_set_size(lp_status_row, LEFT_PANEL_W - GAP, 30);
    lv_obj_set_pos(lp_status_row, GAP, GAP);
    lv_obj_set_style_bg_opa(lp_status_row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(lp_status_row, 0, 0);
    lv_obj_set_style_pad_all(lp_status_row, 0, 0);
    lv_obj_clear_flag(lp_status_row, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_flex_flow(lp_status_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(lp_status_row, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_gap(lp_status_row, 6, 0);

    /* Gateway Status — left */
    lp_panel_title = lv_label_create(lp_status_row);
    lv_label_set_text(lp_panel_title, tr(S_GW_STATUS));
    lv_obj_set_style_text_color(lp_panel_title, c->accent_secondary, 0);
    lv_obj_set_style_text_font(lp_panel_title, CJK_FONT, 0);

    /* LED indicator — middle */
    led_ok = lv_led_create(lp_status_row);
    lv_obj_set_size(led_ok, 14, 14);
    lv_led_set_color(led_ok, g_colors->warning);
    lv_led_off(led_ok);

    /* Status text — right */
    status_label = lv_label_create(lp_status_row);
    lv_label_set_text(status_label, tr(STR_CHECKING));
    lv_obj_set_style_text_color(status_label, c->text_dim, 0);
    lv_obj_set_style_text_font(status_label, FONT(10), 0);

    /* Model name display — below status row */
    char gw_model_buf[256] = {0};
    app_get_current_model(gw_model_buf, sizeof(gw_model_buf));
    if (gw_model_buf[0]) {
        lv_obj_t* model_label = lv_label_create(pl);
        static char model_display[300];
        snprintf(model_display, sizeof(model_display), "Model: %s", gw_model_buf);
        lv_label_set_text(model_label, model_display);
        lv_obj_set_style_text_color(model_label, c->text_dim, 0);
        lv_obj_set_style_text_font(model_label, FONT(10), 0);
        lv_obj_set_pos(model_label, GAP + 8, GAP + 30);
    }

    /* Separator — directly below status row */
    int rg = 12; /* uniform row gap */
    int sep_y = GAP + 30 + 16 + rg; /* status row (30px) + model label (16px) + gap */
    lp_separator = lv_obj_create(pl);
    lv_obj_set_size(lp_separator, LEFT_PANEL_W - GAP * 2, 1);
    lv_obj_set_pos(lp_separator, GAP, sep_y);
    lv_obj_set_style_bg_color(lp_separator, c->border, 0);
    lv_obj_set_style_bg_opa(lp_separator, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(lp_separator, 0, 0);
    lv_obj_set_style_pad_all(lp_separator, 0, 0);
    lv_obj_clear_flag(lp_separator, LV_OBJ_FLAG_SCROLLABLE);

    /* Task list area title */
    int task_title_y = sep_y + 1 + rg; /* 149 */
    lp_task_title = lv_label_create(pl);
    static const I18n S_TASK_LIST = {"Task List", "任务列表"};
    lv_label_set_text(lp_task_title, tr(S_TASK_LIST));
    lv_obj_set_style_text_color(lp_task_title, c->accent, 0);
    lv_obj_set_style_text_font(lp_task_title, CJK_FONT, 0);
    lv_obj_set_width(lp_task_title, LEFT_PANEL_W - GAP * 5);
    lv_label_set_long_mode(lp_task_title, LV_LABEL_LONG_MODE_DOTS);
    lv_obj_set_pos(lp_task_title, GAP, task_title_y);

    /* Task count badge "(N)" */
    lp_task_count = lv_label_create(pl);
    lv_label_set_text(lp_task_count, "(0)");
    lv_obj_set_style_text_color(lp_task_count, c->text_dim, 0);
    lv_obj_set_style_text_font(lp_task_count, CJK_FONT_SMALL, 0);
    lv_obj_set_pos(lp_task_count, GAP + lv_obj_get_width(lp_task_title) + 4, task_title_y + 2);

    /* Task list placeholder */
    int empty_y = task_title_y + 20 + rg; /* 177 */
    g_task_next_y = empty_y; /* First task starts here */
    static const I18n S_NO_TASKS_YET = {"No tasks yet", "暂无任务"};
    task_empty_label = create_styled_label(pl, tr(S_NO_TASKS_YET), c->text_dim, GAP * 2, empty_y, LEFT_PANEL_W - GAP * 4);
    lv_obj_set_style_text_font(task_empty_label, CJK_FONT_SMALL, 0);

    /* Long task progress strip (download/install/other time-consuming operations) */
    lp_progress_panel = lv_obj_create(pl);
    lv_obj_set_size(lp_progress_panel, LEFT_PANEL_W - GAP * 2, SCALE(84));
    lv_obj_align(lp_progress_panel, LV_ALIGN_BOTTOM_MID, 0, -SCALE(30));
    lv_obj_set_style_bg_color(lp_progress_panel, c->surface, 0);
    lv_obj_set_style_border_color(lp_progress_panel, c->border, 0);
    lv_obj_set_style_border_width(lp_progress_panel, 1, 0);
    lv_obj_set_style_radius(lp_progress_panel, SCALE(g_colors->radius_md), 0);
    lv_obj_set_style_pad_all(lp_progress_panel, 6, 0);
    lv_obj_set_style_pad_gap(lp_progress_panel, 4, 0);
    lv_obj_set_flex_flow(lp_progress_panel, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(lp_progress_panel, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_clear_flag(lp_progress_panel, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(lp_progress_panel, LV_OBJ_FLAG_HIDDEN);

    lp_progress_title = lv_label_create(lp_progress_panel);
    lv_label_set_text(lp_progress_title, "Task: -");
    lv_obj_set_style_text_color(lp_progress_title, c->text, 0);
    lv_obj_set_style_text_font(lp_progress_title, CJK_FONT_SMALL, 0);

    lp_progress_step = lv_label_create(lp_progress_panel);
    lv_label_set_text(lp_progress_step, "Step: -");
    lv_obj_set_style_text_color(lp_progress_step, c->text_dim, 0);
    lv_obj_set_style_text_font(lp_progress_step, CJK_FONT_SMALL, 0);

    lp_progress_bar = lv_bar_create(lp_progress_panel);
    lv_obj_set_width(lp_progress_bar, LV_PCT(100));
    lv_obj_set_height(lp_progress_bar, SCALE(10));
    lv_bar_set_range(lp_progress_bar, 0, 100);
    lv_bar_set_value(lp_progress_bar, 0, LV_ANIM_OFF);
    lv_obj_set_style_bg_color(lp_progress_bar, c->panel, LV_PART_MAIN);
    lv_obj_set_style_bg_color(lp_progress_bar, c->btn_action, LV_PART_INDICATOR);
    lv_obj_set_style_radius(lp_progress_bar, 999, 0);

    lp_progress_result = lv_label_create(lp_progress_panel);
    lv_label_set_text(lp_progress_result, "");
    lv_obj_set_style_text_color(lp_progress_result, c->text_dim, 0);
    lv_obj_set_style_text_font(lp_progress_result, CJK_FONT_SMALL, 0);

    /* Hint at bottom - centered, small font */
    lp_hint = lv_label_create(pl);
    lv_label_set_text(lp_hint, tr(STR_AUTOREFRESH));
    lv_obj_set_style_text_color(lp_hint, c->text_dim, 0);
    lv_obj_set_style_text_font(lp_hint, FONT(12), 0);
    lv_obj_align(lp_hint, LV_ALIGN_BOTTOM_MID, 0, -8);

    /* Splitter removed in v2.2.11 — hidden, no drag */
    splitter = lv_obj_create(scr);
    lv_obj_set_size(splitter, 0, 0);  /* zero size — invisible */
    lv_obj_set_pos(splitter, 0, 0);
    lv_obj_add_flag(splitter, LV_OBJ_FLAG_HIDDEN);

    /* ═══ RIGHT PANEL: Controls ═══ */
    int right_x = nav_scaled + SCALE(8) + LEFT_PANEL_W + PANEL_GAP;
    lv_obj_t* pr = lv_obj_create(scr);
    right_panel = pr;
    lv_obj_set_size(pr, RIGHT_PANEL_W, PANEL_H);
    lv_obj_set_pos(pr, right_x, PANEL_TOP);
    lv_obj_set_style_bg_color(pr, c->panel, 0);
    lv_obj_set_style_bg_opa(pr, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(pr, 1, 0);
    lv_obj_set_style_border_color(pr, c->border, 0);
    lv_obj_set_style_radius(pr, SCALE(g_colors->radius_lg), 0);
    lv_obj_set_style_pad_all(pr, 0, 0);
    lv_obj_clear_flag(pr, LV_OBJ_FLAG_SCROLLABLE);

    /* ═══ CONTROL BAR: Chat mode toolbar (Agent / Report / AI行为) ═══ */
    ctrl_bar = lv_obj_create(pr);
    lv_obj_set_size(ctrl_bar, RIGHT_PANEL_W - CHAT_GAP * 2, SCALE(CTRL_BAR_H));
    lv_obj_set_pos(ctrl_bar, CHAT_GAP, 0);
    lv_obj_set_style_bg_opa(ctrl_bar, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(ctrl_bar, 0, 0);
    lv_obj_set_style_border_color(ctrl_bar, c->border, 0);
    lv_obj_set_style_border_width(ctrl_bar, 1, 0);        /* bottom divider */
    lv_obj_set_style_pad_bottom(ctrl_bar, 1, 0);
    lv_obj_clear_flag(ctrl_bar, LV_OBJ_FLAG_SCROLLABLE);

    /* Agent dropdown: OpenClaw / Hermes / Claude */
    const int DD_W = SCALE(90);
    const int DD_H = SCALE(24);
    const int DD_GAP = SCALE(8);
    const int BTN_W = SCALE(70);
    int dd_x = CHAT_GAP;

    ctrl_dd_agent = lv_dropdown_create(ctrl_bar);
    lv_dropdown_set_options(ctrl_dd_agent, "OpenClaw\nHermes\nClaude");
    lv_dropdown_set_selected(ctrl_dd_agent, 0);
    lv_obj_set_size(ctrl_dd_agent, DD_W, DD_H);
    lv_obj_set_pos(ctrl_dd_agent, dd_x, (SCALE(CTRL_BAR_H) - DD_H) / 2);
    lv_obj_set_style_bg_color(ctrl_dd_agent, c->surface, 0);
    lv_obj_set_style_text_color(ctrl_dd_agent, c->text_dim, 0);
    lv_obj_set_style_border_color(ctrl_dd_agent, c->border, 0);
    lv_obj_set_style_text_font(ctrl_dd_agent, CJK_FONT_SMALL, 0);
    lv_obj_set_style_text_font(ctrl_dd_agent, FONT(12), LV_PART_INDICATOR);
    lv_obj_add_event_cb(ctrl_dd_agent, [](lv_event_t* e) {
        /* TODO: switch agent runtime and联动 model */
    }, LV_EVENT_VALUE_CHANGED, nullptr);

    /* Report button: opens last BootCheck report */
    dd_x += DD_W + DD_GAP;
    lv_obj_t* ctrl_btn_report = lv_btn_create(ctrl_bar);
    lv_obj_set_size(ctrl_btn_report, BTN_W, DD_H);
    lv_obj_set_pos(ctrl_btn_report, dd_x, (SCALE(CTRL_BAR_H) - DD_H) / 2);
    lv_obj_set_style_bg_color(ctrl_btn_report, c->btn_secondary, 0);
    lv_obj_set_style_bg_opa(ctrl_btn_report, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(ctrl_btn_report, SCALE(g_colors->radius_sm), 0);
    lv_obj_set_style_border_width(ctrl_btn_report, 0, 0);
    lv_obj_clear_flag(ctrl_btn_report, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_t* lbl_report = lv_label_create(ctrl_btn_report);
    lv_label_set_text(lbl_report, "Report");
    lv_obj_set_style_text_color(lbl_report, c->text, 0);
    lv_obj_center(lbl_report);
    lv_obj_add_event_cb(ctrl_btn_report, work_boot_open_report_cb, LV_EVENT_CLICKED, nullptr);

    /* AI behavior dropdown (Autonomous/Ask/Plan) */
    dd_x += BTN_W + DD_GAP;
    mode_dd_chat_ai_mode = lv_dropdown_create(ctrl_bar);
    lv_dropdown_set_options(mode_dd_chat_ai_mode, "Auto\nAsk\nPlan");
    lv_dropdown_set_selected(mode_dd_chat_ai_mode, (uint16_t)g_ai_interaction_mode);
    lv_obj_set_size(mode_dd_chat_ai_mode, DD_W, DD_H);
    lv_obj_set_pos(mode_dd_chat_ai_mode, dd_x, (SCALE(CTRL_BAR_H) - DD_H) / 2);
    lv_obj_set_style_bg_color(mode_dd_chat_ai_mode, c->surface, 0);
    lv_obj_set_style_text_color(mode_dd_chat_ai_mode, c->text_dim, 0);
    lv_obj_set_style_border_color(mode_dd_chat_ai_mode, c->border, 0);
    lv_obj_set_style_text_font(mode_dd_chat_ai_mode, CJK_FONT_SMALL, 0);
    lv_obj_set_style_text_font(mode_dd_chat_ai_mode, FONT(12), LV_PART_INDICATOR);
    lv_obj_add_event_cb(mode_dd_chat_ai_mode, ai_mode_dropdown_cb, LV_EVENT_VALUE_CHANGED, nullptr);

    dd_x += DD_W + DD_GAP;
    ctrl_btn_dual_view = lv_btn_create(ctrl_bar);
    lv_obj_set_size(ctrl_btn_dual_view, SCALE(110), DD_H);
    lv_obj_set_pos(ctrl_btn_dual_view, dd_x, (SCALE(CTRL_BAR_H) - DD_H) / 2);
    lv_obj_set_style_bg_color(ctrl_btn_dual_view, c->btn_secondary, 0);
    lv_obj_set_style_bg_opa(ctrl_btn_dual_view, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(ctrl_btn_dual_view, SCALE(g_colors->radius_sm), 0);
    lv_obj_set_style_border_width(ctrl_btn_dual_view, 0, 0);
    lv_obj_clear_flag(ctrl_btn_dual_view, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_event_cb(ctrl_btn_dual_view, c2_dual_view_toggle_cb, LV_EVENT_CLICKED, nullptr);
    lv_obj_t* lbl_dual = lv_label_create(ctrl_btn_dual_view);
    lv_obj_set_style_text_color(lbl_dual, c->text, 0);
    lv_obj_set_style_text_font(lbl_dual, CJK_FONT_SMALL, 0);
    lv_obj_center(lbl_dual);
    c2_refresh_dual_view_button();

    int content_w = mode_content_w();
    int content_h = mode_content_h();

    /* Mode switch moved to title-bar right side. Keep pointers for visual update logic. */
    mode_btn_chat = top_mode_chat;
    mode_btn_voice = nullptr;
    mode_btn_work = top_mode_work;

    mode_panel_chat = lv_obj_create(pr);
    lv_obj_set_size(mode_panel_chat, content_w, content_h);
    lv_obj_set_pos(mode_panel_chat, CHAT_GAP, CHAT_GAP + SCALE(CTRL_BAR_H));
    lv_obj_set_style_bg_opa(mode_panel_chat, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(mode_panel_chat, 0, 0);
    lv_obj_set_style_pad_all(mode_panel_chat, 0, 0);
    lv_obj_clear_flag(mode_panel_chat, LV_OBJ_FLAG_SCROLLABLE);

    mode_panel_work = lv_obj_create(pr);
    lv_obj_set_size(mode_panel_work, content_w, content_h);
    lv_obj_set_pos(mode_panel_work, CHAT_GAP, CHAT_GAP + SCALE(CTRL_BAR_H));
    lv_obj_set_style_bg_opa(mode_panel_work, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(mode_panel_work, 0, 0);
    lv_obj_set_style_pad_all(mode_panel_work, 8, 0);
    lv_obj_set_flex_flow(mode_panel_work, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(mode_panel_work, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_set_style_pad_gap(mode_panel_work, 8, 0);
    lv_obj_set_scroll_dir(mode_panel_work, LV_DIR_VER);

    module_placeholder = lv_obj_create(pr);
    lv_obj_set_size(module_placeholder, content_w, content_h);
    lv_obj_set_pos(module_placeholder, CHAT_GAP, CHAT_GAP + SCALE(CTRL_BAR_H));
    lv_obj_set_style_bg_opa(module_placeholder, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(module_placeholder, 0, 0);
    lv_obj_set_style_pad_all(module_placeholder, SCALE(24), 0);
    lv_obj_clear_flag(module_placeholder, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_flex_flow(module_placeholder, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(module_placeholder, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_gap(module_placeholder, SCALE(10), 0);
    module_placeholder_title = lv_label_create(module_placeholder);
    lv_obj_set_style_text_font(module_placeholder_title, CJK_FONT, 0);
    lv_obj_set_style_text_color(module_placeholder_title, c->text, 0);
    lv_label_set_text(module_placeholder_title, tr(I18n{"Tasks Center", "任务中心"}));
    module_placeholder_desc = lv_label_create(module_placeholder);
    lv_label_set_long_mode(module_placeholder_desc, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(module_placeholder_desc, SCALE(420));
    lv_obj_set_style_text_align(module_placeholder_desc, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_font(module_placeholder_desc, CJK_FONT_SMALL, 0);
    lv_obj_set_style_text_color(module_placeholder_desc, c->text_dim, 0);
    lv_label_set_text(module_placeholder_desc, tr(I18n{"Task scheduling, queue and execution shortcuts.", "任务调度、队列与执行快捷入口。"}));

    module_tasks_panel = lv_obj_create(module_placeholder);
    lv_obj_set_width(module_tasks_panel, std::min(content_w - SCALE(40), SCALE(760)));
    lv_obj_set_height(module_tasks_panel, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_color(module_tasks_panel, c->surface, 0);
    lv_obj_set_style_bg_opa(module_tasks_panel, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(module_tasks_panel, 1, 0);
    lv_obj_set_style_border_color(module_tasks_panel, c->border, 0);
    lv_obj_set_style_radius(module_tasks_panel, SCALE(g_colors->radius_lg), 0);
    lv_obj_set_style_pad_all(module_tasks_panel, SCALE(14), 0);
    lv_obj_set_style_pad_gap(module_tasks_panel, SCALE(8), 0);
    lv_obj_set_flex_flow(module_tasks_panel, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(module_tasks_panel, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_clear_flag(module_tasks_panel, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_t* t_title = lv_label_create(module_tasks_panel);
    lv_label_set_text(t_title, tr(I18n{"Task Queue", "任务队列"}));
    lv_obj_set_style_text_color(t_title, c->text, 0);
    lv_obj_set_style_text_font(t_title, CJK_FONT, 0);
    module_tasks_state = lv_label_create(module_tasks_panel);
    lv_label_set_text(module_tasks_state, tr(I18n{"State: ready", "状态：就绪"}));
    lv_obj_set_style_text_color(module_tasks_state, c->text_dim, 0);
    lv_obj_set_style_text_font(module_tasks_state, CJK_FONT_SMALL, 0);
    lv_obj_t* row_tasks_btn = lv_obj_create(module_tasks_panel);
    lv_obj_set_width(row_tasks_btn, LV_PCT(100));
    lv_obj_set_height(row_tasks_btn, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(row_tasks_btn, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(row_tasks_btn, 0, 0);
    lv_obj_set_style_pad_all(row_tasks_btn, 0, 0);
    lv_obj_set_style_pad_gap(row_tasks_btn, SCALE(8), 0);
    lv_obj_set_flex_flow(row_tasks_btn, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(row_tasks_btn, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag(row_tasks_btn, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_t* btn_task_refresh = aw_btn_create(row_tasks_btn, "Refresh Queue", BTN_SECONDARY, SCALE(150), SCALE(34));
    lv_obj_add_event_cb(btn_task_refresh, [](lv_event_t* e) {
        (void)e;
        refresh_tasks_module_data(false);
    }, LV_EVENT_CLICKED, nullptr);
    lv_obj_t* btn_task_run = aw_btn_create(row_tasks_btn, "Sync Sessions", BTN_PRIMARY, SCALE(140), SCALE(34));
    lv_obj_add_event_cb(btn_task_run, [](lv_event_t* e) {
        (void)e;
        refresh_tasks_module_data(true);
    }, LV_EVENT_CLICKED, nullptr);
    lv_obj_t* btn_task_abort_all = aw_btn_create(row_tasks_btn, "Abort All", BTN_SECONDARY, SCALE(120), SCALE(34));
    lv_obj_add_event_cb(btn_task_abort_all, [](lv_event_t* e) {
        (void)e;
        bool ok = session_mgr().abort_all();
        if (ok) {
            if (module_tasks_state) lv_label_set_text(module_tasks_state, tr(I18n{"State: all sessions aborted", "状态：所有会话已终止"}));
            ui_toast_warn(g_lang == Lang::CN ? "已终止全部会话" : "All sessions aborted");
        } else {
            if (module_tasks_state) lv_label_set_text(module_tasks_state, tr(I18n{"State: abort all failed", "状态：全部终止失败"}));
            ui_toast_error(g_lang == Lang::CN ? "终止会话失败" : "Abort sessions failed");
        }
        app_refresh_status();
        refresh_tasks_module_data(false);
    }, LV_EVENT_CLICKED, nullptr);

    lv_obj_t* row_task_tpl = lv_obj_create(module_tasks_panel);
    lv_obj_set_width(row_task_tpl, LV_PCT(100));
    lv_obj_set_height(row_task_tpl, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(row_task_tpl, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(row_task_tpl, 0, 0);
    lv_obj_set_style_pad_all(row_task_tpl, 0, 0);
    lv_obj_set_style_pad_gap(row_task_tpl, SCALE(8), 0);
    lv_obj_set_flex_flow(row_task_tpl, LV_FLEX_FLOW_ROW_WRAP);
    lv_obj_set_flex_align(row_task_tpl, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag(row_task_tpl, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* btn_tpl_health = aw_btn_create(row_task_tpl, "Template: Health Check", BTN_PRIMARY, SCALE(210), SCALE(34));
    lv_obj_add_event_cb(btn_tpl_health, [](lv_event_t* e) {
        (void)e;
        run_tasks_template("Health Check",
            "执行系统健康检查并输出摘要：网关状态、活动会话、运行时状态、最近错误日志。最后给出可执行修复建议。");
    }, LV_EVENT_CLICKED, nullptr);

    lv_obj_t* btn_tpl_workspace = aw_btn_create(row_task_tpl, "Template: Workspace Sync", BTN_SECONDARY, SCALE(230), SCALE(34));
    lv_obj_add_event_cb(btn_tpl_workspace, [](lv_event_t* e) {
        (void)e;
        run_tasks_template("Workspace Sync",
            "检查并同步工作区托管文件（AGENTS/TOOLS/MEMORY），列出变更项并给出下一步建议。不要修改工作区外文件。");
    }, LV_EVENT_CLICKED, nullptr);

    lv_obj_t* btn_tpl_resource = aw_btn_create(row_task_tpl, "Template: Resource Audit", BTN_SECONDARY, SCALE(220), SCALE(34));
    lv_obj_add_event_cb(btn_tpl_resource, [](lv_event_t* e) {
        (void)e;
        run_tasks_template("Resource Audit",
            "扫描 assets 下字体/图标/声音资源，输出缺失项、重复项和命名建议，最终给出修复优先级。不要直接删除文件。");
    }, LV_EVENT_CLICKED, nullptr);

    module_tasks_view = aw_textarea_create(module_tasks_panel, "Task queue snapshot...", false, std::min(content_w - SCALE(80), SCALE(700)), SCALE(180));
    lv_textarea_set_text_selection(module_tasks_view, true);
    refresh_tasks_module_data(false);

    module_files_panel = lv_obj_create(module_placeholder);
    lv_obj_set_width(module_files_panel, std::min(content_w - SCALE(40), SCALE(760)));
    lv_obj_set_height(module_files_panel, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_color(module_files_panel, c->surface, 0);
    lv_obj_set_style_bg_opa(module_files_panel, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(module_files_panel, 1, 0);
    lv_obj_set_style_border_color(module_files_panel, c->border, 0);
    lv_obj_set_style_radius(module_files_panel, SCALE(g_colors->radius_lg), 0);
    lv_obj_set_style_pad_all(module_files_panel, SCALE(14), 0);
    lv_obj_set_style_pad_gap(module_files_panel, SCALE(8), 0);
    lv_obj_set_flex_flow(module_files_panel, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(module_files_panel, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_clear_flag(module_files_panel, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_t* f_title = lv_label_create(module_files_panel);
    lv_label_set_text(f_title, tr(I18n{"Resource Hub", "资源中心"}));
    lv_obj_set_style_text_color(f_title, c->text, 0);
    lv_obj_set_style_text_font(f_title, CJK_FONT, 0);
    module_files_state = lv_label_create(module_files_panel);
    lv_label_set_text(module_files_state, tr(I18n{"State: ready", "状态：就绪"}));
    lv_obj_set_style_text_color(module_files_state, c->text_dim, 0);
    lv_obj_set_style_text_font(module_files_state, CJK_FONT_SMALL, 0);
    lv_obj_t* row_files_btn = lv_obj_create(module_files_panel);
    lv_obj_set_width(row_files_btn, LV_PCT(100));
    lv_obj_set_height(row_files_btn, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(row_files_btn, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(row_files_btn, 0, 0);
    lv_obj_set_style_pad_all(row_files_btn, 0, 0);
    lv_obj_set_style_pad_gap(row_files_btn, SCALE(8), 0);
    lv_obj_set_flex_flow(row_files_btn, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(row_files_btn, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag(row_files_btn, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_t* btn_files_scan = aw_btn_create(row_files_btn, "Scan Workspace", BTN_SECONDARY, SCALE(160), SCALE(34));
    lv_obj_add_event_cb(btn_files_scan, [](lv_event_t* e) {
        (void)e;
        refresh_files_module_data(true);
    }, LV_EVENT_CLICKED, nullptr);
    lv_obj_t* btn_files_open = aw_btn_create(row_files_btn, "Open Asset Dir", BTN_PRIMARY, SCALE(150), SCALE(34));
    lv_obj_add_event_cb(btn_files_open, [](lv_event_t* e) {
        (void)e;
        const char* asset_dir = "assets";
        ShellExecuteA(nullptr, "open", asset_dir, nullptr, nullptr, SW_SHOWNORMAL);
        refresh_files_module_data(false);
        if (module_files_state) lv_label_set_text(module_files_state, tr(I18n{"State: asset folder opened", "状态：资源目录已打开"}));
        ui_log("[Resources] Open asset dir: %s", asset_dir);
    }, LV_EVENT_CLICKED, nullptr);
    lv_obj_t* btn_files_sync = aw_btn_create(row_files_btn, "Sync Managed", BTN_SECONDARY, SCALE(140), SCALE(34));
    lv_obj_add_event_cb(btn_files_sync, [](lv_event_t* e) {
        (void)e;
        bool ok = workspace_sync_managed_sections();
        if (module_files_state) {
            lv_label_set_text(module_files_state,
                ok ? tr(I18n{"State: managed sections synced", "状态：托管段落已同步"})
                   : tr(I18n{"State: managed sync failed", "状态：托管段落同步失败"}));
        }
        if (!ok) ui_toast_error(g_lang == Lang::CN ? "工作区同步失败" : "Workspace sync failed");
        refresh_files_module_data(false);
    }, LV_EVENT_CLICKED, nullptr);
    lv_obj_t* btn_files_open_ws = aw_btn_create(row_files_btn, "Open Workspace", BTN_SECONDARY, SCALE(160), SCALE(34));
    lv_obj_add_event_cb(btn_files_open_ws, [](lv_event_t* e) {
        (void)e;
        std::string ws = workspace_get_root();
        if (!ws.empty()) {
            ShellExecuteA(nullptr, "open", ws.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
            if (module_files_state) lv_label_set_text(module_files_state, tr(I18n{"State: workspace opened", "状态：工作区已打开"}));
            ui_log("[Resources] Open workspace: %s", ws.c_str());
        }
        refresh_files_module_data(false);
    }, LV_EVENT_CLICKED, nullptr);

    lv_obj_t* row_files_filter = lv_obj_create(module_files_panel);
    lv_obj_set_width(row_files_filter, LV_PCT(100));
    lv_obj_set_height(row_files_filter, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(row_files_filter, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(row_files_filter, 0, 0);
    lv_obj_set_style_pad_all(row_files_filter, 0, 0);
    lv_obj_set_style_pad_gap(row_files_filter, SCALE(8), 0);
    lv_obj_set_flex_flow(row_files_filter, LV_FLEX_FLOW_ROW_WRAP);
    lv_obj_set_flex_align(row_files_filter, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag(row_files_filter, LV_OBJ_FLAG_SCROLLABLE);

    module_files_filter_all = aw_btn_create(row_files_filter, "All", BTN_SECONDARY, SCALE(72), SCALE(32));
    lv_obj_add_event_cb(module_files_filter_all, [](lv_event_t* e) {
        (void)e;
        g_module_file_filter = MODULE_FILE_ALL;
        refresh_files_module_data(false);
    }, LV_EVENT_CLICKED, nullptr);
    module_files_filter_fonts = aw_btn_create(row_files_filter, "Fonts", BTN_SECONDARY, SCALE(86), SCALE(32));
    lv_obj_add_event_cb(module_files_filter_fonts, [](lv_event_t* e) {
        (void)e;
        g_module_file_filter = MODULE_FILE_FONTS;
        refresh_files_module_data(false);
    }, LV_EVENT_CLICKED, nullptr);
    module_files_filter_icons = aw_btn_create(row_files_filter, "Icons", BTN_SECONDARY, SCALE(86), SCALE(32));
    lv_obj_add_event_cb(module_files_filter_icons, [](lv_event_t* e) {
        (void)e;
        g_module_file_filter = MODULE_FILE_ICONS;
        refresh_files_module_data(false);
    }, LV_EVENT_CLICKED, nullptr);
    module_files_filter_sounds = aw_btn_create(row_files_filter, "Sounds", BTN_SECONDARY, SCALE(96), SCALE(32));
    lv_obj_add_event_cb(module_files_filter_sounds, [](lv_event_t* e) {
        (void)e;
        g_module_file_filter = MODULE_FILE_SOUNDS;
        refresh_files_module_data(false);
    }, LV_EVENT_CLICKED, nullptr);

    lv_obj_t* row_files_query = lv_obj_create(module_files_panel);
    lv_obj_set_width(row_files_query, LV_PCT(100));
    lv_obj_set_height(row_files_query, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(row_files_query, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(row_files_query, 0, 0);
    lv_obj_set_style_pad_all(row_files_query, 0, 0);
    lv_obj_set_style_pad_gap(row_files_query, SCALE(8), 0);
    lv_obj_set_flex_flow(row_files_query, LV_FLEX_FLOW_ROW_WRAP);
    lv_obj_set_flex_align(row_files_query, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag(row_files_query, LV_OBJ_FLAG_SCROLLABLE);

    module_files_search_input = aw_textarea_create(row_files_query, "Search path keyword...", false, SCALE(250), SCALE(34));
    lv_textarea_set_one_line(module_files_search_input, true);
    lv_obj_add_event_cb(module_files_search_input, [](lv_event_t* e) {
        (void)e;
        const char* text = lv_textarea_get_text(module_files_search_input);
        snprintf(g_module_files_search, sizeof(g_module_files_search), "%s", text ? text : "");
        refresh_files_module_data(false);
    }, LV_EVENT_VALUE_CHANGED, nullptr);

    module_files_ext_dd = lv_dropdown_create(row_files_query);
    lv_dropdown_set_options(module_files_ext_dd, "Ext: Any\n.png\n.svg\n.ttf/.otf\n.wav/.mp3/.ogg\n.json\n.md");
    lv_dropdown_set_selected(module_files_ext_dd, (uint16_t)g_module_file_ext_filter);
    lv_obj_set_size(module_files_ext_dd, SCALE(170), SCALE(34));
    lv_obj_add_event_cb(module_files_ext_dd, [](lv_event_t* e) {
        (void)e;
        g_module_file_ext_filter = (int)lv_dropdown_get_selected(module_files_ext_dd);
        refresh_files_module_data(false);
    }, LV_EVENT_VALUE_CHANGED, nullptr);

    lv_obj_t* btn_files_clear_filter = aw_btn_create(row_files_query, "Clear Filters", BTN_SECONDARY, SCALE(130), SCALE(34));
    lv_obj_add_event_cb(btn_files_clear_filter, [](lv_event_t* e) {
        (void)e;
        g_module_file_filter = MODULE_FILE_ALL;
        g_module_file_ext_filter = 0;
        g_module_files_search[0] = '\0';
        if (module_files_search_input) lv_textarea_set_text(module_files_search_input, "");
        if (module_files_ext_dd) lv_dropdown_set_selected(module_files_ext_dd, 0);
        refresh_files_module_data(false);
    }, LV_EVENT_CLICKED, nullptr);

    module_files_view = aw_textarea_create(module_files_panel, "Resource snapshot...", false, std::min(content_w - SCALE(80), SCALE(700)), SCALE(180));
    lv_textarea_set_text_selection(module_files_view, true);
    refresh_files_module_data(false);

    lv_obj_add_flag(module_files_panel, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(module_placeholder, LV_OBJ_FLAG_HIDDEN);

    {
        int card_w = std::min(content_w - 16, SCALE(560));
        lv_obj_t* sec_runtime = aw_form_section_create(mode_panel_work, "Runtime Mode", card_w);
        aw_form_field_dropdown(sec_runtime, "Control mode",
                               "User controls AnyClaw\nAI controls AnyClaw",
                               (uint16_t)g_control_mode, &mode_dd_control, card_w - 24);
        lv_obj_add_event_cb(mode_dd_control, work_control_mode_cb, LV_EVENT_VALUE_CHANGED, nullptr);
        aw_form_field_dropdown(sec_runtime, "LLM access",
                               "Gateway mode (OpenClaw)\nDirect API mode (paused)",
                               0, &mode_dd_llm, card_w - 24);
        lv_obj_add_event_cb(mode_dd_llm, work_llm_mode_cb, LV_EVENT_VALUE_CHANGED, nullptr);
        aw_form_field_dropdown(sec_runtime, "AI interaction mode",
                               "Autonomous\nAsk\nPlan",
                               (uint16_t)g_ai_interaction_mode, &mode_dd_ai_mode, card_w - 24);
        lv_obj_add_event_cb(mode_dd_ai_mode, ai_mode_dropdown_cb, LV_EVENT_VALUE_CHANGED, nullptr);

        lv_obj_t* row_adv = lv_obj_create(sec_runtime);
        lv_obj_set_width(row_adv, card_w - 24);
        lv_obj_set_height(row_adv, LV_SIZE_CONTENT);
        lv_obj_set_style_bg_opa(row_adv, LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_width(row_adv, 0, 0);
        lv_obj_set_style_pad_all(row_adv, 0, 0);
        lv_obj_set_style_pad_gap(row_adv, 8, 0);
        lv_obj_set_flex_flow(row_adv, LV_FLEX_FLOW_ROW);
        lv_obj_set_flex_align(row_adv, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        lv_obj_clear_flag(row_adv, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_t* lbl_adv = lv_label_create(row_adv);
        lv_label_set_text(lbl_adv, "Show advanced modules (LAN/FTP/KB)");
        lv_obj_set_style_text_color(lbl_adv, c->text, 0);
        lv_obj_set_style_text_font(lbl_adv, CJK_FONT_SMALL, 0);
        mode_sw_work_advanced = lv_switch_create(row_adv);
        if (g_work_show_advanced) lv_obj_add_state(mode_sw_work_advanced, LV_STATE_CHECKED);
        lv_obj_set_style_bg_color(mode_sw_work_advanced, c->btn_secondary, 0);
        lv_obj_set_style_bg_color(mode_sw_work_advanced, c->btn_action, LV_PART_INDICATOR | LV_STATE_CHECKED);
        lv_obj_add_event_cb(mode_sw_work_advanced, work_advanced_sw_cb, LV_EVENT_VALUE_CHANGED, nullptr);

        mode_lbl_work_hint = aw_label_wrap_create(sec_runtime, "", LABEL_HINT, 100);
        lv_obj_set_style_text_color(mode_lbl_work_hint, c->text_dim, 0);
        update_work_mode_hint();
        lv_obj_t* btn_boot_check = aw_btn_create(sec_runtime, "Run Health Check + Auto Repair", BTN_SECONDARY, SCALE(260), SCALE(34));
        lv_obj_add_event_cb(btn_boot_check, work_boot_check_cb, LV_EVENT_CLICKED, nullptr);
        lv_obj_t* btn_open_boot_report = aw_btn_create(sec_runtime, "Last Report", BTN_SECONDARY, SCALE(220), SCALE(34));
        lv_obj_add_event_cb(btn_open_boot_report, work_boot_open_report_cb, LV_EVENT_CLICKED, nullptr);
        mode_lbl_boot_report_hint = aw_label_wrap_create(sec_runtime, "Report: (not generated yet)", LABEL_HINT, 100);
        lv_obj_set_style_text_color(mode_lbl_boot_report_hint, c->text_dim, 0);
        mode_boot_progress_panel = lv_obj_create(sec_runtime);
        lv_obj_set_width(mode_boot_progress_panel, card_w - 24);
        lv_obj_set_height(mode_boot_progress_panel, LV_SIZE_CONTENT);
        lv_obj_set_style_bg_color(mode_boot_progress_panel, c->surface, 0);
        lv_obj_set_style_bg_opa(mode_boot_progress_panel, LV_OPA_COVER, 0);
        lv_obj_set_style_border_width(mode_boot_progress_panel, 1, 0);
        lv_obj_set_style_border_color(mode_boot_progress_panel, c->border, 0);
        lv_obj_set_style_radius(mode_boot_progress_panel, SCALE(g_colors->radius_lg), 0);
        lv_obj_set_style_pad_all(mode_boot_progress_panel, SCALE(10), 0);
        lv_obj_set_style_pad_gap(mode_boot_progress_panel, SCALE(6), 0);
        lv_obj_set_flex_flow(mode_boot_progress_panel, LV_FLEX_FLOW_COLUMN);
        lv_obj_set_flex_align(mode_boot_progress_panel, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
        lv_obj_clear_flag(mode_boot_progress_panel, LV_OBJ_FLAG_SCROLLABLE);

        mode_boot_progress_task = lv_label_create(mode_boot_progress_panel);
        lv_label_set_text(mode_boot_progress_task, "Task: Boot Check");
        lv_obj_set_style_text_color(mode_boot_progress_task, c->text, 0);
        lv_obj_set_style_text_font(mode_boot_progress_task, CJK_FONT_SMALL, 0);

        mode_boot_progress_step = lv_label_create(mode_boot_progress_panel);
        lv_label_set_text(mode_boot_progress_step, "Step: Waiting...");
        lv_obj_set_style_text_color(mode_boot_progress_step, c->text_dim, 0);
        lv_obj_set_style_text_font(mode_boot_progress_step, CJK_FONT_SMALL, 0);

        mode_boot_progress_bar = lv_bar_create(mode_boot_progress_panel);
        lv_obj_set_width(mode_boot_progress_bar, LV_PCT(100));
        lv_obj_set_height(mode_boot_progress_bar, SCALE(8));
        lv_obj_set_style_radius(mode_boot_progress_bar, SCALE(g_colors->radius_sm), 0);
        lv_obj_set_style_bg_color(mode_boot_progress_bar, c->border, LV_PART_MAIN);
        lv_obj_set_style_bg_color(mode_boot_progress_bar, c->accent, LV_PART_INDICATOR);
        lv_bar_set_range(mode_boot_progress_bar, 0, 100);
        lv_bar_set_value(mode_boot_progress_bar, 0, LV_ANIM_OFF);

        mode_boot_progress_result = lv_label_create(mode_boot_progress_panel);
        lv_label_set_text(mode_boot_progress_result, "");
        lv_obj_set_style_text_font(mode_boot_progress_result, CJK_FONT_SMALL, 0);
        lv_obj_set_style_text_color(mode_boot_progress_result, c->text_dim, 0);

        mode_boot_progress_list = lv_textarea_create(mode_boot_progress_panel);
        lv_obj_set_width(mode_boot_progress_list, LV_PCT(100));
        lv_obj_set_height(mode_boot_progress_list, SCALE(110));
        lv_textarea_set_one_line(mode_boot_progress_list, false);
        lv_textarea_set_text(mode_boot_progress_list, "No checks run yet.");
        lv_obj_set_style_radius(mode_boot_progress_list, SCALE(g_colors->radius_md), 0);
        lv_obj_set_style_bg_color(mode_boot_progress_list, c->panel, 0);
        lv_obj_set_style_border_width(mode_boot_progress_list, 1, 0);
        lv_obj_set_style_border_color(mode_boot_progress_list, c->border, 0);
        lv_obj_set_style_text_font(mode_boot_progress_list, CJK_FONT_SMALL, 0);
        lv_obj_set_style_text_color(mode_boot_progress_list, c->text, 0);
        lv_obj_add_state(mode_boot_progress_list, LV_STATE_DISABLED);
        lv_obj_add_flag(mode_boot_progress_panel, LV_OBJ_FLAG_HIDDEN);
        lv_obj_t* btn_retry_failed_attach = aw_btn_create(sec_runtime, "Retry Failed Attachments", BTN_SECONDARY, SCALE(220), SCALE(34));
        lv_obj_add_event_cb(btn_retry_failed_attach, work_retry_failed_attachments_cb, LV_EVENT_CLICKED, nullptr);

        lv_obj_t* sec_trace = aw_form_section_create(mode_panel_work, "Agent Trace", card_w);
        mode_lbl_work_next_step = aw_label_wrap_create(sec_trace, "", LABEL_HINT, 100);
        lv_obj_set_style_text_color(mode_lbl_work_next_step, c->accent, 0);
        mode_ta_work_script = aw_textarea_create(sec_trace, "Agent script output...", false, card_w - 24, SCALE(120));
        lv_textarea_set_text_selection(mode_ta_work_script, true);
        lv_obj_set_style_bg_color(mode_ta_work_script, c->panel, 0);
        lv_obj_set_style_text_color(mode_ta_work_script, c->text, 0);
        lv_obj_set_style_text_font(mode_ta_work_script, CJK_FONT_SMALL, 0);
        update_agent_trace_views();

        lv_obj_t* sec_obs = aw_form_section_create(mode_panel_work, "Observability (Recent Traces)", card_w);
        mode_ta_trace_recent = aw_textarea_create(sec_obs, "Trace timeline...", false, card_w - 24, SCALE(120));
        lv_textarea_set_text_selection(mode_ta_trace_recent, true);
        lv_obj_set_style_bg_color(mode_ta_trace_recent, c->panel, 0);
        lv_obj_set_style_text_color(mode_ta_trace_recent, c->text, 0);
        lv_obj_set_style_text_font(mode_ta_trace_recent, CJK_FONT_SMALL, 0);
        lv_obj_t* btn_trace_refresh = aw_btn_create(sec_obs, "Refresh Traces", BTN_SECONDARY, SCALE(150), SCALE(34));
        lv_obj_add_event_cb(btn_trace_refresh, work_trace_refresh_cb, LV_EVENT_CLICKED, nullptr);
        lv_obj_t* btn_trace_sort = aw_btn_create(sec_obs, "Sort By Latency", BTN_SECONDARY, SCALE(150), SCALE(34));
        lv_obj_add_event_cb(btn_trace_sort, work_trace_toggle_sort_cb, LV_EVENT_CLICKED, nullptr);
        lv_obj_t* btn_trace_filter = aw_btn_create(sec_obs, "Toggle Fail-Only", BTN_SECONDARY, SCALE(150), SCALE(34));
        lv_obj_add_event_cb(btn_trace_filter, work_trace_toggle_fail_filter_cb, LV_EVENT_CLICKED, nullptr);
        work_trace_refresh_cb(nullptr);

        mode_sec_step_stream = aw_form_section_create(mode_panel_work, "Step Stream", card_w);
        mode_work_step_stream = lv_obj_create(mode_sec_step_stream);
        lv_obj_set_size(mode_work_step_stream, card_w - 24, SCALE(g_work_step_stream_h));
        lv_obj_set_style_bg_color(mode_work_step_stream, c->panel, 0);
        lv_obj_set_style_border_color(mode_work_step_stream, c->border, 0);
        lv_obj_set_style_border_width(mode_work_step_stream, 1, 0);
        lv_obj_set_style_radius(mode_work_step_stream, SCALE(g_colors->radius_lg), 0);
        lv_obj_set_style_pad_all(mode_work_step_stream, 12, 0);
        lv_obj_set_style_pad_gap(mode_work_step_stream, 8, 0);
        lv_obj_set_scroll_dir(mode_work_step_stream, LV_DIR_VER);
        lv_obj_set_scrollbar_mode(mode_work_step_stream, LV_SCROLLBAR_MODE_AUTO);
        lv_obj_set_flex_flow(mode_work_step_stream, LV_FLEX_FLOW_COLUMN);
        lv_obj_set_flex_align(mode_work_step_stream, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
        lv_obj_clear_flag(mode_work_step_stream, LV_OBJ_FLAG_SCROLL_CHAIN);
        mode_work_empty_card = lv_obj_create(mode_sec_step_stream);
        lv_obj_set_size(mode_work_empty_card, card_w - 24, SCALE(74));
        lv_obj_set_style_bg_color(mode_work_empty_card, c->panel, 0);
        lv_obj_set_style_border_color(mode_work_empty_card, c->border, 0);
        lv_obj_set_style_border_width(mode_work_empty_card, 1, 0);
        lv_obj_set_style_radius(mode_work_empty_card, SCALE(g_colors->radius_lg), 0);
        lv_obj_set_style_pad_all(mode_work_empty_card, 10, 0);
        lv_obj_set_style_pad_gap(mode_work_empty_card, 6, 0);
        lv_obj_set_flex_flow(mode_work_empty_card, LV_FLEX_FLOW_COLUMN);
        lv_obj_set_flex_align(mode_work_empty_card, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        lv_obj_clear_flag(mode_work_empty_card, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_t* we_icon = lv_label_create(mode_work_empty_card);
        lv_label_set_text(we_icon, "⚡");
        lv_obj_set_style_text_font(we_icon, CJK_FONT, 0);
        lv_obj_t* we_text = lv_label_create(mode_work_empty_card);
        lv_label_set_text(we_text, "AI 工作台已就绪 - 在 Chat 中发送任务");
        lv_obj_set_style_text_color(we_text, c->text_dim, 0);
        lv_obj_set_style_text_font(we_text, CJK_FONT_SMALL, 0);
        lv_obj_t* we_btn = aw_btn_create(mode_work_empty_card, "查看示例任务", BTN_SECONDARY, SCALE(160), SCALE(32));
        static const char* k_work_example = "请读取当前工作区并给我一个可执行的三步改造计划。";
        lv_obj_add_event_cb(we_btn, work_empty_example_cb, LV_EVENT_CLICKED, (void*)k_work_example);
        work_add_step_card("等待任务", "在 Chat 或 Work 输入任务后，将按步骤展示执行过程。", true, false);
        update_work_empty_state();

        mode_work_splitter = lv_obj_create(mode_panel_work);
        lv_obj_set_size(mode_work_splitter, card_w - 24, SCALE(6));
        lv_obj_set_style_bg_color(mode_work_splitter, c->surface, 0);
        lv_obj_set_style_border_width(mode_work_splitter, 0, 0);
        lv_obj_set_style_radius(mode_work_splitter, SCALE(3), 0);
        lv_obj_clear_flag(mode_work_splitter, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_add_flag(mode_work_splitter, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_event_cb(mode_work_splitter, work_vertical_splitter_cb, LV_EVENT_PRESSED, nullptr);
        lv_obj_add_event_cb(mode_work_splitter, work_vertical_splitter_cb, LV_EVENT_PRESSING, nullptr);
        lv_obj_add_event_cb(mode_work_splitter, work_vertical_splitter_cb, LV_EVENT_RELEASED, nullptr);

        mode_sec_work_console = aw_form_section_create(mode_panel_work, "Output Area", card_w);
        mode_work_output_wrap = lv_obj_create(mode_sec_work_console);
        lv_obj_set_size(mode_work_output_wrap, card_w - 24, SCALE(g_work_output_h));
        lv_obj_set_style_bg_color(mode_work_output_wrap, c->panel, 0);
        lv_obj_set_style_border_color(mode_work_output_wrap, c->border, 0);
        lv_obj_set_style_border_width(mode_work_output_wrap, 1, 0);
        lv_obj_set_style_radius(mode_work_output_wrap, SCALE(g_colors->radius_lg), 0);
        lv_obj_set_style_pad_all(mode_work_output_wrap, 12, 0);
        lv_obj_set_scroll_dir(mode_work_output_wrap, LV_DIR_VER);
        lv_obj_set_scrollbar_mode(mode_work_output_wrap, LV_SCROLLBAR_MODE_AUTO);
        lv_obj_set_flex_flow(mode_work_output_wrap, LV_FLEX_FLOW_COLUMN);
        lv_obj_set_flex_align(mode_work_output_wrap, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
        lv_obj_set_style_pad_gap(mode_work_output_wrap, 8, 0);

        mode_lbl_work_output_title = lv_label_create(mode_work_output_wrap);
        lv_label_set_text(mode_lbl_work_output_title, "Output: waiting");
        lv_obj_set_width(mode_lbl_work_output_title, LV_PCT(100));
        lv_obj_set_style_text_color(mode_lbl_work_output_title, c->text, 0);
        lv_obj_set_style_text_font(mode_lbl_work_output_title, CJK_FONT_SMALL, 0);

        mode_lbl_work_renderer = lv_label_create(mode_work_output_wrap);
        lv_label_set_text(mode_lbl_work_renderer, "Renderer: preview");
        lv_obj_set_width(mode_lbl_work_renderer, LV_PCT(100));
        lv_obj_set_style_text_color(mode_lbl_work_renderer, c->accent, 0);
        lv_obj_set_style_text_font(mode_lbl_work_renderer, CJK_FONT_SMALL, 0);

        mode_lbl_work_md_output = lv_label_create(mode_work_output_wrap);
        lv_obj_set_width(mode_lbl_work_md_output, LV_PCT(100));
        lv_obj_set_style_text_color(mode_lbl_work_md_output, c->text, 0);
        lv_obj_set_style_text_font(mode_lbl_work_md_output, CJK_FONT_CHAT, 0);
        lv_label_set_long_mode(mode_lbl_work_md_output, LV_LABEL_LONG_WRAP);
        render_work_md_doc();

        mode_ta_work_prompt = aw_textarea_create(mode_sec_work_console, "输入任务... (Enter 发送, Shift+Enter 换行)", false, card_w - 24, SCALE(96));
        lv_textarea_set_one_line(mode_ta_work_prompt, false);
        lv_textarea_set_text_selection(mode_ta_work_prompt, true);
        lv_obj_add_event_cb(mode_ta_work_prompt, work_prompt_input_cb, LV_EVENT_KEY, nullptr);
        lv_obj_set_style_radius(mode_ta_work_prompt, SCALE(g_colors->radius_lg), 0);
        lv_obj_set_style_pad_all(mode_ta_work_prompt, 12, 0);
        lv_obj_set_style_pad_ver(mode_ta_work_prompt, 10, 0);
        lv_obj_set_style_border_width(mode_ta_work_prompt, 1, 0);
        lv_obj_set_style_border_color(mode_ta_work_prompt, c->border, 0);
        lv_obj_set_style_border_color(mode_ta_work_prompt, c->accent, LV_STATE_FOCUSED);
        lv_obj_set_style_border_width(mode_ta_work_prompt, 2, LV_STATE_FOCUSED);
        lv_obj_t* btn_work_send = aw_btn_create(mode_sec_work_console, "Run In Work", BTN_PRIMARY, SCALE(160), SCALE(40));
        lv_obj_set_style_radius(btn_work_send, SCALE(g_colors->radius_lg), 0);
        lv_obj_add_event_cb(btn_work_send, work_send_cb, LV_EVENT_CLICKED, nullptr);
        lv_obj_t* work_prompt_hint = lv_label_create(mode_sec_work_console);
        lv_label_set_text(work_prompt_hint, "提示: 任务越具体，步骤拆解越准确。");
        lv_obj_set_style_text_color(work_prompt_hint, c->text_dim, 0);
        lv_obj_set_style_text_font(work_prompt_hint, CJK_FONT_SMALL, 0);

        mode_work_chat_panel = aw_form_section_create(mode_panel_work, "Work Chat Panel", card_w);
        lv_obj_add_flag(mode_work_chat_panel, LV_OBJ_FLAG_FLOATING);
        int work_panel_w = content_w * 38 / 100;
        lv_obj_set_width(mode_work_chat_panel, work_panel_w);
        lv_obj_set_height(mode_work_chat_panel, SCALE(220));
        lv_obj_set_pos(mode_work_chat_panel, std::max(SCALE(8), content_w - work_panel_w - SCALE(8)), SCALE(8));
        mode_btn_work_chat_toggle = aw_btn_create(mode_work_chat_panel, "<", BTN_SECONDARY, SCALE(36), SCALE(30));
        lv_obj_add_event_cb(mode_btn_work_chat_toggle, work_chat_toggle_cb, LV_EVENT_CLICKED, nullptr);
        mode_lbl_work_chat_state = lv_label_create(mode_work_chat_panel);
        lv_obj_set_style_text_color(mode_lbl_work_chat_state, c->text_dim, 0);
        lv_obj_set_style_text_font(mode_lbl_work_chat_state, CJK_FONT_SMALL, 0);
        mode_ta_work_chat_feed = aw_textarea_create(mode_work_chat_panel, "Agent summary (mirrored from unified input)...", false, card_w - 24, SCALE(100));
        lv_textarea_set_text_selection(mode_ta_work_chat_feed, true);
        mode_ta_work_chat_input = aw_textarea_create(mode_work_chat_panel, "Unified input only: use main task box above", false, card_w - 24, SCALE(56));
        lv_textarea_set_one_line(mode_ta_work_chat_input, false);
        mode_btn_work_chat_send = aw_btn_create(mode_work_chat_panel, "Send", BTN_PRIMARY, SCALE(100), SCALE(30));
        lv_obj_add_event_cb(mode_btn_work_chat_send, work_chat_send_cb, LV_EVENT_CLICKED, nullptr);
        c2_apply_work_panel_policy();

        lv_obj_t* sec_gemma = aw_form_section_create(mode_panel_work, "Local Gemma 4 Install", card_w);
        lv_obj_t* row_gemma_sw = lv_obj_create(sec_gemma);
        lv_obj_set_width(row_gemma_sw, card_w - 24);
        lv_obj_set_height(row_gemma_sw, LV_SIZE_CONTENT);
        lv_obj_set_style_bg_opa(row_gemma_sw, LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_width(row_gemma_sw, 0, 0);
        lv_obj_set_style_pad_all(row_gemma_sw, 0, 0);
        lv_obj_set_style_pad_gap(row_gemma_sw, 8, 0);
        lv_obj_set_flex_flow(row_gemma_sw, LV_FLEX_FLOW_ROW);
        lv_obj_set_flex_align(row_gemma_sw, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        lv_obj_clear_flag(row_gemma_sw, LV_OBJ_FLAG_SCROLLABLE);

        lv_obj_t* lbl_gemma_sw = lv_label_create(row_gemma_sw);
        lv_label_set_text(lbl_gemma_sw, "Enable local Gemma 4 install");
        lv_obj_set_style_text_color(lbl_gemma_sw, c->text, 0);
        lv_obj_set_style_text_font(lbl_gemma_sw, CJK_FONT_SMALL, 0);

        mode_sw_gemma_install = lv_switch_create(row_gemma_sw);
        if (g_gemma_install_opt_in) lv_obj_add_state(mode_sw_gemma_install, LV_STATE_CHECKED);
        lv_obj_set_style_bg_color(mode_sw_gemma_install, c->btn_secondary, 0);
        lv_obj_set_style_bg_color(mode_sw_gemma_install, c->btn_action, LV_PART_INDICATOR | LV_STATE_CHECKED);
        lv_obj_add_event_cb(mode_sw_gemma_install, gemma_install_sw_cb, LV_EVENT_VALUE_CHANGED, nullptr);

        mode_cb_gemma_2b = lv_checkbox_create(sec_gemma);
        lv_checkbox_set_text(mode_cb_gemma_2b, "Gemma 4 2B (builtin in installer)");
        lv_obj_set_style_text_color(mode_cb_gemma_2b, c->text, 0);
        lv_obj_set_style_text_font(mode_cb_gemma_2b, CJK_FONT_SMALL, 0);
        lv_obj_add_event_cb(mode_cb_gemma_2b, gemma_model_checkbox_cb, LV_EVENT_VALUE_CHANGED, nullptr);

        mode_cb_gemma_9b = lv_checkbox_create(sec_gemma);
        lv_checkbox_set_text(mode_cb_gemma_9b, "Gemma 4 9B (network download)");
        lv_obj_set_style_text_color(mode_cb_gemma_9b, c->text, 0);
        lv_obj_set_style_text_font(mode_cb_gemma_9b, CJK_FONT_SMALL, 0);
        lv_obj_add_event_cb(mode_cb_gemma_9b, gemma_model_checkbox_cb, LV_EVENT_VALUE_CHANGED, nullptr);

        mode_cb_gemma_27b = lv_checkbox_create(sec_gemma);
        lv_checkbox_set_text(mode_cb_gemma_27b, "Gemma 4 27B (network download)");
        lv_obj_set_style_text_color(mode_cb_gemma_27b, c->text, 0);
        lv_obj_set_style_text_font(mode_cb_gemma_27b, CJK_FONT_SMALL, 0);
        lv_obj_add_event_cb(mode_cb_gemma_27b, gemma_model_checkbox_cb, LV_EVENT_VALUE_CHANGED, nullptr);

        mode_lbl_gemma_recommend = aw_label_wrap_create(sec_gemma, "", LABEL_HINT, 100);
        lv_obj_set_style_text_color(mode_lbl_gemma_recommend, c->text_dim, 0);
        if (g_gemma_install_opt_in && g_gemma_model_mask == 0) g_gemma_model_mask = gemma_recommended_mask();
        if (!g_gemma_install_opt_in) g_gemma_model_mask = 0;
        update_gemma_recommend_visuals();

        mode_remote_warning_bar = lv_obj_create(sec_runtime);
        lv_obj_set_width(mode_remote_warning_bar, card_w - 24);
        lv_obj_set_height(mode_remote_warning_bar, SCALE(34));
        lv_obj_set_style_bg_color(mode_remote_warning_bar, c->btn_close, 0);
        lv_obj_set_style_bg_opa(mode_remote_warning_bar, LV_OPA_COVER, 0);
        lv_obj_set_style_border_width(mode_remote_warning_bar, 0, 0);
        lv_obj_set_style_radius(mode_remote_warning_bar, SCALE(g_colors->radius_sm), 0);
        lv_obj_set_style_pad_hor(mode_remote_warning_bar, 8, 0);
        lv_obj_clear_flag(mode_remote_warning_bar, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_t* danger_lbl = lv_label_create(mode_remote_warning_bar);
        lv_label_set_text(danger_lbl, "REMOTE CONTROL ACTIVE - PRESS DISCONNECT IF ABNORMAL");
        lv_obj_set_style_text_color(danger_lbl, lv_color_white(), 0);
        lv_obj_set_style_text_font(danger_lbl, CJK_FONT_SMALL, 0);
        lv_obj_center(danger_lbl);

        lv_obj_t* sec_profile = aw_form_section_create(mode_panel_work, "Profile", card_w);
        aw_form_field_text(sec_profile, "User name", g_profile_user_name, &mode_ta_user_name, false, card_w - 24);
        aw_form_field_text(sec_profile, "User role", g_profile_user_role, &mode_ta_user_role, false, card_w - 24);
        aw_form_field_dropdown(sec_profile, "User avatar", "garlic\nlobster",
                               (uint16_t)(strcmp(g_profile_user_avatar, "lobster") == 0 ? 1 : 0),
                               &mode_dd_user_avatar, card_w - 24);
        aw_form_field_text(sec_profile, "AI name", g_profile_ai_name, &mode_ta_ai_name, false, card_w - 24);
        aw_form_field_text(sec_profile, "AI role", g_profile_ai_role, &mode_ta_ai_role, false, card_w - 24);
        aw_form_field_dropdown(sec_profile, "AI avatar", "lobster\ngarlic",
                               (uint16_t)(strcmp(g_profile_ai_avatar, "garlic") == 0 ? 1 : 0),
                               &mode_dd_ai_avatar, card_w - 24);
        aw_form_field_text(sec_profile, "AI persona", g_profile_ai_persona, &mode_ta_ai_persona, true, card_w - 24);
        aw_form_field_text(sec_profile, "AI skills", g_profile_ai_skills, &mode_ta_ai_skills, true, card_w - 24);

        lv_obj_t* btn_save_profile = aw_btn_create(sec_profile, "Save Profile", BTN_PRIMARY, SCALE(150), SCALE(34));
        lv_obj_add_event_cb(btn_save_profile, work_profile_save_cb, LV_EVENT_CLICKED, nullptr);

        lv_obj_t* sec_profile_cards = aw_form_section_create(mode_panel_work, "Role Cards", card_w);
        lv_obj_t* cards_row = lv_obj_create(sec_profile_cards);
        lv_obj_set_width(cards_row, card_w - 24);
        lv_obj_set_height(cards_row, LV_SIZE_CONTENT);
        lv_obj_set_style_bg_opa(cards_row, LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_width(cards_row, 0, 0);
        lv_obj_set_style_pad_all(cards_row, 0, 0);
        lv_obj_set_style_pad_gap(cards_row, 10, 0);
        lv_obj_set_flex_flow(cards_row, LV_FLEX_FLOW_ROW);
        lv_obj_set_flex_align(cards_row, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
        lv_obj_clear_flag(cards_row, LV_OBJ_FLAG_SCROLLABLE);

        auto create_role_card = [&](lv_obj_t** out_avatar, lv_obj_t** out_text) {
            lv_obj_t* card = lv_obj_create(cards_row);
            lv_obj_set_size(card, std::max(SCALE(180), (card_w - 40) / 2), SCALE(110));
            lv_obj_set_style_bg_color(card, c->surface, 0);
            lv_obj_set_style_border_color(card, c->border, 0);
            lv_obj_set_style_border_width(card, 1, 0);
            lv_obj_set_style_radius(card, SCALE(g_colors->radius_md), 0);
            lv_obj_set_style_pad_all(card, 10, 0);
            lv_obj_set_style_pad_gap(card, 8, 0);
            lv_obj_set_flex_flow(card, LV_FLEX_FLOW_ROW);
            lv_obj_set_flex_align(card, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
            lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);
            *out_avatar = lv_img_create(card);
            lv_obj_set_size(*out_avatar, SCALE(AVATAR_AI_SIZE), SCALE(AVATAR_AI_SIZE));
            *out_text = lv_label_create(card);
            lv_obj_set_style_text_color(*out_text, c->text, 0);
            lv_obj_set_style_text_font(*out_text, CJK_FONT_SMALL, 0);
        };
        create_role_card(&mode_profile_user_avatar_preview, &mode_profile_user_text_preview);
        create_role_card(&mode_profile_ai_avatar_preview, &mode_profile_ai_text_preview);
        update_profile_preview_cards();

        lv_obj_t* sec_remote = aw_form_section_create(mode_panel_work, "Remote Safety", card_w);
        lv_obj_t* remote_row = lv_obj_create(sec_remote);
        lv_obj_set_width(remote_row, card_w - 24);
        lv_obj_set_height(remote_row, LV_SIZE_CONTENT);
        lv_obj_set_style_bg_opa(remote_row, LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_width(remote_row, 0, 0);
        lv_obj_set_style_pad_all(remote_row, 0, 0);
        lv_obj_set_style_pad_gap(remote_row, 8, 0);
        lv_obj_set_flex_flow(remote_row, LV_FLEX_FLOW_ROW);
        lv_obj_set_flex_align(remote_row, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        lv_obj_clear_flag(remote_row, LV_OBJ_FLAG_SCROLLABLE);

        lv_obj_t* remote_label = lv_label_create(remote_row);
        lv_label_set_text(remote_label, "Arm remote guard");
        lv_obj_set_style_text_color(remote_label, c->text, 0);
        lv_obj_set_style_text_font(remote_label, CJK_FONT_SMALL, 0);

        mode_sw_remote_guard = lv_switch_create(remote_row);
        if (g_remote_guard_armed) lv_obj_add_state(mode_sw_remote_guard, LV_STATE_CHECKED);
        lv_obj_set_style_bg_color(mode_sw_remote_guard, c->btn_secondary, 0);
        lv_obj_set_style_bg_color(mode_sw_remote_guard, c->btn_action, LV_PART_INDICATOR | LV_STATE_CHECKED);
        lv_obj_add_event_cb(mode_sw_remote_guard, work_remote_guard_sw_cb, LV_EVENT_VALUE_CHANGED, nullptr);

        mode_lbl_remote_state = aw_label_wrap_create(sec_remote, "", LABEL_HINT, 100);
        lv_obj_set_style_text_color(mode_lbl_remote_state, c->text_dim, 0);
        lv_obj_t* btn_remote_cut = aw_btn_create(sec_remote, "Emergency Disconnect", BTN_DANGER, SCALE(220), SCALE(34));
        lv_obj_add_event_cb(btn_remote_cut, work_remote_disconnect_cb, LV_EVENT_CLICKED, nullptr);
        update_remote_guard_visuals();

        lv_obj_t* sec_remote_flow = aw_form_section_create(mode_panel_work, "Remote Collaboration", card_w);
        mode_ta_remote_host = aw_textarea_create(sec_remote_flow, "Remote host", true, card_w - 24, SCALE(34));
        lv_textarea_set_text(mode_ta_remote_host, "127.0.0.1");
        mode_ta_remote_port = aw_textarea_create(sec_remote_flow, "Remote port", true, card_w - 24, SCALE(34));
        lv_textarea_set_text(mode_ta_remote_port, "21999");
        mode_lbl_remote_session_state = aw_label_wrap_create(sec_remote_flow, "", LABEL_HINT, 100);
        lv_obj_set_style_text_color(mode_lbl_remote_session_state, c->text_dim, 0);

        lv_obj_t* remote_btn_row1 = lv_obj_create(sec_remote_flow);
        lv_obj_set_width(remote_btn_row1, card_w - 24);
        lv_obj_set_height(remote_btn_row1, LV_SIZE_CONTENT);
        lv_obj_set_style_bg_opa(remote_btn_row1, LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_width(remote_btn_row1, 0, 0);
        lv_obj_set_style_pad_all(remote_btn_row1, 0, 0);
        lv_obj_set_style_pad_gap(remote_btn_row1, 8, 0);
        lv_obj_set_flex_flow(remote_btn_row1, LV_FLEX_FLOW_ROW);
        lv_obj_set_flex_align(remote_btn_row1, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        lv_obj_clear_flag(remote_btn_row1, LV_OBJ_FLAG_SCROLLABLE);

        mode_btn_remote_request = aw_btn_create(remote_btn_row1, "Send Request", BTN_PRIMARY, SCALE(140), SCALE(34));
        lv_obj_add_event_cb(mode_btn_remote_request, remote_request_cb, LV_EVENT_CLICKED, nullptr);
        mode_btn_remote_disconnect = aw_btn_create(remote_btn_row1, "Disconnect", BTN_DANGER, SCALE(130), SCALE(34));
        lv_obj_add_event_cb(mode_btn_remote_disconnect, remote_disconnect_cb, LV_EVENT_CLICKED, nullptr);

        lv_obj_t* remote_btn_row2 = lv_obj_create(sec_remote_flow);
        lv_obj_set_width(remote_btn_row2, card_w - 24);
        lv_obj_set_height(remote_btn_row2, LV_SIZE_CONTENT);
        lv_obj_set_style_bg_opa(remote_btn_row2, LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_width(remote_btn_row2, 0, 0);
        lv_obj_set_style_pad_all(remote_btn_row2, 0, 0);
        lv_obj_set_style_pad_gap(remote_btn_row2, 8, 0);
        lv_obj_set_flex_flow(remote_btn_row2, LV_FLEX_FLOW_ROW);
        lv_obj_set_flex_align(remote_btn_row2, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        lv_obj_clear_flag(remote_btn_row2, LV_OBJ_FLAG_SCROLLABLE);

        mode_btn_remote_accept = aw_btn_create(remote_btn_row2, "Accept", BTN_SUCCESS, SCALE(120), SCALE(34));
        lv_obj_add_event_cb(mode_btn_remote_accept, remote_accept_cb, LV_EVENT_CLICKED, nullptr);
        mode_btn_remote_reject = aw_btn_create(remote_btn_row2, "Reject", BTN_SECONDARY, SCALE(120), SCALE(34));
        lv_obj_add_event_cb(mode_btn_remote_reject, remote_reject_cb, LV_EVENT_CLICKED, nullptr);
        update_remote_session_visuals();

        lv_obj_t* sec_cron = aw_form_section_create(mode_panel_work, "Cron Tasks", card_w);
        mode_ta_cron_id = aw_textarea_create(sec_cron, "Cron ID", true, card_w - 24, SCALE(34));
        mode_ta_cron_list = aw_textarea_create(sec_cron, "Cron list output", false, card_w - 24, SCALE(120));
        lv_textarea_set_text_selection(mode_ta_cron_list, true);
        lv_obj_t* cron_row = lv_obj_create(sec_cron);
        lv_obj_set_width(cron_row, card_w - 24);
        lv_obj_set_height(cron_row, LV_SIZE_CONTENT);
        lv_obj_set_style_bg_opa(cron_row, LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_width(cron_row, 0, 0);
        lv_obj_set_style_pad_all(cron_row, 0, 0);
        lv_obj_set_style_pad_gap(cron_row, 8, 0);
        lv_obj_set_flex_flow(cron_row, LV_FLEX_FLOW_ROW);
        lv_obj_set_flex_align(cron_row, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        lv_obj_clear_flag(cron_row, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_t* cron_btn_refresh = aw_btn_create(cron_row, "Refresh", BTN_SECONDARY, SCALE(100), SCALE(34));
        lv_obj_t* cron_btn_run = aw_btn_create(cron_row, "Run", BTN_PRIMARY, SCALE(90), SCALE(34));
        lv_obj_t* cron_btn_enable = aw_btn_create(cron_row, "Enable", BTN_SECONDARY, SCALE(90), SCALE(34));
        lv_obj_t* cron_btn_disable = aw_btn_create(cron_row, "Disable", BTN_SECONDARY, SCALE(90), SCALE(34));
        lv_obj_t* cron_btn_delete = aw_btn_create(cron_row, "Delete", BTN_DANGER, SCALE(90), SCALE(34));
        lv_obj_add_event_cb(cron_btn_refresh, work_cron_refresh_cb, LV_EVENT_CLICKED, nullptr);
        lv_obj_add_event_cb(cron_btn_run, work_cron_run_cb, LV_EVENT_CLICKED, nullptr);
        lv_obj_add_event_cb(cron_btn_enable, work_cron_enable_cb, LV_EVENT_CLICKED, nullptr);
        lv_obj_add_event_cb(cron_btn_disable, work_cron_disable_cb, LV_EVENT_CLICKED, nullptr);
        lv_obj_add_event_cb(cron_btn_delete, work_cron_delete_cb, LV_EVENT_CLICKED, nullptr);
        work_cron_refresh_cb(nullptr);

        lv_obj_t* sec_lan = aw_form_section_create(mode_panel_work, "LAN Chat (Host/Private/Group)", card_w);
        mode_sec_lan = sec_lan;
        mode_ta_lan_host = aw_textarea_create(sec_lan, "LAN host IP, e.g. 192.168.1.10", true, card_w - 24, SCALE(34));
        lv_textarea_set_text(mode_ta_lan_host, "127.0.0.1");
        mode_ta_lan_port = aw_textarea_create(sec_lan, "Port", true, card_w - 24, SCALE(34));
        lv_textarea_set_text(mode_ta_lan_port, "19999");
        mode_ta_lan_nick = aw_textarea_create(sec_lan, "Nickname", true, card_w - 24, SCALE(34));
        lv_textarea_set_text(mode_ta_lan_nick, g_profile_user_name[0] ? g_profile_user_name : "user");
        lv_obj_t* lan_row_a = lv_obj_create(sec_lan);
        lv_obj_set_width(lan_row_a, card_w - 24);
        lv_obj_set_height(lan_row_a, LV_SIZE_CONTENT);
        lv_obj_set_style_bg_opa(lan_row_a, LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_width(lan_row_a, 0, 0);
        lv_obj_set_style_pad_all(lan_row_a, 0, 0);
        lv_obj_set_style_pad_gap(lan_row_a, 8, 0);
        lv_obj_set_flex_flow(lan_row_a, LV_FLEX_FLOW_ROW);
        lv_obj_set_flex_align(lan_row_a, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        lv_obj_clear_flag(lan_row_a, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_t* lan_btn_host = aw_btn_create(lan_row_a, "Start Host", BTN_SECONDARY, SCALE(120), SCALE(34));
        lv_obj_add_event_cb(lan_btn_host, work_lan_start_host_cb, LV_EVENT_CLICKED, nullptr);
        lv_obj_t* lan_btn_conn = aw_btn_create(lan_row_a, "Connect", BTN_PRIMARY, SCALE(120), SCALE(34));
        lv_obj_add_event_cb(lan_btn_conn, work_lan_connect_cb, LV_EVENT_CLICKED, nullptr);
        lv_obj_t* lan_btn_discover = aw_btn_create(lan_row_a, "Discover", BTN_SECONDARY, SCALE(110), SCALE(34));
        lv_obj_add_event_cb(lan_btn_discover, work_lan_discover_cb, LV_EVENT_CLICKED, nullptr);
        mode_ta_lan_to = aw_textarea_create(sec_lan, "Private to user", true, card_w - 24, SCALE(34));
        mode_ta_lan_group = aw_textarea_create(sec_lan, "Group name", true, card_w - 24, SCALE(34));
        mode_ta_lan_msg = aw_textarea_create(sec_lan, "LAN message", false, card_w - 24, SCALE(80));
        lv_obj_t* lan_row_b = lv_obj_create(sec_lan);
        lv_obj_set_width(lan_row_b, card_w - 24);
        lv_obj_set_height(lan_row_b, LV_SIZE_CONTENT);
        lv_obj_set_style_bg_opa(lan_row_b, LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_width(lan_row_b, 0, 0);
        lv_obj_set_style_pad_all(lan_row_b, 0, 0);
        lv_obj_set_style_pad_gap(lan_row_b, 8, 0);
        lv_obj_set_flex_flow(lan_row_b, LV_FLEX_FLOW_ROW_WRAP);
        lv_obj_set_flex_align(lan_row_b, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        lv_obj_clear_flag(lan_row_b, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_t* lan_btn_global = aw_btn_create(lan_row_b, "Send Global", BTN_PRIMARY, SCALE(130), SCALE(34));
        lv_obj_add_event_cb(lan_btn_global, work_lan_send_global_cb, LV_EVENT_CLICKED, nullptr);
        lv_obj_t* lan_btn_pm = aw_btn_create(lan_row_b, "Send Private", BTN_SECONDARY, SCALE(130), SCALE(34));
        lv_obj_add_event_cb(lan_btn_pm, work_lan_send_private_cb, LV_EVENT_CLICKED, nullptr);
        lv_obj_t* lan_btn_cg = aw_btn_create(lan_row_b, "Create Group", BTN_SECONDARY, SCALE(130), SCALE(34));
        lv_obj_add_event_cb(lan_btn_cg, work_lan_create_group_cb, LV_EVENT_CLICKED, nullptr);
        lv_obj_t* lan_btn_jg = aw_btn_create(lan_row_b, "Join Group", BTN_SECONDARY, SCALE(120), SCALE(34));
        lv_obj_add_event_cb(lan_btn_jg, work_lan_join_group_cb, LV_EVENT_CLICKED, nullptr);
        lv_obj_t* lan_btn_sg = aw_btn_create(lan_row_b, "Send Group", BTN_PRIMARY, SCALE(120), SCALE(34));
        lv_obj_add_event_cb(lan_btn_sg, work_lan_send_group_cb, LV_EVENT_CLICKED, nullptr);
        mode_ta_lan_users = aw_textarea_create(sec_lan, "Online users (csv)", true, card_w - 24, SCALE(34));
        mode_ta_lan_groups = aw_textarea_create(sec_lan, "Groups (csv)", true, card_w - 24, SCALE(34));
        mode_ta_lan_feed = aw_textarea_create(sec_lan, "LAN event feed", false, card_w - 24, SCALE(110));
        lv_textarea_set_text_selection(mode_ta_lan_feed, true);

        lv_obj_t* sec_ftp = aw_form_section_create(mode_panel_work, "FTP Upload/Download", card_w);
        mode_sec_ftp = sec_ftp;
        mode_ta_ftp_host = aw_textarea_create(sec_ftp, "FTP host", true, card_w - 24, SCALE(34));
        mode_ta_ftp_port = aw_textarea_create(sec_ftp, "FTP port", true, card_w - 24, SCALE(34));
        lv_textarea_set_text(mode_ta_ftp_port, "21");
        mode_ta_ftp_user = aw_textarea_create(sec_ftp, "FTP username", true, card_w - 24, SCALE(34));
        mode_ta_ftp_pass = aw_textarea_create(sec_ftp, "FTP password", true, card_w - 24, SCALE(34));
        mode_ta_ftp_remote = aw_textarea_create(sec_ftp, "Remote path", true, card_w - 24, SCALE(34));
        mode_ta_ftp_local = aw_textarea_create(sec_ftp, "Local file path", true, card_w - 24, SCALE(34));
        mode_sw_ftp_ftps = lv_switch_create(sec_ftp);
        lv_obj_t* ftp_ftps_lbl = lv_label_create(sec_ftp);
        lv_label_set_text(ftp_ftps_lbl, "Use FTPS (secure)");
        mode_sw_ftp_recursive = lv_switch_create(sec_ftp);
        lv_obj_t* ftp_rec_lbl = lv_label_create(sec_ftp);
        lv_label_set_text(ftp_rec_lbl, "Recursive upload (directory)");
        lv_obj_t* ftp_row = lv_obj_create(sec_ftp);
        lv_obj_set_width(ftp_row, card_w - 24);
        lv_obj_set_height(ftp_row, LV_SIZE_CONTENT);
        lv_obj_set_style_bg_opa(ftp_row, LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_width(ftp_row, 0, 0);
        lv_obj_set_style_pad_all(ftp_row, 0, 0);
        lv_obj_set_style_pad_gap(ftp_row, 8, 0);
        lv_obj_set_flex_flow(ftp_row, LV_FLEX_FLOW_ROW_WRAP);
        lv_obj_set_flex_align(ftp_row, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        lv_obj_clear_flag(ftp_row, LV_OBJ_FLAG_SCROLLABLE);
        mode_btn_ftp_upload = aw_btn_create(ftp_row, "FTP Upload", BTN_PRIMARY, SCALE(120), SCALE(34));
        lv_obj_add_event_cb(mode_btn_ftp_upload, work_ftp_upload_cb, LV_EVENT_CLICKED, nullptr);
        mode_btn_ftp_download = aw_btn_create(ftp_row, "FTP Download", BTN_SECONDARY, SCALE(130), SCALE(34));
        lv_obj_add_event_cb(mode_btn_ftp_download, work_ftp_download_cb, LV_EVENT_CLICKED, nullptr);
        mode_btn_ftp_retry = aw_btn_create(ftp_row, "Retry Last", BTN_SECONDARY, SCALE(120), SCALE(34));
        lv_obj_add_event_cb(mode_btn_ftp_retry, work_ftp_retry_cb, LV_EVENT_CLICKED, nullptr);
        lv_obj_add_state(mode_btn_ftp_retry, LV_STATE_DISABLED);
        mode_btn_ftp_cancel = aw_btn_create(ftp_row, "Cancel FTP", BTN_DANGER, SCALE(120), SCALE(34));
        lv_obj_add_event_cb(mode_btn_ftp_cancel, work_ftp_cancel_cb, LV_EVENT_CLICKED, nullptr);
        lv_obj_add_state(mode_btn_ftp_cancel, LV_STATE_DISABLED);

        lv_obj_t* sec_kb = aw_form_section_create(mode_panel_work, "Local Knowledge Base", card_w);
        mode_sec_kb = sec_kb;
        mode_ta_kb_source = aw_textarea_create(sec_kb, "Source file path", true, card_w - 24, SCALE(34));
        mode_ta_kb_keyword = aw_textarea_create(sec_kb, "Search keyword", true, card_w - 24, SCALE(34));
        lv_obj_t* kb_row = lv_obj_create(sec_kb);
        lv_obj_set_width(kb_row, card_w - 24);
        lv_obj_set_height(kb_row, LV_SIZE_CONTENT);
        lv_obj_set_style_bg_opa(kb_row, LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_width(kb_row, 0, 0);
        lv_obj_set_style_pad_all(kb_row, 0, 0);
        lv_obj_set_style_pad_gap(kb_row, 8, 0);
        lv_obj_set_flex_flow(kb_row, LV_FLEX_FLOW_ROW);
        lv_obj_set_flex_align(kb_row, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        lv_obj_clear_flag(kb_row, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_t* kb_add = aw_btn_create(kb_row, "Add Source", BTN_SECONDARY, SCALE(120), SCALE(34));
        lv_obj_add_event_cb(kb_add, work_kb_add_source_cb, LV_EVENT_CLICKED, nullptr);
        lv_obj_t* kb_import = aw_btn_create(kb_row, "Import Dir", BTN_SECONDARY, SCALE(110), SCALE(34));
        lv_obj_add_event_cb(kb_import, work_kb_import_dir_cb, LV_EVENT_CLICKED, nullptr);
        lv_obj_t* kb_search = aw_btn_create(kb_row, "Search", BTN_PRIMARY, SCALE(100), SCALE(34));
        lv_obj_add_event_cb(kb_search, work_kb_search_cb, LV_EVENT_CLICKED, nullptr);
        lv_obj_t* kb_export = aw_btn_create(kb_row, "Export Manifest", BTN_SECONDARY, SCALE(140), SCALE(34));
        lv_obj_add_event_cb(kb_export, work_kb_export_manifest_cb, LV_EVENT_CLICKED, nullptr);
        mode_ta_kb_result = aw_textarea_create(sec_kb, "KB result", false, card_w - 24, SCALE(100));
        update_work_advanced_visibility();
    }

    /* Cursor-like trace panel (Chat mode): next step + script output */
    {
        int trace_h = chat_trace_panel_h(content_w);
        mode_trace_chat_panel = lv_obj_create(mode_panel_chat);
        lv_obj_set_size(mode_trace_chat_panel, content_w - CHAT_GAP * 2, trace_h);
        lv_obj_set_pos(mode_trace_chat_panel, CHAT_GAP, CHAT_GAP);
        lv_obj_set_style_bg_color(mode_trace_chat_panel, c->surface, 0);
        lv_obj_set_style_border_color(mode_trace_chat_panel, c->border, 0);
        lv_obj_set_style_border_width(mode_trace_chat_panel, 1, 0);
        lv_obj_set_style_radius(mode_trace_chat_panel, SCALE(g_colors->radius_md), 0);
        lv_obj_set_style_pad_all(mode_trace_chat_panel, 8, 0);
        lv_obj_set_style_pad_gap(mode_trace_chat_panel, 6, 0);
        lv_obj_set_flex_flow(mode_trace_chat_panel, LV_FLEX_FLOW_COLUMN);
        lv_obj_set_flex_align(mode_trace_chat_panel, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
        lv_obj_clear_flag(mode_trace_chat_panel, LV_OBJ_FLAG_SCROLLABLE);

        mode_lbl_chat_next_step = lv_label_create(mode_trace_chat_panel);
        lv_obj_set_style_text_color(mode_lbl_chat_next_step, c->accent, 0);
        lv_obj_set_style_text_font(mode_lbl_chat_next_step, CJK_FONT_SMALL, 0);

        mode_ta_chat_script = lv_textarea_create(mode_trace_chat_panel);
        lv_obj_set_width(mode_ta_chat_script, LV_PCT(100));
        lv_obj_set_height(mode_ta_chat_script, trace_h - SCALE(42));
        lv_textarea_set_one_line(mode_ta_chat_script, false);
        lv_textarea_set_text_selection(mode_ta_chat_script, true);
        lv_obj_set_style_bg_color(mode_ta_chat_script, c->panel, 0);
        lv_obj_set_style_text_color(mode_ta_chat_script, c->text, 0);
        lv_obj_set_style_text_font(mode_ta_chat_script, CJK_FONT_SMALL, 0);
        lv_obj_set_style_border_color(mode_ta_chat_script, c->border, 0);
        lv_obj_set_style_border_width(mode_ta_chat_script, 1, 0);
        lv_obj_set_style_radius(mode_ta_chat_script, SCALE(g_colors->radius_sm), 0);
    }

    /* Layout: chat fills space, input pinned to bottom; GAP spacing everywhere */
    int input_h = 96;   /* 默认3行（含LVGL内部padding） */
    int chat_y = chat_top_y_with_trace(content_w);
    /* Chat fills from chat_y down to above input; CHAT_GAP between chat and input */
    int chat_h = content_h - chat_y - input_h - CHAT_GAP;
    chat_cont = lv_obj_create(mode_panel_chat);
    lv_obj_set_size(chat_cont, content_w - CHAT_GAP * 2, chat_h);
    lv_obj_set_pos(chat_cont, CHAT_GAP, chat_y);
    lv_obj_set_style_bg_opa(chat_cont, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(chat_cont, 0, 0);
    lv_obj_set_style_radius(chat_cont, 0, 0);
    lv_obj_set_style_pad_all(chat_cont, 4, 0);
    /* Flex column: messages flow from top */
    lv_obj_set_flex_flow(chat_cont, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(chat_cont, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    /* ═══ Scroll: scrollbar visible, wheel-scroll allowed, drag-scroll blocked ═══ */
    lv_obj_set_scrollbar_mode(chat_cont, LV_SCROLLBAR_MODE_AUTO);  /* scrollbar on right */
    lv_obj_set_scroll_dir(chat_cont, LV_DIR_VER);   /* allow vertical scroll (wheel) */
    lv_obj_clear_flag(chat_cont, LV_OBJ_FLAG_SCROLL_MOMENTUM);
    lv_obj_clear_flag(chat_cont, LV_OBJ_FLAG_SCROLL_ELASTIC);
    lv_obj_clear_flag(chat_cont, LV_OBJ_FLAG_SCROLL_ON_FOCUS);     /* don't auto-scroll on child focus */
    lv_obj_clear_flag(chat_cont, LV_OBJ_FLAG_SCROLL_CHAIN_VER);    /* don't propagate scroll to parent */
    lv_obj_add_event_cb(chat_cont, chat_scroll_begin_cb, LV_EVENT_SCROLL_BEGIN, nullptr);
    /* No chat_focus_cb needed — SCROLL_ON_FOCUS disabled prevents focus-triggered scroll */
    lv_group_add_obj(lv_group_get_default(), chat_cont);
    /* Flex gap between bubbles */
    lv_obj_set_style_pad_gap(chat_cont, GAP, 0);

    /* Force layout so chat_cont width is resolved — children with LV_PCT(100)
     * inside LV_SIZE_CONTENT parents will get correct pixel widths. */
    lv_obj_update_layout(chat_cont);

    mode_chat_empty_card = lv_obj_create(mode_panel_chat);
    lv_obj_set_size(mode_chat_empty_card, content_w - CHAT_GAP * 2, SCALE(92));
    lv_obj_set_pos(mode_chat_empty_card, CHAT_GAP, chat_y + SCALE(8));
    lv_obj_set_style_bg_color(mode_chat_empty_card, c->panel, 0);
    lv_obj_set_style_border_color(mode_chat_empty_card, c->border, 0);
    lv_obj_set_style_border_width(mode_chat_empty_card, 1, 0);
    lv_obj_set_style_radius(mode_chat_empty_card, SCALE(g_colors->radius_lg), 0);
    lv_obj_set_style_pad_all(mode_chat_empty_card, 10, 0);
    lv_obj_set_style_pad_gap(mode_chat_empty_card, 6, 0);
    lv_obj_set_flex_flow(mode_chat_empty_card, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(mode_chat_empty_card, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag(mode_chat_empty_card, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_t* ce_icon = lv_label_create(mode_chat_empty_card);
    lv_label_set_text(ce_icon, "🧄🦞");
    lv_obj_set_style_text_font(ce_icon, CJK_FONT, 0);
    lv_obj_t* ce_text = lv_label_create(mode_chat_empty_card);
    lv_label_set_text(ce_text, "龙虾要吃蒜蓉的 🧄🦞 — 你的AI助手已就位！");
    lv_obj_set_style_text_color(ce_text, c->text_dim, 0);
    lv_obj_set_style_text_font(ce_text, CJK_FONT_SMALL, 0);
    lv_obj_t* ce_suggest_row = lv_obj_create(mode_chat_empty_card);
    lv_obj_set_width(ce_suggest_row, LV_PCT(100));
    lv_obj_set_height(ce_suggest_row, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(ce_suggest_row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(ce_suggest_row, 0, 0);
    lv_obj_set_style_pad_all(ce_suggest_row, 0, 0);
    lv_obj_set_style_pad_gap(ce_suggest_row, 8, 0);
    lv_obj_set_flex_flow(ce_suggest_row, LV_FLEX_FLOW_ROW_WRAP);
    lv_obj_set_flex_align(ce_suggest_row, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag(ce_suggest_row, LV_OBJ_FLAG_SCROLLABLE);
    auto make_chat_suggestion = [&](const char* title, const char* prompt) {
        lv_obj_t* btn = lv_button_create(ce_suggest_row);
        lv_obj_set_size(btn, SCALE(220), SCALE(30));
        lv_obj_set_style_radius(btn, SCALE(g_colors->radius_lg), 0);
        lv_obj_set_style_bg_color(btn, c->surface, 0);
        lv_obj_set_style_border_width(btn, 1, 0);
        lv_obj_set_style_border_color(btn, c->border, 0);
        lv_obj_set_style_pad_hor(btn, 10, 0);
        lv_obj_clear_flag(btn, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_t* lbl = lv_label_create(btn);
        lv_label_set_text(lbl, title);
        lv_obj_set_style_text_color(lbl, c->text, 0);
        lv_obj_set_style_text_font(lbl, CJK_FONT_SMALL, 0);
        lv_obj_center(lbl);
        lv_obj_add_event_cb(btn, chat_empty_suggestion_cb, LV_EVENT_CLICKED, (void*)prompt);
    };
    static const char* k_chat_suggest1 = "帮我整理一下当前工作区的文件结构";
    static const char* k_chat_suggest2 = "今天我需要做什么，请给我优先级清单";
    make_chat_suggestion("💡 试试：帮我整理一下文件", k_chat_suggest1);
    make_chat_suggestion("💡 试试：今天有什么安排", k_chat_suggest2);
    lv_obj_add_flag(mode_chat_empty_card, LV_OBJ_FLAG_HIDDEN);

    /* By default start fresh each launch; optional restore can be enabled by config. */
    if (g_restore_last_session) {
        load_chat_history();
    } else {
        g_chat_messages.clear();
        chat_history[0] = '\0';
    }

    /* Show welcome message only if no history was restored */
    if (g_chat_messages.empty()) {
        static const I18n S_WELCOME = {
            "Hi! I'm AnyClaw, your AI assistant.",
            "嗨！我是 AnyClaw，你的 AI 助手。"
        };
        const char* welcome = tr(S_WELCOME);
        snprintf(chat_history, sizeof(chat_history), "%s\n", welcome);

        lv_obj_t* sys_row = lv_obj_create(chat_cont);
        lv_obj_set_width(sys_row, LV_PCT(100));
        lv_obj_set_height(sys_row, LV_SIZE_CONTENT);
        lv_obj_set_style_max_width(sys_row, LV_PCT(85), 0);
        lv_obj_set_style_bg_opa(sys_row, LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_width(sys_row, 0, 0);
        lv_obj_set_style_pad_all(sys_row, 0, 0);
        lv_obj_set_style_radius(sys_row, 0, 0);
        lv_obj_clear_flag(sys_row, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_flex_flow(sys_row, LV_FLEX_FLOW_ROW);
        lv_obj_set_flex_align(sys_row, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_START);
        lv_obj_set_style_pad_gap(sys_row, 8, 0);

        /* Avatar - lobster icon matching AI messages */
        lv_obj_t* sys_avatar = lv_img_create(sys_row);
        lv_img_set_src(sys_avatar, profile_ai_avatar_src());
        lv_obj_set_size(sys_avatar, SCALE(AVATAR_AI_SIZE), SCALE(AVATAR_AI_SIZE));

        /* Welcome text */
        lv_obj_t* sys_lbl = lv_label_create(sys_row);
        lv_label_set_text(sys_lbl, welcome);
        lv_obj_set_style_text_color(sys_lbl, c->text_dim, 0);
        lv_obj_set_style_text_font(sys_lbl, CJK_FONT, 0);
    }

    /* Scroll chat to show welcome message at bottom */
    chat_force_scroll_bottom();
    update_chat_empty_state();

    /* No fake test messages — real chat via OpenRouter API */

    /* chat_display no longer needed - bubbles are added directly to chat_cont */
    chat_display = nullptr;

    /* ═══ Search Bar (SF-01: hidden by default) ═══ */
    {
        int search_h = SCALE(36);
        g_search_bar = lv_obj_create(mode_panel_chat);
        lv_obj_set_size(g_search_bar, content_w - CHAT_GAP * 2, search_h);
        lv_obj_set_pos(g_search_bar, CHAT_GAP, content_h - search_h - GAP);
        lv_obj_set_style_bg_color(g_search_bar, c->surface, 0);
        lv_obj_set_style_border_width(g_search_bar, 1, 0);
        lv_obj_set_style_border_color(g_search_bar, c->btn_action, 0);
        lv_obj_set_style_radius(g_search_bar, SCALE(g_colors->radius_md), 0);
        lv_obj_set_style_pad_all(g_search_bar, 4, 0);
        lv_obj_set_flex_flow(g_search_bar, LV_FLEX_FLOW_ROW);
        lv_obj_set_flex_align(g_search_bar, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        lv_obj_set_style_pad_gap(g_search_bar, 4, 0);
        lv_obj_clear_flag(g_search_bar, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_add_flag(g_search_bar, LV_OBJ_FLAG_HIDDEN);  /* Hidden initially */

        /* Search input */
        g_search_input = lv_textarea_create(g_search_bar);
        lv_obj_set_flex_grow(g_search_input, 1);
        lv_obj_set_height(g_search_input, LV_SIZE_CONTENT);
        lv_textarea_set_one_line(g_search_input, true);
        lv_textarea_set_placeholder_text(g_search_input, tr({"Search messages...", "搜索消息..."}));
        lv_obj_set_style_bg_color(g_search_input, c->bg, 0);
        lv_obj_set_style_text_color(g_search_input, c->text, 0);
        lv_obj_set_style_text_font(g_search_input, CJK_FONT_SMALL, 0);
        lv_obj_set_style_border_width(g_search_input, 0, 0);
        lv_obj_set_style_radius(g_search_input, SCALE(g_colors->radius_sm), 0);
        lv_obj_add_event_cb(g_search_input, search_execute_cb, LV_EVENT_VALUE_CHANGED, nullptr);

        /* Previous button */
        lv_obj_t* btn_prev = lv_button_create(g_search_bar);
        lv_obj_set_size(btn_prev, SCALE(28), SCALE(26));
        lv_obj_set_style_bg_color(btn_prev, c->btn_secondary, 0);
        lv_obj_set_style_radius(btn_prev, SCALE(g_colors->radius_sm), 0);
        lv_obj_set_style_border_width(btn_prev, 0, 0);
        lv_obj_clear_flag(btn_prev, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_t* prev_lbl = lv_label_create(btn_prev);
        lv_label_set_text(prev_lbl, LV_SYMBOL_UP);
        lv_obj_set_style_text_color(prev_lbl, c->text, 0);
        lv_obj_center(prev_lbl);
        lv_obj_add_event_cb(btn_prev, search_prev_cb, LV_EVENT_CLICKED, nullptr);

        /* Next button */
        lv_obj_t* btn_next = lv_button_create(g_search_bar);
        lv_obj_set_size(btn_next, SCALE(28), SCALE(26));
        lv_obj_set_style_bg_color(btn_next, c->btn_secondary, 0);
        lv_obj_set_style_radius(btn_next, SCALE(g_colors->radius_sm), 0);
        lv_obj_set_style_border_width(btn_next, 0, 0);
        lv_obj_clear_flag(btn_next, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_t* next_lbl = lv_label_create(btn_next);
        lv_label_set_text(next_lbl, LV_SYMBOL_DOWN);
        lv_obj_set_style_text_color(next_lbl, c->text, 0);
        lv_obj_center(next_lbl);
        lv_obj_add_event_cb(btn_next, search_next_cb, LV_EVENT_CLICKED, nullptr);

        /* Results count label */
        g_search_info = lv_label_create(g_search_bar);
        lv_label_set_text(g_search_info, "");
        lv_obj_set_style_text_color(g_search_info, c->text_dim, 0);
        lv_obj_set_style_text_font(g_search_info, CJK_FONT_SMALL, 0);
    }

    /* ═══ Search toggle button (magnifying glass icon) ═══ */
    {
        int search_btn_size = SCALE(20);
        g_search_btn = lv_button_create(mode_panel_chat);
        lv_obj_set_size(g_search_btn, search_btn_size, search_btn_size);
        lv_obj_set_style_bg_color(g_search_btn, c->btn_secondary, 0);
        lv_obj_set_style_bg_opa(g_search_btn, LV_OPA_COVER, 0);
        lv_obj_set_style_radius(g_search_btn, search_btn_size / 2, 0);
        lv_obj_set_style_border_width(g_search_btn, 0, 0);
        lv_obj_clear_flag(g_search_btn, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_clear_flag(g_search_btn, LV_OBJ_FLAG_CLICK_FOCUSABLE);
        lv_obj_t* search_lbl = lv_label_create(g_search_btn);
        lv_label_set_text(search_lbl, LV_SYMBOL_EDIT);
        lv_obj_set_style_text_color(search_lbl, c->text, 0);
        lv_obj_center(search_lbl);
        lv_obj_add_event_cb(g_search_btn, search_toggle_cb, LV_EVENT_CLICKED, nullptr);
    }

    /* Chat input area — pinned to bottom with GAP spacing */
    input_h = 108;  /* Default 3-line height (28px/line + padding) */
    int input_y = content_h - input_h - GAP;

    /* Adjust for search bar if visible */
    if (g_search_visible && g_search_bar && !lv_obj_has_flag(g_search_bar, LV_OBJ_FLAG_HIDDEN)) {
        int search_h = SCALE(36) + GAP;
        input_y -= search_h;
        /* Reposition search bar above input */
        lv_obj_set_pos(g_search_bar, CHAT_GAP, content_h - SCALE(36) - GAP - search_h + SCALE(36));
    }

    /* Fix chat_cont height: recalc with actual input height */
    int real_chat_h = content_h - GAP - input_h - GAP;
    if (real_chat_h > 0) lv_obj_set_height(chat_cont, real_chat_h);

    chat_input = lv_textarea_create(mode_panel_chat);
    lv_obj_set_size(chat_input, content_w - CHAT_GAP * 2, input_h);
    lv_obj_set_pos(chat_input, CHAT_GAP, input_y);
    lv_textarea_set_placeholder_text(chat_input, "输入消息... (Shift+Enter 换行, Ctrl+Enter 任务)");
    lv_textarea_set_one_line(chat_input, false);
    lv_textarea_set_max_length(chat_input, 2000);
    lv_textarea_set_text_selection(chat_input, true);  /* Enable mouse text selection */
    lv_obj_clear_flag(chat_input, LV_OBJ_FLAG_SCROLLABLE);  /* No scrollbar in input */
    lv_obj_set_scrollbar_mode(chat_input, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_style_bg_color(chat_input, c->surface, 0);
    lv_obj_set_style_text_color(chat_input, c->text, 0);
    lv_obj_set_style_text_font(chat_input, CJK_FONT_CHAT, 0);
    lv_obj_set_style_border_color(chat_input, c->border, 0);
    lv_obj_set_style_border_width(chat_input, 1, 0);
    lv_obj_set_style_radius(chat_input, SCALE(g_colors->radius_lg), 0);
    /* Match chat_cont internal padding + extra vertical for line height */
    lv_obj_set_style_pad_all(chat_input, 12, 0);
    lv_obj_set_style_pad_ver(chat_input, 10, 0);
    /* Focus: keep same border as normal state (no blue flash) */
    lv_obj_set_style_border_color(chat_input, c->accent, LV_STATE_FOCUSED);
    /* Selection highlight: light blue */
    lv_obj_set_style_text_color(chat_input, c->text, LV_PART_SELECTED);
    lv_obj_set_style_border_width(chat_input, 2, LV_STATE_FOCUSED);
    lv_obj_set_style_outline_width(chat_input, 0, LV_STATE_FOCUSED);
    lv_obj_add_event_cb(chat_input, chat_input_cb, LV_EVENT_READY, nullptr);
    lv_obj_add_event_cb(chat_input, chat_input_cb, LV_EVENT_KEY, nullptr);
    /* Auto-resize on text change */
    lv_obj_add_event_cb(chat_input, chat_input_resize_cb, LV_EVENT_VALUE_CHANGED, nullptr);
    lv_obj_add_event_cb(chat_input, chat_input_resize_cb, LV_EVENT_INSERT, nullptr);
    /* Note: Ctrl+C/V/X/A handled in main.cpp SDL event loop (LVGL driver drops Ctrl+key) */
    lv_obj_add_event_cb(chat_input, chat_input_right_click_cb, LV_EVENT_LONG_PRESSED, nullptr);  /* Right-click menu */
    lv_group_add_obj(lv_group_get_default(), chat_input);

    mode_lbl_chat_status = lv_label_create(mode_panel_chat);
    lv_obj_set_style_text_font(mode_lbl_chat_status, CJK_FONT_SMALL, 0);
    lv_obj_set_pos(mode_lbl_chat_status, CHAT_GAP, input_y - SCALE(16));
    update_chat_status_label("Ready", false);

    /* ═══ Send button (CI-01-2: 内嵌发送按钮) ═══ */
    {
        int btn_size = SCALE(CHAT_ACTION_BTN_BASE);
        int btn_margin = SCALE(CHAT_ACTION_BTN_MARGIN);
        int btn_gap = SCALE(CHAT_ACTION_BTN_GAP);
        int btn_x = content_w - CHAT_GAP - btn_size - btn_margin;
        int btn_y = input_y + input_h - btn_size - btn_margin;
        btn_send_widget = lv_button_create(mode_panel_chat);
        lv_obj_set_size(btn_send_widget, btn_size, btn_size);
        lv_obj_set_pos(btn_send_widget, btn_x, btn_y);
        lv_obj_set_style_radius(btn_send_widget, btn_size / 2, 0);
        lv_obj_set_style_bg_color(btn_send_widget, c->btn_action, 0);
        apply_btn_gradient(btn_send_widget);
        lv_obj_set_style_bg_opa(btn_send_widget, LV_OPA_COVER, 0);
        lv_obj_set_style_border_width(btn_send_widget, 0, 0);
        lv_obj_clear_flag(btn_send_widget, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_clear_flag(btn_send_widget, LV_OBJ_FLAG_CLICK_FOCUSABLE);
        lv_obj_add_event_cb(btn_send_widget, chat_send_btn_cb, LV_EVENT_CLICKED, nullptr);
        /* Prevent button press from propagating to panel */
        lv_obj_add_event_cb(btn_send_widget, [](lv_event_t* e){ lv_event_stop_processing(e); }, LV_EVENT_PRESSED, nullptr);

        lv_obj_t* send_lbl = lv_label_create(btn_send_widget);
        lv_label_set_text(send_lbl, LV_SYMBOL_RIGHT);
        lv_obj_set_style_text_color(send_lbl, c->text_inverse, 0);
        lv_obj_center(send_lbl);

        /* Restore upload button: left of send button */
        btn_upload_widget = lv_button_create(mode_panel_chat);
        lv_obj_set_size(btn_upload_widget, btn_size, btn_size);
        lv_obj_set_pos(btn_upload_widget, btn_x - btn_size - btn_gap, btn_y);
        lv_obj_set_style_radius(btn_upload_widget, btn_size / 2, 0);
        lv_obj_set_style_bg_color(btn_upload_widget, c->surface, 0);
        lv_obj_set_style_bg_opa(btn_upload_widget, LV_OPA_COVER, 0);
        lv_obj_set_style_border_width(btn_upload_widget, 1, 0);
        lv_obj_set_style_border_color(btn_upload_widget, c->border, 0);
        lv_obj_clear_flag(btn_upload_widget, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_clear_flag(btn_upload_widget, LV_OBJ_FLAG_CLICK_FOCUSABLE);
        lv_obj_add_event_cb(btn_upload_widget, upload_file_click_cb, LV_EVENT_CLICKED, nullptr);
        lv_obj_add_event_cb(btn_upload_widget, [](lv_event_t* e){ lv_event_stop_processing(e); }, LV_EVENT_PRESSED, nullptr);
        lv_obj_t* upload_lbl = lv_label_create(btn_upload_widget);
        lv_label_set_text(upload_lbl, LV_SYMBOL_UPLOAD);
        lv_obj_set_style_text_color(upload_lbl, c->text_inverse, 0);
        lv_obj_center(upload_lbl);

        btn_voice_widget = lv_button_create(mode_panel_chat);
        lv_obj_set_size(btn_voice_widget, btn_size, btn_size);
        lv_obj_set_pos(btn_voice_widget, btn_x - (btn_size + btn_gap) * 2, btn_y);
        lv_obj_set_style_radius(btn_voice_widget, btn_size / 2, 0);
        lv_obj_set_style_bg_color(btn_voice_widget, c->surface, 0);
        lv_obj_set_style_bg_opa(btn_voice_widget, LV_OPA_COVER, 0);
        lv_obj_set_style_border_width(btn_voice_widget, 1, 0);
        lv_obj_set_style_border_color(btn_voice_widget, c->border, 0);
        lv_obj_clear_flag(btn_voice_widget, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_clear_flag(btn_voice_widget, LV_OBJ_FLAG_CLICK_FOCUSABLE);
        lv_obj_add_event_cb(btn_voice_widget, mode_voice_cb, LV_EVENT_CLICKED, nullptr);
        lv_obj_add_event_cb(btn_voice_widget, [](lv_event_t* e){ lv_event_stop_processing(e); }, LV_EVENT_PRESSED, nullptr);
        lv_obj_t* voice_lbl = lv_label_create(btn_voice_widget);
        lv_label_set_text(voice_lbl, "V");
        lv_obj_set_style_text_color(voice_lbl, c->text_inverse, 0);
        lv_obj_center(voice_lbl);

        btn_work_widget = lv_button_create(mode_panel_chat);
        lv_obj_set_size(btn_work_widget, btn_size, btn_size);
        lv_obj_set_pos(btn_work_widget, btn_x - (btn_size + btn_gap) * 3, btn_y);
        lv_obj_set_style_radius(btn_work_widget, btn_size / 2, 0);
        lv_obj_set_style_bg_color(btn_work_widget, c->surface, 0);
        lv_obj_set_style_bg_opa(btn_work_widget, LV_OPA_COVER, 0);
        lv_obj_set_style_border_width(btn_work_widget, 1, 0);
        lv_obj_set_style_border_color(btn_work_widget, c->border, 0);
        lv_obj_clear_flag(btn_work_widget, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_clear_flag(btn_work_widget, LV_OBJ_FLAG_CLICK_FOCUSABLE);
        lv_obj_add_event_cb(btn_work_widget, chat_send_to_work_cb, LV_EVENT_CLICKED, nullptr);
        lv_obj_add_event_cb(btn_work_widget, [](lv_event_t* e){ lv_event_stop_processing(e); }, LV_EVENT_PRESSED, nullptr);
        lv_obj_t* work_lbl = lv_label_create(btn_work_widget);
        lv_label_set_text(work_lbl, "W");
        lv_obj_set_style_text_color(work_lbl, c->text_inverse, 0);
        lv_obj_center(work_lbl);

        /* Track input text changes to update button state */
        lv_obj_add_event_cb(chat_input, chat_input_value_changed_for_btn_cb, LV_EVENT_VALUE_CHANGED, nullptr);
        update_send_button_state();
    }

    /* ═══ FOOTER (minimal) ═══ */
    {
        char footer_text[128];
        snprintf(footer_text, sizeof(footer_text), "AnyClaw v2.2.1 | LVGL 9.6 + SDL2");
        lv_obj_t* footer = lv_label_create(scr);
        lv_label_set_text(footer, footer_text);
        lv_obj_set_style_text_color(footer, c->text_dim, 0);
        lv_obj_set_style_text_font(footer, CJK_FONT_SMALL, 0);
        lv_obj_align(footer, LV_ALIGN_BOTTOM_MID, 0, -8);
    }

    /* Settings panel is lazily initialized on first open to avoid
       startup-time UI stalls caused by building all tabs up front. */

    /* Initial refresh - populates task list */
    run_proactive_startup_once();
    lv_timer_create([](lv_timer_t* t) {
        app_refresh_status();
        lv_timer_del(t);
    }, 220, nullptr);
    sync_ai_mode_dropdowns();
    apply_mode_switch_visuals();

    /* P2-27: Auto-refresh timer (configurable: 15s / 30s / 60s) */
    g_refresh_timer = lv_timer_create(auto_refresh_cb, g_refresh_interval_ms, nullptr);
    if (!g_ui_log_flush_timer) g_ui_log_flush_timer = lv_timer_create(ui_log_flush_timer_cb, 120, nullptr);

    ui_log("[Ready] AnyClaw LVGL v2.2.1 - Bilingual mode");
    ui_log("[Task] Task list initialized");

    /* ═══ Create title bar LAST - ensures it's above all panels ═══ */
    create_title_bar(scr);
    update_main_title();
    apply_nav_module_visuals();

    /* Show loading overlay while OpenClaw is starting */
    loading_show();

    /* Show legal disclaimer on first launch */
    show_disclaimer(scr);

    /* Show startup error modal if any (e.g. "Already Running") */
    show_startup_error(scr);

    /* ═══ Setup wizard gate: async env check to avoid startup blocking ═══ */
    start_wizard_gate_async_check();
    lv_timer_create([](lv_timer_t* t) {
        if (!g_wizard_gate_ready.load()) return;
        EnvCheckResult wiz_env{};
        {
            std::lock_guard<std::mutex> lk(g_wizard_gate_mtx);
            wiz_env = g_wizard_gate_env;
        }
        bool wizard_needed = !is_wizard_completed()
            || !wiz_env.node_ok
            || !wiz_env.node_version_ok
            || !wiz_env.npm_ok
            || !wiz_env.openclaw_ok;
        if (wizard_needed) {
            ui_log("[Wizard] Auto-open setup wizard (node=%d node_ver=%d npm=%d oc=%d completed=%d)",
                   wiz_env.node_ok ? 1 : 0,
                   wiz_env.node_version_ok ? 1 : 0,
                   wiz_env.npm_ok ? 1 : 0,
                   wiz_env.openclaw_ok ? 1 : 0,
                   is_wizard_completed() ? 1 : 0);
            ui_show_setup_wizard();
        }
        lv_timer_del(t);
    }, 200, nullptr);

    /* Create user_data.txt marker for uninstaller */
    {
        const char* appdata = std::getenv("APPDATA");
        if (appdata) {
            std::string marker = std::string(appdata) + "\\AnyClaw_LVGL\\user_data.txt";
            std::ofstream f(marker);
            if (f.is_open()) {
                f << "AnyClaw LVGL User Data Marker\n";
                f << "Keep this file to preserve user data during uninstall.\n";
                f.close();
            }
        }
    }

    /* Keep dropdown/item interactions stable: avoid global label-select hook here. */

    /* ═══ Test mode (disabled: using direct injection above) ═══ */
    /* lv_timer_create(test_inject_messages_cb, 500, nullptr); */
    g_ui_ready = true;
}

