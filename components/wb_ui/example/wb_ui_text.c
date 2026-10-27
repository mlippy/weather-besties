/*
 * A text-only screen design: the smallest complete one. It implements wb_ui.h directly
 * (no ui.h, no artwork, only LVGL's built-in font) to show what a new design needs:
 * read the state, ask wb_core what to say, draw it, and report taps as wb_action_t.
 * Pick it in ui_select.h (WB_UI_EXAMPLE 1). See docs/new-ui.md.
 */
#include "wb_ui.h"

#include <stdio.h>
#include <string.h>

#include "cal.h"
#include "countdown.h"
#include "lvgl.h"
#include "outfit.h"
#include "routine.h"

static wb_action_cb_t s_cb;
static void *s_user;
static wb_clock_cb_t s_clock;
static wb_routine_t s_rt;   /* the Routine ticks live in the screen, like the 2.0 page */

static wb_state_t s_st;     /* the last state, for the timer's redraw */
static bool s_have_st;
static lv_obj_t *s_page, *s_lines, *s_footer, *s_status, *s_status_lbl;

static void on_button(lv_event_t *e)
{
    if (s_cb) {
        s_cb((wb_action_t)(intptr_t)lv_event_get_user_data(e), 0, s_user);
    }
}

static void button(lv_obj_t *parent, const char *text, wb_action_t act, int x)
{
    lv_obj_t *b = lv_button_create(parent);
    lv_obj_set_size(b, 150, 48);
    lv_obj_align(b, LV_ALIGN_BOTTOM_LEFT, x, -16);
    lv_obj_t *l = lv_label_create(b);
    lv_label_set_text(l, text);
    lv_obj_center(l);
    lv_obj_add_event_cb(b, on_button, LV_EVENT_CLICKED, (void *)(intptr_t)act);
}

static void on_minute(lv_timer_t *t)
{
    (void)t;
    if (s_have_st && lv_obj_has_flag(s_status, LV_OBJ_FLAG_HIDDEN)) {
        wb_ui_update(&s_st);
    }
}

void wb_ui_start(wb_action_cb_t on_action, void *user, wb_clock_cb_t clock)
{
    s_cb = on_action;
    s_user = user;
    s_clock = clock;
    wb_routine_init(&s_rt);

#if LV_USE_PERF_MONITOR
    lv_sysmon_hide_performance(lv_display_get_default());   /* LVGL shows it by default */
#endif
    lv_obj_t *scr = lv_screen_active();
    lv_obj_set_style_bg_color(scr, lv_color_white(), 0);

    s_page = lv_obj_create(scr);   /* the weather page */
    lv_obj_remove_style_all(s_page);
    lv_obj_set_size(s_page, LV_PCT(100), LV_PCT(100));
    s_lines = lv_label_create(s_page);
    lv_obj_set_width(s_lines, 760);
    lv_obj_set_pos(s_lines, 20, 16);
    lv_obj_set_style_text_line_space(s_lines, 6, 0);
    s_footer = lv_label_create(s_page);
    lv_obj_align(s_footer, LV_ALIGN_BOTTOM_RIGHT, -20, -30);
    button(s_page, "Refresh", WB_ACT_REFRESH, 20);
    button(s_page, "Update", WB_ACT_UPDATE, 186);
    button(s_page, "Setup", WB_ACT_SETUP, 352);

    s_status = lv_obj_create(scr);   /* boot, setup, connecting, WiFi trouble */
    lv_obj_remove_style_all(s_status);
    lv_obj_set_size(s_status, LV_PCT(100), LV_PCT(100));
    s_status_lbl = lv_label_create(s_status);
    lv_obj_set_width(s_status_lbl, 700);
    lv_obj_set_style_text_align(s_status_lbl, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_center(s_status_lbl);
    button(s_status, "Setup", WB_ACT_SETUP, 20);
    lv_obj_add_flag(s_status, LV_OBJ_FLAG_HIDDEN);

    lv_timer_create(on_minute, 30000, NULL);   /* the routine's minutes count down */
}

static void show_status(const char *text)
{
    lv_label_set_text(s_status_lbl, text);
    lv_obj_remove_flag(s_status, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(s_page, LV_OBJ_FLAG_HIDDEN);
}

void wb_ui_update(const wb_state_t *st)
{
    if (st != &s_st) {
        s_st = *st;
        s_have_st = true;
    }
    lv_obj_add_flag(s_status, LV_OBJ_FLAG_HIDDEN);
    lv_obj_remove_flag(s_page, LV_OBJ_FLAG_HIDDEN);
    wb_routine_set_times(&s_rt, st->leave_min, st->bed_min);

    struct tm now;
    bool have_now = s_clock && s_clock(&now);
    char text[1024], line[256];
    size_t n = 0;
#define ADD(...) (snprintf(line, sizeof(line), __VA_ARGS__), \
                  n += (size_t)snprintf(text + n, n < sizeof(text) ? sizeof(text) - n : 0, "%s\n", line))

    ADD("Good %s%s%s!", have_now && now.tm_hour >= 12 ? "afternoon" : "morning", st->names[0] ? ", " : "",
        st->names);
    if (st->have_wx && have_now) {
        outfit_opts_t o = {.bike_bus_wday = st->bike_bus_wday, .coat_f = st->coat_f, .jacket_f = st->jacket_f,
                           .hoodie_f = st->hoodie_f, .warm_pm_f = st->warm_pm_f};
        outfit_t out;
        if (outfit_make(&st->wx, &now, 0, &o, &out) && out.ok) {
            ADD("Now %.0f°, high %.0f°, low %.0f°, %d%% chance of rain", out.t_now, out.hi, out.lo, out.pop_day);
            ADD("%s", out.advice);
        }
    } else {
        ADD("Waiting for the weather...");
    }
    if (st->alerts.n > 0) {
        ADD("Heads up: %s. %s", st->alerts.a[0].title, st->alerts.a[0].kid);
    }
    if (have_now) {
        char cd[96];
        int today = cal_dayno(now.tm_year + 1900, now.tm_mon + 1, now.tm_mday);
        if (wb_countdown_text(st->events, st->nevents, today, false, cd, sizeof(cd))) {
            ADD("%s", cd);
        }
        wb_routine_view_t v;
        wb_routine_view(&s_rt, &now, &v);
        ADD("%s: %s%s%s", v.title, v.left_text, v.next >= 0 ? ". Next: " : "", v.next >= 0 ? v.word[v.next] : "");
    }
#undef ADD
    lv_label_set_text(s_lines, text);
    lv_label_set_text(s_footer, st->status[0] ? st->status : st->place);
}

void wb_ui_show_boot(const char *msg)
{
    show_status(msg ? msg : "Weather Besties is starting...");
}

void wb_ui_show_setup(const char *ap_name, const char *password, const char *url)
{
    char b[256];
    snprintf(b, sizeof(b), "Set me up!\n\nJoin the WiFi \"%s\" (password %s)\nthen open http://%s", ap_name, password,
             url);
    show_status(b);
}

void wb_ui_show_connecting(const char *ssid)
{
    char b[96];
    snprintf(b, sizeof(b), "Connecting to %s...", ssid);
    show_status(b);
}

void wb_ui_show_wifi_error(const char *ssid, const char *detail)
{
    char b[192];
    snprintf(b, sizeof(b), "Can't reach %s.\n%s", ssid, detail ? detail : "Check the WiFi, or tap Setup to change it.");
    show_status(b);
}
