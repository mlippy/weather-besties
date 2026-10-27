/*
 * Desktop renderer: runs the real UI code (ui.c, wx.c, outfit.c, same fonts
 * and images, same lv_conf.h) and saves each screen as a raw RGB565 frame.
 * Usage: ./render <outdir> <scenario dir>
 *
 * A scenario JSON may carry "sim_clock": "2026-10-27T19:30" to set the wall
 * clock independently of the forecast's own time (the plan's stale/rollover
 * cases). Without it the clock is the forecast time + 7 minutes, as before.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "cJSON.h"
#include "cal.h"
#include "alerts.h"
#include "lvgl.h"
#include "ui.h"
#include "ui_v2_extras.h"
#include "wb_ui.h"
#include "wx.h"

static uint16_t fb[UI_W * UI_H];
static uint32_t ms;
static struct tm now_tm;
static bool now_valid;

static uint32_t tick_cb(void) { return ms; }

static void flush_cb(lv_display_t *d, const lv_area_t *a, uint8_t *px)
{
    (void)a;
    (void)px;
    lv_display_flush_ready(d);
}

static bool time_cb(struct tm *out)
{
    if (!now_valid) {
        return false;
    }
    *out = now_tm;
    return true;
}

static void pump(int frames)
{
    for (int i = 0; i < frames; i++) {
        ms += 40;
        lv_timer_handler();
    }
    lv_obj_invalidate(lv_screen_active());
    lv_refr_now(NULL);
}

static void save(const char *dir, const char *name)
{
    pump(30);
    char path[512];
    snprintf(path, sizeof(path), "%s/%s.rgb565", dir, name);
    FILE *f = fopen(path, "wb");
    fwrite(fb, 2, UI_W * UI_H, f);
    fclose(f);
    printf("saved %s\n", path);
}

/* Text dump of the state just saved (ui_test_dump): check_renders.py
 * asserts on the words (advice sentences, tile/chip labels, the stale
 * header, the settings info lines) as well as the pixels. Call right
 * after the save it belongs to, before any state changes. */
static void dump(const char *dir, const char *name)
{
    char path[512];
    snprintf(path, sizeof(path), "%s/%s.txt", dir, name);
    ui_test_dump(path);
}

static char *slurp(const char *path, size_t *len)
{
    FILE *f = fopen(path, "rb");
    if (!f) {
        fprintf(stderr, "missing %s\n", path);
        exit(1);
    }
    fseek(f, 0, SEEK_END);
    *len = ftell(f);
    fseek(f, 0, SEEK_SET);
    char *b = malloc(*len + 1);
    fread(b, 1, *len, f);
    b[*len] = 0;
    fclose(f);
    return b;
}

static wx_t wx;

/* "2026-10-27T19:30" -> tm (wday filled by wx_day_of_week). */
static bool parse_sim_clock(const char *j, struct tm *out)
{
    cJSON *root = cJSON_Parse(j);
    if (!root) {
        return false;
    }
    const cJSON *sc = cJSON_GetObjectItemCaseSensitive(root, "sim_clock");
    bool ok = false;
    if (cJSON_IsString(sc) && sc->valuestring) {
        int y, mo, d, h, mi;
        if (sscanf(sc->valuestring, "%d-%d-%dT%d:%d", &y, &mo, &d, &h, &mi) == 5) {
            memset(out, 0, sizeof(*out));
            out->tm_year = y - 1900;
            out->tm_mon = mo - 1;
            out->tm_mday = d;
            out->tm_hour = h;
            out->tm_min = mi;
            out->tm_wday = wx_day_of_week(y, mo, d);
            ok = true;
        }
    }
    cJSON_Delete(root);
    return ok;
}

static void load(const char *dir, const char *name)
{
    char path[512];
    snprintf(path, sizeof(path), "%s/%s.json", dir, name);
    size_t len;
    char *j = slurp(path, &len);
    if (!wx_parse_openmeteo(j, len, true, &wx)) {
        fprintf(stderr, "parse failed: %s\n", name);
        exit(1);
    }
    struct tm clk;
    if (parse_sim_clock(j, &clk)) {
        now_tm = clk;
    } else {
        memset(&now_tm, 0, sizeof(now_tm));
        now_tm.tm_year = wx.year - 1900;
        now_tm.tm_mon = wx.month - 1;
        now_tm.tm_mday = wx.day;
        now_tm.tm_hour = wx.hour;
        now_tm.tm_min = wx.minute + 7 > 59 ? 59 : wx.minute + 7;
        now_tm.tm_wday = wx.wday;
    }
    now_valid = true;
    free(j);
    printf("%s: clock %04d-%02d-%02d %02d:%02d, %d hours, %d days\n", name, now_tm.tm_year + 1900,
           now_tm.tm_mon + 1, now_tm.tm_mday, now_tm.tm_hour, now_tm.tm_min, wx.nhours, wx.ndays);
}

static void show(void)
{
    ui_show_weather(&wx);
    ui_refresh_clock();
    pump(10);
}

/* ------------------------------------------------------------------ My Day fixtures (iter-14)
 * Invented events only (this repo is public), built relative to the
 * sim clock currently loaded, so the Today/Tomorrow sections land on
 * the rendered dates. The renderer process runs in the scenario's
 * zone, like the board, so cal.c's mktime/localtime agree with the
 * page. File order is scrambled on purpose: cal.c sorts. One summary
 * is line-folded and one location carries an escaped comma, so the
 * renders also prove unfolding and unescaping. */
static void ymd(char *buf, const struct tm *t)
{
    sprintf(buf, "%04d%02d%02d", t->tm_year + 1900, t->tm_mon + 1, t->tm_mday);
}

static void fixture_cal(bool with_events)
{
    struct tm today = now_tm, tom = now_tm, later = now_tm;
    tom.tm_mday += 1;
    tom.tm_isdst = -1;
    mktime(&tom);
    later.tm_mday += 10;
    later.tm_isdst = -1;
    mktime(&later);
    char d0[9], d1[9], d10[9];
    ymd(d0, &today);
    ymd(d1, &tom);
    ymd(d10, &later);
    char ics[4096];
    int n = 0;
    n += snprintf(ics + n, sizeof(ics) - n,
                  "BEGIN:VCALENDAR\r\nPRODID:-//WeatherBesties Sim//EN\r\nVERSION:2.0\r\n");
    if (with_events) {
        n += snprintf(ics + n, sizeof(ics) - n,
                      "BEGIN:VEVENT\r\nUID:sim-1\r\nDTSTART;TZID=America/New_York:%sT153000\r\n"
                      "DTEND;TZID=America/New_York:%sT163000\r\nSUMMARY:Cleo's soccer practice\r\n"
                      "LOCATION:Roosevelt Field\r\nEND:VEVENT\r\n",
                      d0, d0);
        n += snprintf(ics + n, sizeof(ics) - n,
                      "BEGIN:VEVENT\r\nUID:sim-2\r\nDTSTART;VALUE=DATE:%s\r\nDTEND;VALUE=DATE:%s\r\n"
                      "SUMMARY:Picture Day\r\nEND:VEVENT\r\n",
                      d0, d1);
        n += snprintf(ics + n, sizeof(ics) - n,
                      "BEGIN:VEVENT\r\nUID:sim-3\r\nDTSTART:%sT200000Z\r\nDTEND:%sT204500Z\r\n"
                      "SUMMARY:Video call with Grandma\r\nEND:VEVENT\r\n",
                      d0, d0);
        n += snprintf(ics + n, sizeof(ics) - n,
                      "BEGIN:VEVENT\r\nUID:sim-4\r\nDTSTART;TZID=America/New_York:%sT100000\r\n"
                      "DTEND;TZID=America/New_York:%sT104500\r\nSUMMARY:Lila's library story time\r\nEND:VEVENT\r\n",
                      d0, d0);
        n += snprintf(ics + n, sizeof(ics) - n,
                      "BEGIN:VEVENT\r\nUID:sim-5\r\nDTSTART;TZID=America/New_York:%sT160000\r\n"
                      "DTEND;TZID=America/New_York:%sT163000\r\nSUMMARY:Cleo's piano les\r\n son\r\n"
                      "LOCATION:Music school\r\nEND:VEVENT\r\n",
                      d1, d1);
        n += snprintf(ics + n, sizeof(ics) - n,
                      "BEGIN:VEVENT\r\nUID:sim-6\r\nDTSTART;TZID=America/New_York:%sT080000\r\n"
                      "DTEND;TZID=America/New_York:%sT090000\r\nSUMMARY:School bake sale\r\n"
                      "LOCATION:Watchung School gym\r\nEND:VEVENT\r\n",
                      d1, d1);
    } else {
        /* A real event, just not today or tomorrow: the empty state
         * must come from the dates, not from an empty feed. */
        n += snprintf(ics + n, sizeof(ics) - n,
                      "BEGIN:VEVENT\r\nUID:sim-9\r\nDTSTART;TZID=America/New_York:%sT090000\r\n"
                      "DTEND;TZID=America/New_York:%sT100000\r\nSUMMARY:School play rehearsal\r\nEND:VEVENT\r\n",
                      d10, d10);
    }
    n += snprintf(ics + n, sizeof(ics) - n, "END:VCALENDAR\r\n");
    /* Through this fork's calendar reader (the one the board uses), no filter words: the
     * fixture is a kids-only calendar. */
    static cal_t cal;
    struct tm lt = now_tm;
    lt.tm_isdst = -1;
    mktime(&lt);
    cal_begin(&cal, cal_dayno(now_tm.tm_year + 1900, now_tm.tm_mon + 1, now_tm.tm_mday), (int)lt.tm_gmtoff, "");
    cal_feed(&cal, ics, (size_t)n);
    if (!cal_end(&cal)) {
        fprintf(stderr, "fixture calendar did not parse\n");
        exit(1);
    }
    cal_publish(&cal);
}

/* Records the fixture fetch at a fixed time on the sim clock, so
 * the My Day freshness line renders deterministically whatever
 * the scenario clock says (iter-15). The clock is restored before
 * anything renders for keeps. */
static void fixture_fetched_at(int hour, int min)
{
    struct tm save = now_tm;
    now_tm.tm_hour = hour;
    now_tm.tm_min = min;
    ui_show_calendar();
    now_tm = save;
}

/* The usual pin: a 7:02 AM fetch ("Updated 7:02 AM"). */
static void fixture_fetched(void)
{
    fixture_fetched_at(7, 2);
}

static void sim_brightness_cb(int pct)
{
    printf("brightness saved: %d%%\n", pct);
}

/* A pretend finger, so the tests can hold the screen like a kid does (the board's
 * long-press time is 1.5 s, main/board.c). */
static bool s_touch;
static lv_point_t s_touch_pt;

static void touch_read(lv_indev_t *in, lv_indev_data_t *data)
{
    (void)in;
    data->point = s_touch_pt;
    data->state = s_touch ? LV_INDEV_STATE_PRESSED : LV_INDEV_STATE_RELEASED;
}

/* Holds a finger still at (x, y) for ms, then lifts it. */
static void hold(int x, int y, int hold_ms)
{
    s_touch_pt.x = x;
    s_touch_pt.y = y;
    s_touch = true;
    pump(hold_ms / 40);
    s_touch = false;
    pump(5);
}

int main(int argc, char **argv)
{
    const char *out = argc > 1 ? argv[1] : "out";
    const char *sc = argc > 2 ? argv[2] : "scenarios";

    lv_init();
    lv_tick_set_cb(tick_cb);
    lv_display_t *d = lv_display_create(UI_W, UI_H);
    lv_display_set_buffers(d, fb, NULL, sizeof(fb), LV_DISPLAY_RENDER_MODE_DIRECT);
    lv_display_set_flush_cb(d, flush_cb);

    lv_indev_t *touch = lv_indev_create();   /* before ui_init: it hooks every indev for swipes */
    lv_indev_set_type(touch, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(touch, touch_read);
    lv_indev_set_long_press_time(touch, 1500);

    ui_init("Cleo & Lila");
    v2_set_name_in_title(false);   /* Boris's shots and checks: no names on screen */
    ui_set_time_cb(time_cb);
    ui_info_t info = {.wifi = "HomeWiFi", .place = "Upper Montclair, New Jersey", .ip = "192.168.1.42",
                      .updated = "Weather updated 7:30 AM", .version = "2.0.0"};
    ui_set_info(&info);

    ui_show_boot(NULL);
    save(out, "01_boot");

    ui_show_setup("WeatherBesties-3F2A", "kite7mango", "192.168.4.1");
    save(out, "02_setup");

    /* The brief's two mockups: cold morning warming up; rainy tomorrow laid out
     * the evening before. */
    load(sc, "brief_morning");
    show();
    save(out, "20_morning");
    dump(out, "20_morning");

    load(sc, "brief_evening");
    show();
    save(out, "21_evening");
    dump(out, "21_evening");

    ui_test_set_checks(0x3); /* first two garments laid out (2 of 5: the
                              * umbrella extra is a checkable chip too) */
    pump(10);
    save(out, "22_evening_2_checked");
    dump(out, "22_evening_2_checked");   /* iter-10: the progress pill's screen */

    ui_test_set_checks(0x1F); /* everything laid out: 4 tiles + the
                               * umbrella chip (bit 4). All-done must not
                               * fire on the tiles alone any more. The
                               * rain sprite stays static, but iter-11
                               * overlays the heart gesture on her chest
                               * (the animation itself exists only for
                               * the black dress sprite and is never
                               * stretched over another outfit). */
    pump(10);
    save(out, "23_evening_all_done");
    dump(out, "23_evening_all_done");
    ui_test_set_checks(0);

    /* Pippa proof still: her all-done heart animation plays on its own
     * in the renderer, on the black dress sprite only. The dress
     * scenario's outfit is the dress (4 tiles since iter-08 itemised
     * the tights and dress shoes, no chips since iter-06 canonicalised
     * the prescription to what the sprite wears); lay everything out
     * and pin frame 4 (the eyes-closed heart hold) so the screenshot
     * set shows the gesture deterministically. (The brief morning used
     * to double as this proof; its outfit changed with the panel
     * rounds, most recently to a dress day in iter-07, and the heart
     * frames were never any other sprite's.) */
    load(sc, "dress_morning");
    show();
    ui_test_set_checks(0x3F);
    pump(10);
    ui_test_set_pippa_frame(4);
    pump(10);
    save(out, "23b_morning_all_done_heart");
    ui_test_set_pippa_frame(-1);
    ui_test_set_checks(0);

    load(sc, "brief_morning");
    show();
    ui_test_set_page(3);   /* iter-14: forecast is the fourth page now */
    pump(10);
    save(out, "24_next_days");
    dump(out, "24_next_days");   /* iter-09: the fc*_tiles lines assert the cards' full outfits */
    ui_test_set_page(0);

    load(sc, "stale_2h");
    show();
    save(out, "25_stale");
    dump(out, "25_stale");

    /* Iter-11: a stale prescription is not checkable. Drive the real
     * toggle paths exactly as taps would (a tile and the scarf chip);
     * both must be swallowed by the stale lock, so the screen after
     * them is the stale screen unchanged. */
    ui_test_tap_tile(0);
    ui_test_tap_chip(0);
    pump(10);
    save(out, "25c_stale_tapped");
    dump(out, "25c_stale_tapped");

    /* iter-10: the same stale data on the forecast page: the status
     * pill's header-band placement there, and the forecast ground
     * following the outfit pages' tint rule. */
    ui_test_set_page(3);
    pump(10);
    save(out, "25b_stale_next_days");
    dump(out, "25b_stale_next_days");
    ui_test_set_page(0);

    load(sc, "too_old");
    show();
    save(out, "26_unavailable");
    dump(out, "26_unavailable");

    /* iter-10: the unavailable state's Tomorrow variant: the
     * headline must carry the state on this page too. */
    ui_test_set_page(1);
    pump(10);
    save(out, "26b_unavailable_tomorrow");
    dump(out, "26b_unavailable_tomorrow");
    ui_test_set_page(0);

    load(sc, "rollover");
    show();
    save(out, "27_rollover");
    dump(out, "27_rollover");

    load(sc, "snow_day");
    show();
    save(out, "28_snow");
    dump(out, "28_snow");

    load(sc, "hot_summer");
    show();
    save(out, "29_hot");
    dump(out, "29_hot");

    load(sc, "storm_afternoon");
    show();
    save(out, "30_storm");
    dump(out, "30_storm");

    load(sc, "halloween_cold");
    show();
    save(out, "31_halloween");
    dump(out, "31_halloween");

    load(sc, "brief_morning");
    ui_set_cat_choice(1); /* pink cat */
    show();
    save(out, "32_pink_cat");
    dump(out, "32_pink_cat");
    ui_set_cat_choice(0);

    ui_open_settings();
    save(out, "33_settings");
    dump(out, "33_settings");
    ui_close_settings();

    ui_set_names("Cleo & Lila & Friends");
    load(sc, "long_text");
    show();
    save(out, "34_long_text");
    dump(out, "34_long_text");
    ui_set_names("Cleo & Lila");

    load(sc, "hanukkah_night3");
    show();
    save(out, "35_hanukkah");
    dump(out, "35_hanukkah");

    load(sc, "bike_bus_cold_friday");
    show();
    save(out, "36_bike_bus");
    dump(out, "36_bike_bus");

    /* iter-06 outfit references: the dress day and the pants day,
     * unchecked. Round 4's fidelity failures lived on exactly these
     * two outfits (and the winter kit on the cold days above). */
    load(sc, "dress_morning");
    show();
    save(out, "37_dress_day");
    dump(out, "37_dress_day");

    load(sc, "warm_day");
    show();
    save(out, "38_warm_day");
    dump(out, "38_warm_day");

    /* Iter-12: a payload with no yesterday in it (first boot, partial
     * fetch). The outfit still prescribes, but the compare pill must
     * stay hidden: there is no honest comparison to show. */
    load(sc, "mild_no_yday");
    show();
    save(out, "39_no_yesterday");
    dump(out, "39_no_yesterday");

    /* Iter-14: the My Day page, third screen. The populated state
     * (fixture events on both days), the configured-but-clear state,
     * and the no-calendar-link state, all on the brief_morning clock
     * (Tue Oct 27 2026). Iter-17 adds three more: the link set but
     * nothing ever loaded, a stale feed, and a late-afternoon view
     * (past events dimmed, the next one tagged). */
    setenv("TZ", "America/New_York", 1);
    tzset();
    load(sc, "brief_morning");
    show();
    ui_set_calendar_configured(true);
    ui_test_set_page(2);
    /* The never-loaded state (iter-17) must be captured before any
     * fetch is recorded in this process: once ui_show_calendar has
     * run, the page can never go back to never-fetched. The feed
     * store is still empty here, so there is no data either. */
    pump(10);
    save(out, "44_myday_never");
    dump(out, "44_myday_never");

    fixture_cal(true);
    fixture_fetched();
    pump(10);
    save(out, "40_myday");
    dump(out, "40_myday");

    fixture_cal(false);
    fixture_fetched();
    pump(10);
    save(out, "41_myday_empty");
    dump(out, "41_myday_empty");

    ui_set_calendar_configured(false);
    pump(10);
    save(out, "42_myday_unconfigured");
    dump(out, "42_myday_unconfigured");

    /* Stale (iter-17; iter-18 fixture hygiene): the same populated
     * page at 3:45 PM with the last good fetch still the 7:02 AM
     * one, over six hours old, so the freshness line takes its
     * warn form. Iter-18 loads the afternoon weather first (the
     * same Tuesday, cold all day, fetched at 3:30 PM): Pippa wears
     * the morning's snow look and the weather is fresh, so no
     * weather-stale chip competes with the calendar's own stale
     * stamp. Renders 40, 43 and 45 are the same fictional day;
     * only the clock and the fetch ages differ. */
    load(sc, "myday_afternoon");
    show();
    ui_set_calendar_configured(true);
    fixture_cal(true);
    fixture_fetched();
    now_tm.tm_hour = 15;
    now_tm.tm_min = 45;
    ui_refresh_clock();
    pump(10);
    save(out, "43_myday_stale");
    dump(out, "43_myday_stale");

    /* Late (iter-17; iter-18): the same 3:45 PM clock with a fresh
     * 3:02 PM fetch, isolating the afternoon statuses: story time
     * is over (DONE, dimmed), soccer (3:30-4:30) is in progress
     * (NOW), and the Grandma call is still to come (NEXT). The
     * refresh matters: the fetch helper renders with the clock
     * parked at 3:02, when soccer is still NEXT, so the page must
     * render again at the real 3:45 before it is captured. */
    fixture_fetched_at(15, 2);
    ui_refresh_clock();
    pump(10);
    save(out, "45_myday_late");
    dump(out, "45_myday_late");
    ui_test_set_page(0);
    /* the family's Brittany (pet choice 3; tools/dog/): every outfit, all-done, the
     * forecast minis, Settings and the badge. */
    ui_set_cat_choice(3);
    ui_set_names("Kid");
    v2_set_name_in_title(true);   /* the family's board shows the name in the title */
    static const char *dog_days[][2] = {{"dress_morning", "50_dog_dress"}, {"brief_morning", "51_dog_morning"},
                                        {"brief_evening", "52_dog_rain_tomorrow"}, {"snow_day", "53_dog_snow"},
                                        {"hot_summer", "54_dog_hot"}, {"warm_day", "55_dog_warm"}};
    for (size_t i = 0; i < sizeof(dog_days) / sizeof(dog_days[0]); i++) {
        load(sc, dog_days[i][0]);
        show();
        save(out, dog_days[i][1]);
        dump(out, dog_days[i][1]);
    }
    load(sc, "dress_morning");
    show();
    ui_test_set_checks(0x3F);
    pump(10);
    save(out, "56_dog_all_done");
    ui_test_set_checks(0);
    load(sc, "brief_morning");
    show();
    ui_test_set_page(3);   /* 0 today, 1 tomorrow, 2 My Day, 3 next days */
    save(out, "57_dog_next_days");
    fixture_cal(true);
    fixture_fetched();
    ui_test_set_page(2);
    pump(10);
    save(out, "60_dog_myday");
    {   /* a countdown 11 days out in the My Day header */
        wb_event_t cd = {.name = "Kid's birthday", .countdown = true, .all_day = true};
        int y, m, d;
        cal_date(cal_dayno(now_tm.tm_year + 1900, now_tm.tm_mon + 1, now_tm.tm_mday) + 11, &y, &m, &d);
        cd.year = (uint16_t)y;
        cd.month = (uint8_t)m;
        cd.day = (uint8_t)d;
        ui_set_events(&cd, 1);
        fixture_fetched();
        pump(10);
        save(out, "71_dog_myday_countdown");
        ui_set_events(NULL, 0);
    }
    ui_test_set_page(0);
    static const char *dog_seasons[][2] = {{"halloween_cold", "67_dog_halloween"},
                                           {"christmas_snow", "68_dog_christmas"},
                                           {"hanukkah_night3", "69_dog_hanukkah"},
                                           {"mild_may", "70_dog_may"}};
    for (size_t i = 0; i < sizeof(dog_seasons) / sizeof(dog_seasons[0]); i++) {
        load(sc, dog_seasons[i][0]);
        show();
        save(out, dog_seasons[i][1]);
    }
    ui_test_set_page(0);
    ui_set_brightness_cb(sim_brightness_cb);   /* the board's Settings: a Brightness slider */
    ui_set_brightness(100);
    ui_open_settings();
    save(out, "58_dog_settings");
    ui_close_settings();
    ui_set_brightness(40);   /* dimmed to 40 %: the dark layer over everything */
    load(sc, "brief_morning");
    show();
    save(out, "81_dimmed");
    ui_set_brightness(100);
    ui_show_setup("WeatherBesties-3F2A", "kite7mango", "192.168.4.1");
    save(out, "59_dog_setup");
    ui_set_names("Cleo & Lila");
    v2_set_name_in_title(false);
    ui_set_cat_choice(0);

    /* Settings opens from a 1.5 s hold on an empty spot (the page title) on every page:
     * Today, My Day and Next days. */
    load(sc, "brief_morning");
    show();
    static const struct { int page, x, y; const char *name; } holds[] = {
        {0, 120, 40, "61_hold_today"}, {2, 100, 30, "62_hold_myday"}, {3, 100, 30, "63_hold_next_days"}};
    for (size_t i = 0; i < sizeof(holds) / sizeof(holds[0]); i++) {
        ui_test_set_page(holds[i].page);
        pump(5);
        hold(holds[i].x, holds[i].y, 1800);
        save(out, holds[i].name);
        ui_close_settings();
    }
    ui_test_set_page(0);

    /* This fork's dress-up days: once ui_set_dress_days is called, a light-jacket day is
     * jeans unless it's a dress-up day. A tap on the Dress up pill flips it; a "(dress)"
     * calendar event starts the day dressed. (Last, so Boris's shots keep his rule.) */
    ui_set_cat_choice(3);
    ui_set_dress_days(NULL, 0);
    load(sc, "dress_morning");
    show();
    save(out, "64_dress_off");
    dump(out, "64_dress_off");
    s_touch_pt.x = 340;   /* the pill, right of "Today · <day>" */
    s_touch_pt.y = 88;
    s_touch = true;
    pump(3);
    s_touch = false;
    pump(5);
    save(out, "65_dress_tapped");
    dump(out, "65_dress_tapped");
    ui_test_set_checks(0x3);   /* two ticked: "2 of 4 ready" goes after the pill */
    pump(5);
    save(out, "65b_dress_progress");
    ui_test_set_checks(0);
    int dn = cal_dayno(now_tm.tm_year + 1900, now_tm.tm_mon + 1, now_tm.tm_mday);
    ui_set_dress_days(&dn, 1);
    load(sc, "dress_morning");
    show();
    save(out, "66_dress_calendar");
    dump(out, "66_dress_calendar");
    /* The Routine page (fifth tab): the morning list with two done and the bar running
     * down to 8:30, all done, and the bedtime list in the evening. */
    load(sc, "brief_morning");
    show();
    ui_test_set_page(4);
    pump(5);
    v2_routine_test_tap(0);
    v2_routine_test_tap(1);
    save(out, "74_routine_morning");
    v2_hint_test_show(true);   /* the next step glows after 20 idle seconds */
    save(out, "74b_routine_hint");
    v2_hint_test_show(false);
    for (int i = 2; i < 5; i++) {
        v2_routine_test_tap(i);
    }
    save(out, "75_routine_done");
    load(sc, "brief_evening");
    show();
    ui_test_set_page(4);
    pump(5);
    v2_routine_test_tap(0);
    v2_routine_test_tap(1);
    v2_routine_test_tap(2);
    save(out, "76_routine_bedtime");
    ui_test_set_page(0);
    /* The setup page's jacket cut-offs: the cold-morning scenario in the winter kit by
     * Boris's numbers, a light jacket once the coat cut-off is lowered to 30. */
    ui_set_dress_days(NULL, 0);
    load(sc, "halloween_cold");
    show();
    save(out, "72_temps_default");
    dump(out, "72_temps_default");
    ui_set_outfit_temps(30, 50, 60, 0);
    save(out, "73_temps_lowered");
    dump(out, "73_temps_lowered");
    ui_set_outfit_temps(0, 0, 0, 0);
    {   /* a weather alert: the calm strip in the title's place */
        wx_alerts_t al = {0};
        alerts_add(&al, "Winter Storm Warning", "Severe");
        ui_set_alerts(&al);
        load(sc, "snow_day");
        show();
        save(out, "79_alert_warning");
        al.n = 0;
        alerts_add(&al, "Wind Advisory", "Moderate");
        ui_set_alerts(&al);
        load(sc, "brief_morning");
        show();
        save(out, "80_alert_advisory");
        ui_set_alerts(NULL);
    }
    /* The warm-afternoon rule: 36 feels-like at 8 AM, 64 by afternoon. Boris's numbers
     * give the winter kit; with the rule at 60 it's the light jacket. */
    load(sc, "brief_morning");
    show();
    save(out, "77_cold_then_warm_default");
    dump(out, "77_cold_then_warm_default");
    ui_set_outfit_temps(0, 0, 0, 60);
    save(out, "78_cold_then_warm_jacket");
    dump(out, "78_cold_then_warm_jacket");
    ui_set_outfit_temps(0, 0, 0, 0);
    ui_set_cat_choice(0);

    {   /* The contract (wb_ui.h): the controller's way in. One state, one call; the same
         * scene as the dog's morning plus an alert, built the way main/ builds it. */
        static wb_state_t st;
        load(sc, "brief_morning");
        snprintf(st.names, sizeof(st.names), "Kid");
        st.pet = WB_PET_DOG;
        st.brightness = 100;
        st.leave_min = 8 * 60 + 30;
        st.bed_min = 20 * 60;
        st.bike_bus_wday = -1;
        st.wx = wx;
        st.have_wx = true;
        alerts_add(&st.alerts, "Wind Advisory", "Moderate");
        snprintf(st.status, sizeof(st.status), "Updated 7:30 AM");
        snprintf(st.version, sizeof(st.version), "2.0.0");
        st.seq = 1;
        v2_set_name_in_title(true);   /* the board's setting */
        wb_ui_update(&st);
        ui_refresh_clock();
        pump(10);
        save(out, "82_contract");
        dump(out, "82_contract");
        st.alerts.n = 0;   /* the next update clears the strip */
        st.seq++;
        wb_ui_update(&st);
        v2_set_name_in_title(false);
        ui_set_names("Cleo & Lila");
        ui_set_cat_choice(0);
    }

    ui_show_wifi_error("HomeWiFi", NULL);
    save(out, "10_wifi_error");

    lv_mem_monitor_t mon;
    lv_mem_monitor(&mon);
    printf("LVGL heap: %u of %u bytes used (%u%%), biggest free block %u\n", (unsigned)(mon.total_size - mon.free_size),
           (unsigned)mon.total_size, (unsigned)mon.used_pct, (unsigned)mon.free_biggest_size);
    return 0;
}
