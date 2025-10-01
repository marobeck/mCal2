#ifndef TASK_H
#define TASK_H

#include <sqlite3.h>
#include <time.h>

#include "../defs.h"

typedef enum
{
    INCOMPLETE = 0,
    COMPLETE = 1,
    MFD = 2 // Marked For Deletion
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
typedef struct
{
    char uuid[UUID_LEN]; // UUID string of entry
    char timeblock_uuid[UUID_LEN];
    char name[NAME_LEN]; // Title of entry
    char desc[DESC_LEN]; // Verbose description of entry

    // ----- Task Urgency -----

    time_t due_date;    // Due date
    PRIORITY priority;  // Priority
    TASK_STATUS status; // Completion status of the entry

    // --- Habit parameters ---

    /// 0 if not a habit, -1 for weekday based frequency, otherwise this indicates the number of
    /// times this habit should be done per week
    char frequency;
    unsigned char day_frequency;
} task_t;

// Make a new timeblock
int task_create_db(sqlite3 *db, task_t *tb);

/// @brief Retrieves a number of tasks sorted by date from the database with some offset from
///        the most urgant task (sort ASC (time - priority * PRIORITY_MULTIPLIER))
/// @param db SQLite database object
/// @param taskBuffer Array of objects used to store the retrieved tasks
/// @param count Number of objects to store on the buffer
/// @param offset Offset
int task_retrieve_page_db(sqlite3 *db, task_t *tb_page, unsigned int count, unsigned int offset);

/// @brief Modifies the completion status of the task on the database or removes it from the
///        database
/// @param db SQLite database object
/// @param uuid Task's unique ID
/// @param new_status New status. If this is MFD the object is deleted
/// @returns ESP Error Code
int task_update_status_db(sqlite3 *db, const char *uuid, TASK_STATUS new_status);

// Delete a timeblock
int task_delete_db(sqlite3 *db, const char *UUID);

// --- Entry Recovery ---

int RetrieveTasksSortedDB(sqlite3 *db, task_t *taskBuffer, int count, int offset);

// --- Utility ---
int AddTaskDB(sqlite3 *db, task_t *ent);
void PrintTask(task_t ent);
int PrintTaskDB(sqlite3 *db, const char *id);

#endif
