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
#include "routine/routine_icons.h"

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
#define RT_WINDOW_MIN 60      /* the bar starts filling an hour out and is full at the time */
#define RT_BAR_X 28
#define RT_BAR_Y 98
#define RT_BAR_W 688          /* the bus / the bed waits past its right end */
#define RT_TILE_W 150     /* square-ish buttons, three across with room between */
#define RT_TILE_H 128
#define RT_GAP 14
#define RT_GRID_X 28
#define RT_GRID_Y 140

typedef struct {
    const char *word;
    const lv_image_dsc_t *pic;
} rt_step_t;

static const rt_step_t RT_MORNING[] = {{"Potty", &img_rt_potty},       {"Breakfast", &img_rt_breakfast},
                                       {"Brush teeth", &img_rt_teeth}, {"Brush hair", &img_rt_hair},
                                       {"Get dressed", &img_rt_dress}};
static const rt_step_t RT_BEDTIME[] = {{"Potty", &img_rt_potty},  {"Pajamas", &img_rt_pajamas},
                                       {"Snack", &img_rt_snack},  {"Brush teeth", &img_rt_teeth},
                                       {"Books", &img_rt_books},  {"Snuggle", &img_rt_snuggle}};

static int s_rt_leave = 8 * 60 + 30, s_rt_bed = 20 * 60;   /* minutes after midnight */
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
    const struct tm *now = &s_rt_now;
    int m = now->tm_hour < 12 ? 0 : 1;
    const rt_step_t *items = m == 0 ? RT_MORNING : RT_BEDTIME;
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
        lv_label_set_text(s_rt_lbl[i], items[i].word);
        lv_image_set_src(s_rt_pic[i], items[i].pic);
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
    lv_obj_t *next = NULL;
    for (int i = 0; i < n && !next; i++) {
        if (!((s_rt_checks[m] >> i) & 1)) {
            next = s_rt_row[i];
        }
    }
    v2_hint_target(next);
    lv_label_set_text(s_rt_title, all ? (m == 0 ? (school ? "Ready for school!" : "All done!") : "Ready for bed!")
                                      : (m == 0 ? "Morning routine" : "Bedtime routine"));
    char t[24], buf[64];
    fmt_clock12(target, t, sizeof(t));
    if (!school) {
        lv_label_set_text(s_rt_when, "");
        lv_label_set_text(s_rt_left, "No school today!");
        lv_obj_add_flag(s_rt_track, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(s_rt_runner, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(s_rt_finish, LV_OBJ_FLAG_HIDDEN);
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
        /* Left to right: the fill is the time gone, so it reaches the bus / the bed at the time. */
        int gone = left >= RT_WINDOW_MIN ? 0 : left <= 0 ? RT_WINDOW_MIN : RT_WINDOW_MIN - left;
        int w = RT_BAR_W * gone / RT_WINDOW_MIN;
        /* the bar's colour says how close it is: green, yellow, orange, red */
        uint32_t c = left > 30 ? 0x4CB860 : left > 15 ? 0xF2C230 : left > 5 ? 0xF28C28 : 0xE5483C;
        lv_obj_set_width(s_rt_fill, w < 26 && w > 0 ? 26 : w);
        lv_obj_set_style_bg_color(s_rt_fill, lv_color_hex(c), 0);
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
