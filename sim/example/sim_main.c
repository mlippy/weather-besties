/*
 * Desktop renderer for the text-only example design (components/wb_ui/example/). It talks
 * to the screens only through wb_ui.h, exactly as main/ does on the board.
 * Usage: ./render-example <outdir> <scenario dir>
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "cal.h"
#include "lvgl.h"
#include "wb_ui.h"

#define W 800
#define H 480

static uint16_t fb[W * H];
static uint32_t ms;
static struct tm now_tm;
static wb_state_t st;

static uint32_t tick_cb(void) { return ms; }

static void flush_cb(lv_display_t *d, const lv_area_t *a, uint8_t *px)
{
    (void)a;
    (void)px;
    lv_display_flush_ready(d);
}

static bool clock_cb(struct tm *out)
{
    *out = now_tm;
    return true;
}

static void on_action(wb_action_t act, int value, void *user)
{
    (void)user;
    printf("action %d (%d)\n", (int)act, value);
}

static void save(const char *dir, const char *name)
{
    for (int i = 0; i < 30; i++) {
        ms += 40;
        lv_timer_handler();
    }
    lv_obj_invalidate(lv_screen_active());
    lv_refr_now(NULL);
    char path[512];
    snprintf(path, sizeof(path), "%s/%s.rgb565", dir, name);
    FILE *f = fopen(path, "wb");
    fwrite(fb, 2, W * H, f);
    fclose(f);
    printf("saved %s\n", path);
}

static void load(const char *dir, const char *name)
{
    char path[512];
    snprintf(path, sizeof(path), "%s/%s.json", dir, name);
    FILE *f = fopen(path, "rb");
    if (!f) {
        fprintf(stderr, "missing %s\n", path);
        exit(1);
    }
    fseek(f, 0, SEEK_END);
    long len = ftell(f);
    fseek(f, 0, SEEK_SET);
    char *j = malloc((size_t)len + 1);
    fread(j, 1, (size_t)len, f);
    j[len] = 0;
    fclose(f);
    st.have_wx = wx_parse_openmeteo(j, (size_t)len, true, &st.wx);
    free(j);
    memset(&now_tm, 0, sizeof(now_tm));
    now_tm.tm_year = st.wx.year - 1900;
    now_tm.tm_mon = st.wx.month - 1;
    now_tm.tm_mday = st.wx.day;
    now_tm.tm_hour = st.wx.hour;
    now_tm.tm_min = st.wx.minute;
    now_tm.tm_wday = st.wx.wday;
}

int main(int argc, char **argv)
{
    const char *out = argc > 1 ? argv[1] : "out-example";
    const char *sc = argc > 2 ? argv[2] : "scenarios";

    lv_init();
    lv_tick_set_cb(tick_cb);
    lv_display_t *d = lv_display_create(W, H);
    lv_display_set_buffers(d, fb, NULL, sizeof(fb), LV_DISPLAY_RENDER_MODE_DIRECT);
    lv_display_set_flush_cb(d, flush_cb);

    /* What main/ does at boot. */
    wb_ui_start(on_action, NULL, clock_cb);
    wb_ui_show_boot(NULL);
    save(out, "01_boot");
    wb_ui_show_setup("WeatherBesties-3F2A", "kite7mango", "192.168.4.1");
    save(out, "02_setup");

    /* What main/ does after each weather fetch: fill the state, hand it over. */
    load(sc, "oct_morning");
    snprintf(st.names, sizeof(st.names), "Kid");
    st.leave_min = 8 * 60 + 30;
    st.bed_min = 20 * 60;
    st.bike_bus_wday = -1;
    snprintf(st.place, sizeof(st.place), "Upper Montclair, NJ");
    snprintf(st.status, sizeof(st.status), "Weather updated 7:45 AM");
    st.nevents = 1;
    snprintf(st.events[0].name, sizeof(st.events[0].name), "the class trip");
    {
        int y, m, dd;
        cal_date(cal_dayno(st.wx.year, st.wx.month, st.wx.day) + 3, &y, &m, &dd);
        st.events[0].year = (uint16_t)y, st.events[0].month = (uint8_t)m, st.events[0].day = (uint8_t)dd;
        st.events[0].countdown = true;
    }
    now_tm.tm_hour = 7, now_tm.tm_min = 45;
    wb_ui_update(&st);
    save(out, "03_weather");

    wb_ui_show_wifi_error("HomeWiFi", NULL);
    save(out, "04_wifi_error");
    return 0;
}
