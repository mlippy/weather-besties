#include "events.h"

bool event_on(const wb_event_t *e, int year, int month, int day, int wday)
{
    if (e->wdays) {
        return wday >= 0 && wday <= 6 && ((e->wdays >> wday) & 1);
    }
    return e->year == year && e->month == month && e->day == day;
}
