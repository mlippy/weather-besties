#include "routine.h"

#include <stdio.h>
#include <string.h>

#include "cal.h"

static const struct {
    wb_step_t step;
    const char *word;
} MORNING[] = {{WB_STEP_POTTY, "Potty"}, {WB_STEP_BREAKFAST, "Breakfast"}, {WB_STEP_TEETH, "Brush teeth"},
               {WB_STEP_HAIR, "Brush hair"}, {WB_STEP_DRESS, "Get dressed"}},
  BEDTIME[] = {{WB_STEP_POTTY, "Potty"}, {WB_STEP_PAJAMAS, "Pajamas"}, {WB_STEP_SNACK, "Snack"},
               {WB_STEP_TEETH, "Brush teeth"}, {WB_STEP_BOOKS, "Books"}, {WB_STEP_SNUGGLE, "Snuggle"}};

void wb_routine_init(wb_routine_t *r)
{
    memset(r, 0, sizeof(*r));
    r->leave_min = 8 * 60 + 30;
    r->bed_min = 20 * 60;
    r->day[0] = r->day[1] = -1;
}

void wb_routine_set_times(wb_routine_t *r, int leave_min, int bed_min)
{
    if (leave_min > 0 && leave_min < 24 * 60) {
        r->leave_min = leave_min;
    }
    if (bed_min > 0 && bed_min < 24 * 60) {
        r->bed_min = bed_min;
    }
}

static int mode_of(const struct tm *now)
{
    return now->tm_hour < 12 ? WB_ROUTINE_MORNING : WB_ROUTINE_BEDTIME;
}

static void new_day(wb_routine_t *r, const struct tm *now)
{
    int m = mode_of(now);
    int today = cal_dayno(now->tm_year + 1900, now->tm_mon + 1, now->tm_mday);
    if (r->day[m] != today) {
        r->day[m] = today;   /* a new day starts unchecked */
        r->checks[m] = 0;
    }
}

void wb_routine_toggle(wb_routine_t *r, const struct tm *now, int step)
{
    new_day(r, now);
    if (step >= 0 && step < WB_ROUTINE_STEPS) {
        r->checks[mode_of(now)] ^= (uint8_t)(1u << step);
    }
}

static void fmt_clock12(int min, char *buf, size_t n)
{
    int h = min / 60 % 12;
    snprintf(buf, n, "%d:%02d %s", h ? h : 12, min % 60, min < 12 * 60 ? "AM" : "PM");
}

void wb_routine_view(wb_routine_t *r, const struct tm *now, wb_routine_view_t *v)
{
    new_day(r, now);
    memset(v, 0, sizeof(*v));
    int m = mode_of(now);
    v->mode = (wb_routine_mode_t)m;
    v->nsteps = m == 0 ? (int)(sizeof(MORNING) / sizeof(MORNING[0])) : (int)(sizeof(BEDTIME) / sizeof(BEDTIME[0]));
    v->next = -1;
    int done = 0;
    for (int i = 0; i < v->nsteps; i++) {
        v->step[i] = m == 0 ? MORNING[i].step : BEDTIME[i].step;
        v->word[i] = m == 0 ? MORNING[i].word : BEDTIME[i].word;
        v->done[i] = (r->checks[m] >> i) & 1;
        done += v->done[i];
        if (!v->done[i] && v->next < 0) {
            v->next = i;
        }
    }
    bool school = m == 1 || (now->tm_wday >= 1 && now->tm_wday <= 5);
    bool all = done == v->nsteps;
    v->title = all ? (m == 0 ? (school ? "Ready for school!" : "All done!") : "Ready for bed!")
                   : (m == 0 ? "Morning routine" : "Bedtime routine");
    v->target_min = m == 0 ? r->leave_min : r->bed_min;
    v->minutes_left = v->target_min - (now->tm_hour * 60 + now->tm_min);
    v->show_bar = school;
    if (!school) {
        snprintf(v->left_text, sizeof(v->left_text), "No school today!");
        return;
    }
    char t[16];
    fmt_clock12(v->target_min, t, sizeof(t));
    snprintf(v->when_text, sizeof(v->when_text), "%s %s", m == 0 ? "Leave at" : "Bed at", t);
    int left = v->minutes_left;
    const char *what = m == 0 ? "until we leave" : "until bedtime";
    if (left > 0) {
        snprintf(v->left_text, sizeof(v->left_text), "%d minute%s %s", left, left == 1 ? "" : "s", what);
    } else if (left > -WB_ROUTINE_WINDOW_MIN) {
        snprintf(v->left_text, sizeof(v->left_text), "%s", m == 0 ? "Time to go!" : "Bedtime!");
    } else {
        snprintf(v->left_text, sizeof(v->left_text), "%s", m == 0 ? "Have a great day!" : "Sweet dreams!");
    }
    int gone = left >= WB_ROUTINE_WINDOW_MIN ? 0 : left <= 0 ? WB_ROUTINE_WINDOW_MIN : WB_ROUTINE_WINDOW_MIN - left;
    v->bar_gone_min = gone;
    v->bar_color = left > 30 ? 0x4CB860 : left > 15 ? 0xF2C230 : left > 5 ? 0xF28C28 : 0xE5483C;
}
