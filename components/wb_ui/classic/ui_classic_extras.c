/* Parts of the shared screen interface (ui.h) that only the 2.0 screens use. */
#include "ui.h"

void ui_set_speed_cb(ui_speed_cb_t cb)
{
    (void)cb;   /* no speed meter on the classic screens */
}

void ui_set_bike_bus_day(int wday)
{
    (void)wday;   /* special events (events.h) cover it */
}

void ui_set_cat_cb(ui_cat_cb_t cb)
{
    (void)cb;   /* the pet is chosen on the setup page */
}

/* The 2.0 My Day page's calendar hooks: the classic screens show calendar events in the
 * speech bubble and the day cards instead (net.c hands them over with ui_set_events). */
void ui_show_calendar(void)
{
}

void ui_calendar_fetch_failed(void)
{
}

void ui_set_calendar_configured(bool on)
{
    (void)on;
}

void ui_set_dress_days(const int *daynos, int n)
{
    (void)daynos;   /* the classic screens have no dress look */
    (void)n;
}

void ui_set_routine_times(int leave_min, int bed_min)
{
    (void)leave_min;   /* the Routine page is a 2.0 screen */
    (void)bed_min;
}

void ui_set_outfit_temps(int coat_f, int jacket_f, int hoodie_f, int warm_pm_f)
{
    (void)warm_pm_f;
    (void)coat_f;   /* the classic screens use the BANDS table in advice.c */
    (void)jacket_f;
    (void)hoodie_f;
}

void ui_set_brightness(int pct)
{
    (void)pct;   /* the classic screens keep full brightness */
}

void ui_set_brightness_cb(ui_brightness_cb_t cb)
{
    (void)cb;
}
