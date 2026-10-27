/*
 * The Routine page's logic: the morning list before noon and the bedtime list after, the
 * time bar running over the last hour to leaving / bedtime, the words, and the day's
 * ticks. Plain C; a screen only draws what wb_routine_view() says.
 */
#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

#define WB_ROUTINE_STEPS 6
#define WB_ROUTINE_WINDOW_MIN 60   /* the bar starts an hour out and is full at the time */

typedef enum { WB_ROUTINE_MORNING, WB_ROUTINE_BEDTIME } wb_routine_mode_t;

/* What a step is; screens map it to a picture. */
typedef enum {
    WB_STEP_POTTY, WB_STEP_BREAKFAST, WB_STEP_TEETH, WB_STEP_HAIR, WB_STEP_DRESS,
    WB_STEP_PAJAMAS, WB_STEP_SNACK, WB_STEP_BOOKS, WB_STEP_SNUGGLE,
} wb_step_t;

typedef struct {
    int leave_min, bed_min;   /* minutes after midnight */
    uint8_t checks[2];        /* bit i = step i done; [mode] */
    int day[2];               /* the day (cal_dayno) those ticks belong to */
} wb_routine_t;

typedef struct {
    wb_routine_mode_t mode;
    int nsteps;
    wb_step_t step[WB_ROUTINE_STEPS];
    const char *word[WB_ROUTINE_STEPS];   /* "Potty", "Brush teeth"... */
    bool done[WB_ROUTINE_STEPS];
    int next;                 /* the first step not done, -1 = all done */
    const char *title;        /* "Morning routine", "Ready for school!"... */
    bool show_bar;            /* false on a weekend morning */
    int target_min;           /* leaving / bedtime */
    int minutes_left;         /* negative once it's passed */
    int bar_gone_min;         /* 0..WB_ROUTINE_WINDOW_MIN: how much of the last hour has gone;
                                 fill width = bar width * bar_gone_min / WB_ROUTINE_WINDOW_MIN */
    uint32_t bar_color;       /* green, yellow, orange, red as it gets close */
    char left_text[64];       /* "35 minutes until we leave", "Time to go!", "No school today!" */
    char when_text[32];       /* "Leave at 8:30 AM", "Bed at 8:00 PM", "" */
} wb_routine_view_t;

/* "8:30 AM" from minutes after midnight (the Routine times, sunrise, the status line) */
void wb_fmt_clock12(int min, char *buf, size_t n);

void wb_routine_init(wb_routine_t *r);   /* 8:30 AM and 8:00 PM */
void wb_routine_set_times(wb_routine_t *r, int leave_min, int bed_min);
void wb_routine_toggle(wb_routine_t *r, const struct tm *now, int step);
void wb_routine_view(wb_routine_t *r, const struct tm *now, wb_routine_view_t *out);   /* resets a new day's ticks */

#ifdef __cplusplus
}
#endif
