#include "calendarrepository.h"
#include "log.h"

#include <time.h>
#include <algorithm>
#include <numeric>
#include <cstring>

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

    sortTimeblocks();

    // Load tasks from timeblocks
    for (auto &tb : m_timeblocks)
    {
        m_db.load_tasks(&tb);

        // Sort tasks within this timeblock by descending urgency (highest urgency first)
        std::sort(tb.tasks.begin(), tb.tasks.end(), [](const Task &a, const Task &b)
                  { return a.get_urgency() > b.get_urgency(); });

        // Load habit preview for any habit tasks
        for (auto &task : tb.tasks)
        {
            if (task.status == TaskStatus::HABIT)
            {
                // Load habit completion preview
                habitCompletionPreview(task);
            }
        }
    }
}

void CalendarRepository::habitCompletionPreview(Task &task)
{
    const char *TAG = "CalendarRepository::habitCompletionPreview";
    LOGI(TAG, "Loading habit completion preview for task <%s>", task.name);

    if (task.status != TaskStatus::HABIT)
    {
        LOGW(TAG, "Task <%s> is not a habit; skipping preview load", task.name);
        return;
    }

    // Get current date in ISO 8601 format
    char now_str[11]; // "YYYY-MM-DD" + null terminator
    time_t now = time(nullptr);
    struct tm local_tm = *localtime(&now);
    strftime(now_str, sizeof(now_str), "%Y-%m-%d", &local_tm);

    // Day Frequency mode
    if (task.goal_spec.mode() == GoalSpec::Mode::DayFrequency)
    {
        // Populate completed_days with target completions
        for (size_t i = 0; i < sizeof(task.completed_days) / sizeof(task.completed_days[0]); ++i)
        {
            // Current day to weekday enum
            int wday = (local_tm.tm_wday - i) % 7;
            if (task.goal_spec.has_day(wday))
            {
                task.completed_days[i] = TaskStatus::IN_PROGRESS; // Target completion
            }
            else
            {
                task.completed_days[i] = TaskStatus::INCOMPLETE;
            }
        }
    }

    // Note new due date
    task.update_due_date();

    // Fill task.completed_days with recent completions
    m_db.load_habit_completion_preview(task, now_str);
}

/* -------------------------------------------------------------------------- */
/*                              In-memory access                              */
/* -------------------------------------------------------------------------- */

void CalendarRepository::sortTimeblocks()
{
    // Sort timeblocks by urgency of their top task
    auto top_task_urgency = [](const Timeblock &tb) -> float
    {
        if (tb.tasks.empty())
            return 0.0f;

        // Add bonus for pinned timeblocks, so they always show first but keep order among themselves
        if (tb.status == TimeblockStatus::PINNED)
            return tb.tasks[0].get_urgency() * tb.status_weight(tb.status) + 1000.0f;

        return tb.tasks[0].get_urgency() * tb.status_weight(tb.status);
    };

    std::sort(m_timeblocks.begin(), m_timeblocks.end(), [&](const Timeblock &a, const Timeblock &b)
              { return top_task_urgency(a) > top_task_urgency(b); });
}

const std::vector<Timeblock> &CalendarRepository::timeblocks() const
{
    return m_timeblocks;
}

void CalendarRepository::sortTasks()
{
    // Sort tasks within each timeblock by urgency
    for (auto &tb : m_timeblocks)
    {
        std::sort(tb.tasks.begin(), tb.tasks.end(), [](const Task &a, const Task &b)
                  { return a.get_urgency() > b.get_urgency(); });
    }
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

    try
    {
        m_db.insert_task(task);
    }
    catch (int err)
    {
        LOGE(TAG, "Failed to persist task <%s>: %d", task.name, err);
        return false;
    }

    // Find spot in memory model to insert based on urgency
    float taskUrgency = task.get_urgency();
    for (size_t i = 0; i < m_timeblocks[timeblockIndex].tasks.size(); i++)
    {
        if (taskUrgency > m_timeblocks[timeblockIndex].tasks[i].get_urgency())
        {
            m_timeblocks[timeblockIndex].tasks.insert(m_timeblocks[timeblockIndex].tasks.begin() + i, task);
            LOGI(TAG, "Inserted task <%s> at position %zu in timeblock <%s>", task.name, i, m_timeblocks[timeblockIndex].name);

            // Notify listeners
            emit modelChanged();

            return true;
        }
    }
    m_timeblocks[timeblockIndex].tasks.push_back(task);
    LOGI(TAG, "Appended task <%s> at end of timeblock <%s>", task.name, m_timeblocks[timeblockIndex].name);

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
                               { return std::strncmp(t.uuid, taskUuid, UUID_LEN) == 0; });
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
    LOGI(TAG, "Updating task <%s> to status <%s> (%0.2f)", task.name,
         task.status == TaskStatus::COMPLETE      ? "COMPLETE"
         : task.status == TaskStatus::IN_PROGRESS ? "IN_PROGRESS"
         : task.status == TaskStatus::INCOMPLETE  ? "INCOMPLETE"
                                                  : "UNKNOWN",
         task.get_urgency());

    // Find in-memory model
    Timeblock *parentTimeblock = nullptr;
    Task *existingTask = nullptr;
    size_t taskIdx = 0;
    for (auto &tb : m_timeblocks)
    {
        for (size_t i = 0; i < tb.tasks.size(); i++)
        {
            Task &t = tb.tasks[i];
            if (std::strncmp(t.uuid, task.uuid, UUID_LEN) == 0)
            {
                existingTask = &t;
                parentTimeblock = &tb;
                taskIdx = i;
                break;
            }
        }
        for (size_t i = 0; i < tb.archived_tasks.size(); i++)
        {
            Task &t = tb.archived_tasks[i];
            if (std::strncmp(t.uuid, task.uuid, UUID_LEN) == 0)
            {
                existingTask = &t;
                parentTimeblock = &tb;
                taskIdx = i;
                break;
            }
        }
    }
    if (!existingTask)
    {
        LOGE(TAG, "Task with UUID <%s> not found in memory, no modifications made", task.uuid);
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

    // If completed move to archived tasks
    if (task.status == TaskStatus::COMPLETE)
    {
        // Append to front of archived tasks
        parentTimeblock->archived_tasks.push_back(task);
        rotate(parentTimeblock->archived_tasks.rbegin(), parentTimeblock->archived_tasks.rbegin() + 1, parentTimeblock->archived_tasks.rend());

        parentTimeblock->tasks.erase(parentTimeblock->tasks.begin() + taskIdx);
        LOGI(TAG, "Moved task <%s> to archived tasks in timeblock <%s>", task.name, parentTimeblock->name);
        emit modelChanged();
        return true;
    }
    // If uncompleted move back to main list
    if (existingTask->status == TaskStatus::COMPLETE && task.status != TaskStatus::COMPLETE)
    {
        // Place task into correct position based on urgency
        // Adjust location in timeblock based on urgency
        float taskUrgency = task.get_urgency();
        for (size_t i = 0; i < parentTimeblock->tasks.size(); i++)
        {
            float u = parentTimeblock->tasks[i].get_urgency();
            if (taskUrgency > u || u < 0.00001f)
            {
                // Move task to this position
                parentTimeblock->tasks.insert(parentTimeblock->tasks.begin() + i, task);
                LOGI(TAG, "Moved task <%s> (%0.2f) to position %zu in timeblock <%s>", task.name, taskUrgency, i, parentTimeblock->name);
                break;
            }
        }

        // Remove from archived tasks
        auto &archived = parentTimeblock->archived_tasks;
        auto it = std::find_if(archived.begin(), archived.end(), [&task](const Task &t)
                               { return std::strncmp(t.uuid, task.uuid, UUID_LEN) == 0; });
        if (it != archived.end())
        {
            archived.erase(it);
        }
        LOGI(TAG, "Restored task <%s> to active tasks in timeblock <%s>", task.name, parentTimeblock->name);
        emit modelChanged();
        return true;
    }

    // Otherwise task was and will remain in active tasks
    // Update existing task data
    *existingTask = task;

    // Adjust location in timeblock based on urgency
    float taskUrgency = task.get_urgency();
    for (size_t i = 0; i < parentTimeblock->tasks.size(); i++)
    {
        float u = parentTimeblock->tasks[i].get_urgency();
        if (taskUrgency > u || u < 0.00001f)
        {
            if (i == taskIdx && parentTimeblock->tasks.size() > 1)
            {
                // No move needed
                break;
            }
            // Move task to this position
            parentTimeblock->tasks.erase(parentTimeblock->tasks.begin() + taskIdx);
            parentTimeblock->tasks.insert(parentTimeblock->tasks.begin() + i, task);
            LOGI(TAG, "Moved task <%s> (%0.2f) to position %zu in timeblock <%s>", task.name, taskUrgency, i, parentTimeblock->name);
            break;
        }
    }

    // Notify listeners
    // TODO: Create new signal for just timeblock updates
    emit modelChanged();

    return true;
}

bool CalendarRepository::moveTask(const char *taskUuid, const char *timeblockUuid)
{
    const char *TAG = "CalendarRepository::moveTask";
    LOGI(TAG, "Moving task with UUID <%s> to timeblock UUID <%s>", taskUuid, timeblockUuid);

    // Find task in current timeblock
    Timeblock *currentTb = nullptr;
    std::vector<Task>::iterator taskIt; // Task location in currentTb
    Task movingTask;
    for (auto &tb : m_timeblocks)
    {
        taskIt = std::find_if(tb.tasks.begin(), tb.tasks.end(), [taskUuid](const Task &t)
                              { return std::strncmp(t.uuid, taskUuid, UUID_LEN) == 0; });
        if (taskIt != tb.tasks.end())
        {
            movingTask = *taskIt;
            currentTb = &tb;
            // tb.tasks.erase(taskIt); // Remove from current timeblock
            break;
        }
    }
    if (!currentTb)
    {
        LOGE(TAG, "Task with UUID <%s> not found", taskUuid);
        return false;
    }

    // Update task's timeblock_uuid
    strncpy(movingTask.timeblock_uuid, timeblockUuid, UUID_LEN);

    // Update in database
    try
    {
        m_db.update_task(movingTask);
    }
    catch (int err)
    {
        LOGE(TAG, "Failed to update task in database: %d", err);
        return false;
    }

    // Insert into new timeblock in memory
    for (auto &tb : m_timeblocks)
    {
        if (std::strncmp(tb.uuid, timeblockUuid, UUID_LEN) == 0)
        {
            // Insert based on urgency
            float taskUrgency = movingTask.get_urgency();
            for (size_t i = 0; i < tb.tasks.size(); i++)
            {
                if (taskUrgency > tb.tasks[i].get_urgency())
                {
                    tb.tasks.insert(tb.tasks.begin() + i, movingTask);
                    currentTb->tasks.erase(taskIt); // Remove from old timeblock
                    LOGI(TAG, "Inserted moved task <%s> at position %zu in timeblock <%s>", movingTask.name, i, tb.name);
                    emit modelChanged();
                    return true;
                }
            }
            tb.tasks.push_back(movingTask);
            currentTb->tasks.erase(taskIt); // Remove from old timeblock
            LOGI(TAG, "Appended moved task <%s> at end of timeblock <%s>", movingTask.name, tb.name);
            emit modelChanged();
            return true;
        }
    }

    LOGE(TAG, "Destination timeblock with UUID <%s> not found", timeblockUuid);
    return false;
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
        // Update in-memory model: find the task and refresh its habit preview, then reposition by urgency
        Timeblock *parentTb = nullptr;
        Task *foundTask = nullptr;
        size_t taskIdx = 0;
        for (auto &tb : m_timeblocks)
        {
            for (size_t i = 0; i < tb.tasks.size(); ++i)
            {
                if (std::strncmp(tb.tasks[i].uuid, taskUuid, UUID_LEN) == 0)
                {
                    parentTb = &tb;
                    foundTask = &tb.tasks[i];
                    taskIdx = i;
                    break;
                }
            }
            if (foundTask)
                break;
        }

        if (foundTask && parentTb)
        {
            // Make a copy, refresh preview for the specific date, and compute new urgency
            Task updated = *foundTask;
            updated.update_due_date();
            m_db.load_habit_completion_preview(updated, dateIso8601);

            float newUrgency = updated.get_urgency();

            // Remove original and insert updated at proper sorted position
            parentTb->tasks.erase(parentTb->tasks.begin() + taskIdx);

            size_t insertPos = 0;
            for (; insertPos < parentTb->tasks.size(); ++insertPos)
            {
                if (newUrgency > parentTb->tasks[insertPos].get_urgency())
                    break;
            }
            parentTb->tasks.insert(parentTb->tasks.begin() + insertPos, updated);
            LOGI(TAG, "Updated habit task <%s> and moved to position %zu in timeblock <%s>", updated.name, insertPos, parentTb->name);
        }

        emit modelChanged();
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
        bool dbExists = m_db.habit_entry_exists(taskUuid, dateIso8601); // Double-check database state
        if (dbExists)
        {
            LOGE(TAG, "Failed to remove habit entry from database; still exists after removal attempt");
            return false;
        }
        LOGI(TAG, "Removed habit entry from database");
        // Update in-memory model similarly to addHabitEntry: refresh preview and reposition by urgency
        Timeblock *parentTb = nullptr;
        Task *foundTask = nullptr;
        size_t taskIdx = 0;
        for (auto &tb : m_timeblocks)
        {
            for (size_t i = 0; i < tb.tasks.size(); ++i)
            {
                if (std::strncmp(tb.tasks[i].uuid, taskUuid, UUID_LEN) == 0)
                {
                    parentTb = &tb;
                    foundTask = &tb.tasks[i];
                    taskIdx = i;
                    break;
                }
            }
            if (foundTask)
                break;
        }

        if (foundTask && parentTb)
        {
            Task updated = *foundTask;
            updated.update_due_date();
            m_db.load_habit_completion_preview(updated, dateIso8601);
            // LOGI(TAG, "Refreshed habit completion preview for task <%s> (%s)", updated.name, updated.uuid);
            // for (size_t i = 0; i < sizeof(updated.completed_days) / sizeof(updated.completed_days[0]); ++i)
            // {
            //     LOGI(TAG, "  Day -%zu: %s", i,
            //          updated.completed_days[i] == TaskStatus::COMPLETE      ? "COMPLETE"
            //          : updated.completed_days[i] == TaskStatus::IN_PROGRESS ? "IN_PROGRESS"
            //          : updated.completed_days[i] == TaskStatus::INCOMPLETE  ? "INCOMPLETE"
            //                                                                 : "UNKNOWN");
            // }

            float newUrgency = updated.get_urgency();

            parentTb->tasks.erase(parentTb->tasks.begin() + taskIdx);

            size_t insertPos = 0;
            for (; insertPos < parentTb->tasks.size(); ++insertPos)
            {
                if (newUrgency > parentTb->tasks[insertPos].get_urgency())
                    break;
            }
            parentTb->tasks.insert(parentTb->tasks.begin() + insertPos, updated);
            LOGI(TAG, "Updated habit task <%s> (%0.2f) and moved to position %zu in timeblock <%s>", updated.name, newUrgency, insertPos, parentTb->name);
        }

        emit modelChanged();
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
                           { return std::strncmp(tb.uuid, timeblockUuid, UUID_LEN) == 0; });
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
        if (std::strncmp(t.uuid, tb.uuid, UUID_LEN) == 0)
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