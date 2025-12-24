#include "task.h"
#include <sqlite3.h>

// Append a new task to the database
int task_create_db(sqlite3 *db, Task *tb);

/// @brief Retrieves a number of tasks sorted by date from the database with some offset from
///        the most urgant task (sort ASC (time - priority * PRIORITY_MULTIPLIER))
/// @param db SQLite database object
/// @param taskBuffer Array of objects used to store the retrieved tasks
/// @param count Number of objects to store on the buffer
/// @param offset Offset
int task_retrieve_page_db(sqlite3 *db, Task *tb_page, unsigned int count, unsigned int offset);

/// @brief Modifies the completion status of the task on the database or removes it from the
///        database
/// @param db SQLite database object
/// @param uuid Task's unique ID
/// @param new_status New status. If this is MFD the object is deleted
/// @returns Error Code
int task_update_status_db(sqlite3 *db, const char *uuid, TaskStatus new_status);

// Delete a timeblock
int task_delete_db(sqlite3 *db, const char *UUID);

// --- Entry Recovery ---

int RetrieveTasksSortedDB(sqlite3 *db, Task *taskBuffer, int count, int offset);

// --- Utility ---
int AddTaskDB(sqlite3 *db, Task *ent);
void task_print(Task ent);
int task_print_db(sqlite3 *db, const char *id);