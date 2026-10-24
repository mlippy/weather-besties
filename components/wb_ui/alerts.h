/*
 * Weather alerts from the US National Weather Service (api.weather.gov), boiled
 * down to what a kid needs to hear. Plain C, shared with the desktop renderer.
 */
#pragma once

#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define ALERT_MAX 4

typedef enum { ALERT_ADVISORY = 1, ALERT_WATCH = 2, ALERT_WARNING = 3 } alert_level_t;

typedef struct {
    char event[48];       /* "Winter Storm Warning" */
    char kid[96];         /* "Big snow storm! Bundle up and stay warm." */
    alert_level_t level;
} wx_alert_t;

typedef struct {
    int n;                /* most serious first */
    wx_alert_t a[ALERT_MAX];
} wx_alerts_t;

/* Parses a /alerts/active?point=lat,lon response. Skips alerts that don't matter
 * to a kid (marine, fire weather, frost...). Returns false if the JSON is unreadable. */
bool alerts_parse_nws(const char *json, size_t len, wx_alerts_t *out);

/* Adds one alert by its NWS event name and severity ("Severe", "Moderate"...).
 * Returns false if it was skipped. */
bool alerts_add(wx_alerts_t *al, const char *event, const char *severity);

#ifdef __cplusplus
}
#endif
