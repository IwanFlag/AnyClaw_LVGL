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
    lv_obj_t* scr = lv_screen_active();
    if (!scr) return;
    anim_init(); /* Update animation config for new theme */
}

/* ═══ Reduced motion state ═══ */
static bool g_reduced_motion = false;

void anim_set_reduced_motion(bool enabled) { g_reduced_motion = enabled; }
bool anim_is_reduced_motion()              { return g_reduced_motion; }

/* ═══ Theme-specific effects ═══ */

/* ── Mint breathing glow (Matcha) / Warm glow (Peachy) ── */

static void glow_shadow_opa_cb(void* obj, int32_t v) {
    lv_obj_set_style_shadow_opa((lv_obj_t*)obj, (lv_opa_t)v, 0);
}

static void glow_shadow_width_cb(void* obj, int32_t v) {
    lv_obj_set_style_shadow_width((lv_obj_t*)obj, (int16_t)v, 0);
    lv_obj_set_style_shadow_spread((lv_obj_t*)obj, (int16_t)(v / 3), 0);
}

void anim_mint_glow(lv_obj_t* obj) {
    if (!obj || g_reduced_motion) return;

    /* Set initial glow style */
    lv_obj_set_style_shadow_color(obj, g_colors->accent, 0);
    lv_obj_set_style_shadow_opa(obj, LV_OPA_20, 0);
    lv_obj_set_style_shadow_width(obj, SCALE(12), 0);
    lv_obj_set_style_shadow_spread(obj, SCALE(4), 0);
    lv_obj_set_style_shadow_ofs_x(obj, 0, 0);
    lv_obj_set_style_shadow_ofs_y(obj, 0, 0);

    /* Opacity pulse: 20% → 50% → 20%, 3s cycle */
    uint16_t period = (g_theme == Theme::Peachy) ? 2500 : 3000;
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, obj);
    lv_anim_set_values(&a, LV_OPA_20, LV_OPA_50);
    lv_anim_set_duration(&a, period / 2);
    lv_anim_set_playback_duration(&a, period / 2);
    lv_anim_set_repeat_count(&a, LV_ANIM_REPEAT_INFINITE);
    lv_anim_set_path_cb(&a, &lv_anim_path_ease_in_out);
    lv_anim_set_exec_cb(&a, glow_shadow_opa_cb);
    lv_anim_start(&a);

    /* Width pulse: 12 → 20 → 12 (subtle breathing) */
    lv_anim_set_values(&a, SCALE(12), SCALE(20));
    lv_anim_set_exec_cb(&a, glow_shadow_width_cb);
    lv_anim_start(&a);
}

/* ── Particle burst ── */

#define PARTICLE_SIZE   6    /* 6px dots per Design §8.5 */
#define PARTICLE_COUNT  12
#define PARTICLE_DIST   40   /* scatter distance */
#define PARTICLE_DUR    300  /* 300ms disappear per Design */

static struct {
    lv_obj_t* dot;
    lv_anim_t anim_x;
    lv_anim_t anim_y;
    lv_anim_t anim_opa;
} g_particles[PARTICLE_COUNT];

static void particle_set_x_cb(void* p, int32_t v) {
    lv_obj_t* dot = ((lv_obj_t**)p)[0];
    if (dot) lv_obj_set_style_translate_x(dot, v, 0);
}
static void particle_set_y_cb(void* p, int32_t v) {
    lv_obj_t* dot = ((lv_obj_t**)p)[0];
    if (dot) lv_obj_set_style_translate_y(dot, v, 0);
}
static void particle_set_opa_cb(void* p, int32_t v) {
    lv_obj_t* dot = ((lv_obj_t**)p)[0];
    if (dot) lv_obj_set_style_opa(dot, (lv_opa_t)v, 0);
}

static void particle_delete_cb(lv_anim_t* a) {
    lv_obj_t* dot = (lv_obj_t*)a->user_data;
    if (dot && lv_obj_is_valid(dot)) {
        lv_obj_del(dot);
    }
}

void anim_particles_burst(lv_obj_t* parent, int cx, int cy, lv_color_t color, int count) {
    if (!parent || g_reduced_motion) return;
    if (count > PARTICLE_COUNT) count = PARTICLE_COUNT;
    if (count < 1) count = 6;

    for (int i = 0; i < count; i++) {
        /* Create small circular dot */
        lv_obj_t* dot = lv_obj_create(parent);
        lv_obj_set_size(dot, SCALE(PARTICLE_SIZE), SCALE(PARTICLE_SIZE));
        lv_obj_set_style_radius(dot, LV_RADIUS_CIRCLE, 0);
        lv_obj_set_style_bg_color(dot, color, 0);
        lv_obj_set_style_bg_opa(dot, LV_OPA_COVER, 0);
        lv_obj_set_style_border_width(dot, 0, 0);
        lv_obj_set_style_outline_width(dot, 0, 0);
        lv_obj_set_style_shadow_width(dot, 0, 0);
        lv_obj_set_style_pad_all(dot, 0, 0);
        lv_obj_clear_flag(dot, LV_OBJ_FLAG_SCROLLABLE);
        /* Position at center */
        lv_obj_set_pos(dot, cx, cy);

        /* Calculate scatter direction: evenly distribute around circle */
        float angle = (float)i / count * 6.2832f; /* 2*PI */
        int dist = SCALE(PARTICLE_DIST + (i % 3) * 8); /* varied distance */
        int tx = (int)(lv_cos((int)(angle * 512 / 6.2832f)) * dist / 1024);
        int ty = (int)(lv_sin((int)(angle * 512 / 6.2832f)) * dist / 1024);

        /* Store dot pointer for callback */
        g_particles[i].dot = dot;

        /* Animate X */
        lv_anim_init(&g_particles[i].anim_x);
        lv_anim_set_var(&g_particles[i].anim_x, &g_particles[i].dot);
        lv_anim_set_values(&g_particles[i].anim_x, 0, tx);
        lv_anim_set_duration(&g_particles[i].anim_x, PARTICLE_DUR);
        lv_anim_set_path_cb(&g_particles[i].anim_x, &lv_anim_path_ease_out);
        lv_anim_set_exec_cb(&g_particles[i].anim_x, particle_set_x_cb);
        lv_anim_start(&g_particles[i].anim_x);

        /* Animate Y */
        lv_anim_init(&g_particles[i].anim_y);
        lv_anim_set_var(&g_particles[i].anim_y, &g_particles[i].dot);
        lv_anim_set_values(&g_particles[i].anim_y, 0, ty);
        lv_anim_set_duration(&g_particles[i].anim_y, PARTICLE_DUR);
        lv_anim_set_path_cb(&g_particles[i].anim_y, &lv_anim_path_ease_out);
        lv_anim_set_exec_cb(&g_particles[i].anim_y, particle_set_y_cb);
        lv_anim_start(&g_particles[i].anim_y);

        /* Animate opacity: COVER → 0 */
        lv_anim_init(&g_particles[i].anim_opa);
        lv_anim_set_var(&g_particles[i].anim_opa, &g_particles[i].dot);
        lv_anim_set_values(&g_particles[i].anim_opa, LV_OPA_COVER, LV_OPA_TRANSP);
        lv_anim_set_duration(&g_particles[i].anim_opa, PARTICLE_DUR);
        lv_anim_set_path_cb(&g_particles[i].anim_opa, &lv_anim_path_ease_in);
        lv_anim_set_exec_cb(&g_particles[i].anim_opa, particle_set_opa_cb);
        lv_anim_set_user_data(&g_particles[i].anim_opa, dot);
        lv_anim_set_deleted_cb(&g_particles[i].anim_opa, particle_delete_cb);
        lv_anim_start(&g_particles[i].anim_opa);
    }
}

/* ── Code highlight ── */

static void code_bg_opa_cb(void* obj, int32_t v) {
    lv_obj_set_style_bg_opa((lv_obj_t*)obj, (lv_opa_t)v, 0);
}

void anim_code_highlight(lv_obj_t* code_obj, lv_color_t accent) {
    if (!code_obj || g_reduced_motion) return;

    /* Set code block bg to accent with 8% opacity */
    lv_obj_set_style_bg_color(code_obj, accent, 0);
    lv_obj_set_style_bg_opa(code_obj, LV_OPA_TRANSP, 0);

    /* Fade in bg opacity: 0% → 8% (subtle tint) */
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, code_obj);
    lv_anim_set_values(&a, LV_OPA_TRANSP, (lv_opa_t)(255 * 8 / 100));
    lv_anim_set_duration(&a, DUR_SLOW);
    lv_anim_set_path_cb(&a, &lv_anim_path_ease_out);
    lv_anim_set_exec_cb(&a, code_bg_opa_cb);
    lv_anim_start(&a);
}
