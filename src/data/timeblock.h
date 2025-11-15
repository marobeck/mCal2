#pragma once

#include <time.h>
#include <vector>
#include <stdint.h>

#include "task.h"
#include "habit.h"

class Timeblock
{
private:
    char uuid[UUID_LEN];

    // --- Event functionality ---

    int day_frequency; // Flag indicating which day of the week the timeblock is repeated on; 0 for single event
    time_t duration;   // Length of timeblock
    time_t start;      // For single events; Time since epoch
    time_t day_start;  // For weekly events; Time since start of day

public:
    char *name, *desc;
    std::vector<Task> tasks;

    Timeblock(const char *name, const char *desc, uint8_t day_flags, time_t duration, time_t start_or_day_start);

    // --- Add data ---
    void append(Task &e)
    {
        tasks.push_back(e);
    }

    // --- Get data ---
    /**
     * @brief Determines if a timeblock is applicable to a given time
     * @param time Time since epoch to test
     * @return True if time is within the timeblock period, false if otherwise
     */
    bool timeblock_is_active(time_t time);
};