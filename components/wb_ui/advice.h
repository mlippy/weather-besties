/* "What should I wear?" logic. Plain C, shared with the desktop renderer. */
#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "wx.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Same order as icons.CLOTHES in tools/icons.py. */
typedef enum {
    WEAR_TSHIRT, WEAR_SWEATER, WEAR_JACKET, WEAR_COAT, WEAR_BEANIE, WEAR_MITTENS,
    WEAR_UMBRELLA, WEAR_BOOTS, WEAR_SUNGLASSES, WEAR_WATER, WEAR_COUNT
} wear_t;

#define ADVICE_MAX_WEAR 3
#define ADVICE_MAX_EVENTS 2  /* events mentioned in one tip */

/* Outdoor events: at or below this feels-like at the event's start, remind about gloves. */
#define EVENT_COLD_F 50.0f
/* At or above this, remind about water. */
#define EVENT_HOT_F 85.0f
/* Rain chance at the event's start that earns an umbrella reminder. */
#define EVENT_RAIN_POP 50
/* A drop this big in the daily high counts as "a lot colder" (and means gloves). */
#define BIG_CHANGE_F 12.0f

/* A special event: something on the calendar the advice should mention ("Soccer", "Field trip").
 * Repeats on weekdays, or happens once on a date. */
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

typedef struct {
    const wb_event_t *events;
    int nevents;
} advice_opts_t;

/* Does the event happen on this date? wday: 0 = Sunday. */
bool event_on(const wb_event_t *e, int year, int month, int day, int wday);

typedef struct {
    char headline[64];   /* "Chilly 48° morning" (absolute) */
    char compare[80];    /* "A lot colder and cloudier than yesterday" (relative, may be empty) */
    char tip[192];       /* "Bring a jacket! Soccer today: wear gloves!" */
    int nwear;
    wear_t wear[ADVICE_MAX_WEAR];
    bool tomorrow;       /* advice is about tomorrow (shown in the evening) */
    int nevents;         /* events mentioned in the tip */
} advice_t;

void advice_make(const wx_t *wx, const advice_opts_t *opts, advice_t *out);

/* Compares two days (daily high and hours of sun). ref is "yesterday", "today", "Sat"...
 * Returns false when there's nothing to compare against. */
bool advice_compare(const wx_t *wx, const wx_day_t *prev, const wx_day_t *cur, const char *ref, char *buf,
                    size_t len, float *dtemp_f);

/* One short line for a day in the forecast page, e.g. "Bring an umbrella". */
void advice_day_line(const wx_t *wx, int day_idx, char *buf, size_t len);

/* Compact change from the day before, e.g. "6° warmer than Sat". */
void advice_day_trend(const wx_t *wx, int day_idx, char *buf, size_t len);

const char *wear_label(wear_t w);

#ifdef __cplusplus
}
#endif
