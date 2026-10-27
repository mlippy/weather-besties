#include "net.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "alerts.h"
#include "app.h"
#include "board.h"
#include "cJSON.h"
#include "cal.h"
#include "ota.h"
#include "routine.h"
#include "esp_crt_bundle.h"
#include "esp_event.h"
#include "esp_heap_caps.h"
#include "esp_http_client.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_netif_sntp.h"
#include "esp_timer.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"
#include "wb_request.h"
#include "wx.h"

static const char *TAG = "net";

#define EV_CONNECTED BIT0
#define EV_REFRESH BIT1
#define EV_PORTAL BIT2
#define EV_UPDATE BIT3

#define REFRESH_MS (15 * 60 * 1000)
#define RETRY_QUICK_MS (10 * 1000)
#define RETRY_FAST_MS (60 * 1000)
#define RETRY_SLOW_MS (5 * 60 * 1000)
#define CONNECT_WAIT_MS (30 * 1000)
/* Keep showing the last alerts through this many failed alert checks in a row (15 min each). */
#define ALERT_KEEP_FAILS 4
#define CAL_REFRESH_US (60LL * 60 * 1000 * 1000)
#define CAL_MAX_BYTES (8 * 1024 * 1024)
#define USER_AGENT WB_USER_AGENT_PREFIX WB_VERSION WB_USER_AGENT_SUFFIX
#define WIFI_INIT_RETRY_MS (10 * 1000)

static EventGroupHandle_t s_ev;
static wb_settings_t *S;
static esp_timer_handle_t s_reconnect;
static volatile bool s_portal;
static char s_ip[20];
static volatile int s_utc_offset;
static volatile bool s_have_offset;
static wx_t s_wx;
static wx_alerts_t s_alerts;
static int s_alert_fails;
static wb_event_t s_cal_events[CAL_MAX];
static int s_ncal;
static int64_t s_cal_at = -1;     /* esp_timer time of the last good read */
static int s_cal_day = -1;        /* the day it was read for */
static char s_cal_status[96] = "Not checked yet.";

/* ------------------------------------------------------------------ time */

bool net_local_time(struct tm *out)
{
    time_t now = time(NULL);
    if (now < 1700000000 || !s_have_offset) {
        return false;
    }
    now += s_utc_offset;
    gmtime_r(&now, out);
    return true;
}

/* Point the C library at the weather location's offset, so the
 * calendar parser's mktime/localtime read ICS local times in the
 * same zone the screen shows (net_local_time above does the sum by
 * hand and does not care). Fixed offset only, no DST rules of its
 * own: it is re-set from the fresh offset on every fetch, so it
 * tracks the location through DST changes. */
static void set_local_tz(int utc_offset_s)
{
    static int applied = 0x7fffffff;
    if (utc_offset_s == applied) {
        return;
    }
    applied = utc_offset_s;
    /* A POSIX TZ offset is what you add to local time to get UTC:
     * the opposite sign of the usual "UTC+x" offset. The zone name needs 3+ letters:
     * libc rejects a 2-letter one and quietly falls back to UTC (Boris's iter-29). */
    int posix = -utc_offset_s;
    int sign = posix < 0 ? -1 : 1;
    int a = posix < 0 ? -posix : posix;
    char tz[24];
    if (a % 3600 == 0) {
        snprintf(tz, sizeof(tz), "WBT%d", sign * (a / 3600));
    } else {
        snprintf(tz, sizeof(tz), "WBT%d:%02d", sign * (a / 3600), (a % 3600) / 60);
    }
    setenv("TZ", tz, 1);
    tzset();
}

/* ------------------------------------------------------------------ ui glue */

void net_ui_action(wb_action_t a)
{
    if (a == WB_ACT_SETUP) {
        xEventGroupSetBits(s_ev, EV_PORTAL);
    } else if (a == WB_ACT_REFRESH) {
        xEventGroupSetBits(s_ev, EV_REFRESH);
    } else if (a == WB_ACT_UPDATE) {
        xEventGroupSetBits(s_ev, EV_UPDATE);
    }
}

#define UI_LOCKED(code)              \
    do {                             \
        if (board_lvgl_lock(-1)) {   \
            code;                    \
            board_lvgl_unlock();     \
        }                            \
    } while (0)

/* ------------------------------------------------------------------ wifi */

static void reconnect_cb(void *arg)
{
    (void)arg;
    if (!s_portal) {
        esp_wifi_connect();
    }
}

static void on_wifi(void *arg, esp_event_base_t base, int32_t id, void *data)
{
    (void)arg;
    if (base == WIFI_EVENT && id == WIFI_EVENT_STA_START) {
        if (!s_portal) {
            esp_wifi_connect();
        }
    } else if (base == WIFI_EVENT && id == WIFI_EVENT_STA_DISCONNECTED) {
        xEventGroupClearBits(s_ev, EV_CONNECTED);
        s_ip[0] = 0;
        if (!s_portal) {
            wifi_event_sta_disconnected_t *d = data;
            ESP_LOGW(TAG, "wifi disconnected (reason %d), retrying in 5 s", d ? d->reason : -1);
            esp_timer_stop(s_reconnect);
            esp_timer_start_once(s_reconnect, 5 * 1000 * 1000);
        }
    } else if (base == IP_EVENT && id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t *e = data;
        snprintf(s_ip, sizeof(s_ip), IPSTR, IP2STR(&e->ip_info.ip));
        ESP_LOGI(TAG, "got ip %s", s_ip);
        xEventGroupSetBits(s_ev, EV_CONNECTED);
        /* Iter-35: the wifi stage of the memory log, once per boot:
         * the driver's static buffers are internal by design, so this
         * is the baseline the two fetch stages spend from. */
        static bool mem_logged;
        if (!mem_logged) {
            mem_logged = true;
            board_mem_log("wifi");
        }
    }
}

/* One-time network stack bring-up plus the WiFi driver init. Returns
 * false (and logs) when a step fails, so net_task can wait and retry:
 * esp_wifi_init's static RX buffers come out of internal DRAM, and on a
 * memory-tight boot the init can fail with ESP_ERR_NO_MEM. Aborting
 * here used to reboot-loop the device forever; a failed init now just
 * means "no network yet", like a router that is still booting. */
static bool wifi_init(void)
{
    static bool stack_up;
    if (!stack_up) {
        esp_err_t e = esp_netif_init();
        if (e != ESP_OK && e != ESP_ERR_INVALID_STATE) {
            ESP_LOGE(TAG, "esp_netif_init failed: %s", esp_err_to_name(e));
            return false;
        }
        e = esp_event_loop_create_default();
        if (e != ESP_OK && e != ESP_ERR_INVALID_STATE) {
            ESP_LOGE(TAG, "esp_event_loop_create_default failed: %s", esp_err_to_name(e));
            return false;
        }
        esp_netif_create_default_wifi_sta();
        esp_netif_create_default_wifi_ap();
        stack_up = true;
    }

    /* Default buffer counts, deliberately not trimmed: this board has a
     * weak-signal history, and fewer RX buffers make marginal reception
     * worse. The internal DRAM for them comes from the calendar stores
     * moving to PSRAM (see cal.c), not from starving the radio. */
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_err_t e = esp_wifi_init(&cfg);
    if (e == ESP_ERR_INVALID_STATE) {
        e = ESP_OK; /* already initialised; an earlier attempt got this far */
    }
    if (e != ESP_OK) {
        ESP_LOGE(TAG, "esp_wifi_init failed: %s", esp_err_to_name(e));
        esp_wifi_deinit();
        return false;
    }
    e = esp_wifi_set_storage(WIFI_STORAGE_RAM);
    if (e != ESP_OK) {
        ESP_LOGE(TAG, "esp_wifi_set_storage failed: %s", esp_err_to_name(e));
        return false;
    }

    static bool hooks_up;
    if (!hooks_up) {
        e = esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, on_wifi, NULL);
        if (e != ESP_OK) {
            ESP_LOGE(TAG, "wifi event handler failed: %s", esp_err_to_name(e));
            return false;
        }
        e = esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, on_wifi, NULL);
        if (e != ESP_OK) {
            ESP_LOGE(TAG, "ip event handler failed: %s", esp_err_to_name(e));
            return false;
        }
        const esp_timer_create_args_t t = {.callback = reconnect_cb, .name = "reconnect"};
        e = esp_timer_create(&t, &s_reconnect);
        if (e != ESP_OK) {
            ESP_LOGE(TAG, "reconnect timer failed: %s", esp_err_to_name(e));
            return false;
        }
        hooks_up = true;
    }
    return true;
}

static bool wifi_start_sta(void)
{
    wifi_config_t wc = {0};
    memcpy(wc.sta.ssid, S->ssid, strnlen(S->ssid, sizeof(wc.sta.ssid)));
    memcpy(wc.sta.password, S->pass, strnlen(S->pass, sizeof(wc.sta.password)));
    wc.sta.threshold.authmode = S->pass[0] ? WIFI_AUTH_WEP : WIFI_AUTH_OPEN;
    wc.sta.pmf_cfg.capable = true;
    esp_err_t e = esp_wifi_set_mode(WIFI_MODE_STA);
    if (e != ESP_OK) {
        ESP_LOGE(TAG, "esp_wifi_set_mode failed: %s", esp_err_to_name(e));
        return false;
    }
    e = esp_wifi_set_config(WIFI_IF_STA, &wc);
    if (e != ESP_OK) {
        ESP_LOGE(TAG, "esp_wifi_set_config failed: %s", esp_err_to_name(e));
        return false;
    }
    e = esp_wifi_start();
    if (e != ESP_OK && e != ESP_ERR_INVALID_STATE) {
        ESP_LOGE(TAG, "esp_wifi_start failed: %s", esp_err_to_name(e));
        return false;
    }
    /* Wall powered, so no modem sleep. Sleeping between DTIM beacons adds
     * delay to every round trip, and on a weak signal that pushed TLS +
     * forecast downloads past the HTTP timeout. */
    esp_wifi_set_ps(WIFI_PS_NONE);
    return true;
}

/* ------------------------------------------------------------------ http */

/* Why the last net_http_get failed, for the "can't reach" screen (no serial cable needed). */
static char s_net_err[32];

bool net_http_get(const char *url, char **body, size_t *len)
{
    *body = NULL;
    *len = 0;
    esp_http_client_config_t cfg = {
        .url = url,
        .crt_bundle_attach = esp_crt_bundle_attach,
        .timeout_ms = 30000,
        .buffer_size = WB_HTTP_RX_BUF,
        /* Holds the request line plus all headers. The default (512) is too
         * small for the forecast request; sim/test_request.c checks the fit. */
        .buffer_size_tx = WB_HTTP_TX_BUF,
        .user_agent = USER_AGENT,
    };
    esp_http_client_handle_t c = esp_http_client_init(&cfg);
    if (!c) {
        return false;
    }
    bool ok = false;
    size_t cap = 16 * 1024;
    char *buf = heap_caps_malloc(cap, MALLOC_CAP_SPIRAM);
    esp_err_t oe = buf ? esp_http_client_open(c, 0) : ESP_ERR_NO_MEM;
    if (oe != ESP_OK) {
        ESP_LOGW(TAG, "http open failed (%s): %s", esp_err_to_name(oe), url);
        snprintf(s_net_err, sizeof(s_net_err), "%s", esp_err_to_name(oe));
        goto out;
    }
    esp_http_client_fetch_headers(c);
    int status = esp_http_client_get_status_code(c);
    size_t n = 0;
    while (1) {
        if (cap - n < 1024) {
            if (cap >= 256 * 1024) {
                break;
            }
            char *nb = heap_caps_realloc(buf, cap * 2, MALLOC_CAP_SPIRAM);
            if (!nb) {
                break;
            }
            buf = nb;
            cap *= 2;
        }
        int r = esp_http_client_read(c, buf + n, cap - n - 1);
        if (r <= 0) {
            break;
        }
        n += r;
    }
    buf[n] = 0;
    if (status == 200 && n > 0) {
        ok = true;
        *body = buf;
        *len = n;
        buf = NULL;
    } else {
        ESP_LOGW(TAG, "http status %d, %u bytes", status, (unsigned)n);
        snprintf(s_net_err, sizeof(s_net_err), "HTTP %d", status);
    }
out:
    free(buf);
    esp_http_client_close(c);
    esp_http_client_cleanup(c);
    return ok;
}

/* Streams a (possibly big) download to cb a chunk at a time, following redirects. cb returns
 * false to stop early (then this returns false). */
bool net_http_stream(const char *url, bool (*cb)(const char *, size_t, void *), void *ctx, int *status_out,
                     size_t *total)
{
    *total = 0;
    *status_out = 0;
    esp_http_client_config_t cfg = {
        .url = url,
        .crt_bundle_attach = esp_crt_bundle_attach,
        .timeout_ms = 30000,
        .buffer_size = 4096,
        .buffer_size_tx = WB_HTTP_TX_BUF,
        .user_agent = USER_AGENT,
        .max_redirection_count = 5,
    };
    esp_http_client_handle_t c = esp_http_client_init(&cfg);
    char *buf = heap_caps_malloc(4096, MALLOC_CAP_SPIRAM);
    bool ok = false;
    if (!c || !buf) {
        goto out;
    }
    for (int hop = 0; hop < 5; hop++) {
        if (esp_http_client_open(c, 0) != ESP_OK) {
            goto out;
        }
        esp_http_client_fetch_headers(c);
        int st = esp_http_client_get_status_code(c);
        *status_out = st;
        if (st == 301 || st == 302 || st == 303 || st == 307 || st == 308) {
            esp_http_client_flush_response(c, NULL);
            esp_http_client_set_redirection(c);
            esp_http_client_close(c);
            continue;
        }
        if (st != 200) {
            goto out;
        }
        while (*total < CAL_MAX_BYTES) {
            int r = esp_http_client_read(c, buf, 4096);
            if (r <= 0) {
                break;
            }
            *total += (size_t)r;
            if (!cb(buf, (size_t)r, ctx)) {
                goto out;
            }
        }
        ok = esp_http_client_is_complete_data_received(c);
        break;
    }
out:
    free(buf);
    if (c) {
        esp_http_client_close(c);
        esp_http_client_cleanup(c);
    }
    return ok;
}

/* ------------------------------------------------------------------ geocode + weather */

static bool geocode(void)
{
    char url[WB_URL_MAX];
    if (wb_geocode_url(url, sizeof(url), S->place_query) >= (int)sizeof(url)) {
        ESP_LOGE(TAG, "geocode url too long");
        return false;
    }
    char *body;
    size_t len;
    if (!net_http_get(url, &body, &len)) {
        return false;
    }
    double lat, lon;
    char name[64];
    bool ok = wx_parse_geocode(body, len, &lat, &lon, name, sizeof(name));
    free(body);
    if (ok) {
        S->lat = lat;
        S->lon = lon;
        S->has_coords = true;
        snprintf(S->place_name, sizeof(S->place_name), "%s", name);
        settings_save(S);
        ESP_LOGI(TAG, "'%s' -> %s (%.4f, %.4f)", S->place_query, name, lat, lon);
    } else {
        ESP_LOGW(TAG, "no match for '%s'", S->place_query);
    }
    return ok;
}

static bool fetch_weather(void)
{
    char url[WB_URL_MAX];
    if (wb_forecast_url(url, sizeof(url), S->lat, S->lon, S->celsius) >= (int)sizeof(url)) {
        ESP_LOGE(TAG, "forecast url too long");
        return false;
    }
    char *body;
    size_t len;
    if (!net_http_get(url, &body, &len)) {
        return false;
    }
    static wx_t fresh;
    bool ok = wx_parse_openmeteo(body, len, !S->celsius, &fresh);
    free(body);
    if (!ok) {
        ESP_LOGW(TAG, "could not parse forecast");
        return false;
    }
    s_wx = fresh;
    s_utc_offset = s_wx.utc_offset_s;
    s_have_offset = true;
    set_local_tz(s_utc_offset);
    ESP_LOGI(TAG, "now %.1f (feels %.1f) code %d, %d periods", s_wx.temp, s_wx.feels, s_wx.code, s_wx.nperiods);
    return true;
}

/* US National Weather Service alerts for the spot. Outside the US this just fails quietly. */
static void fetch_alerts(void)
{
    if (!S->alerts) {
        s_alerts.n = 0;
        return;
    }
    char url[WB_URL_MAX];
    wb_alerts_url(url, sizeof(url), S->lat, S->lon);
    char *body;
    size_t len;
    static wx_alerts_t fresh;
    bool ok = net_http_get(url, &body, &len);
    if (ok) {
        ok = alerts_parse_nws(body, len, &fresh);
        free(body);
    }
    if (ok) {
        s_alerts = fresh;
        s_alert_fails = 0;
        for (int i = 0; i < s_alerts.n; i++) {
            ESP_LOGI(TAG, "alert: %s", s_alerts.a[i].event);
        }
    } else if (++s_alert_fails >= ALERT_KEEP_FAILS) {
        s_alerts.n = 0; /* stale; better none than an old one */
    }
}

const char *net_calendar_status(void)
{
    return s_cal_status;
}

static bool cal_chunk(const char *data, size_t len, void *ctx)
{
    cal_feed((cal_t *)ctx, data, len);
    return true;
}

/* The family calendar, read hourly (or on Refresh, or when the day changes). Keeps the last good
 * read if a fetch fails: its events carry their dates, so they stay right. */
static void fetch_calendar(bool forced)
{
    if (!S->ical_url[0]) {
        s_ncal = 0;
        return;
    }
    int today = cal_dayno(s_wx.year, s_wx.month, s_wx.day);
    int64_t now = esp_timer_get_time();
    if (!forced && s_cal_at >= 0 && now - s_cal_at < CAL_REFRESH_US && today == s_cal_day) {
        return;
    }
    static cal_t cal;
    cal_begin(&cal, today, s_wx.utc_offset_s, S->cal_words);
    int status;
    size_t total;
    bool ok = net_http_stream(S->ical_url, cal_chunk, &cal, &status, &total);
    bool seen = cal_end(&cal);
    char when[16];
    wb_fmt_clock12(s_wx.hour * 60 + s_wx.minute, when, sizeof(when));
    if (ok && seen) {
        s_ncal = cal_to_events(&cal, s_cal_events, 0, CAL_MAX);
        s_cal_at = now;
        s_cal_day = today;
        wb_state_t *st = app_state();
        UI_LOCKED({
            cal_publish(&cal);   /* My Day's list (the screens read it while drawing) */
            st->ndress = cal_dress_days(&cal, st->dress_days, WB_DRESS_MAX);   /* "(dress)" events */
            st->cal_reads++;
        });
        snprintf(s_cal_status, sizeof(s_cal_status), "Read at %s: %d events in the calendar, %d coming up.", when,
                 cal.nevents_seen, s_ncal);
    } else if (status == 404 || status == 403 || status == 401) {
        s_ncal = 0;
        snprintf(s_cal_status, sizeof(s_cal_status), "Google says the link doesn't work (%d). Copy it again?",
                 status);
    } else if (ok) {
        snprintf(s_cal_status, sizeof(s_cal_status), "That link isn't a calendar. Use the iCal address.");
    } else {
        snprintf(s_cal_status, sizeof(s_cal_status), "Couldn't reach the calendar at %s (%d). Trying again soon.",
                 when, status);
    }
    if (!(ok && seen)) {
        app_state()->cal_fails++;   /* My Day marks what it shows as old */
    }
    ESP_LOGI(TAG, "calendar: %s (%u bytes)", s_cal_status, (unsigned)total);
}

/* Fills the state's weather, alerts, events and grown-up lines, and hands it to the screens. */
static void show_weather(bool fresh_ok, const char *last_update)
{
    wb_state_t *st = app_state();
    UI_LOCKED({
        snprintf(st->wifi, sizeof(st->wifi), "%s", S->ssid);
        snprintf(st->place, sizeof(st->place), "%s", S->place_name);
        snprintf(st->ip, sizeof(st->ip), "%s", s_ip);
        snprintf(st->version, sizeof(st->version), "%s", WB_VERSION);
        st->updated[0] = 0;
        if (last_update[0]) {
            snprintf(st->updated, sizeof(st->updated), "Weather updated %s", last_update);
        }
        if (fresh_ok) {
            snprintf(st->status, sizeof(st->status), "Updated %s", last_update);
        } else {
            snprintf(st->status, sizeof(st->status), "Offline. Last update %s", last_update);
        }
        st->status_warn = !fresh_ok;
        app_set_events(s_cal_events, s_ncal);
        st->alerts = s_alerts;
        st->wx = s_wx;
        st->have_wx = s_wx.valid;
    });
    app_publish();
}

/* ------------------------------------------------------------------ main loop */

static void go_portal(void)
{
    s_portal = true;
    esp_timer_stop(s_reconnect);
    portal_run(S); /* does not return */
}

static void net_task(void *arg)
{
    (void)arg;
    /* A failed WiFi bring-up is retried, never fatal: the screen is
     * already up, and the loop below simply has no network until an
     * attempt lands (see wifi_init). While retries run, the boot
     * screen switches to its connecting state, so the page never sits
     * on unexplained old (or no) weather; the first successful fetch
     * replaces it like any normal boot. */
    bool init_shown = false;
    while (!wifi_init()) {
        if (!init_shown) {
            UI_LOCKED(wb_ui_show_connecting(S->ssid));
            init_shown = true;
        }
        ESP_LOGW(TAG, "wifi bring-up failed, retrying in %u s", (unsigned)(WIFI_INIT_RETRY_MS / 1000));
        vTaskDelay(pdMS_TO_TICKS(WIFI_INIT_RETRY_MS));
    }

    if (!S->ssid[0]) {
        go_portal();
    }

    UI_LOCKED(wb_ui_show_connecting(S->ssid));
    while (!wifi_start_sta()) {
        ESP_LOGW(TAG, "wifi start failed, retrying in %u s", (unsigned)(WIFI_INIT_RETRY_MS / 1000));
        vTaskDelay(pdMS_TO_TICKS(WIFI_INIT_RETRY_MS));
    }

    /* Wait for the first connection, showing an error (with a setup button) if it takes too long. */
    bool shown_error = false;
    while (1) {
        EventBits_t b = xEventGroupWaitBits(s_ev, EV_CONNECTED | EV_PORTAL, pdFALSE, pdFALSE,
                                            pdMS_TO_TICKS(CONNECT_WAIT_MS));
        if (b & EV_PORTAL) {
            go_portal();
        }
        if (b & EV_CONNECTED) {
            break;
        }
        if (!shown_error) {
            UI_LOCKED(wb_ui_show_wifi_error(S->ssid, NULL));
            shown_error = true;
        }
    }

    esp_sntp_config_t sntp = ESP_NETIF_SNTP_DEFAULT_CONFIG("pool.ntp.org");
    esp_netif_sntp_init(&sntp);

    if (!S->has_coords) {
        UI_LOCKED(wb_ui_show_boot("Finding your town..."));
        while (!geocode()) {
            UI_LOCKED(wb_ui_show_wifi_error(S->ssid, "Couldn't find your town. Check the spelling in setup."));
            EventBits_t b = xEventGroupWaitBits(s_ev, EV_PORTAL | EV_REFRESH, pdTRUE, pdFALSE,
                                                pdMS_TO_TICKS(RETRY_FAST_MS));
            if (b & EV_PORTAL) {
                go_portal();
            }
        }
    }

    UI_LOCKED(wb_ui_show_boot("Asking the sky..."));
    bool have = false;
    bool forced = true;
    int fails = 0;
    char last[16] = "";
    while (1) {
        /* Don't hammer the API while WiFi is down; the driver keeps reconnecting on its own. */
        xEventGroupWaitBits(s_ev, EV_CONNECTED, pdFALSE, pdFALSE, pdMS_TO_TICKS(CONNECT_WAIT_MS));
        bool ok = (xEventGroupGetBits(s_ev) & EV_CONNECTED) && fetch_weather();
        if (ok) {
            board_mem_log("weather");
            if (!have) {
                /* WiFi and the secure download both work, so now we know how much
                 * internal RAM they leave; give the spare to the drawing area. */
                board_grow_draw_buffer();
            }
            ota_mark_good();   /* a new version that gets the weather is a keeper */
            fetch_alerts();
            fetch_calendar(forced);
            board_mem_log("calendar");
            board_lvgl_stack_log("calendar");   /* the lvgl stack's deepest known path (Boris's iter-36) */
            fails = 0;
            have = true;
            struct tm t;
            if (net_local_time(&t)) {
                wb_fmt_clock12(t.tm_hour * 60 + t.tm_min, last, sizeof(last));
            } else {
                wb_fmt_clock12(s_wx.hour * 60 + s_wx.minute, last, sizeof(last));
            }
            show_weather(true, last);

            /* Nightly update check, around 3 AM, once a day. */
            static int s_update_day = -1;
            struct tm lt;
            if (S->auto_update && net_local_time(&lt) && lt.tm_hour == 3 && lt.tm_yday != s_update_day) {
                s_update_day = lt.tm_yday;
                ota_run(false);              /* restarts if it installs something */
                show_weather(true, last);    /* else put the weather back */
            }
        } else {
            fails++;
            if (have) {
                show_weather(false, last);
            } else if (!(xEventGroupGetBits(s_ev) & EV_CONNECTED)) {
                UI_LOCKED(wb_ui_show_wifi_error(S->ssid, NULL));
            } else if (fails >= 2) {
                /* One blip on a weak link is normal; only show the error once a quick retry also fails. */
                char msg[160];
                snprintf(msg, sizeof(msg), "Can't reach the weather service. Trying again soon.\n(%s, %u KB free)",
                         s_net_err[0] ? s_net_err : "no answer", (unsigned)(board_free_internal() / 1024));
                UI_LOCKED(wb_ui_show_boot(msg));
            }
        }
        uint32_t wait = ok ? REFRESH_MS
                      : fails < 3 ? RETRY_QUICK_MS
                      : fails < 6 ? RETRY_FAST_MS
                                  : RETRY_SLOW_MS;
        size_t free_int = board_free_internal();
        ESP_LOGI(TAG, "internal RAM free %u KB, drawing area %d lines", (unsigned)(free_int / 1024),
                 board_draw_lines());
        board_lvgl_stack_log("cycle");
        /* Iter-35 guardrail: under 60 KB internal free after a fetch
         * cycle, the next TLS handshake is living on borrowed room;
         * say so loudly. (The 24 KB line below stays the shrink
         * trigger; 40 KB is a rule of thumb, not a measured floor,
         * and the largest free block in the mem[] lines is the
         * number that actually decides whether a big allocation
         * still fits.) */
        if (free_int < 60 * 1024) {
            ESP_LOGW(TAG, "internal RAM low: %u KB free after fetch cycle", (unsigned)(free_int / 1024));
        }
        if (!ok) {
            ESP_LOGW(TAG, "weather fetch failed (%d in a row), retrying in %u s", fails, (unsigned)(wait / 1000));
            if (free_int < 24 * 1024) {
                board_shrink_draw_buffer();   /* maybe memory: hand it back to WiFi */
            }
        }
        EventBits_t b = xEventGroupWaitBits(s_ev, EV_REFRESH | EV_PORTAL | EV_UPDATE, pdTRUE, pdFALSE,
                                            pdMS_TO_TICKS(wait));
        if (b & EV_PORTAL) {
            go_portal();
        }
        if (b & EV_UPDATE) {
            ota_run(true);   /* restarts if it installs something; else back to the weather */
        }
        forced = (b & EV_REFRESH) != 0;
    }
}

static void *json_malloc(size_t n)
{
    return heap_caps_malloc(n, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
}

void net_start(wb_settings_t *s)
{
    S = s;
    /* Alert responses can be a few hundred KB of JSON; keep the parse tree out of internal RAM. */
    cJSON_Hooks hooks = {.malloc_fn = json_malloc, .free_fn = free};
    cJSON_InitHooks(&hooks);
    s_ev = xEventGroupCreate();
    xTaskCreatePinnedToCore(net_task, "net", 12288, NULL, 5, NULL, 0);
}
