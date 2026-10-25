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

void ui_set_cat_choice(int choice)
{
    (void)choice;   /* the classic screens have their own two cats */
}

void ui_set_cat_cb(ui_cat_cb_t cb)
{
    (void)cb;
}
