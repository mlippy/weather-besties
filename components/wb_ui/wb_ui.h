/*
 * The screens, as the controller sees them. This is the whole contract: a screen design
 * (2.0, classic, or a new one) implements these, and main/ calls nothing else.
 *
 *   wb_ui_start()    once, with the LVGL lock held, after the display is up
 *   wb_ui_update()   whenever the state changed (weather, settings, calendar...)
 *   wb_ui_show_*()   the four status screens: startup message, setup hotspot,
 *                    connecting, WiFi trouble. The next wb_ui_update() leaves them.
 *
 * What a person does on the screen comes back through one callback (wb_action_t).
 * Call everything from the LVGL thread or with the LVGL lock held. The screens copy
 * what they need from the state, so the controller may change it after the call.
 */
#pragma once

#include <stdbool.h>
#include <time.h>

#include "wb_state.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    WB_ACT_SETUP,            /* change WiFi or town: start the setup hotspot */
    WB_ACT_REFRESH,          /* fetch the weather, alerts and calendar now */
    WB_ACT_UPDATE,           /* check for a firmware update now */
    WB_ACT_SET_PET,          /* value: a wb_pet_t; save it */
    WB_ACT_SET_BRIGHTNESS,   /* value: 30..100; save it (the screen already shows it) */
} wb_action_t;

typedef void (*wb_action_cb_t)(wb_action_t act, int value, void *user);

/* Fills local time; returns false while the clock isn't set yet. */
typedef bool (*wb_clock_cb_t)(struct tm *local_out);

void wb_ui_start(wb_action_cb_t on_action, void *user, wb_clock_cb_t clock);
void wb_ui_update(const wb_state_t *st);

void wb_ui_show_boot(const char *msg);   /* NULL = the design's own startup text */
void wb_ui_show_setup(const char *ap_name, const char *password, const char *url);
void wb_ui_show_connecting(const char *ssid);
void wb_ui_show_wifi_error(const char *ssid, const char *detail);   /* detail: NULL = generic */

#ifdef __cplusplus
}
#endif
