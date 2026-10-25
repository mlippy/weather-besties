cmake_minimum_required(VERSION 3.16)
include($ENV{IDF_PATH}/tools/cmake/project.cmake)

# LVGL is configured only by components/lv_conf.h (shared with the desktop renderer in sim/).
idf_build_set_property(COMPILE_DEFINITIONS "LV_KCONFIG_IGNORE" APPEND)

# The image's own version (checked by over-the-air updates) comes from WB_VERSION in main/net.h.
file(STRINGS "${CMAKE_CURRENT_LIST_DIR}/main/net.h" _wb_ver REGEX "#define WB_VERSION ")
string(REGEX MATCH "\"([^\"]+)\"" _wb_ver_q "${_wb_ver}")
set(PROJECT_VER "${CMAKE_MATCH_1}")

project(weather_besties)

# LVGL 9.5's bin decoder logs an enum with a %lu format (lv_bin_decoder.c), which only
# compiles once LV_BIN_DECODER_RAM_LOAD is on (needed for the RLE-compressed artwork).
idf_component_get_property(_lvgl_lib lvgl COMPONENT_LIB)
target_compile_options(${_lvgl_lib} PRIVATE -Wno-error=format)
