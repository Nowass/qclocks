#include "settings_store.hpp"
#include <cstring>

// Default values applied on first boot (spec section 15.3).
static void apply_defaults(AppSettings *s)
{
    s->brightness_day     = 96;
    s->brightness_night   = 24;
    s->night_mode_enabled = true;
    s->night_start_hour   = 22;
    s->night_start_minute = 0;
    s->night_end_hour     = 6;
    s->night_end_minute   = 0;
    strncpy(s->timezone, "CET-1CEST,M3.5.0/2,M10.5.0/3", sizeof(s->timezone) - 1);
    s->timezone[sizeof(s->timezone) - 1] = '\0';
    s->ota_url[0]   = '\0';
    strncpy(s->hostname, "qclocks", sizeof(s->hostname) - 1);
    s->hostname[sizeof(s->hostname) - 1] = '\0';
}

bool settings_init(void) { return true; }

bool settings_load(AppSettings *out)
{
    if (!out) return false;
    apply_defaults(out);
    return true;
}

bool settings_save(const AppSettings *in)
{
    (void)in;
    return true;
}

bool settings_reset(void) { return true; }
