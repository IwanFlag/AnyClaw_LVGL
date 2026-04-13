/* ═══════════════════════════════════════════════════════════════
 *  sound.h - Sound system (v2.2.1)
 *  AnyClaw LVGL — Theme-aware audio playback
 * ═══════════════════════════════════════════════════════════════ */
#ifndef ANYCLAW_SOUND_H
#define ANYCLAW_SOUND_H

/* ── Sound event IDs ── */
typedef enum {
    SND_MSG_INCOMING,    /* New AI message arrived */
    SND_MSG_SENT,        /* User message sent */
    SND_SUCCESS,         /* Task completed, Ready */
    SND_ERROR,           /* Error occurred */
    SND_CLICK,           /* Button/UI click */
    SND_SWITCH,          /* Theme/mode switch */
    SND_POPUP,           /* Dialog/popup appear */
    SND_TYPING,          /* Typing indicator tick */
    SND_COUNT
} SoundId;

/* ── Init / Cleanup ── */
void sound_init();
void sound_cleanup();

/* ── Playback ── */
void sound_play(SoundId id);

/* ── Settings ── */
void sound_set_enabled(bool enabled);
bool sound_is_enabled();
void sound_set_volume(int volume_0_100);
int  sound_get_volume();

/* ── Reload on theme change ── */
void sound_reload_theme();

#endif /* ANYCLAW_SOUND_H */
