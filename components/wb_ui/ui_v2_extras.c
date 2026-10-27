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
#include "countdown.h"
#include "dress.h"
#include "routine.h"
#include "assets/assets.h"
#include "routine/routine_icons.h"

/* Countdowns for My Day's header pill (countdown.c picks one). Other special events and
 * calendar reminders aren't shown on 2.0 yet. */
static wb_event_t s_events[EVENT_LIST_MAX];
static int s_nevents;

void ui_set_events(const wb_event_t *events, int n)
{
    s_nevents = n < EVENT_LIST_MAX ? n : EVENT_LIST_MAX;
    memcpy(s_events, events, sizeof(s_events[0]) * s_nevents);
}

/* ------------------------------------------------------------------ weather alerts */

/* While an alert is on, a calm strip takes the page title's place on Today and Tomorrow:
 * the kid title and line from alerts.c, purple for a warning and blue for a watch or
 * advisory, no warning sign. The official name stays a grown-up thing. */
static wx_alerts_t s_alerts;
static lv_obj_t *s_al_title_lbl, *s_al_strip, *s_al_head, *s_al_line;

void ui_set_alerts(const wx_alerts_t *alerts)
{
    if (alerts) {
        s_alerts = *alerts;
    } else {
        s_alerts.n = 0;
    }
}

void v2_alert_attach(lv_obj_t *page, lv_obj_t *title)
{
    s_al_title_lbl = title;
    s_al_strip = lv_obj_create(page);
    lv_obj_remove_style_all(s_al_strip);
    lv_obj_remove_flag(s_al_strip, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_pos(s_al_strip, 20, 6);
    lv_obj_set_height(s_al_strip, 60);
    lv_obj_set_style_radius(s_al_strip, 16, 0);
    lv_obj_set_style_bg_opa(s_al_strip, LV_OPA_COVER, 0);
    lv_obj_set_style_pad_hor(s_al_strip, 14, 0);
    s_al_head = lv_label_create(s_al_strip);
    lv_obj_set_style_text_font(s_al_head, &font_chip, 0);
    lv_obj_set_style_text_color(s_al_head, lv_color_white(), 0);
    lv_label_set_long_mode(s_al_head, LV_LABEL_LONG_MODE_DOTS);
    lv_obj_set_pos(s_al_head, 0, 4);
    s_al_line = lv_label_create(s_al_strip);
    lv_obj_set_style_text_font(s_al_line, &font_tiny, 0);
    lv_obj_set_style_text_color(s_al_line, lv_color_white(), 0);
    lv_label_set_long_mode(s_al_line, LV_LABEL_LONG_MODE_DOTS);
    lv_obj_set_pos(s_al_line, 0, 32);
    lv_obj_add_flag(s_al_strip, LV_OBJ_FLAG_HIDDEN);
}

/* Shows the strip (and hides the title) when there's an alert; x_right is where the
 * weather block starts. */
static void alert_apply(int x_right)
{
    if (!s_al_strip) {
        return;
    }
    if (s_alerts.n <= 0) {
        lv_obj_add_flag(s_al_strip, LV_OBJ_FLAG_HIDDEN);
        lv_obj_remove_flag(s_al_title_lbl, LV_OBJ_FLAG_HIDDEN);
        return;
    }
    const wx_alert_t *al = &s_alerts.a[0];
    int w = x_right - 20 - 12;
    lv_obj_set_width(s_al_strip, w);
    lv_obj_set_style_bg_color(s_al_strip, lv_color_hex(al->level == ALERT_WARNING ? 0x7B61C9 : 0x3E8ED0), 0);
    lv_label_set_text(s_al_head, al->title);
    lv_label_set_text(s_al_line, al->kid);
    lv_obj_set_width(s_al_head, w - 28);
    lv_obj_set_width(s_al_line, w - 28);
    lv_obj_add_flag(s_al_title_lbl, LV_OBJ_FLAG_HIDDEN);
    lv_obj_remove_flag(s_al_strip, LV_OBJ_FLAG_HIDDEN);
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
    alert_apply(icon_x - V2_THERMO_GAP - V2_THERMO_W);   /* the weather block sets the alert strip's width */
    if (!s_v2_thermo_made) {
        return;
    }
    lv_obj_set_x(s_v2_thermo.root, icon_x - V2_THERMO_GAP - V2_THERMO_W);
    thermo_set(&s_v2_thermo, temp_f);
    lv_obj_remove_flag(s_v2_thermo.root, LV_OBJ_FLAG_HIDDEN);
}

void v2_thermo_hide(void)
{
    alert_apply(780);
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
    v2_raise(b);
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

static bool s_dress_mode;                  /* false = Boris's rule: every light-jacket day */
static wb_dress_t s_dress;                 /* the calendar's days and the pill's taps (dress.c) */
static lv_obj_t *s_dress_pill, *s_dress_lbl;
static void (*s_dress_changed)(void);
static int s_dress_shown = -1;             /* the day the pill is showing */

void ui_set_dress_days(const int *daynos, int n)
{
    s_dress_mode = true;
    wb_dress_set_calendar(&s_dress, daynos, n);
}

bool v2_dress_wanted(int year, int month, int day)
{
    return !s_dress_mode || wb_dress_on(&s_dress, cal_dayno(year, month, day));
}

static void on_dress_pill(lv_event_t *e)
{
    (void)e;
    if (s_dress_shown < 0) {
        return;
    }
    wb_dress_toggle(&s_dress, s_dress_shown);
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
    v2_raise(s_dress_pill);
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
    bool on = wb_dress_on(&s_dress, s_dress_shown);
    /* Off: an outlined question. On: filled in the dress's own pink, with a check. */
    lv_label_set_text(s_dress_lbl, on ? LV_SYMBOL_OK "  Dress-up" : "Dress up?");
    lv_obj_set_style_bg_color(s_dress_pill, lv_color_hex(on ? 0xF7A1C4 : 0xFFFFFF), 0);
    lv_obj_set_style_border_color(s_dress_pill, lv_color_hex(on ? 0xE8467C : 0xC9C3D6), 0);
    lv_obj_set_style_text_color(s_dress_lbl, lv_color_hex(0x1E2340), 0);
    lv_obj_set_pos(s_dress_pill, x, y);
    lv_obj_remove_flag(s_dress_pill, LV_OBJ_FLAG_HIDDEN);
}

/* ------------------------------------------------------------------ My Day countdown */

static lv_obj_t *s_cd_pill, *s_cd_lbl;

void v2_countdown_create(lv_obj_t *page)
{
    s_cd_pill = lv_obj_create(page);
    lv_obj_remove_style_all(s_cd_pill);
    lv_obj_remove_flag(s_cd_pill, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_radius(s_cd_pill, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(s_cd_pill, lv_color_hex(0xFDE3EE), 0);   /* the dress pill's pink, softer */
    lv_obj_set_style_bg_opa(s_cd_pill, LV_OPA_COVER, 0);
    lv_obj_set_style_pad_hor(s_cd_pill, 14, 0);
    s_cd_lbl = lv_label_create(s_cd_pill);
    lv_obj_set_style_text_font(s_cd_lbl, &font_small, 0);
    lv_obj_set_style_text_color(s_cd_lbl, lv_color_hex(0xB8205A), 0);
    lv_label_set_long_mode(s_cd_lbl, LV_LABEL_LONG_MODE_DOTS);
    lv_obj_add_flag(s_cd_pill, LV_OBJ_FLAG_HIDDEN);
}

bool v2_countdown_text(int year, int month, int day, char *buf, size_t n, bool short_form)
{
    return wb_countdown_text(s_events, s_nevents, cal_dayno(year, month, day), short_form, buf, n);
}

void v2_countdown_show(int year, int month, int day, int x_left, int x_right, int y)
{
    char buf[96];
    if (!s_cd_pill || x_right - x_left < 120 || !v2_countdown_text(year, month, day, buf, sizeof(buf), false)) {
        if (s_cd_pill) {
            lv_obj_add_flag(s_cd_pill, LV_OBJ_FLAG_HIDDEN);
        }
        return;
    }
    lv_point_t sz;
    lv_text_get_size(&sz, buf, &font_small, 0, 0, LV_COORD_MAX, LV_TEXT_FLAG_NONE);
    int max_w = x_right - x_left - 28;   /* the pill's padding */
    if (sz.x > max_w) {   /* "11 sleeps until ...!" before cutting the name short */
        v2_countdown_text(year, month, day, buf, sizeof(buf), true);
        lv_text_get_size(&sz, buf, &font_small, 0, 0, LV_COORD_MAX, LV_TEXT_FLAG_NONE);
    }
    lv_label_set_text(s_cd_lbl, buf);
    lv_obj_set_size(s_cd_lbl, sz.x + 2 < max_w ? sz.x + 2 : max_w, font_small.line_height);   /* one line */
    lv_obj_set_size(s_cd_pill, LV_SIZE_CONTENT, 30);
    lv_obj_center(s_cd_lbl);
    lv_obj_set_pos(s_cd_pill, x_left, y);
    lv_obj_remove_flag(s_cd_pill, LV_OBJ_FLAG_HIDDEN);
}

void v2_dress_hide(void)
{
    if (s_dress_pill) {
        lv_obj_add_flag(s_dress_pill, LV_OBJ_FLAG_HIDDEN);
    }
}

/* ------------------------------------------------------------------ Routine page */

/* Morning (midnight to noon) and bedtime (noon on): a checklist and a bar that shrinks
 * over the last hour before leaving for school / bedtime. Checks reset each day. */

#define RT_ROWS WB_ROUTINE_STEPS
#define RT_BAR_X 28
#define RT_BAR_Y 98
#define RT_BAR_W 688          /* the bus / the bed waits past its right end */
#define RT_TILE_W 150     /* square-ish buttons, three across with room between */
#define RT_TILE_H 128
#define RT_GAP 14
#define RT_GRID_X 28
#define RT_GRID_Y 140

/* routine.c says what each tile is; these are the pictures */
static const lv_image_dsc_t *rt_pic(wb_step_t step)
{
    switch (step) {
    case WB_STEP_POTTY: return &img_rt_potty;
    case WB_STEP_BREAKFAST: return &img_rt_breakfast;
    case WB_STEP_TEETH: return &img_rt_teeth;
    case WB_STEP_HAIR: return &img_rt_hair;
    case WB_STEP_DRESS: return &img_rt_dress;
    case WB_STEP_PAJAMAS: return &img_rt_pajamas;
    case WB_STEP_SNACK: return &img_rt_snack;
    case WB_STEP_BOOKS: return &img_rt_books;
    case WB_STEP_SNUGGLE: return &img_rt_snuggle;
    }
    return NULL;
}

static float s_moon = -1;                  /* tonight's moon phase, 0 new .. 0.5 full; -1 unknown */
static int s_sunrise = -1, s_sunset = -1;  /* minutes after midnight */
static lv_obj_t *s_rt_moon, *s_rt_moon_shadow, *s_rt_sun;

void v2_sun_moon_set(const wx_t *wx)
{
    s_moon = wx_moon_phase(wx->year, wx->month, wx->day, 21);
    s_sunrise = wx->days[0].sunrise_min;
    s_sunset = wx->days[0].sunset_min;
}
static lv_obj_t *s_rt_page, *s_rt_title, *s_rt_when, *s_rt_left, *s_rt_track, *s_rt_fill, *s_rt_pet;
static lv_obj_t *s_rt_runner, *s_rt_finish;   /* the pet's head riding the bar; the bus / the bed */
static const lv_image_dsc_t *s_rt_head_img;
static lv_obj_t *s_rt_row[RT_ROWS], *s_rt_box[RT_ROWS], *s_rt_tick[RT_ROWS], *s_rt_lbl[RT_ROWS], *s_rt_pic[RT_ROWS];
static wb_routine_t s_rt;                      /* times and today's ticks (routine.c) */
static bool s_rt_ready;
static struct tm s_rt_now;
static const lv_image_dsc_t *s_rt_pet_img;

static wb_routine_t *rt(void)
{
    if (!s_rt_ready) {
        wb_routine_init(&s_rt);
        s_rt_ready = true;
    }
    return &s_rt;
}

void ui_set_routine_times(int leave_min, int bed_min)
{
    wb_routine_set_times(rt(), leave_min, bed_min);
}

static void fmt_clock12(int min, char *buf, size_t n)
{
    int h = min / 60 % 12;
    snprintf(buf, n, "%d:%02d %s", h ? h : 12, min % 60, min < 12 * 60 ? "AM" : "PM");
}

static void rt_draw(void);

static void on_rt_row(lv_event_t *e)
{
    int i = (int)(intptr_t)lv_event_get_user_data(e);
    wb_routine_toggle(rt(), &s_rt_now, i);
    rt_draw();
}

static lv_obj_t *rt_box(lv_obj_t *parent)
{
    lv_obj_t *o = lv_obj_create(parent);
    lv_obj_remove_style_all(o);
    lv_obj_remove_flag(o, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);
    return o;
}

static void rt_bounce(void *obj, int32_t v)
{
    lv_obj_set_style_translate_y(obj, v, 0);
}

lv_obj_t *v2_routine_create(lv_obj_t *scr)
{
    s_rt_page = rt_box(scr);
    lv_obj_set_size(s_rt_page, 800, 480);
    lv_obj_add_flag(s_rt_page, LV_OBJ_FLAG_CLICKABLE);   /* a long press here opens Settings too */

    s_rt_title = lv_label_create(s_rt_page);
    lv_obj_set_style_text_font(s_rt_title, &font_h1, 0);
    lv_obj_set_style_text_color(s_rt_title, lv_color_hex(0x1E2340), 0);
    lv_obj_set_pos(s_rt_title, 28, 4);

    s_rt_when = lv_label_create(s_rt_page);   /* "Leave at 8:30 AM" */
    lv_obj_set_style_text_font(s_rt_when, &font_label, 0);
    lv_obj_set_style_text_color(s_rt_when, lv_color_hex(0x545A72), 0);
    lv_obj_set_style_text_align(s_rt_when, LV_TEXT_ALIGN_RIGHT, 0);
    lv_obj_set_width(s_rt_when, 360);
    lv_obj_set_pos(s_rt_when, 412, 16);

    s_rt_left = lv_label_create(s_rt_page);   /* "35 minutes until we leave" */
    lv_obj_set_style_text_font(s_rt_left, &font_label, 0);
    lv_obj_set_style_text_color(s_rt_left, lv_color_hex(0x1E2340), 0);
    lv_obj_set_pos(s_rt_left, 28, 60);

    s_rt_track = rt_box(s_rt_page);
    lv_obj_set_pos(s_rt_track, RT_BAR_X, RT_BAR_Y);
    lv_obj_set_size(s_rt_track, RT_BAR_W, 26);
    lv_obj_set_style_radius(s_rt_track, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(s_rt_track, lv_color_hex(0xECE4D5), 0);
    lv_obj_set_style_bg_opa(s_rt_track, LV_OPA_COVER, 0);
    s_rt_fill = rt_box(s_rt_track);
    lv_obj_set_pos(s_rt_fill, 0, 0);
    lv_obj_set_height(s_rt_fill, 26);
    lv_obj_set_style_radius(s_rt_fill, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_opa(s_rt_fill, LV_OPA_COVER, 0);

    /* The finish line: where the bar is headed (the bus in the morning, the bed at night),
     * and the pet's head running along the fill toward it, with a little bounce. */
    s_rt_finish = lv_image_create(s_rt_page);
    lv_obj_set_pos(s_rt_finish, RT_BAR_X + RT_BAR_W + 8, RT_BAR_Y + 13 - 23);
    s_rt_runner = lv_image_create(s_rt_page);
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, s_rt_runner);
    lv_anim_set_exec_cb(&a, rt_bounce);
    lv_anim_set_values(&a, 0, -5);
    lv_anim_set_duration(&a, 450);
    lv_anim_set_playback_duration(&a, 450);
    lv_anim_set_repeat_count(&a, LV_ANIM_REPEAT_INFINITE);
    lv_anim_start(&a);

    for (int i = 0; i < RT_ROWS; i++) {
        lv_obj_t *r = lv_button_create(s_rt_page);
        lv_obj_remove_style_all(r);
        lv_obj_set_pos(r, RT_GRID_X + (i % 3) * (RT_TILE_W + RT_GAP), RT_GRID_Y + (i / 3) * (RT_TILE_H + RT_GAP));
        lv_obj_set_size(r, RT_TILE_W, RT_TILE_H);
        lv_obj_set_style_radius(r, 20, 0);
        lv_obj_set_style_bg_opa(r, LV_OPA_COVER, 0);
        lv_obj_set_style_border_width(r, 3, 0);
        lv_obj_add_event_cb(r, on_rt_row, LV_EVENT_CLICKED, (void *)(intptr_t)i);
        v2_raise(r);
        s_rt_row[i] = r;
        s_rt_pic[i] = lv_image_create(r);   /* the step's picture, the big target */
        lv_obj_align(s_rt_pic[i], LV_ALIGN_TOP_MID, 0, 10);
        s_rt_box[i] = rt_box(r);   /* the check circle, top right */
        lv_obj_set_size(s_rt_box[i], 30, 30);
        lv_obj_align(s_rt_box[i], LV_ALIGN_TOP_RIGHT, -6, 6);
        lv_obj_set_style_radius(s_rt_box[i], LV_RADIUS_CIRCLE, 0);
        lv_obj_set_style_border_width(s_rt_box[i], 2, 0);
        lv_obj_set_style_bg_opa(s_rt_box[i], LV_OPA_COVER, 0);
        s_rt_tick[i] = lv_label_create(s_rt_box[i]);
        lv_obj_set_style_text_font(s_rt_tick[i], &font_chip, 0);
        lv_obj_set_style_text_color(s_rt_tick[i], lv_color_white(), 0);
        lv_label_set_text(s_rt_tick[i], LV_SYMBOL_OK);
        lv_obj_center(s_rt_tick[i]);
        s_rt_lbl[i] = lv_label_create(r);   /* a short word under it, for grown-ups and readers */
        lv_obj_set_style_text_font(s_rt_lbl[i], &font_chip, 0);
        lv_obj_set_style_text_color(s_rt_lbl[i], lv_color_hex(0x1E2340), 0);
        lv_obj_align(s_rt_lbl[i], LV_ALIGN_BOTTOM_MID, 0, -8);
    }

    /* Tonight's moon and the sun's times, top right under "Leave at" (the classic screens'
     * corner): a pale disc with a shadow disc sliding across it, no artwork. */
    s_rt_sun = lv_label_create(s_rt_page);
    lv_obj_set_style_text_font(s_rt_sun, &font_small, 0);
    lv_obj_set_style_text_color(s_rt_sun, lv_color_hex(0x545A72), 0);
    s_rt_moon = rt_box(s_rt_page);
    lv_obj_set_size(s_rt_moon, 28, 28);
    lv_obj_set_style_radius(s_rt_moon, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_clip_corner(s_rt_moon, true, 0);
    lv_obj_set_style_bg_opa(s_rt_moon, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_color(s_rt_moon, lv_color_hex(0xFFF1B8), 0);
    lv_obj_set_style_border_width(s_rt_moon, 1, 0);
    lv_obj_set_style_border_color(s_rt_moon, lv_color_hex(0x2B3450), 0);
    lv_obj_set_style_border_opa(s_rt_moon, 90, 0);
    s_rt_moon_shadow = rt_box(s_rt_moon);
    lv_obj_set_size(s_rt_moon_shadow, 28, 28);
    lv_obj_set_style_radius(s_rt_moon_shadow, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_opa(s_rt_moon_shadow, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_color(s_rt_moon_shadow, lv_color_hex(0x56628C), 0);

    s_rt_pet = lv_image_create(s_rt_page);
    lv_obj_add_flag(s_rt_page, LV_OBJ_FLAG_HIDDEN);
    return s_rt_page;
}

static void rt_draw(void)
{
    wb_routine_view_t v;
    wb_routine_view(rt(), &s_rt_now, &v);
    int m = v.mode == WB_ROUTINE_MORNING ? 0 : 1;

    for (int i = 0; i < RT_ROWS; i++) {
        if (i >= v.nsteps) {
            lv_obj_add_flag(s_rt_row[i], LV_OBJ_FLAG_HIDDEN);
            continue;
        }
        bool on = v.done[i];
        lv_obj_remove_flag(s_rt_row[i], LV_OBJ_FLAG_HIDDEN);
        lv_label_set_text(s_rt_lbl[i], v.word[i]);
        lv_image_set_src(s_rt_pic[i], rt_pic(v.step[i]));
        lv_obj_set_style_bg_color(s_rt_row[i], lv_color_hex(on ? 0xE8F6EE : 0xFFFFFF), 0);
        lv_obj_set_style_border_color(s_rt_row[i], lv_color_hex(on ? 0x8FD3A8 : 0xE7D9BE), 0);
        lv_obj_set_style_bg_color(s_rt_box[i], lv_color_hex(on ? 0x2FA866 : 0xFFFFFF), 0);
        lv_obj_set_style_border_color(s_rt_box[i], lv_color_hex(on ? 0x2FA866 : 0xC9C3D6), 0);
        if (on) {
            lv_obj_remove_flag(s_rt_tick[i], LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_obj_add_flag(s_rt_tick[i], LV_OBJ_FLAG_HIDDEN);
        }
    }

    v2_hint_target(v.next >= 0 ? s_rt_row[v.next] : NULL);
    lv_label_set_text(s_rt_title, v.title);
    lv_label_set_text(s_rt_when, v.when_text);
    lv_label_set_text(s_rt_left, v.left_text);
    if (!v.show_bar) {
        lv_obj_add_flag(s_rt_track, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(s_rt_runner, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(s_rt_finish, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_remove_flag(s_rt_track, LV_OBJ_FLAG_HIDDEN);
        /* Left to right: the fill is the time gone, so it reaches the bus / the bed at the time. */
        int w = RT_BAR_W * v.bar_gone_min / WB_ROUTINE_WINDOW_MIN;
        lv_obj_set_width(s_rt_fill, w < 26 && w > 0 ? 26 : w);
        lv_obj_set_style_bg_color(s_rt_fill, lv_color_hex(v.bar_color), 0);
        if (w == 0) {
            lv_obj_add_flag(s_rt_fill, LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_obj_remove_flag(s_rt_fill, LV_OBJ_FLAG_HIDDEN);
        }
        lv_image_set_src(s_rt_finish, m == 0 ? &img_rt_bus : &img_rt_bed);
        lv_obj_remove_flag(s_rt_finish, LV_OBJ_FLAG_HIDDEN);
        if (s_rt_head_img) {
            int hw = (int)s_rt_head_img->header.w, hh = (int)s_rt_head_img->header.h;
            int x = RT_BAR_X + w - hw / 2;
            if (x < RT_BAR_X - 4) {
                x = RT_BAR_X - 4;
            }
            if (x > RT_BAR_X + RT_BAR_W - hw + 4) {
                x = RT_BAR_X + RT_BAR_W - hw + 4;
            }
            lv_image_set_src(s_rt_runner, s_rt_head_img);
            lv_obj_set_pos(s_rt_runner, x, RT_BAR_Y + 13 - hh / 2);
            lv_obj_remove_flag(s_rt_runner, LV_OBJ_FLAG_HIDDEN);
        }
    }

    if (s_moon >= 0) {
        /* waxing: the shadow slides off to the left; waning: it comes back from the right */
        int sx = s_moon < 0.5f ? -(int)(s_moon * 2 * 29) : 29 - (int)((s_moon - 0.5f) * 2 * 29);
        lv_obj_set_x(s_rt_moon_shadow, sx);
        char rise[16], set[16], sb[48] = "";
        if (s_sunrise >= 0 && s_sunset >= 0) {
            fmt_clock12(s_sunrise, rise, sizeof(rise));
            fmt_clock12(s_sunset, set, sizeof(set));
            snprintf(sb, sizeof(sb), "Sunrise %s  Sunset %s", rise, set);
        }
        lv_label_set_text(s_rt_sun, sb);
        lv_point_t sz;
        lv_text_get_size(&sz, sb, &font_small, 0, 0, LV_COORD_MAX, LV_TEXT_FLAG_NONE);
        lv_obj_set_pos(s_rt_sun, 772 - sz.x, 58);
        lv_obj_set_pos(s_rt_moon, 772 - sz.x - (sb[0] ? 38 : 28), 56);
        lv_obj_remove_flag(s_rt_moon, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_label_set_text(s_rt_sun, "");
        lv_obj_add_flag(s_rt_moon, LV_OBJ_FLAG_HIDDEN);
    }

    if (s_rt_pet_img) {
        lv_image_set_src(s_rt_pet, s_rt_pet_img);
        lv_obj_set_pos(s_rt_pet, 772 - (int)s_rt_pet_img->header.w, 414 - (int)s_rt_pet_img->header.h);
    }
}

void v2_routine_render(const struct tm *now, const lv_image_dsc_t *pet, const lv_image_dsc_t *head)
{
    s_rt_now = *now;
    s_rt_pet_img = pet;
    s_rt_head_img = head;
    rt_draw();
}

#ifdef WB_SIM
void v2_routine_test_tap(int row)
{
    lv_obj_send_event(s_rt_row[row], LV_EVENT_CLICKED, NULL);
}
#endif

/* ------------------------------------------------------------------ what can be tapped */

#define RAISE_EDGE 0x1E2340     /* the ink colour, soft: reads on cream, lavender and white */
#define HINT_IDLE_MS 20000
#define HINT_COLOR 0xE8467C     /* the dress pill's pink */

void v2_raise(lv_obj_t *o)
{
    lv_obj_set_style_shadow_width(o, 2, 0);
    lv_obj_set_style_shadow_spread(o, 0, 0);
    lv_obj_set_style_shadow_offset_y(o, 4, 0);
    lv_obj_set_style_shadow_color(o, lv_color_hex(RAISE_EDGE), 0);
    lv_obj_set_style_shadow_opa(o, 70, 0);
    /* pressed: it sinks onto its edge */
    lv_obj_set_style_shadow_offset_y(o, 1, LV_STATE_PRESSED);
    lv_obj_set_style_translate_y(o, 3, LV_STATE_PRESSED);
    lv_obj_set_ext_click_area(o, 6);
}

static lv_obj_t *s_hint;
static bool s_hint_on;
static bool s_hint_force;   /* the renderer has no real idle time */
static lv_timer_t *s_hint_timer;

static void hint_outline(void *obj, int32_t w)
{
    lv_obj_set_style_outline_width(obj, w, 0);
}

static void hint_stop(void)
{
    if (s_hint && s_hint_on) {
        lv_anim_delete(s_hint, hint_outline);
        lv_obj_set_style_outline_width(s_hint, 0, 0);
    }
    s_hint_on = false;
}

static void hint_tick(lv_timer_t *t)
{
    (void)t;
    bool idle = s_hint_force || lv_display_get_inactive_time(NULL) > HINT_IDLE_MS;
    bool shown = s_hint && lv_obj_is_visible(s_hint);
    if (!idle || !shown) {
        hint_stop();
        return;
    }
    if (s_hint_on) {
        return;
    }
    s_hint_on = true;
    lv_obj_set_style_outline_color(s_hint, lv_color_hex(HINT_COLOR), 0);
    lv_obj_set_style_outline_opa(s_hint, LV_OPA_COVER, 0);
    lv_obj_set_style_outline_pad(s_hint, 4, 0);
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, s_hint);
    lv_anim_set_exec_cb(&a, hint_outline);
    lv_anim_set_values(&a, 2, 7);
    lv_anim_set_duration(&a, 700);
    lv_anim_set_playback_duration(&a, 700);
    lv_anim_set_repeat_delay(&a, 1600);
    lv_anim_set_repeat_count(&a, LV_ANIM_REPEAT_INFINITE);
    lv_anim_start(&a);
}

void v2_hint_target(lv_obj_t *o)
{
    if (o == s_hint) {
        return;
    }
    hint_stop();
    s_hint = o;
    if (!s_hint_timer) {
        s_hint_timer = lv_timer_create(hint_tick, 500, NULL);
    }
}

#ifdef WB_SIM
/* Shows the hint now, as if the screen had been left alone (the renderer has no idle). */
void v2_hint_test_show(bool on)
{
    s_hint_force = on;   /* as if the screen had been left alone for 20 s */
    hint_tick(NULL);
}
#endif

/* ------------------------------------------------------------------ brightness */

#define BRIGHT_MIN 30

static lv_obj_t *s_dim;                        /* the see-through dark layer, on the top layer */
static lv_obj_t *s_br_row, *s_br_slider, *s_br_speed_pill;
static ui_brightness_cb_t s_br_cb;
static int s_bright = 50;   /* until the board says (settings default) */

static void dim_apply(void)
{
    if (!s_dim) {
        s_dim = lv_obj_create(lv_layer_top());
        lv_obj_remove_style_all(s_dim);
        lv_obj_remove_flag(s_dim, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_size(s_dim, LV_PCT(100), LV_PCT(100));
        lv_obj_set_style_bg_color(s_dim, lv_color_black(), 0);
    }
    /* 100 % -> clear; 30 % -> about 70 % black over the picture */
    lv_obj_set_style_bg_opa(s_dim, (lv_opa_t)((100 - s_bright) * 255 / 100), 0);
    if (s_bright >= 100) {
        lv_obj_add_flag(s_dim, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_remove_flag(s_dim, LV_OBJ_FLAG_HIDDEN);
    }
}

void ui_set_brightness(int pct)
{
    s_bright = pct < BRIGHT_MIN ? BRIGHT_MIN : pct > 100 ? 100 : pct;
    dim_apply();
    if (s_br_slider) {
        lv_slider_set_value(s_br_slider, s_bright, LV_ANIM_OFF);
    }
}

static void on_br_slider(lv_event_t *e)
{
    s_bright = lv_slider_get_value(s_br_slider);
    dim_apply();   /* live while dragging */
    if (lv_event_get_code(e) == LV_EVENT_RELEASED && s_br_cb) {
        s_br_cb(s_bright);   /* saved when let go */
    }
}

void v2_brightness_attach(lv_obj_t *settings, lv_obj_t *speed_pill, int x, int y, int w)
{
    s_br_speed_pill = speed_pill;
    s_br_row = lv_obj_create(settings);
    lv_obj_remove_style_all(s_br_row);
    lv_obj_remove_flag(s_br_row, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_pos(s_br_row, x, y);
    lv_obj_set_size(s_br_row, w, 48);
    lv_obj_t *l = lv_label_create(s_br_row);
    lv_obj_set_style_text_font(l, &font_body, 0);
    lv_obj_set_style_text_color(l, lv_color_hex(0x1E2340), 0);
    lv_label_set_text(l, "Brightness");
    lv_obj_align(l, LV_ALIGN_LEFT_MID, 0, 0);
    s_br_slider = lv_slider_create(s_br_row);
    lv_obj_remove_style_all(s_br_slider);
    lv_slider_set_range(s_br_slider, BRIGHT_MIN, 100);
    lv_slider_set_value(s_br_slider, s_bright, LV_ANIM_OFF);
    lv_obj_set_size(s_br_slider, w - 170, 14);
    lv_obj_align(s_br_slider, LV_ALIGN_LEFT_MID, 150, 0);
    lv_obj_set_style_radius(s_br_slider, LV_RADIUS_CIRCLE, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(s_br_slider, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_bg_color(s_br_slider, lv_color_hex(0xDDE3EC), LV_PART_MAIN);
    lv_obj_set_style_radius(s_br_slider, LV_RADIUS_CIRCLE, LV_PART_INDICATOR);
    lv_obj_set_style_bg_opa(s_br_slider, LV_OPA_COVER, LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(s_br_slider, lv_color_hex(0xF2C230), LV_PART_INDICATOR);   /* sunny */
    lv_obj_set_style_radius(s_br_slider, LV_RADIUS_CIRCLE, LV_PART_KNOB);
    lv_obj_set_style_bg_opa(s_br_slider, LV_OPA_COVER, LV_PART_KNOB);
    lv_obj_set_style_bg_color(s_br_slider, lv_color_white(), LV_PART_KNOB);
    lv_obj_set_style_border_width(s_br_slider, 3, LV_PART_KNOB);
    lv_obj_set_style_border_color(s_br_slider, lv_color_hex(0x1E2340), LV_PART_KNOB);
    lv_obj_set_style_pad_all(s_br_slider, 12, LV_PART_KNOB);   /* a 38 px knob: easy to grab */
    lv_obj_set_style_shadow_width(s_br_slider, 2, LV_PART_KNOB);
    lv_obj_set_style_shadow_offset_y(s_br_slider, 3, LV_PART_KNOB);
    lv_obj_set_style_shadow_opa(s_br_slider, 70, LV_PART_KNOB);
    lv_obj_set_ext_click_area(s_br_slider, 16);
    lv_obj_add_event_cb(s_br_slider, on_br_slider, LV_EVENT_VALUE_CHANGED, NULL);
    lv_obj_add_event_cb(s_br_slider, on_br_slider, LV_EVENT_RELEASED, NULL);
    lv_obj_add_flag(s_br_row, LV_OBJ_FLAG_HIDDEN);   /* until the board asks for it */
}

void ui_set_brightness_cb(ui_brightness_cb_t cb)
{
    s_br_cb = cb;
    if (s_br_row) {   /* the slider takes the Speed test pill's place */
        lv_obj_remove_flag(s_br_row, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(s_br_speed_pill, LV_OBJ_FLAG_HIDDEN);
    }
}
