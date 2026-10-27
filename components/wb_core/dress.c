#include "dress.h"

#include <string.h>

void wb_dress_set_calendar(wb_dress_t *d, const int *days, int n)
{
    d->ncal = 0;
    for (int i = 0; i < n && d->ncal < WB_DRESS_MAX; i++) {
        d->cal[d->ncal++] = days[i];
    }
}

bool wb_dress_on(const wb_dress_t *d, int day)
{
    for (int i = d->ntap - 1; i >= 0; i--) {
        if (d->tap[i].day == day) {
            return d->tap[i].on;   /* a tap beats the calendar */
        }
    }
    for (int i = 0; i < d->ncal; i++) {
        if (d->cal[i] == day) {
            return true;
        }
    }
    return false;
}

void wb_dress_toggle(wb_dress_t *d, int day)
{
    bool on = !wb_dress_on(d, day);
    int i = 0;
    while (i < d->ntap && d->tap[i].day != day) {
        i++;
    }
    if (i == d->ntap) {
        if (d->ntap == WB_DRESS_MAX) {   /* forget the oldest */
            memmove(&d->tap[0], &d->tap[1], sizeof(d->tap[0]) * (WB_DRESS_MAX - 1));
            d->ntap--;
            i = d->ntap;
        }
        d->ntap++;
    }
    d->tap[i].day = day;
    d->tap[i].on = on;
}
