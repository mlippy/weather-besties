/*
 * A kid's thermometer: a tube on a fixed 0-100 °F scale, so "halfway up" always
 * means the same thing, with faint color zones and a fill whose color says how it
 * feels (purple freezing ... red hot). A light-blue notch on the left marks freezing,
 * and an optional band on the right shows the day's low to high. Pure LVGL, its own file
 * so it can move to other layouts.
 */
#pragma once

#include <stdint.h>

#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

#define THERMO_MIN_F 0.0f
#define THERMO_MAX_F 100.0f
#define THERMO_FREEZE_F 32.0f   /* marked with a tick */

typedef struct {
    lv_obj_t *root;
    lv_obj_t *tube, *fill, *bulb, *glass, *bulb_glass, *freeze;
    lv_obj_t *range;   /* the day's low-to-high band beside the tube (hidden until set) */
    int w, tube_w, border, tube_h;
} thermo_t;

/* x, y: top-left of the whole thing; w: width (the bulb's diameter; 40 big, 24 mini);
 * h: total height including the bulb. */
void thermo_create(thermo_t *t, lv_obj_t *parent, int x, int y, int w, int h);
void thermo_set(thermo_t *t, float temp_f);
/* The day's range as a band just right of the tube, from lo_f up to hi_f; lo_f > hi_f hides it. */
void thermo_set_range(thermo_t *t, float lo_f, float hi_f);
/* Outline color, so it reads on light and dark (night) backgrounds. */
void thermo_set_outline(thermo_t *t, lv_color_t c);

/* The fill color for a temperature (same cut-offs as the clothing advice). */
uint32_t thermo_color(float temp_f);

#ifdef __cplusplus
}
#endif
