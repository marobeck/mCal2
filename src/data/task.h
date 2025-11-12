#pragma once

#include <time.h>

#include "defs.h"

typedef enum
{
    INCOMPLETE,
    IN_PROGRESS,
    COMPLETE,
    MFD // Marked For Deletion
} TASK_STATUS;

typedef enum
{
    PRIORITY_LOW = 0,
    PRIORITY_MEDIUM = 1,
    PRIORITY_HIGH = 2,
    PRIORITY_VERY_HIGH = 4
} PRIORITY;

/** Task Struct
 * UUID string used for id to match with JSON format.
 */
class Task
{
public:
    char name[NAME_LEN]; // Title of entry
    char desc[DESC_LEN]; // Verbose description of entry
    Task *prereq = NULL; // Task that must be completed prior to completing this one.

    /* -------------------------------- Location -------------------------------- */
    char uuid[UUID_LEN]; // UUID string of entry
    char timeblock_uuid[UUID_LEN];

    // ----- Task Urgency -----

    int urgency = 0; // Cached urgancy value dirived from due date and priority

    time_t due_date = 0;               // Due date, 0 = undated
    PRIORITY priority = PRIORITY_HIGH; // Priority
    TASK_STATUS status = INCOMPLETE;   // Completion status of the entry

    // --- Habit parameters ---

    /// 0 if not a habit, -1 for weekday based frequency, otherwise this indicates the number of
    /// times this habit should be done per week
    char frequency = 0;
    unsigned char day_frequency = 0;
    time_t completed_days[10]; // last 10 timestamps of days where task was complete

    /* -------------------------------------------------------------------------- */
    /*                                  Functions                                 */
    /* -------------------------------------------------------------------------- */

    /* ------------------------------ Constructors ------------------------------ */

    // Creates empty task
    Task();

    /* ------------------------------ Handle habit ------------------------------ */

    void update_due_date();
};