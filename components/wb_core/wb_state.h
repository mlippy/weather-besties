/*
 * Everything the screens show, in one struct. The controller (main/) fills it in and
 * hands it to the screens with wb_ui_update(); the screens read it and never change it.
 * A new screen design only needs this, wb_ui.h, and the logic in this component.
 *
 * Plain C, no LVGL and no board: the PC renderer and tests build it as is.
 */
#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "alerts.h"
#include "events.h"
#include "wx.h"

#ifdef __cplusplus
extern "C" {
#endif

#define WB_DRESS_DAYS_MAX 8

typedef enum {
    WB_PET_BLACK_CAT = 0,
    WB_PET_PINK_CAT = 1,
    WB_PET_BOTH_CATS = 2,   /* taking turns */
    WB_PET_DOG = 3,
} wb_pet_t;

typedef struct {
    uint32_t seq;            /* goes up by one on every wb_ui_update(); screens can skip work when unchanged */

    /* ---- chosen on the setup page (or in Settings on the screen) ---- */
    char names[48];          /* the greeting's name(s); "" = none */
    char pet_name[24];       /* the dog's name; "" shows "Dog" */
    wb_pet_t pet;
    int brightness;          /* 30..100 % */
    int leave_min, bed_min;  /* Routine page: leaving for school, bedtime (minutes after midnight) */
    int coat_f, jacket_f, hoodie_f;   /* outfit cut-offs, feels-like °F at departure; 0 = the engine's own */
    int warm_pm_f;           /* a warm afternoon makes a coat morning a light-jacket one; 0 = off */
    int bike_bus_wday;       /* 0 = Sunday ... 6 = Saturday, -1 = none */
    bool celsius;

    /* ---- what the board knows ---- */
    bool have_wx;
    wx_t wx;                 /* the latest forecast (valid when have_wx) */
    wx_alerts_t alerts;      /* most serious first; n == 0 = none */
    int nevents;
    wb_event_t events[EVENT_LIST_MAX];   /* special events from setup + calendar items */
    int ndress;
    int dress_days[WB_DRESS_DAYS_MAX];   /* dress-up days from the calendar (cal_dayno) */

    /* the family calendar: its events for My Day are in cal.c (cal_count / cal_get) */
    bool cal_configured;
    uint32_t cal_reads;      /* goes up on every good read */
    uint32_t cal_fails;      /* goes up on every failed read */

    /* ---- for grown-ups: Settings and the footer ---- */
    char wifi[40];
    char place[64];
    char ip[20];
    char updated[48];
    char version[24];
    char status[96];         /* "Weather updated 7:30 AM", "Offline. Last update ..." */
    bool status_warn;
} wb_state_t;

#ifdef __cplusplus
}
#endif
