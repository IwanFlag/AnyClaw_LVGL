/* ═══════════════════════════════════════════════════════════════
 *  anim.cpp - Animation utilities implementation
 *  AnyClaw LVGL — Theme-aware animation helpers
 * ═══════════════════════════════════════════════════════════════ */
#include "anim.h"
#include "theme.h"
#include "app_config.h"

/* ── Theme-specific animation configs ── */
static const AnimConfig ANIM_MATCHA = {
    DUR_INSTANT, DUR_FAST, DUR_NORMAL, DUR_SLOW, DUR_VERY_SLOW,
    0  /* no extra bounce */
};

static const AnimConfig ANIM_PEACHY = {
    80, 120, 180, 280, 450,   /* slightly faster, snappier */
    1  /* bounce enabled */
};

static const AnimConfig ANIM_MOCHI = {
    120, 180, 250, 350, 600,  /* slower, quieter */
    0  /* no bounce */
};

static const AnimConfig ANIM_CLASSIC = {
    DUR_INSTANT, DUR_FAST, DUR_NORMAL, DUR_SLOW, DUR_VERY_SLOW,
    0
};

static const AnimConfig ANIM_LIGHT = {
    DUR_INSTANT, DUR_FAST, DUR_NORMAL, DUR_SLOW, DUR_VERY_SLOW,
    0
};

const AnimConfig* g_anim = &ANIM_MATCHA;

void anim_init() {
    switch (g_theme) {
        case Theme::Peachy:  g_anim = &ANIM_PEACHY;  break;
        case Theme::Mochi:   g_anim = &ANIM_MOCHI;   break;
        case Theme::Classic: g_anim = &ANIM_CLASSIC;  break;
        case Theme::Light:   g_anim = &ANIM_LIGHT;    break;
        default:             g_anim = &ANIM_MATCHA;   break;
    }
}

/* ── Path lookup ── */
lv_anim_path_cb_t anim_get_path(AnimEase ease) {
    switch (ease) {
        case ANIM_EASE_OUT:    return &lv_anim_path_ease_out;
        case ANIM_EASE_IN:     return &lv_anim_path_ease_in;
        case ANIM_EASE_IN_OUT: return &lv_anim_path_ease_in_out;
        case ANIM_SPRING:      return &lv_anim_path_overshoot;
        case ANIM_BOUNCE:      return g_anim->bounce_enabled
                                    ? &lv_anim_path_bounce
                                    : &lv_anim_path_overshoot;
        case ANIM_LINEAR:
        default:               return &lv_anim_path_linear;
    }
}

/* ── Callbacks ── */
static void set_opa_cb(void* obj, int32_t v) {
    lv_obj_set_style_opa((lv_obj_t*)obj, (lv_opa_t)v, 0);
}

static void set_scale_x_cb(void* obj, int32_t v) {
    lv_obj_set_style_transform_scale_x((lv_obj_t*)obj, (int16_t)v, 0);
}

static void set_scale_y_cb(void* obj, int32_t v) {
    lv_obj_set_style_transform_scale_y((lv_obj_t*)obj, (int16_t)v, 0);
}

static void set_height_cb(void* obj, int32_t v) {
    lv_obj_set_height((lv_obj_t*)obj, (int32_t)v);
}

static void set_width_cb(void* obj, int32_t v) {
    lv_obj_set_width((lv_obj_t*)obj, (int32_t)v);
}

static void set_y_cb(void* obj, int32_t v) {
    lv_obj_set_style_translate_y((lv_obj_t*)obj, (int32_t)v, 0);
}

/* ── Helpers ── */

void anim_fade_in(lv_obj_t* obj, uint16_t duration, AnimEase ease) {
    if (!obj) return;
    lv_obj_set_style_opa(obj, LV_OPA_TRANSP, 0);
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, obj);
    lv_anim_set_values(&a, LV_OPA_TRANSP, LV_OPA_COVER);
    lv_anim_set_duration(&a, duration);
    lv_anim_set_path_cb(&a, anim_get_path(ease));
    lv_anim_set_exec_cb(&a, set_opa_cb);
    lv_anim_start(&a);
}

void anim_fade_out(lv_obj_t* obj, uint16_t duration, AnimEase ease) {
    if (!obj) return;
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, obj);
    lv_anim_set_values(&a, lv_obj_get_style_opa(obj, 0), LV_OPA_TRANSP);
    lv_anim_set_duration(&a, duration);
    lv_anim_set_path_cb(&a, anim_get_path(ease));
    lv_anim_set_exec_cb(&a, set_opa_cb);
    lv_anim_start(&a);
}

void anim_scale_in(lv_obj_t* obj, uint16_t duration, AnimEase ease) {
    if (!obj) return;
    lv_obj_set_style_opa(obj, LV_OPA_TRANSP, 0);
    lv_obj_set_style_transform_scale_x(obj, 488, 0);  /* ~95% of 512 */
    lv_obj_set_style_transform_scale_y(obj, 488, 0);
    lv_obj_set_style_transform_pivot_x(obj, lv_pct(50), 0);
    lv_obj_set_style_transform_pivot_y(obj, lv_pct(50), 0);

    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, obj);
    lv_anim_set_duration(&a, duration);
    lv_anim_set_path_cb(&a, anim_get_path(ease));

    lv_anim_set_values(&a, LV_OPA_TRANSP, LV_OPA_COVER);
    lv_anim_set_exec_cb(&a, set_opa_cb);
    lv_anim_start(&a);

    lv_anim_set_values(&a, 488, 512);
    lv_anim_set_exec_cb(&a, set_scale_x_cb);
    lv_anim_start(&a);

    lv_anim_set_exec_cb(&a, set_scale_y_cb);
    lv_anim_start(&a);
}

void anim_scale_out(lv_obj_t* obj, uint16_t duration, AnimEase ease) {
    if (!obj) return;
    lv_obj_set_style_transform_pivot_x(obj, lv_pct(50), 0);
    lv_obj_set_style_transform_pivot_y(obj, lv_pct(50), 0);

    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, obj);
    lv_anim_set_duration(&a, duration);
    lv_anim_set_path_cb(&a, anim_get_path(ease));

    lv_anim_set_values(&a, LV_OPA_COVER, LV_OPA_TRANSP);
    lv_anim_set_exec_cb(&a, set_opa_cb);
    lv_anim_start(&a);

    lv_anim_set_values(&a, 512, 488);
    lv_anim_set_exec_cb(&a, set_scale_x_cb);
    lv_anim_start(&a);

    lv_anim_set_exec_cb(&a, set_scale_y_cb);
    lv_anim_start(&a);
}

void anim_slide_fade_in(lv_obj_t* obj, int offset_y, uint16_t duration, AnimEase ease) {
    if (!obj) return;
    lv_obj_set_style_opa(obj, LV_OPA_TRANSP, 0);
    lv_obj_set_style_translate_y(obj, offset_y, 0);

    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, obj);
    lv_anim_set_duration(&a, duration);
    lv_anim_set_path_cb(&a, anim_get_path(ease));

    lv_anim_set_values(&a, LV_OPA_TRANSP, LV_OPA_COVER);
    lv_anim_set_exec_cb(&a, set_opa_cb);
    lv_anim_start(&a);

    lv_anim_set_values(&a, offset_y, 0);
    lv_anim_set_exec_cb(&a, set_y_cb);
    lv_anim_start(&a);
}

void anim_btn_press(lv_obj_t* obj) {
    if (!obj) return;
    lv_obj_set_style_transform_pivot_x(obj, lv_pct(50), 0);
    lv_obj_set_style_transform_pivot_y(obj, lv_pct(50), 0);

    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, obj);
    lv_anim_set_duration(&a, g_anim->dur_fast);
    lv_anim_set_path_cb(&a, &lv_anim_path_ease_in);

    lv_anim_set_values(&a, 512, 496);  /* 100% → 97% */
    lv_anim_set_exec_cb(&a, set_scale_x_cb);
    lv_anim_start(&a);

    lv_anim_set_exec_cb(&a, set_scale_y_cb);
    lv_anim_start(&a);
}

void anim_btn_release(lv_obj_t* obj) {
    if (!obj) return;

    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, obj);
    lv_anim_set_duration(&a, g_anim->dur_fast);
    lv_anim_set_path_cb(&a, anim_get_path(ANIM_SPRING));

    lv_anim_set_values(&a, lv_obj_get_style_transform_scale_x(obj, 0), 512);
    lv_anim_set_exec_cb(&a, set_scale_x_cb);
    lv_anim_start(&a);

    lv_anim_set_exec_cb(&a, set_scale_y_cb);
    lv_anim_start(&a);
}

void anim_pulse_opa(lv_obj_t* obj, uint16_t period_ms) {
    if (!obj) return;
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, obj);
    lv_anim_set_values(&a, LV_OPA_TRANSP, LV_OPA_COVER);
    lv_anim_set_duration(&a, period_ms / 2);
    lv_anim_set_playback_duration(&a, period_ms / 2);
    lv_anim_set_repeat_count(&a, LV_ANIM_REPEAT_INFINITE);
    lv_anim_set_path_cb(&a, &lv_anim_path_ease_in_out);
    lv_anim_set_exec_cb(&a, set_opa_cb);
    lv_anim_start(&a);
}

void anim_height(lv_obj_t* obj, int from_h, int to_h, uint16_t duration, AnimEase ease) {
    if (!obj) return;
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, obj);
    lv_anim_set_values(&a, from_h, to_h);
    lv_anim_set_duration(&a, duration);
    lv_anim_set_path_cb(&a, anim_get_path(ease));
    lv_anim_set_exec_cb(&a, set_height_cb);
    lv_anim_start(&a);
}

void anim_width(lv_obj_t* obj, int from_w, int to_w, uint16_t duration, AnimEase ease) {
    if (!obj) return;
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, obj);
    lv_anim_set_values(&a, from_w, to_w);
    lv_anim_set_duration(&a, duration);
    lv_anim_set_path_cb(&a, anim_get_path(ease));
    lv_anim_set_exec_cb(&a, set_width_cb);
    lv_anim_start(&a);
}

void anim_scroll_to(lv_obj_t* obj, int32_t y) {
    if (!obj) return;
    lv_obj_scroll_to_y(obj, y, LV_ANIM_ON);
}

void anim_crossfade(lv_obj_t* from, lv_obj_t* to, uint16_t duration) {
    if (from) anim_fade_out(from, duration, ANIM_EASE_IN);
    if (to)   anim_fade_in(to, duration, ANIM_EASE_OUT);
}

void anim_theme_transition() {
    /* Theme transition: re-apply theme to all widgets with animation.
       Since LVGL doesn't support color animation natively, we do a
       quick opacity flash on the screen to smooth the visual jump. */
    lv_obj_t* scr = lv_screen_active();
    if (!scr) return;
    anim_init(); /* Update animation config for new theme */
}
