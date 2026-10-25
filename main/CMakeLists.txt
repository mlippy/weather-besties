idf_component_register(
    SRCS "main.c" "board.c" "net.c" "portal.c" "settings.c" "ota.c" "lv_mem_psram.c"
    INCLUDE_DIRS "."
    REQUIRES wb_ui lvgl esp_lcd esp_lcd_touch esp_lcd_touch_gt911 esp_driver_i2c esp_driver_gpio
             esp_wifi esp_netif esp_event esp_http_client esp_http_server mbedtls nvs_flash
             dns_server esp_timer json app_update esp_app_format)

# LVGL calls lv_mem_psram.c's functions, but liblvgl links after libmain: keep them.
foreach(sym lv_mem_init lv_mem_deinit lv_mem_add_pool lv_mem_remove_pool lv_malloc_core lv_realloc_core
            lv_free_core lv_mem_monitor_core lv_mem_test_core)
    target_link_libraries(${COMPONENT_LIB} INTERFACE "-u ${sym}")
endforeach()
