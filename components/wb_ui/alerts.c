#include "alerts.h"

#include <stdio.h>
#include <string.h>

#include "cJSON.h"

/* Alerts for boats, firefighters and farmers. */
static const char *SKIP[] = {"Marine", "Small Craft", "Gale", "Seas", "Brisk Wind", "Hurricane Force Wind",
                             "Frost", "Freeze", "Red Flag", "Fire Weather", "Low Water", "Hydrologic", "Test"};

/* First match wins, so the specific names come before the general ones.
 * Written for kids: calm, no scary words, grown-ups are in charge. The official
 * name still shows in Settings for the grown-ups. */
static const struct {
    const char *key;
    const char *title;
    const char *kid;
} KID[] = {
    {"Tornado Warning", "Stay close to a grown-up", "Strong storms nearby. The grown-ups are keeping watch."},
    {"Tornado", "Stormy day ahead", "Storms could get strong. Stay near a grown-up."},
    {"Extreme Wind", "Super strong wind", "A good day to stay inside and cozy."},
    {"Hurricane", "Big storm coming", "Your family has a plan. Cozy inside together."},
    {"Tropical Storm", "Big rainy, windy storm", "A good day for indoor fun."},
    {"Severe Thunderstorm Warning", "Thunderstorm nearby", "Time to head inside until it passes."},
    {"Thunderstorm", "Thunder might rumble", "If you hear thunder, head inside."},
    {"Flood", "Extra puddly day", "Stay away from big puddles and streams."},
    {"Blizzard", "Big snowstorm", "Stay inside where it's warm and cozy."},
    {"Ice Storm", "Icy and slippery", "Walk slowly, like a penguin!"},
    {"Winter Storm", "Big snow on the way", "Cozy coat, hat and boots!"},
    {"Winter Weather", "Snowy, slippery day", "Boots on, and walk like a penguin!"},
    {"Snow", "Lots of snow", "Bundle up!"},
    {"Wind Chill", "Extra cold day", "Hat, gloves, and cover your cheeks!"},
    {"Cold", "Extra cold day", "Hat, gloves, and cover your cheeks!"},
    {"Heat", "Extra hot day", "Lots of water and shady spots."},
    {"Wind", "Super windy", "Hold on to your hat!"},
    {"Air Quality", "Hazy air today", "A good day for indoor play."},
    {"Smoke", "Hazy air today", "A good day for indoor play."},
    {"Dust", "Dusty air today", "A good day for indoor play."},
    {"Fog", "Foggy day", "Hold a grown-up's hand near streets."},
    {"Rip Current", "Big waves today", "Splash on the sand, not in the ocean."},
    {"Beach Hazards", "Big waves today", "Splash on the sand, not in the ocean."},
    {"Surf", "Big waves today", "Splash on the sand, not in the ocean."},
};

static bool ends_with(const char *s, const char *suffix)
{
    size_t n = strlen(s), m = strlen(suffix);
    return n >= m && strcmp(s + n - m, suffix) == 0;
}

bool alerts_add(wx_alerts_t *al, const char *event, const char *severity)
{
    if (!event || !event[0]) {
        return false;
    }
    for (size_t i = 0; i < sizeof(SKIP) / sizeof(SKIP[0]); i++) {
        if (strstr(event, SKIP[i])) {
            return false;
        }
    }
    if (strcmp(event, "Storm Warning") == 0 || strcmp(event, "Storm Watch") == 0) {
        return false; /* marine; "Winter Storm Warning" and friends are fine */
    }
    alert_level_t lvl;
    if (ends_with(event, "Warning") || ends_with(event, "Emergency")) {
        lvl = ALERT_WARNING;
    } else if (ends_with(event, "Watch")) {
        lvl = ALERT_WATCH;
    } else if (ends_with(event, "Advisory")) {
        lvl = ALERT_ADVISORY;
    } else if (severity && (strcmp(severity, "Severe") == 0 || strcmp(severity, "Extreme") == 0)) {
        lvl = ALERT_ADVISORY; /* a statement, but a serious one */
    } else {
        return false;
    }
    for (int i = 0; i < al->n; i++) {
        if (strcmp(al->a[i].event, event) == 0) {
            return false; /* the same alert from a neighboring office or zone */
        }
    }

    wx_alert_t x = {.level = lvl};
    snprintf(x.event, sizeof(x.event), "%s", event);
    snprintf(x.title, sizeof(x.title), "%s", "Weather heads-up");
    snprintf(x.kid, sizeof(x.kid), "%s", "Ask a grown-up about today's weather.");
    for (size_t i = 0; i < sizeof(KID) / sizeof(KID[0]); i++) {
        if (strstr(event, KID[i].key)) {
            snprintf(x.title, sizeof(x.title), "%s", KID[i].title);
            snprintf(x.kid, sizeof(x.kid), "%s", KID[i].kid);
            break;
        }
    }

    /* Keep the list sorted, most serious first; when full, the least serious falls off. */
    int pos = al->n;
    while (pos > 0 && al->a[pos - 1].level < lvl) {
        pos--;
    }
    if (pos >= ALERT_MAX) {
        return false;
    }
    int last = al->n < ALERT_MAX ? al->n : ALERT_MAX - 1;
    memmove(&al->a[pos + 1], &al->a[pos], (size_t)(last - pos) * sizeof(al->a[0]));
    al->a[pos] = x;
    if (al->n < ALERT_MAX) {
        al->n++;
    }
    return true;
}

static const char *str(const cJSON *obj, const char *key)
{
    const cJSON *it = cJSON_GetObjectItemCaseSensitive(obj, key);
    return cJSON_IsString(it) ? it->valuestring : NULL;
}

bool alerts_parse_nws(const char *json, size_t len, wx_alerts_t *out)
{
    memset(out, 0, sizeof(*out));
    cJSON *root = cJSON_ParseWithLength(json, len);
    if (!root) {
        return false;
    }
    const cJSON *features = cJSON_GetObjectItemCaseSensitive(root, "features");
    bool ok = cJSON_IsArray(features);
    const cJSON *f;
    cJSON_ArrayForEach(f, features)
    {
        const cJSON *p = cJSON_GetObjectItemCaseSensitive(f, "properties");
        const char *status = str(p, "status");
        const char *type = str(p, "messageType");
        if ((status && strcmp(status, "Actual") != 0) || (type && strcmp(type, "Cancel") == 0)) {
            continue;
        }
        alerts_add(out, str(p, "event"), str(p, "severity"));
    }
    cJSON_Delete(root);
    return ok;
}
