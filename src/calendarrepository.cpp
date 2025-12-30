#include "calendarrepository.h"
#include "log.h"

#include <time.h>

/* -------------------------------------------------------------------------- */
/*                                Constructors                                */
/* -------------------------------------------------------------------------- */

CalendarRepository::CalendarRepository()
{
    loadAll();
}

CalendarRepository::~CalendarRepository()
{
}

/* -------------------------------------------------------------------------- */
/*                                Load from DB                                */
/* -------------------------------------------------------------------------- */

void CalendarRepository::loadAll()
{
    // Clear current in-memory model
    m_timeblocks.clear();

    // Load timeblocks from database
    m_db.load_timeblocks(m_timeblocks);

    // Load tasks from timeblocks
    for (auto &tb : m_timeblocks)
    {
        m_db.load_tasks(&tb);
    }
}

void CalendarRepository::habitCompletionPreview(Task &task)
{
    const char *TAG = "CalendarRepository::habitCompletionPreview";
    LOGI(TAG, "Loading habit completion preview for task <%s>", task.name);

    // Get current date in ISO 8601 format
    char now_str[11]; // "YYYY-MM-DD" + null terminator
    {
        time_t now = time(nullptr);
        strftime(now_str, sizeof(now_str), "%Y-%m-%d", localtime(&now));
    }

    // Fill task.completed_days with recent completions
    m_db.load_habit_completion_preview(task, now_str);
}

/* -------------------------------------------------------------------------- */
/*                              In-memory access                              */
/* -------------------------------------------------------------------------- */

const std::vector<Timeblock> &CalendarRepository::timeblocks() const
{
    return m_timeblocks;
}

/* -------------------------------------------------------------------------- */
/*                                  Modifiers                                 */
/* -------------------------------------------------------------------------- */

/* ---------------------------------- Tasks --------------------------------- */

bool CalendarRepository::addTask(Task &task, size_t timeblockIndex)
{
    const char *TAG = "CalendarRepository::addTask";
    LOGI(TAG, "Adding task <%s> to timeblock <%s>", task.name, m_timeblocks[timeblockIndex].name);

    // Append to in-memory model
    if (timeblockIndex >= m_timeblocks.size() || timeblockIndex < 0)
    {
        LOGE(TAG, "Invalid timeblock index %zu", timeblockIndex);
        return false;
    }

    // Ensure task has an id and timeblock_uuid set
    if (task.uuid[0] == '\0')
    {
        generate_uuid(task.uuid);
    }

    // copy the timeblock's uuid into the task
    strncpy(task.timeblock_uuid, m_timeblocks[timeblockIndex].uuid, UUID_LEN);
    m_timeblocks[timeblockIndex].tasks.push_back(task);

    try
    {
        m_db.insert_task(task);
    }
    catch (int err)
    {
        LOGE(TAG, "Failed to persist task <%s>: %d", task.name, err);
        m_timeblocks[timeblockIndex].tasks.pop_back(); // Rollback in-memory addition

        return false;
    }

    LOGI(TAG, "Persisted task <%s> to database", task.name);

    // Notify listeners
    emit modelChanged();

    return true;
}

bool CalendarRepository::removeTask(const char *taskUuid)
{
    const char *TAG = "CalendarRepository::removeTask";
    LOGI(TAG, "Removing task with UUID <%s>", taskUuid);

    // Find and remove from in-memory model
    for (auto &tb : m_timeblocks)
    {
        auto &tasks = tb.tasks;
        auto it = std::find_if(tasks.begin(), tasks.end(), [taskUuid](const Task &t)
                               { return strcmp(t.uuid, taskUuid) == 0; });
        if (it != tasks.end())
        {
            tasks.erase(it);
            // Remove from database
            try
            {
                m_db.delete_task(taskUuid);
            }
            catch (int err)
            {
                LOGE(TAG, "Failed to delete task from database: %d", err);
                return false;
            }

            // Notify listeners
            emit modelChanged();

            return true;
        }
    }

    LOGE(TAG, "Task with UUID <%s> not found", taskUuid);
    return false;
}

bool CalendarRepository::updateTask(const Task &task)
{
    const char *TAG = "CalendarRepository::updateTask";
    LOGI(TAG, "Updating task <%s>", task.name);

    // Find in-memory model
    Task *existingTask = nullptr;
    for (auto &tb : m_timeblocks)
    {
        for (auto &t : tb.tasks)
        {
            if (strcmp(t.uuid, task.uuid) == 0)
            {
                existingTask = &t;
                break;
            }
        }
    }
    if (!existingTask)
    {
        LOGE(TAG, "Task with UUID <%s> not found in memory", task.uuid);
        return false;
    }

    // Update in database
    try
    {
        m_db.update_task(task);
    }
    catch (int err)
    {
        LOGE(TAG, "Failed to update task in database: %d", err);
        return false;
    }

    // Update in-memory model
    *existingTask = task;

    // Notify listeners
    emit modelChanged();

    return true;
}

/* --------------------------------- Habits --------------------------------- */

bool CalendarRepository::addHabitEntry(const char *taskUuid, const char *dateIso8601)
{
    const char *TAG = "CalendarRepository::addHabitEntry";
    LOGI(TAG, "Adding habit entry for task UUID <%s> on date %s", taskUuid, dateIso8601);

    try
    {
        m_db.add_habit_entry(taskUuid, dateIso8601);
        LOGI(TAG, "Persisted habit entry to database");
        return true;
    }
    catch (int err)
    {
        LOGE(TAG, "Failed to persist habit entry: %d", err);
        return false;
    }
}

bool CalendarRepository::removeHabitEntry(const char *taskUuid, const char *dateIso8601)
{
    const char *TAG = "CalendarRepository::removeHabitEntry";
    LOGI(TAG, "Removing habit entry for task UUID <%s> on date %s", taskUuid, dateIso8601);

    try
    {
        m_db.remove_habit_entry(taskUuid, dateIso8601);
        LOGI(TAG, "Removed habit entry from database");
        return true;
    }
    catch (int err)
    {
        LOGE(TAG, "Failed to remove habit entry: %d", err);
        return false;
    }
}

bool CalendarRepository::habitEntryExists(const char *taskUuid, const char *dateIso8601)
{
    const char *TAG = "CalendarRepository::habitEntryExists";
    LOGI(TAG, "Checking existence of habit entry for task UUID <%s> on date %s", taskUuid, dateIso8601);

    try
    {
        bool exists = m_db.habit_entry_exists(taskUuid, dateIso8601);
        LOGI(TAG, "Habit entry existence: %s", exists ? "true" : "false");
        return exists;
    }
    catch (int err)
    {
        LOGE(TAG, "Failed to check habit entry existence: %d", err);
        return false;
    }
}

// --- Helper functions; convert time_t to ISO8601 date string ---

bool CalendarRepository::addHabitEntry(const char *taskUuid, time_t date)
{
    const char *TAG = "CalendarRepository::addHabitEntry";
    char dateIso8601[11]; // YYYY-MM-DD + null terminator
    struct tm *tm_info = localtime(&date);
    strftime(dateIso8601, sizeof(dateIso8601), "%Y-%m-%d", tm_info);
    return addHabitEntry(taskUuid, dateIso8601);
}
bool CalendarRepository::removeHabitEntry(const char *taskUuid, time_t date)
{
    const char *TAG = "CalendarRepository::removeHabitEntry";
    char dateIso8601[11]; // YYYY-MM-DD + null terminator
    struct tm *tm_info = localtime(&date);
    strftime(dateIso8601, sizeof(dateIso8601), "%Y-%m-%d", tm_info);
    return removeHabitEntry(taskUuid, dateIso8601);
}
bool CalendarRepository::habitEntryExists(const char *taskUuid, time_t date)
{
    const char *TAG = "CalendarRepository::habitEntryExists";
    char dateIso8601[11]; // YYYY-MM-DD + null terminator
    struct tm *tm_info = localtime(&date);
    strftime(dateIso8601, sizeof(dateIso8601), "%Y-%m-%d", tm_info);
    return habitEntryExists(taskUuid, dateIso8601);
}

/* ------------------------------- Taskblocks ------------------------------- */

bool CalendarRepository::addTimeblock(Timeblock &tb)
{
    const char *TAG = "CalendarRepository::addTimeblock";
    LOGI(TAG, "Adding timeblock <%s>", tb.name);

    // Ensure timeblock has an id
    if (tb.uuid[0] == '\0')
    {
        generate_uuid(tb.uuid);
    }

    // Append to in-memory model
    m_timeblocks.push_back(tb);

    try
    {
        m_db.insert_timeblock(tb);
    }
    catch (int err)
    {
        LOGE(TAG, "Failed to persist timeblock <%s>: %d", tb.name, err);
        m_timeblocks.pop_back(); // Rollback in-memory addition

        return false;
    }

    LOGI(TAG, "Persisted timeblock <%s> to database", tb.name);

    // Notify listeners
    emit modelChanged();

    return true;
}

bool CalendarRepository::removeTimeblock(const char *timeblockUuid)
{
    const char *TAG = "CalendarRepository::removeTimeblock";
    LOGI(TAG, "Removing timeblock with UUID <%s>", timeblockUuid);

    // Find and remove from in-memory model
    auto it = std::find_if(m_timeblocks.begin(), m_timeblocks.end(), [timeblockUuid](const Timeblock &tb)
                           { return strcmp(tb.uuid, timeblockUuid) == 0; });
    if (it != m_timeblocks.end())
    {
        m_timeblocks.erase(it);
        // Remove from database
        try
        {
            m_db.delete_timeblock(timeblockUuid);
        }
        catch (int err)
        {
            LOGE(TAG, "Failed to delete timeblock from database: %d", err);
            return false;
        }

        // Notify listeners
        emit modelChanged();

        return true;
    }

    LOGE(TAG, "Timeblock with UUID <%s> not found", timeblockUuid);
    return false;
}

bool CalendarRepository::updateTimeblock(const Timeblock &tb)
{
    const char *TAG = "CalendarRepository::updateTimeblock";
    LOGI(TAG, "Updating timeblock <%s>", tb.name);

    // Find in-memory model
    Timeblock *existingTb = nullptr;
    for (auto &t : m_timeblocks)
    {
        if (strcmp(t.uuid, tb.uuid) == 0)
        {
            existingTb = &t;
            break;
        }
    }
    if (!existingTb)
    {
        LOGE(TAG, "Timeblock with UUID <%s> not found in memory", tb.uuid);
        return false;
    }

    // Update in database
    try
    {
        m_db.update_timeblock(tb);
    }
    catch (int err)
    {
        LOGE(TAG, "Failed to update timeblock in database: %d", err);
        return false;
    }

    // Update in-memory model
    *existingTb = tb;

    // Notify listeners
    emit modelChanged();

    return true;
}