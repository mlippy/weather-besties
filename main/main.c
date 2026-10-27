/*
 * Weather Besties: a kids' weather station.
 * Board: Waveshare ESP32-S3-Touch-LCD-4.3. Weather: Open-Meteo (no API key).
 */
#include <stdio.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "board.h"
#include "esp_log.h"
#include "lvgl.h"
#include "net.h"
#include "nvs_flash.h"
#include "ota.h"
#include "app.h"
#include "settings.h"
#include "wb_ui.h"

static const char *TAG = "main";
static wb_settings_t S;
static wb_state_t s_state;   /* what the screens show (wb_state.h) */

/* Optional "screen off at night": checked once a second in the LVGL thread. */
static void power_timer(lv_timer_t *t)
{
    (void)t;
    if (!S.night_off) {
        if (!board_backlight_is_on()) {
            board_backlight(true);
        }
        return;
    }
    struct tm now;
    if (!net_local_time(&now)) {
        return;
    }
    int h = now.tm_hour;
    bool night = S.off_from < S.off_to ? (h >= S.off_from && h < S.off_to) : (h >= S.off_from || h < S.off_to);
    if (S.off_from == S.off_to) {
        night = false;
    }
    if (night && board_ms_since_touch() > 60 * 1000) {
        if (board_backlight_is_on()) {
            board_backlight(false);
        }
    } else if (!night && !board_backlight_is_on()) {
        board_backlight(true);
    }
}

wb_state_t *app_state(void)
{
    return &s_state;
}

void app_publish(void)
{
    if (board_lvgl_lock(-1)) {
        s_state.seq++;
        wb_ui_update(&s_state);
        board_lvgl_unlock();
    }
}

/* The settings parts of the state, from what's saved. */
static void state_from_settings(void)
{
    snprintf(s_state.names, sizeof(s_state.names), "%s", S.names);
    snprintf(s_state.pet_name, sizeof(s_state.pet_name), "%s", S.pet_name);
    s_state.pet = (wb_pet_t)S.cat;
    s_state.brightness = S.brightness;
    s_state.leave_min = S.leave_min;
    s_state.bed_min = S.bed_min;
    s_state.coat_f = S.coat_f;
    s_state.jacket_f = S.jacket_f;
    s_state.hoodie_f = S.hoodie_f;
    s_state.warm_pm_f = S.warm_pm_f;
    s_state.bike_bus_wday = -1;   /* no bike bus here; special events cover it */
    s_state.celsius = S.celsius;
    s_state.cal_configured = S.ical_url[0] != 0;
    int n = S.nevents < EVENT_MAX ? S.nevents : EVENT_MAX;
    memcpy(s_state.events, S.events, (size_t)n * sizeof(s_state.events[0]));
    s_state.nevents = n;
}

/* What a person did on the screen (wb_ui.h). Runs in the LVGL thread. */
static void on_action(wb_action_t act, int value, void *user)
{
    (void)user;
    switch (act) {
    case WB_ACT_SET_PET:
        S.cat = (uint8_t)value;
        settings_save(&S);
        s_state.pet = (wb_pet_t)value;
        app_publish();   /* show it right away */
        break;
    case WB_ACT_SET_BRIGHTNESS:
        S.brightness = (uint8_t)value;
        settings_save(&S);
        s_state.brightness = value;   /* the screen already shows it */
        break;
    default:
        net_ui_action(act);   /* Setup, Refresh, Update */
        break;
    }
}

void app_main(void)
{
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);
    settings_load(&S);
    ota_boot_check();

    /* Not ESP_ERROR_CHECK: an abort here reboot-loops silently and the
     * board just looks dead (iter-33's PSRAM framebuffer failure). Halt
     * instead, re-logging every 10 s so the failure stays on the console. */
    esp_err_t berr = board_init();
    if (berr != ESP_OK) {
        ESP_LOGE(TAG, "board_init failed: %s; halting, no reboot loop (power-cycle to retry)",
                 esp_err_to_name(berr));
        for (;;) {
            vTaskDelay(pdMS_TO_TICKS(10000));
            ESP_LOGE(TAG, "still halted: board_init failed: %s", esp_err_to_name(berr));
        }
    }

    board_mem_log("board_init");

    state_from_settings();
    board_lvgl_lock(-1);
    wb_ui_start(on_action, NULL, net_local_time);
    s_state.seq++;
    wb_ui_update(&s_state);   /* the settings; the weather comes from net.c */
    lv_timer_create(power_timer, 1000, NULL);
    lv_refr_now(NULL);
    board_lvgl_unlock();
    board_mem_log("ui_init");

    board_backlight(true);
    board_start_lvgl_task();

    ESP_LOGI(TAG, "Weather Besties %s starting", WB_VERSION);
    net_start(&S);
}
