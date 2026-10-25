#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <time.h>

#include "settings.h"
#include "ui.h"

#define WB_VERSION "1.7.8"

void net_start(wb_settings_t *s);
void net_ui_action(ui_action_t a, void *user);  /* UI button callback */
bool net_local_time(struct tm *out);            /* local clock at the weather location */
const char *net_calendar_status(void);          /* one line for the setup page */

/* Shared with portal.c */
bool net_http_get(const char *url, char **body, size_t *len);
bool net_http_stream(const char *url, bool (*cb)(const char *, size_t, void *), void *ctx, int *status_out,
                     size_t *total);
void portal_run(wb_settings_t *s);  /* never returns: restarts after the form is saved */
