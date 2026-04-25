#include "time_service.hpp"

bool time_service_init(void)   { return true; }
bool time_service_is_valid(void) { return false; }

bool time_service_get_local_tm(struct tm *out_tm)
{
    (void)out_tm;
    return false;
}

time_t time_service_get_epoch(void) { return 0; }
