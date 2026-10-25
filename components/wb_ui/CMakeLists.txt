# Shared UI, weather parsing and advice logic (also compiled by sim/ on a PC).
# ui_select.h picks the screens: this fork's classic ones (classic/) or Boris's 2.0 (ui.c).
file(STRINGS "${CMAKE_CURRENT_LIST_DIR}/ui_select.h" _wb_sel REGEX "^#define WB_UI_CLASSIC ")
string(REGEX MATCH "[01]" WB_UI_CLASSIC "${_wb_sel}")

set(WB_SHARED_SRCS "wx.c" "wb_request.c" "events.c" "cal.c" "alerts.c" "update.c" "thermo.c")
# The family's Brittany (tools/dog/): heads for both designs, full-body sprites for 2.0.
include(${CMAKE_CURRENT_LIST_DIR}/dog/dog_assets.cmake)

if(WB_UI_CLASSIC)
    include(${CMAKE_CURRENT_LIST_DIR}/classic/assets/assets_list.cmake)
    list(TRANSFORM WB_ASSET_SRCS PREPEND "classic/")
    set(WB_UI_SRCS "classic/ui.c" "classic/advice.c" "classic/ui_classic_extras.c"
        ${WB_ASSET_SRCS} ${WB_DOG_HEAD_SRCS})
else()
    include(${CMAKE_CURRENT_LIST_DIR}/assets/assets_list.cmake)
    include(${CMAKE_CURRENT_LIST_DIR}/assets/pippa_assets_list.cmake)
    set(WB_UI_SRCS "ui.c" "cat.c" "outfit.c" "ui_v2_extras.c" ${WB_ASSET_SRCS} ${WB_PIPPA_ASSET_SRCS}
        ${WB_DOG_HEAD_SRCS} ${WB_DOG_STAGE_SRCS})
endif()
message(STATUS "Weather Besties screens: ${WB_UI_CLASSIC} (1 = classic, 0 = 2.0)")

idf_component_register(
    SRCS ${WB_SHARED_SRCS} ${WB_UI_SRCS}
    INCLUDE_DIRS "."
    REQUIRES lvgl json)

target_compile_definitions(${COMPONENT_LIB} PRIVATE LV_LVGL_H_INCLUDE_SIMPLE)
target_compile_options(${COMPONENT_LIB} PRIVATE -Wno-format-truncation)
