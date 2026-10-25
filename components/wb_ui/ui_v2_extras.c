/*
 * Pieces of the shared screen interface (ui.h) that the 2.0 screens (ui.c) don't
 * show yet. Kept in their own file so ui.c stays as Boris wrote it.
 */
#include <stdio.h>
#include <string.h>

#include "ui.h"
#include "ui_v2_extras.h"
#include "thermo.h"
#include "cal.h"
#include "assets/assets.h"

void ui_set_events(const wb_event_t *events, int n)
{
    (void)events;
    (void)n;   /* 2.0 doesn't show special events or calendar reminders yet */
}

void ui_set_alerts(const wx_alerts_t *alerts)
{
    (void)alerts;   /* 2.0 doesn't show weather alerts yet */
}

/* ------------------------------------------------------------------ thermometer */

#define V2_THERMO_W 22
#define V2_THERMO_H 70
#define V2_THERMO_GAP 8   /* to the weather icon */

static thermo_t s_v2_thermo;
static bool s_v2_thermo_made;

void v2_thermo_create(lv_obj_t *page)
{
    thermo_create(&s_v2_thermo, page, 0, 2, V2_THERMO_W, V2_THERMO_H);
    thermo_set_outline(&s_v2_thermo, lv_color_hex(0x2B3450));
    lv_obj_add_flag(s_v2_thermo.root, LV_OBJ_FLAG_HIDDEN);
    s_v2_thermo_made = true;
}

void v2_thermo_show(int icon_x, float temp_f)
{
    if (!s_v2_thermo_made) {
        return;
    }
    lv_obj_set_x(s_v2_thermo.root, icon_x - V2_THERMO_GAP - V2_THERMO_W);
    thermo_set(&s_v2_thermo, temp_f);
    lv_obj_remove_flag(s_v2_thermo.root, LV_OBJ_FLAG_HIDDEN);
}

void v2_thermo_hide(void)
{
    if (s_v2_thermo_made) {
        lv_obj_add_flag(s_v2_thermo.root, LV_OBJ_FLAG_HIDDEN);
    }
}

/* ------------------------------------------------------------------ Update in Settings */

static void (*s_v2_fire)(ui_action_t);

static void on_update_pill(lv_event_t *e)
{
    (void)e;
    ui_close_settings();
    if (s_v2_fire) {
        s_v2_fire(UI_ACT_UPDATE);   /* the startup screen shows "Checking for updates..." */
    }
}

void v2_update_pill_create(lv_obj_t *settings, int x_right, int y, void (*fire)(ui_action_t))
{
    s_v2_fire = fire;
    /* Same look as the Speed test pill beside it. */
    lv_obj_t *b = lv_button_create(settings);
    lv_obj_remove_style_all(b);
    lv_obj_set_style_radius(b, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(b, lv_color_hex(0xEDF1F5), 0);
    lv_obj_set_style_bg_opa(b, LV_OPA_COVER, 0);
    lv_obj_set_style_pad_hor(b, 26, 0);
    lv_obj_set_size(b, LV_SIZE_CONTENT, 48);
    lv_obj_t *l = lv_label_create(b);
    lv_label_set_text(l, "Update");
    lv_obj_set_style_text_font(l, &font_body, 0);
    lv_obj_set_style_text_color(l, lv_color_hex(0x1E2340), 0);   /* COL_INK */
    lv_obj_center(l);
    lv_obj_add_event_cb(b, on_update_pill, LV_EVENT_CLICKED, NULL);
    lv_obj_update_layout(b);
    lv_obj_set_pos(b, x_right - lv_obj_get_width(b), y);
}

/* ------------------------------------------------------------------ her name in the title */

static bool s_name_in_title = true;

void v2_set_name_in_title(bool on)
{
    s_name_in_title = on;
}

const char *v2_named_title(const char *his_title, const char *names, const char *const *formats, int n,
                           int max_w)
{
    static char buf[96];
    if (!s_name_in_title || !names || !names[0]) {
        return his_title;
    }
    for (int i = 0; i < n; i++) {
        snprintf(buf, sizeof(buf), formats[i], names);
        lv_point_t sz;
        lv_text_get_size(&sz, buf, &font_h1, 0, 0, LV_COORD_MAX, LV_TEXT_FLAG_NONE);
        if (sz.x <= max_w) {
            return buf;
        }
    }
    return his_title;
}

/* ------------------------------------------------------------------ dress-up days */

#define DRESS_MAX 8

static bool s_dress_mode;                  /* false = Boris's rule: every light-jacket day */
static int s_dress_cal[DRESS_MAX], s_ndress_cal;
static struct {
    int dayno;
    bool on;
} s_dress_tap[DRESS_MAX];                  /* the pill's choices, newest last */
static int s_ndress_tap;
static lv_obj_t *s_dress_pill, *s_dress_lbl;
static void (*s_dress_changed)(void);
static int s_dress_shown = -1;             /* the day the pill is showing */

void ui_set_dress_days(const int *daynos, int n)
{
    s_dress_mode = true;
    s_ndress_cal = 0;
    for (int i = 0; i < n && s_ndress_cal < DRESS_MAX; i++) {
        s_dress_cal[s_ndress_cal++] = daynos[i];
    }
}

static bool dress_on(int dn)
{
    for (int i = s_ndress_tap - 1; i >= 0; i--) {
        if (s_dress_tap[i].dayno == dn) {
            return s_dress_tap[i].on;   /* a tap beats the calendar */
        }
    }
    for (int i = 0; i < s_ndress_cal; i++) {
        if (s_dress_cal[i] == dn) {
            return true;
        }
    }
    return false;
}

bool v2_dress_wanted(int year, int month, int day)
{
    return !s_dress_mode || dress_on(cal_dayno(year, month, day));
}

static void on_dress_pill(lv_event_t *e)
{
    (void)e;
    if (s_dress_shown < 0) {
        return;
    }
    bool on = !dress_on(s_dress_shown);
    int i = 0;
    while (i < s_ndress_tap && s_dress_tap[i].dayno != s_dress_shown) {
        i++;
    }
    if (i == s_ndress_tap) {
        if (s_ndress_tap == DRESS_MAX) {   /* forget the oldest */
            memmove(&s_dress_tap[0], &s_dress_tap[1], sizeof(s_dress_tap[0]) * (DRESS_MAX - 1));
            s_ndress_tap--;
            i = s_ndress_tap;
        }
        s_ndress_tap++;
    }
    s_dress_tap[i].dayno = s_dress_shown;
    s_dress_tap[i].on = on;
    if (s_dress_changed) {
        s_dress_changed();
    }
}

void v2_dress_create(lv_obj_t *page, void (*changed)(void))
{
    s_dress_changed = changed;
    s_dress_pill = lv_button_create(page);
    lv_obj_remove_style_all(s_dress_pill);
    lv_obj_set_style_radius(s_dress_pill, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_opa(s_dress_pill, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(s_dress_pill, 2, 0);
    lv_obj_set_style_pad_hor(s_dress_pill, 16, 0);
    lv_obj_set_size(s_dress_pill, LV_SIZE_CONTENT, 36);
    s_dress_lbl = lv_label_create(s_dress_pill);
    lv_obj_set_style_text_font(s_dress_lbl, &font_chip, 0);
    lv_obj_center(s_dress_lbl);
    lv_obj_add_event_cb(s_dress_pill, on_dress_pill, LV_EVENT_CLICKED, NULL);
    lv_obj_add_flag(s_dress_pill, LV_OBJ_FLAG_HIDDEN);
}

void v2_dress_show(bool fits, int year, int month, int day, int x, int y)
{
    if (!s_dress_pill) {
        return;
    }
    if (!s_dress_mode || !fits) {
        s_dress_shown = -1;
        lv_obj_add_flag(s_dress_pill, LV_OBJ_FLAG_HIDDEN);
        return;
    }
    s_dress_shown = cal_dayno(year, month, day);
    bool on = dress_on(s_dress_shown);
    /* Off: an outlined question. On: filled in the dress's own pink, with a check. */
    lv_label_set_text(s_dress_lbl, on ? LV_SYMBOL_OK "  Dress-up day" : "Dress up?");
    lv_obj_set_style_bg_color(s_dress_pill, lv_color_hex(on ? 0xF7A1C4 : 0xFFFFFF), 0);
    lv_obj_set_style_border_color(s_dress_pill, lv_color_hex(on ? 0xE8467C : 0xC9C3D6), 0);
    lv_obj_set_style_text_color(s_dress_lbl, lv_color_hex(0x1E2340), 0);
    lv_obj_set_pos(s_dress_pill, x, y);
    lv_obj_remove_flag(s_dress_pill, LV_OBJ_FLAG_HIDDEN);
}
