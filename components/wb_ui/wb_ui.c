/*
 * The contract (wb_ui.h) on top of the setters both designs already implement (ui.h).
 * Shared by 2.0 and classic: a design only has to provide ui.h's functions, and this file
 * maps the controller's state onto them, calling each setter only when its part changed.
 */
#include "wb_ui.h"

#include <string.h>

#include "ui.h"

_Static_assert((int)UI_ACT_SETUP == (int)WB_ACT_SETUP && (int)UI_ACT_REFRESH == (int)WB_ACT_REFRESH &&
                   (int)UI_ACT_UPDATE == (int)WB_ACT_UPDATE,
               "on_action passes ui.h's actions straight through as wb_action_t");

static wb_action_cb_t s_cb;
static void *s_user;

/* The parts of the state the setters below compare: what the screens were last given.
 * Only these, not the whole ~4 KB state. */
#define SEEN_FIELDS(X)                                                                             \
    X(names) X(pet_name) X(pet) X(brightness) X(leave_min) X(bed_min) X(coat_f) X(jacket_f)      \
    X(hoodie_f) X(warm_pm_f) X(bike_bus_wday) X(ndress) X(dress_days) X(cal_configured)            \
    X(cal_reads) X(cal_fails)
#define SEEN_DECL(f) __typeof__(((wb_state_t *)0)->f) f;
#define SEEN_COPY(f) memcpy(&s_last.f, &st->f, sizeof(st->f));
static struct {
    SEEN_FIELDS(SEEN_DECL)
} s_last;
static bool s_have_last;

static void on_action(ui_action_t a, void *user)
{
    (void)user;
    if (s_cb) {
        s_cb((wb_action_t)a, 0, s_user);   /* UI_ACT_SETUP/REFRESH/UPDATE line up with WB_ACT_* */
    }
}

static void on_pet(int choice)
{
    if (s_cb) {
        s_cb(WB_ACT_SET_PET, choice, s_user);
    }
}

static void on_brightness(int pct)
{
    if (s_cb) {
        s_cb(WB_ACT_SET_BRIGHTNESS, pct, s_user);
    }
}

void wb_ui_start(wb_action_cb_t on_act, void *user, wb_clock_cb_t clock)
{
    s_cb = on_act;
    s_user = user;
    s_have_last = false;
    ui_init(NULL);
    ui_set_action_cb(on_action, NULL);
    ui_set_cat_cb(on_pet);
    ui_set_brightness_cb(on_brightness);   /* 2.0: the Brightness slider in Settings */
    ui_set_time_cb(clock);
    ui_set_dress_days(NULL, 0);            /* 2.0: the dress on dress-up days only */
}

#define CHANGED(field) (!s_have_last || memcmp(&st->field, &s_last.field, sizeof(st->field)) != 0)

void wb_ui_update(const wb_state_t *st)
{
    /* Settings first, so the render that ui_show_weather() starts uses them. */
    if (CHANGED(names)) {
        ui_set_names(st->names);
    }
    if (CHANGED(pet_name)) {
        ui_set_pet_name(st->pet_name);
    }
    if (CHANGED(pet)) {
        ui_set_cat_choice((int)st->pet);
    }
    if (CHANGED(brightness)) {
        ui_set_brightness(st->brightness);
    }
    if (CHANGED(leave_min) || CHANGED(bed_min)) {
        ui_set_routine_times(st->leave_min, st->bed_min);
    }
    if (CHANGED(coat_f) || CHANGED(jacket_f) || CHANGED(hoodie_f) || CHANGED(warm_pm_f)) {
        ui_set_outfit_temps(st->coat_f, st->jacket_f, st->hoodie_f, st->warm_pm_f);
    }
    if (CHANGED(bike_bus_wday)) {
        ui_set_bike_bus_day(st->bike_bus_wday);
    }
    if (CHANGED(ndress) || CHANGED(dress_days)) {
        ui_set_dress_days(st->dress_days, st->ndress);
    }
    if (CHANGED(cal_configured)) {
        ui_set_calendar_configured(st->cal_configured);
    }
    if (s_have_last && st->cal_reads != s_last.cal_reads) {
        ui_show_calendar();          /* a fresh read: My Day records the time */
    }
    if (s_have_last && st->cal_fails != s_last.cal_fails) {
        ui_calendar_fetch_failed();  /* My Day marks what it shows as old */
    }

    /* The day's data: cheap to hand over every time. */
    ui_set_events(st->events, st->nevents);
    ui_set_alerts(&st->alerts);
    ui_info_t info = {0};
    memcpy(info.wifi, st->wifi, sizeof(info.wifi));
    memcpy(info.place, st->place, sizeof(info.place));
    memcpy(info.ip, st->ip, sizeof(info.ip));
    memcpy(info.updated, st->updated, sizeof(info.updated));
    memcpy(info.version, st->version, sizeof(info.version));
    ui_set_info(&info);
    ui_set_footer(st->place, st->status, st->status_warn);
    if (st->have_wx) {
        ui_show_weather(&st->wx);    /* last: it draws the page */
    }

    SEEN_FIELDS(SEEN_COPY)
    s_have_last = true;
}

void wb_ui_show_boot(const char *msg)
{
    ui_show_boot(msg);
}

void wb_ui_show_setup(const char *ap_name, const char *password, const char *url)
{
    ui_show_setup(ap_name, password, url);
}

void wb_ui_show_connecting(const char *ssid)
{
    ui_show_connecting(ssid);
}

void wb_ui_show_wifi_error(const char *ssid, const char *detail)
{
    ui_show_wifi_error(ssid, detail);
}
