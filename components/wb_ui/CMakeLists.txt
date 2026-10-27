# The screens (also compiled by sim/ on a PC). All the logic is in ../wb_core.
# ui_select.h picks the screens: this fork's classic ones (classic/), Boris's 2.0 (ui.c), or the
# text-only example (example/).
file(STRINGS "${CMAKE_CURRENT_LIST_DIR}/ui_select.h" _wb_sel REGEX "^#define WB_UI_CLASSIC ")
string(REGEX MATCH "[01]" WB_UI_CLASSIC "${_wb_sel}")
file(STRINGS "${CMAKE_CURRENT_LIST_DIR}/ui_select.h" _wb_ex REGEX "^#define WB_UI_EXAMPLE ")
string(REGEX MATCH "[01]" WB_UI_EXAMPLE "${_wb_ex}")

set(WB_SHARED_SRCS "wb_ui.c" "thermo.c")   # wb_ui.c: the contract (wb_ui.h) over each design's setters
# The family's Brittany (tools/dog/): heads for both designs, full-body sprites for 2.0.
include(${CMAKE_CURRENT_LIST_DIR}/dog/dog_assets.cmake)
# The Routine page's step pictures (tools/routine/), 2.0 only.
include(${CMAKE_CURRENT_LIST_DIR}/routine/routine_icons.cmake)

if(WB_UI_EXAMPLE)
    # implements wb_ui.h itself, so none of the shared setter code
    set(WB_SHARED_SRCS)
    set(WB_UI_SRCS "example/wb_ui_text.c")
elseif(WB_UI_CLASSIC)
    include(${CMAKE_CURRENT_LIST_DIR}/classic/assets/assets_list.cmake)
    list(TRANSFORM WB_ASSET_SRCS PREPEND "classic/")
    set(WB_UI_SRCS "classic/ui.c" "classic/ui_classic_extras.c"
        ${WB_ASSET_SRCS} ${WB_DOG_HEAD_SRCS})
else()
    include(${CMAKE_CURRENT_LIST_DIR}/assets/assets_list.cmake)
    include(${CMAKE_CURRENT_LIST_DIR}/assets/pippa_assets_list.cmake)
    set(WB_UI_SRCS "ui.c" "ui_v2_extras.c" ${WB_ASSET_SRCS} ${WB_PIPPA_ASSET_SRCS}
        ${WB_DOG_HEAD_SRCS} ${WB_DOG_STAGE_SRCS} ${WB_ROUTINE_SRCS})
endif()
message(STATUS "Weather Besties screens: classic ${WB_UI_CLASSIC}, example ${WB_UI_EXAMPLE}")

idf_component_register(
    SRCS ${WB_SHARED_SRCS} ${WB_UI_SRCS}
    INCLUDE_DIRS "."
    REQUIRES lvgl json wb_core)

target_compile_definitions(${COMPONENT_LIB} PRIVATE LV_LVGL_H_INCLUDE_SIMPLE)
target_compile_options(${COMPONENT_LIB} PRIVATE -Wno-format-truncation)
