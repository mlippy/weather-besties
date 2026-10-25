#include "settings.h"

#include <stdio.h>
#include <string.h>

#include "esp_log.h"
#include "nvs.h"

static const char *TAG = "settings";
#define NS "wb"

/* Special events the board starts with. Add rows here, or on the phone setup page. Examples:
 *   {.name = "Soccer", .wdays = 1 << 6, .hour = 9, .outside = true}                     every Saturday, 9 AM
 *   {.name = "Bike bus", .wdays = 1 << 5, .hour = 8, .outside = true}                   every Friday, 8 AM
 *   {.name = "Field trip", .year = 2026, .month = 11, .day = 5, .hour = 10, .outside = true}   one day only
 *   {.name = "Library", .wdays = (1 << 2) | (1 << 4), .hour = 9}                         Tue + Thu, indoors
 *   {.name = "Piper's birthday", .year = 2026, .month = 11, .day = 3, .all_day = true, .countdown = true}
 * wdays: bit 0 = Sunday ... bit 6 = Saturday. */
static const wb_event_t DEFAULT_EVENTS[] = {
    {.name = ""}, /* none yet */
};

/* Stored as one blob; bump the version if wb_event_t changes. */
#define EVENTS_VER 2
typedef struct {
    uint8_t ver, n;
    wb_event_t ev[EVENT_MAX];
} events_blob_t;

void settings_defaults(wb_settings_t *s)
{
    memset(s, 0, sizeof(*s));
    snprintf(s->place_query, sizeof(s->place_query), "Upper Montclair, NJ");
    snprintf(s->place_name, sizeof(s->place_name), "Upper Montclair, New Jersey");
    s->lat = 40.8462;
    s->lon = -74.2013;
    s->has_coords = true;
    snprintf(s->names, sizeof(s->names), "Piper");
    snprintf(s->cal_words, sizeof(s->cal_words), "Piper");
    s->celsius = false;
    s->night_off = false;
    s->alerts = true;
    s->auto_update = true;
    s->off_from = 21;
    s->off_to = 6;
    for (size_t i = 0; i < sizeof(DEFAULT_EVENTS) / sizeof(DEFAULT_EVENTS[0]) && s->nevents < EVENT_MAX; i++) {
        if (DEFAULT_EVENTS[i].name[0]) {
            s->events[s->nevents++] = DEFAULT_EVENTS[i];
        }
    }
}

static void get_str(nvs_handle_t h, const char *key, char *dst, size_t cap)
{
    size_t len = cap;
    if (nvs_get_str(h, key, dst, &len) != ESP_OK) {
        /* keep default */
    }
}

static void get_u8(nvs_handle_t h, const char *key, uint8_t *dst)
{
    uint8_t v;
    if (nvs_get_u8(h, key, &v) == ESP_OK) {
        *dst = v;
    }
}

static void get_double(nvs_handle_t h, const char *key, double *dst)
{
    double v;
    size_t len = sizeof(v);
    if (nvs_get_blob(h, key, &v, &len) == ESP_OK && len == sizeof(v)) {
        *dst = v;
    }
}

void settings_load(wb_settings_t *s)
{
    settings_defaults(s);
    nvs_handle_t h;
    if (nvs_open(NS, NVS_READONLY, &h) != ESP_OK) {
        ESP_LOGI(TAG, "no saved settings, using defaults");
        return;
    }
    get_str(h, "ssid", s->ssid, sizeof(s->ssid));
    get_str(h, "pass", s->pass, sizeof(s->pass));
    get_str(h, "pq", s->place_query, sizeof(s->place_query));
    get_str(h, "pn", s->place_name, sizeof(s->place_name));
    get_str(h, "names", s->names, sizeof(s->names));
    get_str(h, "ical", s->ical_url, sizeof(s->ical_url));
    get_str(h, "calw", s->cal_words, sizeof(s->cal_words));
    get_double(h, "lat", &s->lat);
    get_double(h, "lon", &s->lon);
    uint8_t b = s->has_coords;
    get_u8(h, "coords", &b);
    s->has_coords = b;
    b = s->celsius;
    get_u8(h, "cel", &b);
    s->celsius = b;
    b = s->night_off;
    get_u8(h, "noff", &b);
    s->night_off = b;
    b = s->alerts;
    get_u8(h, "alrt", &b);
    s->alerts = b;
    b = s->auto_update;
    get_u8(h, "autoup", &b);
    s->auto_update = b;
    get_u8(h, "nfrom", &s->off_from);
    get_u8(h, "nto", &s->off_to);
    events_blob_t eb;
    size_t len = sizeof(eb);
    if (nvs_get_blob(h, "events", &eb, &len) == ESP_OK && len == sizeof(eb) && eb.ver == EVENTS_VER &&
        eb.n <= EVENT_MAX) {
        s->nevents = eb.n;
        memcpy(s->events, eb.ev, sizeof(s->events));
        for (int i = 0; i < EVENT_MAX; i++) {
            s->events[i].name[EVENT_NAME_LEN - 1] = 0;
        }
    }
    nvs_close(h);
    ESP_LOGI(TAG, "loaded: wifi='%s' place='%s' (%.4f, %.4f)", s->ssid, s->place_name, s->lat, s->lon);
}

void settings_save(const wb_settings_t *s)
{
    nvs_handle_t h;
    if (nvs_open(NS, NVS_READWRITE, &h) != ESP_OK) {
        ESP_LOGE(TAG, "nvs_open failed");
        return;
    }
    nvs_set_str(h, "ssid", s->ssid);
    nvs_set_str(h, "pass", s->pass);
    nvs_set_str(h, "pq", s->place_query);
    nvs_set_str(h, "pn", s->place_name);
    nvs_set_str(h, "names", s->names);
    nvs_set_str(h, "ical", s->ical_url);
    nvs_set_str(h, "calw", s->cal_words);
    nvs_set_blob(h, "lat", &s->lat, sizeof(s->lat));
    nvs_set_blob(h, "lon", &s->lon, sizeof(s->lon));
    nvs_set_u8(h, "coords", s->has_coords);
    nvs_set_u8(h, "cel", s->celsius);
    nvs_set_u8(h, "noff", s->night_off);
    nvs_set_u8(h, "alrt", s->alerts);
    nvs_set_u8(h, "autoup", s->auto_update);
    nvs_set_u8(h, "nfrom", s->off_from);
    nvs_set_u8(h, "nto", s->off_to);
    events_blob_t eb = {.ver = EVENTS_VER, .n = s->nevents};
    memcpy(eb.ev, s->events, sizeof(eb.ev));
    nvs_set_blob(h, "events", &eb, sizeof(eb));
    nvs_erase_key(h, "bbus"); /* the old single bike-bus day */
    nvs_commit(h);
    nvs_close(h);
}
