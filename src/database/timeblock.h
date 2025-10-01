#ifndef TIMEBLOCK_H
#define TIMEBLOCK_H

#include <time.h>

#include "task.h"
#include "habit.h"

typedef struct
{
    char uuid[UUID_LEN];
    char name[NAME_LEN], desc[DESC_LEN];

    // --- Event functionality ---

    int day_frequency; // Flag indicating which day of the week the timeblock is repeated on; 0 for single event
    time_t duration;   // Length of timeblock
    time_t start;      // For single events; Time since epoch
    time_t day_start;  // For weekly events; Time since start of day

    task_t *tasks;
} timeblock_t;

// Interface
timeblock_t create_timeblock(const char *name, const char *desc, int day_flags, time_t duration, time_t start_or_day_start);
int timeblock_is_active(const timeblock_t *tb, time_t now);

#endif