#pragma once

#include <ctime>
#include <cstdint>

// Initialise the time service (registers SNTP, sets TZ).
bool time_service_init(void);

// Returns true if at least one successful NTP sync has been completed.
bool time_service_is_valid(void);

// Fill *out_tm with the current local time.
// Returns false if time is not yet valid.
bool time_service_get_local_tm(struct tm *out_tm);

// Returns the current UNIX epoch (0 if not valid).
time_t time_service_get_epoch(void);
