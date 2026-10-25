/* Tests for components/wb_ui/cal.c. Run: make test */
#include <stdio.h>
#include <string.h>

#include "cal.h"

static int fails;
#define CHECK(cond, ...)                         \
    do {                                         \
        if (!(cond)) {                           \
            printf("FAIL %s:%d: ", __FILE__, __LINE__); \
            printf(__VA_ARGS__);                 \
            printf("\n");                        \
            fails++;                             \
        }                                        \
    } while (0)

/* Today is Saturday 2026-10-24 in New Jersey (UTC-4). */
static const char *ICS =
    "BEGIN:VCALENDAR\r\nVERSION:2.0\r\nX-WR-CALNAME:Family\r\n"
    /* weekly soccer, Saturdays 11 AM, outdoors via the sun emoji */
    "BEGIN:VEVENT\r\nDTSTART;TZID=America/New_York:20260905T110000\r\nDTEND;TZID=America/New_York:20260905T120000\r\n"
    "RRULE:FREQ=WEEKLY;BYDAY=SA\r\nUID:soccer@x\r\nSUMMARY:Kid soccer \xE2\x98\x80\xEF\xB8\x8F\r\n"
    "BEGIN:VALARM\r\nACTION:DISPLAY\r\nDESCRIPTION:Reminder\r\nTRIGGER:-PT30M\r\nEND:VALARM\r\nEND:VEVENT\r\n"
    /* grown-up event, filtered out */
    "BEGIN:VEVENT\r\nDTSTART:20261024T140000Z\r\nUID:dentist@x\r\nSUMMARY:Dad dentist\r\nEND:VEVENT\r\n"
    /* UTC time, 9 PM UTC = 5 PM local, Sunday; folded summary line */
    "BEGIN:VEVENT\r\nDTSTART:20261025T210000Z\r\nUID:party@x\r\nSUMMARY:Family pizza\r\n  night\r\nEND:VEVENT\r\n"
    /* birthday, yearly, countdown via (countdown) */
    "BEGIN:VEVENT\r\nDTSTART;VALUE=DATE:20190312\r\nDTEND;VALUE=DATE:20190313\r\nRRULE:FREQ=YEARLY\r\n"
    "UID:bday@x\r\nSUMMARY:Grandma's birthday (countdown)\r\nEND:VEVENT\r\n"
    "BEGIN:VEVENT\r\nDTSTART;VALUE=DATE:20181120\r\nRRULE:FREQ=YEARLY\r\nUID:pb@x\r\n"
    "SUMMARY:Kid\xE2\x80\x99s birthday \xF0\x9F\x8E\x82\r\nEND:VEVENT\r\n"
    /* library every Sunday, but not this one (EXDATE) */
    "BEGIN:VEVENT\r\nDTSTART;TZID=America/New_York:20260104T100000\r\nRRULE:FREQ=WEEKLY;BYDAY=SU\r\n"
    "EXDATE;TZID=America/New_York:20261025T100000\r\nUID:lib@x\r\nSUMMARY:Kid: Library\r\nEND:VEVENT\r\n"
    /* swim Sat + Sun, this Saturday moved to 3 PM, this Sunday cancelled */
    "BEGIN:VEVENT\r\nDTSTART;TZID=America/New_York:20260101T090000\r\nRRULE:FREQ=WEEKLY;BYDAY=SA,SU;UNTIL=20271231T000000Z\r\n"
    "UID:swim@x\r\nSUMMARY:Kid swim\r\nEND:VEVENT\r\n"
    "BEGIN:VEVENT\r\nRECURRENCE-ID;TZID=America/New_York:20261024T090000\r\nDTSTART;TZID=America/New_York:20261024T150000\r\n"
    "UID:swim@x\r\nSUMMARY:Kid swim\r\nEND:VEVENT\r\n"
    "BEGIN:VEVENT\r\nRECURRENCE-ID;TZID=America/New_York:20261025T090000\r\nDTSTART;TZID=America/New_York:20261025T090000\r\n"
    "UID:swim@x\r\nSTATUS:CANCELLED\r\nSUMMARY:Kid swim\r\nEND:VEVENT\r\n"
    /* all-day trip over the weekend, multi-day */
    "BEGIN:VEVENT\r\nDTSTART;VALUE=DATE:20261023\r\nDTEND;VALUE=DATE:20261027\r\nUID:trip@x\r\n"
    "SUMMARY:Family trip to Maine\r\nEND:VEVENT\r\n"
    /* scouts: second Sunday of the month (not this month's 25th) */
    "BEGIN:VEVENT\r\nDTSTART;TZID=America/New_York:20260111T130000\r\nRRULE:FREQ=MONTHLY;BYDAY=2SU\r\n"
    "UID:scouts@x\r\nSUMMARY:Kid scouts\r\nEND:VEVENT\r\n"
    /* finished series (COUNT) */
    "BEGIN:VEVENT\r\nDTSTART;TZID=America/New_York:20260103T100000\r\nRRULE:FREQ=WEEKLY;COUNT=3\r\n"
    "UID:old@x\r\nSUMMARY:Kid art class\r\nEND:VEVENT\r\n"
    /* last Friday of the month... not this weekend */
    "BEGIN:VEVENT\r\nDTSTART;VALUE=DATE:20260130\r\nRRULE:FREQ=MONTHLY;BYDAY=-1FR\r\nUID:pj@x\r\n"
    "SUMMARY:Kid PJ day\r\nEND:VEVENT\r\n"
    /* dress-up days: a tag needs no filter word; a plain event isn't one */
    "BEGIN:VEVENT\r\nDTSTART;VALUE=DATE:20261025\r\nUID:pic@x\r\nSUMMARY:Picture day (dress)\r\nEND:VEVENT\r\n"
    "BEGIN:VEVENT\r\nDTSTART;VALUE=DATE:20261031\r\nUID:party@x\r\nSUMMARY:Party \xF0\x9F\x91\x97\r\nEND:VEVENT\r\n"
    /* a countdown later this year */
    "BEGIN:VEVENT\r\nDTSTART;VALUE=DATE:20261126\r\nUID:tg@x\r\nSUMMARY:Thanksgiving at Grandma's \xF0\x9F\x8E\x89\r\nEND:VEVENT\r\n"
    "END:VCALENDAR\r\n";

static const cal_item_t *find(const cal_t *c, const char *name, int day)
{
    for (int i = 0; i < c->n; i++) {
        if (!strcmp(c->items[i].name, name) && (day < 0 || c->items[i].dayno == day)) {
            return &c->items[i];
        }
    }
    return NULL;
}

static void run(size_t chunk)
{
    cal_t c;
    int today = cal_dayno(2026, 10, 24);
    cal_begin(&c, today, -4 * 3600, "Kid, Family");
    size_t len = strlen(ICS);
    for (size_t i = 0; i < len; i += chunk) {
        cal_feed(&c, ICS + i, len - i < chunk ? len - i : chunk);
    }
    CHECK(cal_end(&c), "calendar seen");
    if (chunk == 4096) {
        for (int i = 0; i < c.n; i++) {
            int y, m, d;
            cal_date(c.items[i].dayno, &y, &m, &d);
            printf("  %04d-%02d-%02d %5d  %-22s%s%s\n", y, m, d, c.items[i].minute, c.items[i].name,
                   c.items[i].outside ? " outside" : "", c.items[i].countdown ? " countdown" : "");
        }
    }
    const cal_item_t *it;
    it = find(&c, "Soccer", today);
    CHECK(it && it->minute == 11 * 60 && it->outside, "soccer today 11:00 outside (chunk %zu)", chunk);
    CHECK(!find(&c, "Dad dentist", -1), "dentist filtered");
    it = find(&c, "Pizza night", today + 1);
    CHECK(it && it->minute == 17 * 60, "pizza Sunday 17:00 from UTC, folded line");
    CHECK(!find(&c, "Grandma's birthday", -1), "Grandma's birthday (March) is more than 60 days out");
    CHECK(!find(&c, "Library", today + 1), "library EXDATE");
    it = find(&c, "Swim", today);
    CHECK(it && it->minute == 15 * 60, "swim moved to 3 PM");
    int swims = 0;
    for (int i = 0; i < c.n; i++) {
        swims += !strcmp(c.items[i].name, "Swim");
    }
    CHECK(swims == 1, "swim: only the moved one (got %d)", swims);
    CHECK(find(&c, "Trip to Maine", today) && find(&c, "Trip to Maine", today + 1), "trip both days");
    CHECK(!find(&c, "Scouts", -1), "scouts is the 2nd Sunday");
    CHECK(!find(&c, "Art class", -1), "art class ended");
    CHECK(!find(&c, "PJ day", -1), "PJ day is the last Friday");
    it = find(&c, "Thanksgiving at Grandma's", cal_dayno(2026, 11, 26));
    CHECK(it && it->countdown && it->minute == -1, "thanksgiving countdown, tagged so no filter word needed");
    it = find(&c, "Kid's birthday", cal_dayno(2026, 11, 20));
    CHECK(it && it->countdown, "Kid's birthday, yearly, curly apostrophe, cake tag");
    it = find(&c, "Picture day", today + 1);
    CHECK(it && it->dress, "picture day tagged (dress), kept without a filter word, tag removed");
    int dd[8];
    int nd = cal_dress_days(&c, dd, 8);
    CHECK(nd == 1 && dd[0] == today + 1, "one dress-up day, tomorrow (the party is a week out): %d", nd);
}

int main(void)
{
    CHECK(cal_wday(cal_dayno(2026, 10, 24)) == 6, "Oct 24 2026 is a Saturday");
    int y, m, d;
    cal_date(cal_dayno(2028, 2, 29), &y, &m, &d);
    CHECK(y == 2028 && m == 2 && d == 29, "round trip");
    run(4096);
    run(1);
    run(7);
    printf(fails ? "%d FAILED\n" : "all calendar tests passed\n", fails);
    return fails != 0;
}
