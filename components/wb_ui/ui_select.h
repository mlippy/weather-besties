/*
 * Which screen design to build. Change the number, then rebuild (idf.py build, or
 * make in sim/). Both use the same weather, settings, setup page, calendar, alerts
 * and over-the-air updates; only the screens and their artwork differ, and only the
 * chosen one's artwork goes into the firmware.
 *
 *   1  Classic: Piper's screens (components/wb_ui/classic/): big temperature with a
 *      thermometer, the "what to wear" speech bubble, calm weather alerts, special
 *      events and calendar reminders, countdowns, sun and moon.
 *   0  2.0: Boris's clothing guide (components/wb_ui/ui.c): Pippa dressed for the
 *      day, garment tiles to check off, Today / Tomorrow / Next days. Events,
 *      calendar reminders and alerts aren't shown there yet.
 */
#pragma once

#define WB_UI_CLASSIC 1
