/*
 * A kid's thermometer: a tube on a fixed 0-100 °F scale, so "halfway up" always
 * means the same thing, with faint color zones and a fill whose color says how it
 * feels (purple freezing ... red hot). Pure LVGL, its own file so it can move to
 * other layouts.
 */
#pragma once

#include <stdint.h>

#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

#define THERMO_MIN_F 0.0f
#define THERMO_MAX_F 100.0f

typedef struct {
    lv_obj_t *root;
    lv_obj_t *tube, *fill, *bulb, *glass, *bulb_glass;
    int tube_h;
} thermo_t;

/* x, y: top-left of the whole thing; h: total height including the bulb. Width is 40 px. */
void thermo_create(thermo_t *t, lv_obj_t *parent, int x, int y, int h);
void thermo_set(thermo_t *t, float temp_f);
/* Outline color, so it reads on light and dark (night) backgrounds. */
void thermo_set_outline(thermo_t *t, lv_color_t c);

/* The fill color for a temperature (same cut-offs as the clothing advice). */
uint32_t thermo_color(float temp_f);

#ifdef __cplusplus
}
#endif
