#include "thermo.h"

#define ZONE_OPA 70   /* the faint "where is cold, where is hot" stripes */

/* Upper edge (°F) and color of each zone. Same cut-offs as the clothing bands in advice.c,
 * so the color matches the advice: freezing, very cold, cold, chilly, cool, nice, warm, hot. */
static const struct {
    float max_f;
    uint32_t color;
} ZONES[] = {
    {15, 0x7B61C9},  /* purple */
    {28, 0x2F6BD8},  /* deep blue */
    {40, 0x3E9BE0},  /* blue */
    {54, 0x2BB3B1},  /* teal */
    {64, 0x4CB860},  /* green */
    {77, 0xF2C230},  /* yellow */
    {87, 0xF28C28},  /* orange */
    {999, 0xE5483C}, /* red */
};
#define NZONES ((int)(sizeof(ZONES) / sizeof(ZONES[0])))

uint32_t thermo_color(float f)
{
    for (int i = 0; i < NZONES; i++) {
        if (f <= ZONES[i].max_f) {
            return ZONES[i].color;
        }
    }
    return ZONES[NZONES - 1].color;
}

/* Tube coordinates: 0 at the top of the tube; the scale runs from just under the rounded
 * top down to where the tube meets the bulb. */
static int scale_top(const thermo_t *t)
{
    return t->tube_w / 2;
}

static int scale_bottom(const thermo_t *t)
{
    return t->tube_h - t->w / 2 - t->border - 1;
}

static int y_for(const thermo_t *t, float f)
{
    if (f < THERMO_MIN_F) {
        f = THERMO_MIN_F;
    } else if (f > THERMO_MAX_F) {
        f = THERMO_MAX_F;
    }
    float k = (f - THERMO_MIN_F) / (THERMO_MAX_F - THERMO_MIN_F);
    return scale_bottom(t) - (int)(k * (float)(scale_bottom(t) - scale_top(t)) + 0.5f);
}

static lv_obj_t *plain(lv_obj_t *parent)
{
    lv_obj_t *o = lv_obj_create(parent);
    lv_obj_remove_style_all(o);
    lv_obj_remove_flag(o, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);
    return o;
}

void thermo_create(thermo_t *t, lv_obj_t *parent, int x, int y, int w, int h)
{
    t->w = w;
    t->tube_w = w / 2;
    t->border = w >= 32 ? 3 : 2;
    t->root = plain(parent);
    lv_obj_set_pos(t->root, x, y);
    lv_obj_set_size(t->root, w, h);
    t->tube_h = h - w / 2;   /* the tube runs down to the bulb's middle */
    const int tx = (w - t->tube_w) / 2;

    /* tube: white glass with the faint zones, then the fill on top */
    t->tube = plain(t->root);
    lv_obj_set_pos(t->tube, tx, 0);
    lv_obj_set_size(t->tube, t->tube_w, t->tube_h);
    lv_obj_set_style_radius(t->tube, t->tube_w / 2, 0);
    lv_obj_set_style_clip_corner(t->tube, true, 0);
    lv_obj_set_style_bg_opa(t->tube, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_color(t->tube, lv_color_white(), 0);
    float lo = THERMO_MIN_F - 1000;
    for (int i = 0; i < NZONES; i++) {
        int top = i == NZONES - 1 ? 0 : y_for(t, ZONES[i].max_f);
        int bottom = i == 0 ? t->tube_h : y_for(t, lo);
        lo = ZONES[i].max_f;
        lv_obj_t *z = plain(t->tube);
        lv_obj_set_pos(z, 0, top);
        lv_obj_set_size(z, t->tube_w, bottom - top);
        lv_obj_set_style_bg_opa(z, ZONE_OPA, 0);
        lv_obj_set_style_bg_color(z, lv_color_hex(ZONES[i].color), 0);
    }
    t->fill = plain(t->tube);
    lv_obj_set_x(t->fill, 0);
    lv_obj_set_width(t->fill, t->tube_w);
    lv_obj_set_style_bg_opa(t->fill, LV_OPA_COVER, 0);

    /* outline of the tube, drawn over the zones and the fill */
    t->glass = plain(t->root);
    lv_obj_set_pos(t->glass, tx, 0);
    lv_obj_set_size(t->glass, t->tube_w, t->tube_h);
    lv_obj_set_style_radius(t->glass, t->tube_w / 2, 0);
    lv_obj_set_style_border_width(t->glass, t->border, 0);

    /* freezing tick: a short line through the tube at 32 F, poking out on both sides */
    t->freeze = plain(t->root);
    int fy = y_for(t, THERMO_FREEZE_F);
    lv_obj_set_pos(t->freeze, tx - t->tube_w / 3, fy - t->border / 2);
    lv_obj_set_size(t->freeze, t->tube_w + 2 * (t->tube_w / 3), t->border);
    lv_obj_set_style_radius(t->freeze, t->border, 0);
    lv_obj_set_style_bg_opa(t->freeze, LV_OPA_COVER, 0);

    /* bulb, then a little neck that hides the bulb's outline where the tube joins it */
    t->bulb = plain(t->root);
    lv_obj_set_pos(t->bulb, 0, h - w);
    lv_obj_set_size(t->bulb, w, w);
    lv_obj_set_style_radius(t->bulb, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_opa(t->bulb, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(t->bulb, t->border, 0);
    t->bulb_glass = plain(t->root);
    lv_obj_set_pos(t->bulb_glass, tx + t->border, h - w - 2);
    lv_obj_set_size(t->bulb_glass, t->tube_w - 2 * t->border, t->border + 6);
    lv_obj_set_style_bg_opa(t->bulb_glass, LV_OPA_COVER, 0);

    thermo_set_outline(t, lv_color_hex(0x2B3450));
    thermo_set(t, 50);
}

void thermo_set(thermo_t *t, float f)
{
    lv_color_t c = lv_color_hex(thermo_color(f));
    int top = y_for(t, f);
    lv_obj_set_y(t->fill, top);
    lv_obj_set_height(t->fill, t->tube_h - top);
    lv_obj_set_style_bg_color(t->fill, c, 0);
    lv_obj_set_style_bg_color(t->bulb, c, 0);
    lv_obj_set_style_bg_color(t->bulb_glass, c, 0);
}

void thermo_set_outline(thermo_t *t, lv_color_t c)
{
    lv_obj_set_style_border_color(t->glass, c, 0);
    lv_obj_set_style_border_color(t->bulb, c, 0);
    lv_obj_set_style_bg_color(t->freeze, c, 0);
}
