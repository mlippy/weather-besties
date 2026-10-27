/*
 * Which screen design to build. Change the number, then rebuild (idf.py build, or
 * make in sim/). Both use the same weather, settings, setup page, calendar, alerts
 * and over-the-air updates; only the screens and their artwork differ, and only the
 * chosen one's artwork goes into the firmware.
 *
 *   1  Classic: this fork's own screens (components/wb_ui/classic/): big temperature with a
 *      thermometer, the "what to wear" speech bubble, calm weather alerts, special
 *      events and calendar reminders, countdowns, sun and moon.
 *   0  2.0: Boris's clothing guide (components/wb_ui/ui.c): Pippa (or the dog) dressed
 *      for the day, garment tiles to check off, Today / Tomorrow / My Day (the
 *      family calendar) / Next days, plus this fork's thermometer and Update pill
 *      (ui_v2_extras.c). Special events, countdowns and alerts aren't shown there yet.
 *      The family's board runs this one.
 *
 * WB_UI_EXAMPLE 1 builds neither: the text-only example (components/wb_ui/example/), the
 * smallest design that implements the contract in wb_ui.h. A starting point for your own;
 * see docs/new-ui.md.
 */
#pragma once

#define WB_UI_CLASSIC 0
#define WB_UI_EXAMPLE 0

