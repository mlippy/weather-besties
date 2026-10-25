/*
 * Pieces of the shared screen interface (ui.h) that the 2.0 screens (ui.c) don't
 * show yet. Kept in their own file so ui.c stays as Boris wrote it.
 */
#include "ui.h"

void ui_set_events(const wb_event_t *events, int n)
{
    (void)events;
    (void)n;   /* 2.0 doesn't show special events or calendar reminders yet */
}

void ui_set_alerts(const wx_alerts_t *alerts)
{
    (void)alerts;   /* 2.0 doesn't show weather alerts yet */
}
