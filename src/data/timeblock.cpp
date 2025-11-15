#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "timeblock.h"
#include "../log.h"
#include "uuid/uuid.h"

#define SECONDS_IN_DAY 86400

// Create a new timeblock
Timeblock::Timeblock(const char *name_, const char *desc_, uint8_t day_flags,
                     time_t duration, time_t start_or_day_start)
{
    // generate_uuid(tb.uuid);
    name = strdup(name_);
    desc = strdup(desc_);
    day_frequency = day_flags;
    duration = duration;

    if (day_flags == 0)
    {
        start = start_or_day_start; // Single event
        day_start = 0;
    }
    else
    {
        day_start = start_or_day_start; // Weekly
        start = 0;
    }

    tasks = {}; // Empty vector
}

// Check if a timeblock is currently active
bool Timeblock::timeblock_is_active(time_t now)
{
    struct tm now_tm;
    localtime_r(&now, &now_tm);

    if (day_frequency == 0)
    {
        // Single event
        return (now >= start) && (now <= start + duration);
    }
    else
    {
        // Weekly recurring
        int weekday = now_tm.tm_wday;          // Sunday = 0, Saturday = 6
        int weekday_flag = 1 << (6 - weekday); // Map weekday to flag mask

        // Test if it is the correct day
        if (!(day_frequency & weekday_flag))
            return false;

        // Time since start of current day
        time_t seconds_today = now_tm.tm_hour * 3600 + now_tm.tm_min * 60 + now_tm.tm_sec;
        return (seconds_today >= day_start) &&
               (seconds_today <= day_start + duration);
    }
}
