/* ═══════════════════════════════════════════════════════════════
 *  anim.h - Animation utilities (v2.2.1)
 *  AnyClaw LVGL — Theme-aware animation helpers
 * ═══════════════════════════════════════════════════════════════ */
#ifndef ANYCLAW_ANIM_H
#define ANYCLAW_ANIM_H

#include "lvgl.h"

/* ── Duration tokens (Design §8.2) ── */
#define DUR_INSTANT    100   /* Micro-interactions (hover) */
#define DUR_FAST       150   /* Button feedback, checkbox */
#define DUR_NORMAL     200   /* Most transitions */
#define DUR_SLOW       300   /* Dialogs, panels, theme switch */
#define DUR_VERY_SLOW  500   /* Wizard steps */

/* ── Easing curve presets ── */
typedef enum {
    ANIM_EASE_OUT,      /* Element enter:  cubic-bezier(0, 0, 0.2, 1) */
    ANIM_EASE_IN,       /* Element exit:   cubic-bezier(0.4, 0, 1, 1) */
    ANIM_EASE_IN_OUT,   /* Position/size:  cubic-bezier(0.4, 0, 0.2, 1) */
    ANIM_SPRING,        /* Elastic bounce */
    ANIM_BOUNCE,        /* Peachy-only extra bounce */
    ANIM_LINEAR,        /* Constant speed */
} AnimEase;

/* ── Theme animation config ── */
struct AnimConfig {
    uint16_t  dur_instant;      /* ms */
    uint16_t  dur_fast;
    uint16_t  dur_normal;
    uint16_t  dur_slow;
    uint16_t  dur_very_slow;
    uint8_t   bounce_enabled;   /* 1 = use bounce curve (Peachy) */
};

extern const AnimConfig* g_anim;

void anim_init();

/* ── Path getter (maps AnimEase → lv_anim_path callback) ── */
lv_anim_path_cb_t anim_get_path(AnimEase ease);

/* ── Common animation helpers ── */

/* Fade in: opacity 0 → LV_OPA_COVER */
void anim_fade_in(lv_obj_t* obj, uint16_t duration, AnimEase ease);

/* Fade out: opacity current → 0 */
void anim_fade_out(lv_obj_t* obj, uint16_t duration, AnimEase ease);

/* Scale appear: 95% → 100% + fade in (popup/dialog entrance) */
void anim_scale_in(lv_obj_t* obj, uint16_t duration, AnimEase ease);

/* Scale disappear: 100% → 95% + fade out (popup/dialog exit) */
void anim_scale_out(lv_obj_t* obj, uint16_t duration, AnimEase ease);

/* Slide up + fade (message appear) */
void anim_slide_fade_in(lv_obj_t* obj, int offset_y, uint16_t duration, AnimEase ease);

/* Button press: scale(0.97) */
void anim_btn_press(lv_obj_t* obj);

/* Button release: scale(1.0) spring */
void anim_btn_release(lv_obj_t* obj);

/* Opacity pulse (typing cursor, loading dots) */
void anim_pulse_opa(lv_obj_t* obj, uint16_t period_ms);

/* Smooth height transition */
void anim_height(lv_obj_t* obj, int from_h, int to_h, uint16_t duration, AnimEase ease);

/* Smooth width transition */
void anim_width(lv_obj_t* obj, int from_w, int to_w, uint16_t duration, AnimEase ease);

/* Scroll to Y with animation */
void anim_scroll_to(lv_obj_t* obj, int32_t y);

/* Cross-fade between two objects */
void anim_crossfade(lv_obj_t* from, lv_obj_t* to, uint16_t duration);

/* Theme switch: global color transition (300ms) */
void anim_theme_transition();

/* ── Theme-specific effects (Design §8.5) ── */

/* Mint breathing glow: accent box-shadow pulsing, 3s cycle (Matcha)
   Creates a subtle glow animation on a container's shadow.
   Pass NULL to stop. */
void anim_mint_glow(lv_obj_t* obj);

/* Particle burst: N small dots scatter from center + fade out (Matcha: green, Peachy: warm)
   parent: container for particles
   cx, cy: center point (relative to parent)
   color: particle color
   count: number of particles (6-12 recommended) */
void anim_particles_burst(lv_obj_t* parent, int cx, int cy, lv_color_t color, int count);

/* Code block highlight: monospace bg color fade-in (all themes)
   code_obj: the code block container
   accent: highlight bg color */
void anim_code_highlight(lv_obj_t* code_obj, lv_color_t accent);

/* ── Reduced motion (§8.6) ── */
void anim_set_reduced_motion(bool enabled);
bool anim_is_reduced_motion();

#endif /* ANYCLAW_ANIM_H */
