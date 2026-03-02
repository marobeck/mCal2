#pragma once

#include <time.h>
#include <vector>
#include <stdint.h>

#include "uuid.h"
#include "goalspec.h"
#include "task.h"

enum class TimeblockStatus
{
    /**
     * Timeblock is in use
     */
    ONGOING = 0,
    /**
     * Timeblock was manually stopped before completion
     * Timeblock remains in active list but has urgancy of 0
     */
    STOPPED = 1,
    /**
     * Timeblock completed as scheduled
     * Timeblock is moved to archive upon completion
     */
    DONE = 2,
    /**
     * Timeblock is pinned to top of list, so always shows first
     */
    PINNED = 3
};

class Timeblock
{
public:
    // --- Location ---
    // Used for database fetching
    UUID uuid; // UUID of entry

    // --- Event functionality ---

    GoalSpec day_frequency; // Days of the week this timeblock occurs on; 0 = single event
    time_t duration;        // Length of timeblock
    time_t start;           // For single events; Time since epoch
    time_t day_start;       // For weekly events; Time since start of day

    char *name, *desc;
    std::vector<Task *> tasks;

    TimeblockStatus status = TimeblockStatus::ONGOING;
    time_t completed_datetime = 0; // Time since epoch when timeblock was completed; 0 if not completed

    Timeblock() = default;
    Timeblock(const char *name, const char *desc, uint8_t day_flags, time_t duration, time_t start_or_day_start);

    // --- Add data ---

    void append(Task *e)
    {
        /// Tasks know which timeblock they belong to via their timeblock_uuid field
        /// and persist in memory on the heap, so we can just store pointers to them in the timeblock's task list.
        tasks.push_back(e);
    }

    // --- Get data ---

    /**
     * @brief Determines if a timeblock is applicable to a given time
     * @param time Time since epoch to test
     * @return True if time is within the timeblock period, false if otherwise
     */
    bool timeblock_is_active(time_t time);

    /**
     * Get weight multiplier for status
     */
    float status_weight(TimeblockStatus s) const;

    // --- DEBUG ---
    void print() const;
};