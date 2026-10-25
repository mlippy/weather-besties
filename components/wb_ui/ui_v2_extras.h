/*
 * This fork's additions to the 2.0 screens, kept out of ui.c so his file only carries the
 * one-line hooks that call these.
 */
#pragma once

#include "lvgl.h"
#include <time.h>

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

/* Dress-up days (ui_set_dress_days, the Dress up pill). v2_dress_wanted is the outfit
 * engine's dress_wanted hook. The pill sits right of the day chip on Today / Tomorrow when
 * the weather fits the dress; tapping it flips that day and calls changed(). */
bool v2_dress_wanted(int year, int month, int day);
void v2_dress_create(lv_obj_t *page, void (*changed)(void));
void v2_dress_show(bool fits, int year, int month, int day, int x, int y);
/* Hides the pill: once something is checked off, Boris's "2 of 4 ready" chip takes its
 * place (unchecking everything brings it back). */
void v2_dress_hide(void);

/* My Day's countdown pill, between the page title and the date: the nearest countdown
 * (from ui_set_events) in the classic footer's words, "11 more sleeps until ...!". */
void v2_countdown_create(lv_obj_t *page);
void v2_countdown_show(int year, int month, int day, int x_left, int x_right, int y);
bool v2_countdown_text(int year, int month, int day, char *buf, size_t n, bool short_form);

/* The Routine page (fifth tab): morning checklist before noon with a bar that empties
 * at leaving time, bedtime checklist after with one that empties at bedtime. */
lv_obj_t *v2_routine_create(lv_obj_t *scr);
void v2_routine_render(const struct tm *now, const lv_image_dsc_t *pet);
#ifdef WB_SIM
void v2_routine_test_tap(int row);
#endif

/* Making it obvious what a kid can tap. v2_raise gives a tappable thing the one
 * "button" look: a soft edge underneath, a push-down while pressed, a bigger tap area.
 * Status pills never get it. v2_hint_target names the next thing to tap (NULL = none):
 * after 20 idle seconds it glows gently until the screen is touched. */
void v2_raise(lv_obj_t *o);
void v2_hint_target(lv_obj_t *o);
#ifdef WB_SIM
void v2_hint_test_show(bool on);
#endif

/* An "Update" pill in Settings, right-aligned to x_right at y: closes Settings and asks
 * for a firmware check now (fire is ui.c's action sender). */
void v2_update_pill_create(lv_obj_t *settings, int x_right, int y, void (*fire)(ui_action_t));

#ifdef __cplusplus
}
#endif
