#include "countdown.h"

#include <stdio.h>

#include "cal.h"

bool wb_countdown_text(const wb_event_t *events, int n, int today, bool short_form, char *buf, size_t cap)
{
    const wb_event_t *best = NULL;
    int best_d = CAL_COUNTDOWN_DAYS + 1;
    for (int i = 0; i < n; i++) {
        const wb_event_t *e = &events[i];
        if (!e->countdown || e->wdays || !e->year) {
            continue;   /* one-date countdown events only */
        }
        int d = cal_dayno(e->year, e->month, e->day) - today;
        if (d >= 0 && d < best_d) {
            best = e;
            best_d = d;
        }
    }
    if (!best) {
        return false;
    }
    if (best_d == 0) {
        snprintf(buf, cap, "%s is today!", best->name);
    } else {
        snprintf(buf, cap, "%d %ssleep%s until %s!", best_d, short_form ? "" : "more ", best_d == 1 ? "" : "s",
                 best->name);
    }
    return true;
}
