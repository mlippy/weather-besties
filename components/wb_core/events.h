/*
 * Special events: things on the calendar the advice should mention ("Soccer",
 * "Field trip"), typed on the setup page or read from the family Google Calendar
 * (cal.c). Repeats on weekdays, or happens once on a date. Plain C.
 */
#pragma once

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define EVENT_MAX 8         /* typed on the setup page */
#define EVENT_LIST_MAX 32   /* plus the ones from the calendar */
#define EVENT_NAME_LEN 32

typedef struct {
    char name[EVENT_NAME_LEN];
    uint8_t wdays;       /* bit 0 = Sunday ... bit 6 = Saturday; 0 = one date instead */
    uint16_t year;       /* one-off date, used when wdays == 0 */
    uint8_t month, day;
    uint8_t hour;        /* starts at, local 0..23 */
    bool outside;        /* weather reminders (gloves, umbrella, water) apply */
    bool all_day;        /* no start time: mentioned all day, weather checked at 9 AM */
    bool countdown;      /* one-date events: count the sleeps until it (shown in the footer) */
} wb_event_t;

/* Does the event happen on this date? wday: 0 = Sunday. */
bool event_on(const wb_event_t *e, int year, int month, int day, int wday);

#ifdef __cplusplus
}
#endif
