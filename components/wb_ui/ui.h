/*
 * The parts each screen design implements (2.0: ui.c + ui_v2_extras.c; classic:
 * classic/ui.c + ui_classic_extras.c). The controller doesn't call these: it uses the
 * contract in wb_ui.h, and wb_ui.c maps that onto them. The desktop renderer calls them
 * directly to set up its scenes. Pure LVGL; call from the LVGL thread.
 */
#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <time.h>

#include "alerts.h"
#include "events.h"
#include "wx.h"

#ifdef __cplusplus
extern "C" {
#endif

#define UI_W 800
#define UI_H 480

typedef enum {
    UI_ACT_SETUP,     /* user asked to change WiFi / town */
    UI_ACT_REFRESH,   /* user asked for fresh weather now */
    UI_ACT_UPDATE,    /* user asked to check for a firmware update now */
} ui_action_t;

typedef void (*ui_action_cb_t)(ui_action_t act, void *user);

/* Fills local time. Return false while the clock isn't set yet. */
typedef bool (*ui_time_cb_t)(struct tm *local_out);

typedef struct {
    char wifi[40];
    char place[64];
    char ip[20];
    char updated[48];
    char version[24];
} ui_info_t;

/* Speed test (tap "Updated ..." on the screen): called with old_mode when the mode
 * changes and once a second while it's on; fill info with a short readout. */
typedef void (*ui_speed_cb_t)(bool old_mode, char *info, size_t n);
void ui_set_speed_cb(ui_speed_cb_t cb);

/* Screen brightness, 30..100 %. The board's backlight can only switch on and off (it's
 * behind the IO expander), so the screens dim the picture itself: a dark see-through layer
 * over everything. On 2.0, setting the callback swaps Settings' Speed test pill for a
 * Brightness slider; the callback gets the new value when the slider is let go (to save). */
typedef void (*ui_brightness_cb_t)(int pct);
void ui_set_brightness(int pct);
void ui_set_brightness_cb(ui_brightness_cb_t cb);

void ui_init(const char *names);
void ui_set_names(const char *names);
void ui_set_pet_name(const char *name);   /* the dog's name from setup; "" shows "Dog" */
/* Dress-up days from the calendar (day numbers, cal_dayno). Calling this at all switches
 * the 2.0 screens to this fork's rule: the dress only on dress-up days (calendar or the
 * Dress up pill), jeans on other light-jacket days. Without it: Boris's every-mild-day. */
void ui_set_dress_days(const int *daynos, int n);
/* The Routine page's times, minutes after midnight: leaving for school, bedtime. */
void ui_set_routine_times(int leave_min, int bed_min);
/* The 2.0 outfit's jacket cut-offs (feels-like °F at departure): winter kit at or below
 * coat_f, light jacket to jacket_f, hoodie to hoodie_f. 0 keeps Boris's 45 / 57 / 64.
 * warm_pm_f: an afternoon at least this warm makes a coat morning (above freezing) a
 * light-jacket one; 0 = off. */
void ui_set_outfit_temps(int coat_f, int jacket_f, int hoodie_f, int warm_pm_f);
void ui_set_bike_bus_day(int wday);  /* 0 = Sunday ... 6 = Saturday, -1 = none. Default Friday. */
void ui_set_action_cb(ui_action_cb_t cb, void *user);
void ui_set_events(const wb_event_t *events, int n);  /* special events for the advice; copied */
void ui_set_alerts(const wx_alerts_t *alerts);         /* NULL or n == 0 clears; shown on the next ui_show_weather */
void ui_set_time_cb(ui_time_cb_t cb);

void ui_show_boot(const char *msg);
/* password: the setup hotspot's WPA2 password (letters and digits only, for the QR code) */
void ui_show_setup(const char *ap_name, const char *password, const char *url);
void ui_show_connecting(const char *ssid);
void ui_show_wifi_error(const char *ssid, const char *detail);

void ui_show_weather(const wx_t *wx);
void ui_show_calendar(void);   /* fresh kids-calendar data was parsed (cal.c); records the fetch time */
void ui_calendar_fetch_failed(void);   /* a calendar fetch failed: any data on screen is stale (iter-15) */
/* Whether a kids-calendar link is configured at all: the My Day page
 * shows its "ask a grown-up" state when it is not. The firmware sets
 * this from settings (cal_url non-empty); the sim defaults to false
 * and opts in per scenario. */
void ui_set_calendar_configured(bool on);
void ui_set_footer(const char *place, const char *status, bool warn);  /* place: kept for callers, shown in Settings */
void ui_set_info(const ui_info_t *info);
void ui_open_settings(void);
void ui_close_settings(void);
void ui_show_page(int page);   /* 0 = today, 1 = tomorrow, 2 = my day, 3 = next days */
void ui_refresh_clock(void);

/* Cat choice from Settings: 0 = black cat, 1 = pink cat, 2 = take turns. */
typedef void (*ui_cat_cb_t)(int choice);
void ui_set_cat_choice(int choice);
void ui_set_cat_cb(ui_cat_cb_t cb);

#ifdef WB_SIM
/* Desktop-renderer test hooks. Check mask: bits 0..3 = main tiles,
 * bits 4..9 = extras chip slots (ui.c EXTRA_ORDER order). */
void ui_test_set_checks(uint16_t mask);
void ui_test_set_page(int page);
void ui_test_tap_tile(int i);   /* drive the tile toggle path exactly as a tap does */
void ui_test_tap_chip(int j);   /* drive the extras-chip toggle path exactly as a tap does */
void ui_test_set_pippa_frame(int frame);   /* pin Pippa's heart animation to a frame, -1 releases */
void ui_test_dump(const char *path);       /* last outfit render's state as text (see ui.c) */
/* Iter-35 layout gate: walk the visible page's object tree and
 * count violations (sibling overlaps, children outside the parent's
 * content area, label text taller than its box outside DOTS mode,
 * objects in the tab bar area). Prints one line per violation. */
int ui_test_layout_check(const char *tag);
/* Iter-36: outline every visible object of the active screen in
 * 1 px red (and restore the real borders on false), for the
 * debug render of the worst-case My Day fixture. */
void ui_test_outline(bool on);
/* Iter-37: scroll the My Day list to its very bottom, so a
 * render can prove the last card is reachable and lands clear
 * of the tab bar; the layout gate runs on the scrolled state. */
void ui_test_scroll_myday_bottom(void);
#endif

#ifdef __cplusplus
}
#endif
