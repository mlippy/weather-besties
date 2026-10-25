/*
 * This fork's additions to the 2.0 screens, kept out of ui.c so his file only carries the
 * one-line hooks that call these.
 */
#pragma once

#include "lvgl.h"
#include "ui.h"

#ifdef __cplusplus
extern "C" {
#endif

/* The fixed-scale thermometer (thermo.h) in the weather block, just left of the icon.
 * Create once on the outfit page; show it at the icon's x with the temperature the block
 * shows in big type (now on Today, the high on Tomorrow), in °F; hide it when the icon is. */
void v2_thermo_create(lv_obj_t *page);
void v2_thermo_show(int icon_x, float temp_f);
void v2_thermo_hide(void);

/* The name from setup in the title is on by default; Boris's design keeps names off the screen,
 * and his render checks run with it off. */
void v2_set_name_in_title(bool on);

/* Puts the name from setup in the page title: the first of the "%s" formats
 * that fits max_w at the title font, else his own title. Empty names: his title. */
const char *v2_named_title(const char *his_title, const char *names, const char *const *formats, int n,
                           int max_w);

/* An "Update" pill in Settings, right-aligned to x_right at y: closes Settings and asks
 * for a firmware check now (fire is ui.c's action sender). */
void v2_update_pill_create(lv_obj_t *settings, int x_right, int y, void (*fire)(ui_action_t));

#ifdef __cplusplus
}
#endif
