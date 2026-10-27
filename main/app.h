/*
 * The controller's one copy of what the screens show (wb_state.h). net.c fills in the
 * weather, alerts and calendar parts; main.c the settings parts. app_publish() hands it
 * to the screens through the contract in wb_ui.h.
 */
#pragma once

#include "wb_state.h"

wb_state_t *app_state(void);   /* change it from the net task, or with the LVGL lock held */
void app_publish(void);        /* takes the LVGL lock and calls wb_ui_update() */
