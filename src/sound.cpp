/* ═══════════════════════════════════════════════════════════════
 *  sound.cpp - Sound system implementation
 *  AnyClaw LVGL — Theme-aware audio playback via SDL2
 * ═══════════════════════════════════════════════════════════════ */
#include "sound.h"
#include "theme.h"
#include "app_log.h"
#include <cstdio>
#include <cstring>

#ifdef __has_include
#  if __has_include(<SDL.h>)
#    include <SDL.h>
#    define SOUND_SDL 1
#  else
#    define SOUND_SDL 0
#  endif
#else
#  include <SDL.h>
#  define SOUND_SDL 1
#endif

/* ── State ── */
static bool g_sound_enabled = true;
static int  g_sound_volume  = 80;  /* 0-100 */

/* ── Sound file names (per event) ── */
static const char* const SND_NAMES[SND_COUNT] = {
    "msg_incoming.wav",
    "msg_sent.wav",
    "success.wav",
    "error.wav",
    "click.wav",
    "switch.wav",
    "popup.wav",
    "typing.wav",
};

#if SOUND_SDL

/* ── Loaded audio chunks ── */
static SDL_AudioSpec  g_specs[SND_COUNT];
static Uint8*         g_bufs[SND_COUNT];
static Uint32         g_lens[SND_COUNT];
static bool           g_loaded[SND_COUNT];

/* ── Audio device ── */
static SDL_AudioDeviceID g_dev = 0;

/* ── Theme directory name ── */
static const char* get_sound_theme_dir() {
    switch (g_theme) {
        case Theme::Peachy:  return "peachy";
        case Theme::Mochi:   return "mochi";
        default:             return "matcha";
    }
}

/* ── Load a single WAV file ── */
static bool load_sound(int idx, const char* base_dir) {
    char path[512];
    snprintf(path, sizeof(path), "%s/sounds/%s/%s",
             base_dir, get_sound_theme_dir(), SND_NAMES[idx]);

    if (SDL_LoadWAV(path, &g_specs[idx], &g_bufs[idx], &g_lens[idx])) {
        g_loaded[idx] = true;
        LOG_D("SOUND", "Loaded: %s", path);
        return true;
    }

    /* Fallback: try matcha theme */
    snprintf(path, sizeof(path), "%s/sounds/matcha/%s", base_dir, SND_NAMES[idx]);
    if (SDL_LoadWAV(path, &g_specs[idx], &g_bufs[idx], &g_lens[idx])) {
        g_loaded[idx] = true;
        LOG_D("SOUND", "Loaded fallback: %s", path);
        return true;
    }

    g_loaded[idx] = false;
    LOG_W("SOUND", "Not found: %s", SND_NAMES[idx]);
    return false;
}

/* ── Free a single sound ── */
static void free_sound(int idx) {
    if (g_loaded[idx]) {
        SDL_FreeWAV(g_bufs[idx]);
        g_bufs[idx] = nullptr;
        g_loaded[idx] = false;
    }
}

/* ── Audio callback (pushes data to device) ── */
struct PlayState {
    Uint8* data;
    Uint32 len;
    Uint32 pos;
};

static PlayState g_play[SND_COUNT];

static void audio_callback(void* userdata, Uint8* stream, int len) {
    (void)userdata;
    SDL_memset(stream, 0, len);
    for (int i = 0; i < SND_COUNT; i++) {
        if (!g_play[i].data || g_play[i].pos >= g_play[i].len) continue;
        Uint32 remain = g_play[i].len - g_play[i].pos;
        Uint32 mix_len = (Uint32)len < remain ? (Uint32)len : remain;
        float vol = g_sound_volume / 100.0f;
        /* Simple volume scaling + mix */
        for (Uint32 j = 0; j < mix_len; j++) {
            int16_t sample = (int16_t)(g_play[i].data[g_play[i].pos + j]);
            int mixed = (int)(stream[j]) + (int)(sample * vol);
            if (mixed > 127) mixed = 127;
            if (mixed < -128) mixed = -128;
            stream[j] = (Uint8)mixed;
        }
        g_play[i].pos += mix_len;
        if (g_play[i].pos >= g_play[i].len) {
            g_play[i].data = nullptr;
        }
    }
}

void sound_init() {
    if (SDL_WasInit(SDL_INIT_AUDIO) == 0) {
        if (SDL_InitSubSystem(SDL_INIT_AUDIO) < 0) {
            LOG_W("SOUND", "SDL audio init failed: %s", SDL_GetError());
            return;
        }
    }

    SDL_AudioSpec wanted;
    SDL_zero(wanted);
    wanted.freq = 44100;
    wanted.format = AUDIO_S16SYS;
    wanted.channels = 1;
    wanted.samples = 1024;
    wanted.callback = audio_callback;

    g_dev = SDL_OpenAudioDevice(nullptr, 0, &wanted, nullptr, 0);
    if (g_dev == 0) {
        LOG_W("SOUND", "SDL_OpenAudio failed: %s", SDL_GetError());
        return;
    }
    SDL_PauseAudioDevice(g_dev, 0);
    LOG_I("SOUND", "Audio device opened: dev=%u", g_dev);

    /* Try loading from exe-relative paths */
    const char* search_paths[] = { "assets", "..\\assets", "..\\..\\assets", nullptr };
    for (int p = 0; search_paths[p]; p++) {
        bool any = false;
        for (int i = 0; i < SND_COUNT; i++) {
            if (!g_loaded[i]) any |= load_sound(i, search_paths[p]);
        }
        if (any) break;
    }
}

void sound_cleanup() {
    if (g_dev) {
        SDL_CloseAudioDevice(g_dev);
        g_dev = 0;
    }
    for (int i = 0; i < SND_COUNT; i++) {
        free_sound(i);
    }
}

void sound_play(SoundId id) {
    if (!g_sound_enabled || id < 0 || id >= SND_COUNT) return;
    if (!g_loaded[id]) return;
    if (!g_dev) return;

    SDL_LockAudioDevice(g_dev);
    g_play[id].data = g_bufs[id];
    g_play[id].len  = g_lens[id];
    g_play[id].pos  = 0;
    SDL_UnlockAudioDevice(g_dev);
}

void sound_reload_theme() {
    if (!g_dev) return;
    SDL_PauseAudioDevice(g_dev, 1);

    /* Free old sounds */
    for (int i = 0; i < SND_COUNT; i++) {
        free_sound(i);
    }

    /* Reload from new theme */
    const char* search_paths[] = { "assets", "..\\assets", "..\\..\\assets", nullptr };
    for (int p = 0; search_paths[p]; p++) {
        bool any = false;
        for (int i = 0; i < SND_COUNT; i++) {
            if (!g_loaded[i]) any |= load_sound(i, search_paths[p]);
        }
        if (any) break;
    }

    SDL_PauseAudioDevice(g_dev, 0);
    LOG_I("SOUND", "Theme sounds reloaded: %s", get_sound_theme_dir());
}

#else /* No SDL */

void sound_init()          { LOG_W("SOUND", "SDL not available, sound disabled"); }
void sound_cleanup()       {}
void sound_play(SoundId)   {}
void sound_reload_theme()  {}

#endif /* SOUND_SDL */

/* ── Settings (always available) ── */
void sound_set_enabled(bool enabled) { g_sound_enabled = enabled; }
bool sound_is_enabled()              { return g_sound_enabled; }
void sound_set_volume(int v)         { g_sound_volume = v < 0 ? 0 : (v > 100 ? 100 : v); }
int  sound_get_volume()              { return g_sound_volume; }
