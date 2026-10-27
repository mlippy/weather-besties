/*
 * Dress-up days (2.0 screens): the dress look only on days the calendar tags (dress) or a
 * dress emoji, or that someone tapped Dress up on; a tap beats the calendar. Plain C.
 */
#pragma once

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define WB_DRESS_MAX 8

typedef struct {
    int cal[WB_DRESS_MAX];
    int ncal;
    struct {
        int day;
        bool on;
    } tap[WB_DRESS_MAX];   /* newest last */
    int ntap;
} wb_dress_t;

void wb_dress_set_calendar(wb_dress_t *d, const int *days, int n);   /* days: cal_dayno */
bool wb_dress_on(const wb_dress_t *d, int day);
void wb_dress_toggle(wb_dress_t *d, int day);   /* the Dress up pill; forgets the oldest tap when full */

#ifdef __cplusplus
}
#endif
