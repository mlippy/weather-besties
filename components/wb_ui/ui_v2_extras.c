/*
 * Pieces of the shared screen interface (ui.h) that the 2.0 screens (ui.c) don't
 * show yet. Kept in their own file so ui.c stays as Boris wrote it.
 */
#include <stdio.h>

#include "ui.h"
#include "ui_v2_extras.h"
#include "thermo.h"
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
