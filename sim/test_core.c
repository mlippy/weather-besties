/* Tests for the screen logic in components/wb_core (countdown, dress-up days, routine).
 * Run: make test */
#include <stdio.h>
#include <string.h>

#include "cal.h"
#include "countdown.h"
#include "dress.h"
#include "routine.h"

#include "test_check.h"

static struct tm at(int y, int mo, int d, int h, int mi)
{
    struct tm t = {0};
    t.tm_year = y - 1900;
    t.tm_mon = mo - 1;
    t.tm_mday = d;
    t.tm_wday = cal_wday(cal_dayno(y, mo, d));
    t.tm_hour = h;
    t.tm_min = mi;
    return t;
}

static void test_countdown(void)
{
    wb_event_t ev[4];
    memset(ev, 0, sizeof(ev));
    snprintf(ev[0].name, sizeof(ev[0].name), "Trip");
    ev[0].year = 2026, ev[0].month = 11, ev[0].day = 7, ev[0].countdown = true;
    snprintf(ev[1].name, sizeof(ev[1].name), "Party");
    ev[1].year = 2026, ev[1].month = 10, ev[1].day = 28, ev[1].countdown = true;
    snprintf(ev[2].name, sizeof(ev[2].name), "Swim");   /* weekly: never a countdown */
    ev[2].wdays = 0x40, ev[2].countdown = true;
    snprintf(ev[3].name, sizeof(ev[3].name), "Dentist");   /* not ticked */
    ev[3].year = 2026, ev[3].month = 10, ev[3].day = 27;

    char buf[96];
    int today = cal_dayno(2026, 10, 27);
    CHECK(wb_countdown_text(ev, 4, today, false, buf, sizeof(buf)) && !strcmp(buf, "1 more sleep until Party!"),
          "nearest: %s", buf);
    CHECK(wb_countdown_text(ev, 4, today + 1, false, buf, sizeof(buf)) && !strcmp(buf, "Party is today!"), "today: %s",
          buf);
    CHECK(wb_countdown_text(ev, 4, today + 2, true, buf, sizeof(buf)) && !strcmp(buf, "9 sleeps until Trip!"),
          "short form: %s", buf);
    CHECK(!wb_countdown_text(ev, 4, today + 12, false, buf, sizeof(buf)), "all past: %s", buf);
    CHECK(!wb_countdown_text(ev, 1, today - CAL_COUNTDOWN_DAYS - 5, false, buf, sizeof(buf)), "too far off: %s", buf);
}

static void test_dress(void)
{
    wb_dress_t d;
    memset(&d, 0, sizeof(d));
    int cal[] = {100, 103};
    wb_dress_set_calendar(&d, cal, 2);
    CHECK(wb_dress_on(&d, 100) && !wb_dress_on(&d, 101), "calendar days");
    wb_dress_toggle(&d, 100);
    CHECK(!wb_dress_on(&d, 100), "a tap beats the calendar");
    wb_dress_toggle(&d, 101);
    CHECK(wb_dress_on(&d, 101), "a tap turns a plain day on");
    wb_dress_toggle(&d, 101);
    CHECK(!wb_dress_on(&d, 101) && d.ntap == 2, "a second tap turns it back off, same slot (%d taps)", d.ntap);
    for (int i = 0; i < WB_DRESS_MAX + 2; i++) {
        wb_dress_toggle(&d, 200 + i);
    }
    CHECK(d.ntap == WB_DRESS_MAX && wb_dress_on(&d, 100) && wb_dress_on(&d, 200 + WB_DRESS_MAX + 1),
          "full: the oldest taps are forgotten");
    wb_dress_set_calendar(&d, NULL, 0);
    CHECK(!wb_dress_on(&d, 103), "calendar cleared");
}

static void test_routine(void)
{
    wb_routine_t r;
    wb_routine_view_t v;
    wb_routine_init(&r);

    struct tm tue = at(2026, 10, 27, 7, 55);   /* 35 minutes before 8:30 */
    wb_routine_view(&r, &tue, &v);
    CHECK(v.mode == WB_ROUTINE_MORNING && v.nsteps == 5 && v.next == 0, "morning list");
    CHECK(v.step[1] == WB_STEP_BREAKFAST && !strcmp(v.word[1], "Breakfast"), "steps and words");
    CHECK(!strcmp(v.title, "Morning routine") && v.show_bar, "title: %s", v.title);
    CHECK(!strcmp(v.left_text, "35 minutes until we leave"), "left: %s", v.left_text);
    CHECK(!strcmp(v.when_text, "Leave at 8:30 AM"), "when: %s", v.when_text);
    CHECK(v.bar_gone_min == 25 && v.bar_color == 0x4CB860, "bar: %d %06X", v.bar_gone_min, (unsigned)v.bar_color);

    for (int i = 0; i < 5; i++) {
        wb_routine_toggle(&r, &tue, i);
    }
    wb_routine_toggle(&r, &tue, 2);
    wb_routine_view(&r, &tue, &v);
    CHECK(v.next == 2 && !v.done[2] && v.done[3], "untick: next is %d", v.next);
    wb_routine_toggle(&r, &tue, 2);
    tue.tm_min = 27, tue.tm_hour = 8;
    wb_routine_view(&r, &tue, &v);
    CHECK(v.next == -1 && !strcmp(v.title, "Ready for school!"), "all done: %s", v.title);
    CHECK(v.bar_color == 0xE5483C && v.bar_gone_min == 57, "red near the time");
    tue.tm_hour = 8, tue.tm_min = 45;
    wb_routine_view(&r, &tue, &v);
    CHECK(!strcmp(v.left_text, "Time to go!") && v.bar_gone_min == WB_ROUTINE_WINDOW_MIN, "past: %s", v.left_text);
    tue.tm_hour = 9, tue.tm_min = 40;
    wb_routine_view(&r, &tue, &v);
    CHECK(!strcmp(v.left_text, "Have a great day!"), "long past: %s", v.left_text);

    struct tm wed = at(2026, 10, 28, 7, 0);
    wb_routine_view(&r, &wed, &v);
    CHECK(v.next == 0 && !v.done[4] && v.bar_gone_min == 0, "a new day starts unchecked");

    struct tm sat = at(2026, 10, 31, 9, 0);
    wb_routine_view(&r, &sat, &v);
    CHECK(!v.show_bar && !strcmp(v.left_text, "No school today!") && !v.when_text[0], "weekend morning");
    for (int i = 0; i < 5; i++) {
        wb_routine_toggle(&r, &sat, i);
    }
    wb_routine_view(&r, &sat, &v);
    CHECK(!strcmp(v.title, "All done!"), "weekend all done: %s", v.title);

    wb_routine_set_times(&r, 8 * 60, 19 * 60 + 30);
    struct tm night = at(2026, 10, 31, 19, 10);
    wb_routine_view(&r, &night, &v);
    CHECK(v.mode == WB_ROUTINE_BEDTIME && v.nsteps == 6 && v.step[5] == WB_STEP_SNUGGLE, "bedtime list");
    CHECK(v.show_bar && !strcmp(v.when_text, "Bed at 7:30 PM"), "bed time: %s", v.when_text);
    CHECK(!strcmp(v.left_text, "20 minutes until bedtime") && v.bar_color == 0xF2C230, "bedtime left: %s",
          v.left_text);
    wb_routine_set_times(&r, 0, 24 * 60);   /* out of range: kept */
    CHECK(r.leave_min == 8 * 60 && r.bed_min == 19 * 60 + 30, "bad times ignored");
}

int main(void)
{
    test_countdown();
    test_dress();
    test_routine();
    if (fails) {
        printf("%d core test(s) failed\n", fails);
        return 1;
    }
    printf("All core tests passed.\n");
    return 0;
}
