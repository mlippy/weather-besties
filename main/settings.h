#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "advice.h"

typedef struct {
    char ssid[33];
    char pass[65];
    char place_query[64];  /* what was typed: "Upper Montclair, NJ" or "07043" */
    char place_name[64];   /* what the geocoder found */
    double lat, lon;
    bool has_coords;
    char names[48];        /* shown in the greeting */
    char ical_url[320];    /* Google Calendar "secret address in iCal format"; empty = none */
    char cal_words[64];    /* keep calendar events whose title has one of these ("Piper, Family") */
    bool celsius;
    bool night_off;        /* screen off at night (tap to wake) */
    bool alerts;           /* show weather.gov alerts on the screen */
    uint8_t off_from, off_to;  /* hours, local time */
    uint8_t nevents;
    wb_event_t events[EVENT_MAX];  /* special events: soccer, field trips... */
} wb_settings_t;

void settings_defaults(wb_settings_t *s);
void settings_load(wb_settings_t *s);
void settings_save(const wb_settings_t *s);
