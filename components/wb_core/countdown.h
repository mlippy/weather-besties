/*
 * The nearest countdown ("11 more sleeps until Trip to Grandma's!"): from special events
 * with Countdown ticked and calendar titles tagged (countdown), a cake or a party popper.
 * Plain C; both screen designs use it.
 */
#pragma once

#include <stdbool.h>
#include <stddef.h>

#include "events.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Writes the nearest countdown from today (a cal_dayno) up to CAL_COUNTDOWN_DAYS ahead into
 * buf. short_form drops "more" ("11 sleeps until ...!") for tight spots. False if none. */
bool wb_countdown_text(const wb_event_t *events, int n, int today, bool short_form, char *buf, size_t cap);

#ifdef __cplusplus
}
#endif
