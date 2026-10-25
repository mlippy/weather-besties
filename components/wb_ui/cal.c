#include "cal.h"

#include <ctype.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ------------------------------------------------------------------ dates */

int cal_dayno(int y, int m, int d)
{
    /* days_from_civil, Howard Hinnant */
    y -= m <= 2;
    int era = (y >= 0 ? y : y - 399) / 400;
    int yoe = y - era * 400;
    int doy = (153 * (m + (m > 2 ? -3 : 9)) + 2) / 5 + d - 1;
    int doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
    return era * 146097 + doe - 719468;
}

void cal_date(int z, int *y, int *m, int *d)
{
    z += 719468;
    int era = (z >= 0 ? z : z - 146096) / 146097;
    int doe = z - era * 146097;
    int yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365;
    int doy = doe - (365 * yoe + yoe / 4 - yoe / 100);
    int mp = (5 * doy + 2) / 153;
    *d = doy - (153 * mp + 2) / 5 + 1;
    *m = mp < 10 ? mp + 3 : mp - 9;
    *y = yoe + era * 400 + (*m <= 2);
}

int cal_wday(int dayno)
{
    int w = (dayno + 4) % 7; /* 1970-01-01 was a Thursday */
    return w < 0 ? w + 7 : w;
}

static int days_in_month(int y, int m)
{
    static const int dim[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    bool leap = (y % 4 == 0 && y % 100 != 0) || y % 400 == 0;
    return m == 2 && leap ? 29 : dim[m - 1];
}

/* "20261024", "20261024T090000" or "20261024T130000Z" -> local day + minute (-1 = all day). */
static bool parse_dt(const cal_t *c, const char *v, int *day, int *min)
{
    int y, mo, d, hh = 0, mm = 0;
    if (strlen(v) < 8 || sscanf(v, "%4d%2d%2d", &y, &mo, &d) != 3 || mo < 1 || mo > 12 || d < 1 || d > 31) {
        return false;
    }
    *day = cal_dayno(y, mo, d);
    if (v[8] != 'T') {
        *min = -1;
        return true;
    }
    if (sscanf(v + 9, "%2d%2d", &hh, &mm) != 2) {
        return false;
    }
    int t = hh * 60 + mm;
    if (strchr(v, 'Z')) {
        t += c->utc_offset_s / 60;
        while (t < 0) {
            t += 1440;
            (*day)--;
        }
        while (t >= 1440) {
            t -= 1440;
            (*day)++;
        }
    }
    *min = t;
    return true;
}

/* ------------------------------------------------------------------ results */

static int item_cmp(const cal_item_t *a, int dayno, int minute)
{
    if (a->dayno != dayno) {
        return a->dayno < dayno ? -1 : 1;
    }
    return a->minute < minute ? -1 : a->minute > minute ? 1 : 0;
}

/* Sorted insert; when full, the latest one falls off. */
static void add_item(cal_t *c, const cal_item_t *it)
{
    int pos = c->n;
    while (pos > 0 && item_cmp(&c->items[pos - 1], it->dayno, it->minute) > 0) {
        pos--;
    }
    if (pos >= CAL_MAX) {
        return;
    }
    int last = c->n < CAL_MAX ? c->n : CAL_MAX - 1;
    memmove(&c->items[pos + 1], &c->items[pos], (size_t)(last - pos) * sizeof(c->items[0]));
    c->items[pos] = *it;
    if (c->n < CAL_MAX) {
        c->n++;
    }
}

/* ------------------------------------------------------------------ titles */

static bool icase_at(const char *s, const char *w, size_t n)
{
    for (size_t i = 0; i < n; i++) {
        if (!s[i] || tolower((unsigned char)s[i]) != tolower((unsigned char)w[i])) {
            return false;
        }
    }
    return true;
}

static char *icase_find(char *s, const char *w)
{
    size_t n = strlen(w);
    for (; *s; s++) {
        if (icase_at(s, w, n)) {
            return s;
        }
    }
    return NULL;
}

static void cut(char *s, size_t n)
{
    memmove(s, s + n, strlen(s + n) + 1);
}

/* Removes every occurrence of tag; true if there was one. */
static bool take_tag(char *s, const char *tag)
{
    bool found = false;
    char *p;
    while ((p = icase_find(s, tag)) != NULL) {
        cut(p, strlen(tag));
        found = true;
    }
    return found;
}

/* Next comma-separated word of the filter list into w; returns the rest or NULL. */
static const char *next_word(const char *list, char *w, size_t cap)
{
    while (*list == ',' || *list == ' ') {
        list++;
    }
    if (!*list) {
        return NULL;
    }
    size_t n = 0;
    while (*list && *list != ',') {
        if (n + 1 < cap) {
            w[n++] = *list;
        }
        list++;
    }
    while (n && w[n - 1] == ' ') {
        n--;
    }
    w[n] = 0;
    return list;
}

static void tidy(char *s)
{
    /* collapse spaces, trim spaces and joining punctuation at the ends */
    char *o = s;
    for (char *p = s; *p; p++) {
        if (*p == ' ' && (o == s || o[-1] == ' ')) {
            continue;
        }
        *o++ = *p;
    }
    *o = 0;
    size_t n = strlen(s);
    while (n && strchr(" :-,&/|", s[n - 1])) {
        s[--n] = 0;
    }
    size_t lead = 0;
    while (s[lead] && strchr(" :-,&/|", s[lead])) {
        lead++;
    }
    cut(s, lead);
}

/* Cleans up a title for the screen and reads its tags. False if the filter words say skip it. */
static bool title_for_screen(const cal_t *c, const char *summary, char *out, size_t cap, bool *outside,
                             bool *countdown)
{
    char s[128];
    snprintf(s, sizeof(s), "%s", summary);

    *outside = take_tag(s, "(outside)") | take_tag(s, "\xE2\x98\x80");        /* sun */
    *countdown = take_tag(s, "(countdown)") | take_tag(s, "\xF0\x9F\x8E\x89")  /* party popper */
                 | take_tag(s, "\xF0\x9F\x8E\x82");                            /* cake */

    /* The fonts are ASCII: keep curly apostrophes, drop the rest (emoji, accents). */
    char *o = s;
    for (const char *p = s; *p;) {
        unsigned char ch = (unsigned char)*p;
        if (ch < 0x80) {
            *o++ = (char)(ch < 0x20 ? ' ' : ch);
            p++;
        } else if (!strncmp(p, "\xE2\x80\x99", 3) || !strncmp(p, "\xE2\x80\x98", 3)) {
            *o++ = '\'';
            p += 3;
        } else {
            p++;
            while (((unsigned char)*p & 0xC0) == 0x80) {
                p++;
            }
        }
    }
    *o = 0;

    /* Keep titles with a filter word (tagged ones always). A filter word leading the title is
     * dropped from the name ("Kid: Soccer" -> "Soccer"); possessives stay ("Kid's birthday"). */
    tidy(s);
    char kept[128];
    snprintf(kept, sizeof(kept), "%s", s);
    bool any = false, match = *outside || *countdown;
    char w[32];
    for (const char *l = c->words; (l = next_word(l, w, sizeof(w))) != NULL;) {
        any = true;
        size_t n = strlen(w);
        for (char *p = s; n && (p = icase_find(p, w)) != NULL; p += n) {
            bool start_ok = p == s || !isalnum((unsigned char)p[-1]);
            bool end_ok = !isalnum((unsigned char)p[n]);
            if (start_ok && end_ok) {
                match = true;
                if (p == s && p[n] != '\'') {
                    cut(p, n);
                }
                break;
            }
        }
    }
    if (any && !match) {
        return false;
    }
    tidy(s);
    if (!s[0]) {
        snprintf(s, sizeof(s), "%s", kept);
        tidy(s);
    }
    if (!s[0]) {
        return false;
    }
    s[0] = (char)toupper((unsigned char)s[0]);

    /* Fit, breaking at a space if it's too long, and not ending on "at", "to"... */
    if (strlen(s) >= cap) {
        s[cap - 1] = 0;
        char *sp = strrchr(s, ' ');
        if (sp && sp - s > (long)cap / 2) {
            *sp = 0;
        }
        static const char *dangling[] = {"at", "to", "and", "the", "with", "for", "of", "in", "on", "a"};
        for (bool again = true; again;) {
            again = false;
            tidy(s);
            sp = strrchr(s, ' ');
            for (size_t i = 0; sp && i < sizeof(dangling) / sizeof(dangling[0]); i++) {
                if (!strcmp(sp + 1, dangling[i])) {
                    *sp = 0;
                    again = true;
                }
            }
        }
    }
    snprintf(out, cap, "%s", s);
    return true;
}

/* ------------------------------------------------------------------ repeating events */

static bool byday_match(const cal_ev_t *e, int y, int m, int d, int wd)
{
    int dim = days_in_month(y, m);
    for (int i = 0; i < e->nbyday; i++) {
        if (e->byday_wd[i] != wd) {
            continue;
        }
        int ord = e->byday_ord[i];
        if (ord == 0 || (ord > 0 && (d - 1) / 7 + 1 == ord) || (ord < 0 && (dim - d) / 7 + 1 == -ord)) {
            return true;
        }
    }
    return false;
}

static bool bymonthday_match(const cal_ev_t *e, int y, int m, int d)
{
    int dim = days_in_month(y, m);
    for (int i = 0; i < e->nbymonthday; i++) {
        int md = e->bymonthday[i];
        if ((md > 0 && d == md) || (md < 0 && d == dim + md + 1)) {
            return true;
        }
    }
    return false;
}

static int monday_week(int dayno)
{
    return (dayno - (cal_wday(dayno) + 6) % 7) / 7;
}

static bool rule_match(const cal_ev_t *e, int day)
{
    int y, m, d, sy, sm, sd;
    cal_date(day, &y, &m, &d);
    cal_date(e->start_day, &sy, &sm, &sd);
    int wd = cal_wday(day);
    if (e->bymonth && !((e->bymonth >> m) & 1)) {
        return false;
    }
    switch (e->freq) {
    case 'D':
        if ((day - e->start_day) % e->interval) {
            return false;
        }
        if (e->nbyday && !byday_match(e, y, m, d, wd)) {
            return false;
        }
        return !e->nbymonthday || bymonthday_match(e, y, m, d);
    case 'W':
        if ((monday_week(day) - monday_week(e->start_day)) % e->interval) {
            return false;
        }
        return e->nbyday ? byday_match(e, y, m, d, wd) : wd == cal_wday(e->start_day);
    case 'M':
        if (((y - sy) * 12 + (m - sm)) % e->interval) {
            return false;
        }
        break;
    case 'Y':
        if ((y - sy) % e->interval || (!e->bymonth && m != sm)) {
            return false;
        }
        break;
    default:
        return false;
    }
    if (e->nbyday) {
        return byday_match(e, y, m, d, wd);
    }
    if (e->nbymonthday) {
        return bymonthday_match(e, y, m, d);
    }
    return d == sd;
}

static bool excluded(const cal_ev_t *e, int day)
{
    for (int i = 0; i < e->nex; i++) {
        if (e->exdates[i] == day) {
            return true;
        }
    }
    return false;
}

/* ------------------------------------------------------------------ one event */

/* iCal text escapes: \\, \; \, and \n. */
static void unescape(char *out, size_t cap, const char *in)
{
    size_t o = 0;
    for (const char *p = in; *p && o + 1 < cap; p++) {
        if (*p == '\\' && p[1]) {
            p++;
            out[o++] = (*p == 'n' || *p == 'N') ? ' ' : *p;
        } else {
            out[o++] = *p;
        }
    }
    out[o] = 0;
}

static void finish_event(cal_t *c)
{
    cal_ev_t *e = &c->ev;
    c->nevents_seen++;
    if (e->has_recur && c->nover < CAL_OVERRIDES) {
        /* This replaces (or cancels) one instance of a repeating event. */
        c->over[c->nover].uid = e->uid;
        c->over[c->nover].dayno = e->recur_day;
        c->nover++;
    }
    if (!e->has_start || e->cancelled) {
        return;
    }

    cal_item_t it = {.minute = e->start_allday ? -1 : e->start_min, .uid = e->uid, .is_override = e->has_recur};
    char s[128];
    unescape(s, sizeof(s), e->summary);
    if (!title_for_screen(c, s, it.name, sizeof(it.name), &it.outside, &it.countdown)) {
        return;
    }
    title_for_screen(c, s, it.title, sizeof(it.title), &it.outside, &it.countdown);
    unescape(it.location, sizeof(it.location), e->location);
    if (e->has_end && !e->start_allday && e->end_min >= 0) {
        int d = (e->end_day - e->start_day) * 1440 + e->end_min - e->start_min;
        it.dur_min = d > 0 ? d : 0;
    }

    /* Everything today and tomorrow; countdowns up to CAL_COUNTDOWN_DAYS ahead. */
    int w0 = c->today;
    int w1 = c->today + (it.countdown ? CAL_COUNTDOWN_DAYS : 1);

    if (!e->freq) {
        if (it.countdown || !e->start_allday) {
            if (e->start_day >= w0 && e->start_day <= w1) {
                it.dayno = e->start_day;
                add_item(c, &it);
            }
            return;
        }
        /* all day, maybe several days ("Trip to Maine") */
        int last = e->has_end && e->end_day > e->start_day ? e->end_day - 1 : e->start_day;
        for (int d = e->start_day > w0 ? e->start_day : w0; d <= last && d <= w1; d++) {
            it.dayno = d;
            add_item(c, &it);
        }
        return;
    }

    int to = e->until_day < w1 ? e->until_day : w1;
    int from = e->count ? e->start_day : (e->start_day > w0 ? e->start_day : w0);
    int seen = 0;
    for (int d = from; d <= to; d++) {
        if (!rule_match(e, d) && d != e->start_day) {
            continue;
        }
        if (e->count && ++seen > e->count) {
            break;
        }
        if (d >= w0 && !excluded(e, d)) {
            it.dayno = d;
            add_item(c, &it);
            if (it.countdown) {
                break; /* the next one is enough to count down to */
            }
        }
    }
}

/* ------------------------------------------------------------------ properties */

static uint32_t fnv(const char *s)
{
    uint32_t h = 2166136261u;
    for (; *s; s++) {
        h = (h ^ (unsigned char)*s) * 16777619u;
    }
    return h;
}

static int wd_code(const char *s)
{
    static const char *n[] = {"SU", "MO", "TU", "WE", "TH", "FR", "SA"};
    for (int i = 0; i < 7; i++) {
        if (!strncmp(s, n[i], 2)) {
            return i;
        }
    }
    return -1;
}

static void parse_rrule(cal_t *c, cal_ev_t *e, char *v)
{
    e->interval = 1;
    for (char *part = strtok(v, ";"); part; part = strtok(NULL, ";")) {
        char *eq = strchr(part, '=');
        if (!eq) {
            continue;
        }
        *eq++ = 0;
        if (!strcmp(part, "FREQ")) {
            e->freq = !strcmp(eq, "DAILY") ? 'D' : !strcmp(eq, "WEEKLY") ? 'W' : !strcmp(eq, "MONTHLY") ? 'M'
                    : !strcmp(eq, "YEARLY") ? 'Y' : 'X';
        } else if (!strcmp(part, "INTERVAL")) {
            e->interval = atoi(eq) > 0 ? atoi(eq) : 1;
        } else if (!strcmp(part, "COUNT")) {
            e->count = atoi(eq);
        } else if (!strcmp(part, "UNTIL")) {
            int d, mi;
            if (parse_dt(c, eq, &d, &mi)) {
                e->until_day = d;
            }
        } else if (!strcmp(part, "BYDAY")) {
            for (char *t = eq; *t && e->nbyday < 8;) {
                int ord = (int)strtol(t, &t, 10);
                int wd = wd_code(t);
                if (wd >= 0) {
                    e->byday_ord[e->nbyday] = (int8_t)ord;
                    e->byday_wd[e->nbyday] = (int8_t)wd;
                    e->nbyday++;
                }
                t = strchr(t, ',');
                if (!t) {
                    break;
                }
                t++;
            }
        } else if (!strcmp(part, "BYMONTHDAY")) {
            for (char *t = eq; t && *t && e->nbymonthday < 8;) {
                e->bymonthday[e->nbymonthday++] = (int8_t)strtol(t, &t, 10);
                t = *t == ',' ? t + 1 : NULL;
            }
        } else if (!strcmp(part, "BYMONTH")) {
            for (char *t = eq; t && *t;) {
                int m = (int)strtol(t, &t, 10);
                if (m >= 1 && m <= 12) {
                    e->bymonth |= (uint16_t)(1u << m);
                }
                t = *t == ',' ? t + 1 : NULL;
            }
        }
    }
    if (e->freq == 'X') {
        e->freq = 0; /* hourly and friends: treat as a single event */
    }
}

static void handle_line(cal_t *c, char *ln)
{
    /* NAME;PARAM=x;PARAM="a:b":VALUE */
    char *colon = NULL;
    bool q = false;
    for (char *p = ln; *p; p++) {
        if (*p == '"') {
            q = !q;
        } else if (*p == ':' && !q) {
            colon = p;
            break;
        }
    }
    if (!colon) {
        return;
    }
    *colon = 0;
    char *v = colon + 1;
    char *params = strchr(ln, ';');
    if (params) {
        *params++ = 0;
    }
    const char *name = ln;

    if (!strcmp(name, "BEGIN")) {
        if (!strcmp(v, "VCALENDAR")) {
            c->seen = true;
        } else if (!strcmp(v, "VEVENT") && !c->in_event) {
            c->in_event = true;
            c->depth_other = 0;
            memset(&c->ev, 0, sizeof(c->ev));
            c->ev.until_day = INT_MAX;
        } else if (c->in_event) {
            c->depth_other++;
        }
        return;
    }
    if (!strcmp(name, "END")) {
        if (!c->in_event) {
            return;
        }
        if (c->depth_other > 0) {
            c->depth_other--;
        } else if (!strcmp(v, "VEVENT")) {
            c->in_event = false;
            finish_event(c);
        }
        return;
    }
    if (!c->in_event || c->depth_other > 0) {
        return;
    }

    cal_ev_t *e = &c->ev;
    if (!strcmp(name, "SUMMARY")) {
        snprintf(e->summary, sizeof(e->summary), "%s", v);
    } else if (!strcmp(name, "LOCATION")) {
        snprintf(e->location, sizeof(e->location), "%s", v);
    } else if (!strcmp(name, "DTSTART")) {
        e->has_start = parse_dt(c, v, &e->start_day, &e->start_min);
        e->start_allday = e->has_start && e->start_min < 0;
    } else if (!strcmp(name, "DTEND")) {
        e->has_end = parse_dt(c, v, &e->end_day, &e->end_min);
    } else if (!strcmp(name, "RRULE")) {
        parse_rrule(c, e, v);
    } else if (!strcmp(name, "EXDATE")) {
        for (char *t = strtok(v, ","); t && e->nex < 16; t = strtok(NULL, ",")) {
            int d, mi;
            if (parse_dt(c, t, &d, &mi)) {
                e->exdates[e->nex++] = d;
            }
        }
    } else if (!strcmp(name, "RECURRENCE-ID")) {
        int mi;
        e->has_recur = parse_dt(c, v, &e->recur_day, &mi);
    } else if (!strcmp(name, "UID")) {
        e->uid = fnv(v);
    } else if (!strcmp(name, "STATUS")) {
        e->cancelled = !strcmp(v, "CANCELLED");
    }
}

/* ------------------------------------------------------------------ feeding */

void cal_begin(cal_t *c, int today, int utc_offset_s, const char *words)
{
    memset(c, 0, sizeof(*c));
    c->today = today;
    c->utc_offset_s = utc_offset_s;
    snprintf(c->words, sizeof(c->words), "%s", words ? words : "");
}

static void flush_line(cal_t *c)
{
    c->line[c->llen] = 0;
    if (c->llen) {
        handle_line(c, c->line);
    }
    c->llen = 0;
}

/* Lines end in CRLF; a line starting with a space or tab continues the one before. */
void cal_feed(cal_t *c, const char *data, size_t len)
{
    for (size_t i = 0; i < len; i++) {
        char ch = data[i];
        if (ch == '\r') {
            continue;
        }
        if (c->eol) {
            c->eol = false;
            if (ch == ' ' || ch == '\t') {
                continue; /* folded: keep going on the same logical line */
            }
            flush_line(c);
        }
        if (ch == '\n') {
            c->eol = true;
        } else if (c->llen + 1 < sizeof(c->line)) {
            c->line[c->llen++] = ch;
        }
    }
}

bool cal_end(cal_t *c)
{
    flush_line(c);
    /* Drop instances of repeating events that were moved or cancelled. */
    int o = 0;
    for (int i = 0; i < c->n; i++) {
        bool drop = false;
        for (int k = 0; k < c->nover && !c->items[i].is_override; k++) {
            drop |= c->over[k].uid == c->items[i].uid && c->over[k].dayno == c->items[i].dayno;
        }
        if (!drop) {
            c->items[o++] = c->items[i];
        }
    }
    c->n = o;
    return c->seen;
}

int cal_to_events(const cal_t *c, wb_event_t *out, int n, int max)
{
    for (int i = 0; i < c->n && n < max; i++) {
        const cal_item_t *it = &c->items[i];
        wb_event_t *e = &out[n++];
        memset(e, 0, sizeof(*e));
        snprintf(e->name, sizeof(e->name), "%s", it->name);
        int y, m, d;
        cal_date(it->dayno, &y, &m, &d);
        e->year = (uint16_t)y;
        e->month = (uint8_t)m;
        e->day = (uint8_t)d;
        e->all_day = it->minute < 0;
        e->hour = (uint8_t)(it->minute < 0 ? 9 : it->minute / 60);
        e->outside = it->outside;
        e->countdown = it->countdown;
    }
    return n;
}

/* ------------------------------------------------------------------ My Day */

static cal_event_t s_pub[CAL_MAX_EVENTS];
static int s_npub;

static time_t local_time_of(int dayno, int minute)
{
    int y, m, d;
    cal_date(dayno, &y, &m, &d);
    struct tm t = {.tm_year = y - 1900, .tm_mon = m - 1, .tm_mday = d,
                   .tm_hour = minute / 60, .tm_min = minute % 60, .tm_isdst = -1};
    return mktime(&t);
}

void cal_publish(const cal_t *c)
{
    s_npub = 0;
    for (int i = 0; i < c->n && s_npub < CAL_MAX_EVENTS; i++) {
        const cal_item_t *it = &c->items[i];
        if (it->dayno > c->today + 1) {
            continue;   /* a countdown further out: the main screen's, not My Day's */
        }
        cal_event_t *ev = &s_pub[s_npub++];
        memset(ev, 0, sizeof(*ev));
        ev->all_day = it->minute < 0;
        ev->start = local_time_of(it->dayno, ev->all_day ? 0 : it->minute);
        ev->end = ev->all_day ? local_time_of(it->dayno + 1, 0)
                              : ev->start + (time_t)(it->dur_min > 0 ? it->dur_min : 0) * 60;
        snprintf(ev->summary, sizeof(ev->summary), "%s", it->title);
        snprintf(ev->location, sizeof(ev->location), "%s", it->location);
    }
}

int cal_count(void)
{
    return s_npub;
}

const cal_event_t *cal_get(int i)
{
    return i >= 0 && i < s_npub ? &s_pub[i] : NULL;
}
