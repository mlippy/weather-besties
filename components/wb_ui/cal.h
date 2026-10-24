/*
 * Reads an iCalendar (.ics) feed, like a Google Calendar "secret address in iCal
 * format", a chunk at a time, and keeps the few events the screen cares about.
 * Plain C, shared with the desktop renderer and its tests.
 *
 * Handles one-off, all-day (also multi-day) and repeating events (DAILY, WEEKLY,
 * MONTHLY, YEARLY with INTERVAL, COUNT, UNTIL, BYDAY like 2TU or -1FR, BYMONTHDAY,
 * BYMONTH), EXDATE, moved or cancelled single instances (RECURRENCE-ID), and
 * cancelled events. Times with a TZID are taken as local to the weather location;
 * UTC times are shifted by its UTC offset.
 *
 * Title tags (removed from the shown name):
 *   "(outside)" or a sun emoji      weather reminders apply
 *   "(countdown)", party or cake    counts the sleeps until it
 */
#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "advice.h"

#ifdef __cplusplus
extern "C" {
#endif

#define CAL_MAX 24          /* occurrences kept */
#define CAL_NAME_LEN 32
#define CAL_COUNTDOWN_DAYS 60
#define CAL_OVERRIDES 48

typedef struct {
    char name[CAL_NAME_LEN];
    int dayno;               /* days since 1970-01-01, local */
    int minute;              /* start, minutes after local midnight; -1 = all day */
    bool outside;
    bool countdown;
    uint32_t uid;            /* internal */
    bool is_override;        /* internal */
} cal_item_t;

/* One VEVENT while it's being read (internal). */
typedef struct {
    char summary[96];
    bool has_start, start_allday, has_end, cancelled, has_recur;
    int start_day, start_min, end_day, end_min;
    char freq;               /* 0, 'D', 'W', 'M', 'Y' */
    int interval, count, until_day;
    int nbyday;
    int8_t byday_ord[8];     /* 0 = every; 2 = second; -1 = last */
    int8_t byday_wd[8];      /* 0 = Sunday */
    int nbymonthday;
    int8_t bymonthday[8];
    uint16_t bymonth;        /* bit 1 = January */
    int nex;
    int exdates[16];
    uint32_t uid;
    int recur_day;
} cal_ev_t;

typedef struct {
    /* settings, set before feeding */
    int today;               /* dayno of today, local */
    int utc_offset_s;        /* local = UTC + this */
    char words[64];          /* comma-separated; keep only titles with one of these ("Piper, Family").
                                Empty keeps everything. */

    /* results */
    int n;
    cal_item_t items[CAL_MAX];   /* sorted by day, then time */
    int nevents_seen;

    /* parser state (internal) */
    char line[600];
    size_t llen;
    bool eol, in_event, seen;
    int depth_other;         /* inside VALARM and friends */
    cal_ev_t ev;
    int nover;
    struct { uint32_t uid; int dayno; } over[CAL_OVERRIDES];
} cal_t;

void cal_begin(cal_t *c, int today, int utc_offset_s, const char *words);
void cal_feed(cal_t *c, const char *data, size_t len);
/* Returns false if no calendar was seen in the data. */
bool cal_end(cal_t *c);

/* Appends the calendar's events to an event list for the advice. Returns the new count. */
int cal_to_events(const cal_t *c, wb_event_t *out, int n, int max);

/* Day numbers <-> dates (proleptic Gregorian). wday: 0 = Sunday. */
int cal_dayno(int y, int m, int d);
void cal_date(int dayno, int *y, int *m, int *d);
int cal_wday(int dayno);

#ifdef __cplusplus
}
#endif
