#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "timeblock.h"
#include "../log.h"
#include "uuid/uuid.h"

#define SECONDS_IN_DAY 86400

// Utility: Generate UUID string
void generate_uuid(char *uuid_buf) {
    uuid_t binuuid;
    uuid_generate_random(binuuid);
    uuid_unparse_lower(binuuid, uuid_buf);
}

// Create a new timeblock
timeblock_t create_timeblock(const char *name, const char *desc, int day_flags,
                             time_t duration, time_t start_or_day_start) {
    timeblock_t tb;
    memset(&tb, 0, sizeof(tb));

    generate_uuid(tb.uuid);
    strncpy(tb.name, name, NAME_LEN);
    strncpy(tb.desc, desc, DESC_LEN);
    tb.day_frequency = day_flags;
    tb.duration = duration;

    if (day_flags == 0) {
        tb.start = start_or_day_start; // Single event
        tb.day_start = 0;
    } else {
        tb.day_start = start_or_day_start; // Weekly
        tb.start = 0;
    }

    tb.tasks = NULL;
    return tb;
}

// Check if a timeblock is currently active
int timeblock_is_active(const timeblock_t *tb, time_t now) {
    struct tm now_tm;
    localtime_r(&now, &now_tm);

    if (tb->day_frequency == 0) {
        // Single event
        return (now >= tb->start) && (now <= tb->start + tb->duration);
    } else {
        // Weekly recurring
        int weekday = now_tm.tm_wday; // Sunday = 0, Saturday = 6
        int weekday_flag = 1 << (6 - weekday); // Map weekday to flag mask

        if (!(tb->day_frequency & weekday_flag))
            return 0;

        // Time since start of current day
        time_t seconds_today = now_tm.tm_hour * 3600 + now_tm.tm_min * 60 + now_tm.tm_sec;
        return (seconds_today >= tb->day_start) &&
               (seconds_today <= tb->day_start + tb->duration);
    }
}
