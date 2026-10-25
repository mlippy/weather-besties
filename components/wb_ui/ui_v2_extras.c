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

/* Countdowns (special events with Countdown ticked, calendar titles tagged (countdown),
 * a cake or a party popper): kept here for My Day's header pill. Other special events and
 * calendar reminders aren't shown on 2.0 yet. */
#define CD_MAX 24
static struct {
    int dayno;
    char name[EVENT_NAME_LEN];
} s_cd[CD_MAX];
static int s_ncd;

void ui_set_events(const wb_event_t *events, int n)
{
    s_ncd = 0;
    for (int i = 0; i < n && s_ncd < CD_MAX; i++) {
        const wb_event_t *e = &events[i];
        if (e->countdown && !e->wdays && e->year) {
            s_cd[s_ncd].dayno = cal_dayno(e->year, e->month, e->day);
            snprintf(s_cd[s_ncd].name, sizeof(s_cd[s_ncd].name), "%s", e->name);
            s_ncd++;
        }
    }
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
    int today = cal_dayno(year, month, day);
    int best = -1, best_d = CAL_COUNTDOWN_DAYS + 1;
    for (int i = 0; i < s_ncd; i++) {
        int d = s_cd[i].dayno - today;
        if (d >= 0 && d < best_d) {
            best = i;
            best_d = d;
        }
    }
    if (best < 0) {
        return false;
    }
    if (best_d == 0) {
        snprintf(buf, n, "%s is today!", s_cd[best].name);
    } else {
        snprintf(buf, n, "%d %ssleep%s until %s!", best_d, short_form ? "" : "more ", best_d == 1 ? "" : "s",
                 s_cd[best].name);
    }
    return true;
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

#define RT_ROWS 6
#define RT_WINDOW_MIN 60      /* the bar is full an hour out and empty at the time */
#define RT_ROW_H 42
#define RT_ROW_GAP 4
#define RT_LIST_Y 138

static const char *const RT_MORNING[] = {"Go potty", "Eat breakfast", "Brush teeth", "Brush hair", "Get dressed"};
static const char *const RT_BEDTIME[] = {"Go potty", "Pajamas on", "Have a snack", "Brush teeth", "Read books",
                                         "Snuggle"};

static int s_rt_leave = 8 * 60 + 30, s_rt_bed = 20 * 60;   /* minutes after midnight */
static lv_obj_t *s_rt_page, *s_rt_title, *s_rt_when, *s_rt_left, *s_rt_track, *s_rt_fill, *s_rt_pet;
static lv_obj_t *s_rt_row[RT_ROWS], *s_rt_box[RT_ROWS], *s_rt_tick[RT_ROWS], *s_rt_lbl[RT_ROWS];
static uint8_t s_rt_checks[2];                 /* bit i = row i done; [0] morning, [1] bedtime */
static int s_rt_day[2] = {-1, -1};             /* the day those checks belong to */
static struct tm s_rt_now;
static const lv_image_dsc_t *s_rt_pet_img;

void ui_set_routine_times(int leave_min, int bed_min)
{
    if (leave_min > 0 && leave_min < 24 * 60) {
        s_rt_leave = leave_min;
    }
    if (bed_min > 0 && bed_min < 24 * 60) {
        s_rt_bed = bed_min;
    }
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
    int m = s_rt_now.tm_hour < 12 ? 0 : 1;
    s_rt_checks[m] ^= (uint8_t)(1u << i);
    rt_draw();
}

static lv_obj_t *rt_box(lv_obj_t *parent)
{
    lv_obj_t *o = lv_obj_create(parent);
    lv_obj_remove_style_all(o);
    lv_obj_remove_flag(o, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);
    return o;
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
    lv_obj_set_pos(s_rt_track, 28, 98);
    lv_obj_set_size(s_rt_track, 744, 26);
    lv_obj_set_style_radius(s_rt_track, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(s_rt_track, lv_color_hex(0xECE4D5), 0);
    lv_obj_set_style_bg_opa(s_rt_track, LV_OPA_COVER, 0);
    s_rt_fill = rt_box(s_rt_track);
    lv_obj_set_pos(s_rt_fill, 0, 0);
    lv_obj_set_height(s_rt_fill, 26);
    lv_obj_set_style_radius(s_rt_fill, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_opa(s_rt_fill, LV_OPA_COVER, 0);

    for (int i = 0; i < RT_ROWS; i++) {
        lv_obj_t *r = lv_button_create(s_rt_page);
        lv_obj_remove_style_all(r);
        lv_obj_set_pos(r, 28, RT_LIST_Y + i * (RT_ROW_H + RT_ROW_GAP));
        lv_obj_set_size(r, 480, RT_ROW_H);
        lv_obj_set_style_radius(r, 14, 0);
        lv_obj_set_style_bg_opa(r, LV_OPA_COVER, 0);
        lv_obj_set_style_border_width(r, 2, 0);
        lv_obj_add_event_cb(r, on_rt_row, LV_EVENT_CLICKED, (void *)(intptr_t)i);
        s_rt_row[i] = r;
        s_rt_box[i] = rt_box(r);   /* the check circle */
        lv_obj_set_size(s_rt_box[i], 30, 30);
        lv_obj_align(s_rt_box[i], LV_ALIGN_LEFT_MID, 8, 0);
        lv_obj_set_style_radius(s_rt_box[i], LV_RADIUS_CIRCLE, 0);
        lv_obj_set_style_border_width(s_rt_box[i], 2, 0);
        lv_obj_set_style_bg_opa(s_rt_box[i], LV_OPA_COVER, 0);
        s_rt_tick[i] = lv_label_create(s_rt_box[i]);
        lv_obj_set_style_text_font(s_rt_tick[i], &font_chip, 0);
        lv_obj_set_style_text_color(s_rt_tick[i], lv_color_white(), 0);
        lv_label_set_text(s_rt_tick[i], LV_SYMBOL_OK);
        lv_obj_center(s_rt_tick[i]);
        s_rt_lbl[i] = lv_label_create(r);
        lv_obj_set_style_text_font(s_rt_lbl[i], &font_label, 0);
        lv_obj_set_style_text_color(s_rt_lbl[i], lv_color_hex(0x1E2340), 0);
        lv_obj_align(s_rt_lbl[i], LV_ALIGN_LEFT_MID, 50, 0);
    }

    s_rt_pet = lv_image_create(s_rt_page);
    lv_obj_add_flag(s_rt_page, LV_OBJ_FLAG_HIDDEN);
    return s_rt_page;
}

static void rt_draw(void)
{
    const struct tm *now = &s_rt_now;
    int m = now->tm_hour < 12 ? 0 : 1;
    const char *const *items = m == 0 ? RT_MORNING : RT_BEDTIME;
    int n = m == 0 ? (int)(sizeof(RT_MORNING) / sizeof(RT_MORNING[0]))
                   : (int)(sizeof(RT_BEDTIME) / sizeof(RT_BEDTIME[0]));
    int today = cal_dayno(now->tm_year + 1900, now->tm_mon + 1, now->tm_mday);
    if (s_rt_day[m] != today) {
        s_rt_day[m] = today;   /* a new day starts unchecked */
        s_rt_checks[m] = 0;
    }
    bool school = m == 1 || (now->tm_wday >= 1 && now->tm_wday <= 5);
    int target = m == 0 ? s_rt_leave : s_rt_bed;
    int now_min = now->tm_hour * 60 + now->tm_min;
    int left = target - now_min;

    int done = 0;
    for (int i = 0; i < RT_ROWS; i++) {
        if (i >= n) {
            lv_obj_add_flag(s_rt_row[i], LV_OBJ_FLAG_HIDDEN);
            continue;
        }
        bool on = (s_rt_checks[m] >> i) & 1;
        done += on;
        lv_obj_remove_flag(s_rt_row[i], LV_OBJ_FLAG_HIDDEN);
        lv_label_set_text(s_rt_lbl[i], items[i]);
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

    bool all = done == n;
    lv_label_set_text(s_rt_title, all ? (m == 0 ? (school ? "Ready for school!" : "All done!") : "Ready for bed!")
                                      : (m == 0 ? "Morning routine" : "Bedtime routine"));
    char t[24], buf[64];
    fmt_clock12(target, t, sizeof(t));
    if (!school) {
        lv_label_set_text(s_rt_when, "");
        lv_label_set_text(s_rt_left, "No school today!");
        lv_obj_add_flag(s_rt_track, LV_OBJ_FLAG_HIDDEN);
    } else {
        snprintf(buf, sizeof(buf), "%s %s", m == 0 ? "Leave at" : "Bed at", t);
        lv_label_set_text(s_rt_when, buf);
        const char *what = m == 0 ? "until we leave" : "until bedtime";
        if (left > 0) {
            snprintf(buf, sizeof(buf), "%d minute%s %s", left, left == 1 ? "" : "s", what);
        } else if (left > -RT_WINDOW_MIN) {
            snprintf(buf, sizeof(buf), "%s", m == 0 ? "Time to go!" : "Bedtime!");
        } else {
            snprintf(buf, sizeof(buf), "%s", m == 0 ? "Have a great day!" : "Sweet dreams!");
        }
        lv_label_set_text(s_rt_left, buf);
        lv_obj_remove_flag(s_rt_track, LV_OBJ_FLAG_HIDDEN);
        int w = left >= RT_WINDOW_MIN ? 744 : left <= 0 ? 0 : 744 * left / RT_WINDOW_MIN;
        /* the bar's colour says how close it is: green, yellow, orange, red */
        uint32_t c = left > 30 ? 0x4CB860 : left > 15 ? 0xF2C230 : left > 5 ? 0xF28C28 : 0xE5483C;
        lv_obj_set_width(s_rt_fill, w < 26 && w > 0 ? 26 : w);
        lv_obj_set_style_bg_color(s_rt_fill, lv_color_hex(c), 0);
        if (w == 0) {
            lv_obj_add_flag(s_rt_fill, LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_obj_remove_flag(s_rt_fill, LV_OBJ_FLAG_HIDDEN);
        }
    }

    if (s_rt_pet_img) {
        lv_image_set_src(s_rt_pet, s_rt_pet_img);
        lv_obj_set_pos(s_rt_pet, 772 - (int)s_rt_pet_img->header.w, 414 - (int)s_rt_pet_img->header.h);
    }
}

void v2_routine_render(const struct tm *now, const lv_image_dsc_t *pet)
{
    s_rt_now = *now;
    s_rt_pet_img = pet;
    rt_draw();
}

#ifdef WB_SIM
void v2_routine_test_tap(int row)
{
    lv_obj_send_event(s_rt_row[row], LV_EVENT_CLICKED, NULL);
}
#endif
