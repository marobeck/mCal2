#include "calendarrepository.h"
#include "uuid.h"
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
/*                                  Accessors                                 */
/* -------------------------------------------------------------------------- */

const std::vector<Timeblock> &CalendarRepository::timeblocks() const
{
    return m_timeblocks;
}

const TaskHash &CalendarRepository::tasks() const
{
    return m_tasks;
}

/* -------------------------------------------------------------------------- */
/*                                Load from DB                                */
/* -------------------------------------------------------------------------- */

void CalendarRepository::loadAll()
{
    const char *TAG = "CalendarRepository::loadAll";
    LOGI(TAG, "Loading all timeblocks and tasks from database...");

    // Clear current in-memory model
    m_timeblocks.clear();

    // Load timeblocks and task data from database
    m_db.load_timeblocks(m_timeblocks);
    m_db.load_tasks(m_tasks);

    // Load habit preview for any habit tasks
    for (auto &[uuid, taskptr] : m_tasks)
    {
        if (taskptr->status == TaskStatus::HABIT)
        {
            // Load habit completion preview
            habitCompletionPreview(*taskptr);
        }
    }

    // Link tasks to their timeblocks based on timeblock_uuid
    for (auto &tb : m_timeblocks)
    {
        tb.tasks = getTasksForTimeblock(tb.uuid);
    }
}

void CalendarRepository::habitCompletionPreview(Task &task)
{
    const char *TAG = "CalendarRepository::habitCompletionPreview";
    // LOGI(TAG, "Loading habit completion preview for task <%s>", task.name);

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

void CalendarRepository::habitCompletionStats(const char *taskUuid, std::vector<time_t> &completionDates)
{
    const char *TAG = "CalendarRepository::habitCompletionStats";
    completionDates.clear();
    try
    {
        m_db.get_habit_entries(taskUuid, completionDates);
    }
    catch (int err)
    {
        LOGE(TAG, "Failed to load habit entries for task %s: %d", taskUuid, err);
    }
}

/* -------------------------------------------------------------------------- */
/*                              In-memory access                              */
/* -------------------------------------------------------------------------- */

// Load all timeblocks and tasks from database into memory, linking them together based on timeblock_uuid and sorting them by urgency
std::vector<Task *> CalendarRepository::getTasksForTimeblock(const UUID &timeblockUuid)
{
    std::vector<Task *> result;

    // Find tasks with matching timeblock_uuid and add pointers to them to the result vector
    for (auto &[uuid, taskPtr] : m_tasks)
    {
        if (taskPtr->timeblock_uuid == timeblockUuid)
            result.push_back(taskPtr.get());
    }

    // Sort tasks by urgency
    sortTasks(result);

    return result;
}

// Sort timeblocks between each other
void CalendarRepository::sortTimeblocks()
{
    // Sort tasks within each timeblock first so that timeblock sorting can use task urgency
    for (auto &tb : m_timeblocks)
    {
        sortTasks(tb.tasks);
    }

    // Sort timeblocks by urgency of their top task
    auto top_task_urgency = [](const Timeblock &tb) -> float
    {
        if (tb.tasks.empty())
            return 0.0f;

        // Add bonus for pinned timeblocks, so they always show first but keep order among themselves
        if (tb.status == TimeblockStatus::PINNED)
            return tb.tasks[0]->get_urgency() * tb.status_weight(tb.status) + 1000.0f;

        return tb.tasks[0]->get_urgency() * tb.status_weight(tb.status);
    };

    std::sort(m_timeblocks.begin(), m_timeblocks.end(), [&](const Timeblock &a, const Timeblock &b)
              { return top_task_urgency(a) > top_task_urgency(b); });
}

// Sort tasks within a timeblock by urgency and completion status
// Completed tasks are always at the bottom, sorted by completion time (most recent first). Incomplete tasks are sorted by urgency.
void CalendarRepository::sortTasks(std::vector<Task *> &tasks)
{
    std::sort(tasks.begin(), tasks.end(),
              [](Task *a, Task *b)
              {
                  const bool aCompleted = (a->status == TaskStatus::COMPLETE);
                  const bool bCompleted = (b->status == TaskStatus::COMPLETE);

                  if (aCompleted != bCompleted)
                      return !aCompleted;

                  if (!aCompleted)
                  {
                      return a->get_urgency() > b->get_urgency();
                  }

                  return a->get_completed_time() > b->get_completed_time();
              });
}

Task *CalendarRepository::findTaskByUuid(const char *uuid)
{
    auto it = m_tasks.find(uuid);
    if (it != m_tasks.end())
    {
        return it->second.get();
    }
    return nullptr;
}

Timeblock *CalendarRepository::findTimeblockByUuid(const char *uuid)
{
    for (auto &tb : m_timeblocks)
    {
        if (std::strncmp(tb.uuid, uuid, UUID_LEN) == 0)
        {
            return &tb;
        }
    }
    return nullptr;
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
        generate_uuid(task.uuid.value);
    }

    // copy the timeblock's uuid into the task
    strncpy(task.timeblock_uuid.value, m_timeblocks[timeblockIndex].uuid, UUID_LEN);

    // Persist to database
    try
    {
        m_db.insert_task(task);
    }
    catch (int err)
    {
        LOGE(TAG, "Failed to persist task <%s>: %d", task.name, err);
        return false;
    }

    // Insert into in-memory model
    m_tasks[task.uuid] = std::make_unique<Task>(task);

    // Find spot in memory model to insert based on urgency
    Task *taskPtr = m_tasks[task.uuid].get(); // Get pointer to the newly added task in the map
    float taskUrgency = taskPtr->get_urgency();
    for (size_t i = 0; i < m_timeblocks[timeblockIndex].tasks.size(); i++)
    {
        if (taskUrgency > m_timeblocks[timeblockIndex].tasks[i]->get_urgency())
        {
            m_timeblocks[timeblockIndex].tasks.insert(m_timeblocks[timeblockIndex].tasks.begin() + i, taskPtr);
            LOGI(TAG, "Inserted task <%s> at position %zu in timeblock <%s>", task.name, i, m_timeblocks[timeblockIndex].name);

            // Notify listeners
            emit modelChanged();

            return true;
        }
    }
    m_timeblocks[timeblockIndex].tasks.push_back(taskPtr);
    LOGI(TAG, "Appended task <%s> at end of timeblock <%s>", task.name, m_timeblocks[timeblockIndex].name);

    // Notify listeners
    emit modelChanged();

    return true;
}

bool CalendarRepository::removeTask(const char *taskUuid)
{
    const char *TAG = "CalendarRepository::removeTask";
    LOGI(TAG, "Removing task with UUID <%s>", taskUuid);

    // Find task in in-memory model
    Task *taskToRemove = findTaskByUuid(taskUuid);
    if (!taskToRemove)
    {
        LOGE(TAG, "Task with UUID <%s> not found in memory, cannot remove", taskUuid);
        return false;
    }
    Timeblock *tb = findTimeblockByUuid(taskToRemove->timeblock_uuid);
    if (!tb)
    {
        LOGE(TAG, "Timeblock with UUID <%s> not found in memory, cannot remove task <%s>", taskToRemove->timeblock_uuid.value, taskToRemove->name);
        return false;
    }

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

    // Remove from in-memory model
    auto &tasks = tb->tasks;
    auto it = std::find_if(tasks.begin(), tasks.end(), [taskToRemove](Task *t)
                           { return t == taskToRemove; });
    if (it != tasks.end())
    {
        tasks.erase(it);
    }
    else
    {
        LOGW(TAG, "Task <%s> not found in timeblock <%s>", taskToRemove->name, tb->name);
        return false;
    }

    // Remove from task map
    m_tasks.erase(taskUuid);

    // Notify listeners
    emit modelChanged();

    return true;
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

    // Find task in in-memory model
    Task *existingTask = findTaskByUuid(task.uuid);

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

    // Update in-memory model
    *existingTask = task;

    // Notify listeners
    emit modelChanged();

    return true;
}

bool CalendarRepository::moveTask(const char *taskUuid, const char *timeblockUuid)
{
    const char *TAG = "CalendarRepository::moveTask";
    LOGI(TAG, "Moving task with UUID <%s> to timeblock UUID <%s>", taskUuid, timeblockUuid);

    // Find task in in-memory model
    Task *movingTask = findTaskByUuid(taskUuid);
    if (!movingTask)
    {
        LOGE(TAG, "Task with UUID <%s> not found in memory, cannot move", taskUuid);
        return false;
    }

    // Find current timeblock of the task
    Timeblock *currentTb = findTimeblockByUuid(movingTask->timeblock_uuid);

    // Update task's timeblock_uuid
    strncpy(movingTask->timeblock_uuid.value, timeblockUuid, UUID_LEN);

    // Move in database by changing the timeblock_uuid field of the task
    try
    {
        m_db.update_task(*movingTask);
    }
    catch (int err)
    {
        LOGE(TAG, "Failed to update task's timeblock in database: %d", err);
        return false;
    }

    // Add to new timeblock's task list at correct position based on urgency
    Timeblock *newTb = findTimeblockByUuid(timeblockUuid);
    if (newTb)
    {
        float taskUrgency = movingTask->get_urgency();
        auto &newTasks = newTb->tasks;
        size_t insertPos = 0;
        for (; insertPos < newTasks.size(); ++insertPos)
        {
            if (taskUrgency > newTasks[insertPos]->get_urgency())
                break;
        }
        newTasks.insert(newTasks.begin() + insertPos, movingTask);
    }
    else
    {
        LOGW(TAG, "New timeblock with UUID <%s> not found", timeblockUuid);
        return false;
    }

    // Remove from current timeblock's task list
    if (currentTb)
    {
        auto &tasks = currentTb->tasks;
        // ptr comparison since tasks are stored as pointers in timeblocks
        auto it = std::find_if(tasks.begin(), tasks.end(), [movingTask](const Task *t)
                               { return t == movingTask; });
        if (it != tasks.end())
        {
            tasks.erase(it);
        }
    }
    else
    {
        LOGW(TAG, "Current timeblock for task <%s> not found; task may be in an inconsistent state", movingTask->name);
        return false;
    }

    // Notify listeners of change
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
    }
    catch (int err)
    {
        LOGE(TAG, "Failed to persist habit entry: %d", err);
        return false;
    }

    // Update in-memory model: find the task and refresh its habit preview, then reposition by urgency
    Task *habit = findTaskByUuid(taskUuid);
    if (habit)
    {
        habit->update_due_date();
        m_db.load_habit_completion_preview(*habit, dateIso8601);
    }
    else
    {
        LOGE(TAG, "Task with UUID <%s> not found in memory, cannot update habit preview", taskUuid);
        m_db.remove_habit_entry(taskUuid, dateIso8601); // Rollback database change since task doesn't exist in memory
        return false;
    }

    emit modelChanged();
    return true;
}

bool CalendarRepository::removeHabitEntry(const char *taskUuid, const char *dateIso8601)
{
    const char *TAG = "CalendarRepository::removeHabitEntry";
    LOGI(TAG, "Removing habit entry for task UUID <%s> on date %s", taskUuid, dateIso8601);

    try
    {
        m_db.remove_habit_entry(taskUuid, dateIso8601);
        LOGI(TAG, "Removed habit entry from database");
    }
    catch (int err)
    {
        LOGE(TAG, "Failed to remove habit entry: %d", err);
        return false;
    }

    // Update in-memory model: find the task and refresh its habit preview, then reposition by urgency
    Task *habit = findTaskByUuid(taskUuid);
    if (habit)
    {
        habit->update_due_date();
        m_db.load_habit_completion_preview(*habit, dateIso8601);
    }
    else
    {
        LOGE(TAG, "Task with UUID <%s> not found in memory, cannot update habit preview", taskUuid);
        m_db.add_habit_entry(taskUuid, dateIso8601); // Rollback database change since task doesn't exist in memory
        return false;
    }

    emit modelChanged();
    return true;
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

/* ------------------------------- Entry links ------------------------------ */

/**
 * Add a link between two tasks and update the in-memory model.
 */
bool CalendarRepository::addEntryLink(Task *parentTask, Task *childTask, LinkType linkType)
{
    const char *TAG = "CalendarRepository::addEntryLink";

    try
    {
        m_db.add_entry_link(parentTask->uuid, childTask->uuid, linkType);
        LOGI(TAG, "Added entry link in database: <%s> --(%d)--> <%s>", parentTask->name, static_cast<int>(linkType), childTask->name);
    }
    catch (int err)
    {
        LOGE(TAG, "Failed to add entry link to database: %d", err);
        return false;
    }

    // Update in-memory model
    if (linkType == LinkType::DEPENDENCY)
    {
        parentTask->prerequisites.push_back(childTask);
        LOGI(TAG, "Added dependency link in memory: <%s> depends on <%s>", parentTask->name, childTask->name);
    }
    else if (linkType == LinkType::HABIT_TRIGGER)
    {
        // For habit triggers, we might want to maintain a separate list or handle differently; for now we'll just log it
        LOGI(TAG, "Added habit trigger link in memory: <%s> is triggered by <%s>", parentTask->name, childTask->name);
    }
    else
    {
        LOGW(TAG, "Unknown link type %d; no in-memory update performed", static_cast<int>(linkType));
    }

    return true;
}

bool CalendarRepository::removeEntryLink(Task *parentTask, Task *childTask, LinkType linkType)
{
    const char *TAG = "CalendarRepository::removeEntryLink";

    try
    {
        m_db.remove_entry_link(parentTask->uuid, childTask->uuid, linkType);
        LOGI(TAG, "Removed entry link in database: <%s> --(%d)--> <%s>", parentTask->name, static_cast<int>(linkType), childTask->name);
    }
    catch (int err)
    {
        LOGE(TAG, "Failed to remove entry link from database: %d", err);
        return false;
    }

    // Update in-memory model
    if (linkType == LinkType::DEPENDENCY)
    {
        parentTask->prerequisites.erase(std::remove(parentTask->prerequisites.begin(), parentTask->prerequisites.end(), childTask), parentTask->prerequisites.end());
        LOGI(TAG, "Removed dependency link in memory: <%s> no longer depends on <%s>", parentTask->name, childTask->name);
    }
    else if (linkType == LinkType::HABIT_TRIGGER)
    {
        // For habit triggers, we might want to maintain a separate list or handle differently; for now we'll just log it
        LOGI(TAG, "Removed habit trigger link in memory: <%s> is no longer triggered by <%s>", parentTask->name, childTask->name);
    }
    else
    {
        LOGW(TAG, "Unknown link type %d; no in-memory update performed", static_cast<int>(linkType));
    }

    return true;
}

bool CalendarRepository::removeAllLinksForTask(Task *task)
{
    const char *TAG = "CalendarRepository::removeAllLinksForTask";

    try
    {
        m_db.remove_all_links_for_task(task->uuid);
        LOGI(TAG, "Removed all entry links for task <%s> from database", task->name);
    }
    catch (int err)
    {
        LOGE(TAG, "Failed to remove all entry links for task from database: %d", err);
        return false;
    }

    // --- Update in-memory model ---

    // Find and remove all links where this task is the parent
    for (Task *prereq : task->prerequisites)
    {
        prereq->prerequisites.erase(std::remove(prereq->prerequisites.begin(), prereq->prerequisites.end(), task), prereq->prerequisites.end());
        LOGI(TAG, "Removed prerequisite link in memory: <%s> no longer depends on <%s>", prereq->name, task->name);
    }

    task->prerequisites.clear();
    LOGI(TAG, "Cleared all prerequisite links in memory for task <%s>", task->name);
    return true;
}

void CalendarRepository::getLinkedEntries(Task *task)
{
    const char *TAG = "CalendarRepository::getLinkedEntries";
    std::vector<char *> linkedUuid;

    try
    {
        m_db.get_linked_entries(task->uuid, task->status == TaskStatus::HABIT ? LinkType::HABIT_TRIGGER : LinkType::DEPENDENCY, linkedUuid);
    }
    catch (int err)
    {
        LOGE(TAG, "Failed to load linked entries from database: %d", err);
        return;
    }

    for (const char *uuid : linkedUuid)
    {
        Task *linkedTask = findTaskByUuid(uuid);
        if (linkedTask)
        {
            task->prerequisites.push_back(linkedTask);
        }
    }

    // Cleanup linkedUuid strings
    for (char *uuid : linkedUuid)
    {
        free(uuid);
    }

    for (Task *prereq : task->prerequisites)
    {
        LOGI(TAG, "Loaded linked task <%s> (%s) for task <%s>", prereq->name, prereq->uuid, task->name);
    }
}

/* ------------------------------- Taskblocks ------------------------------- */

bool CalendarRepository::addTimeblock(Timeblock &tb)
{
    const char *TAG = "CalendarRepository::addTimeblock";
    LOGI(TAG, "Adding timeblock <%s>", tb.name);

    // Ensure timeblock has an id
    if (tb.uuid[0] == '\0')
    {
        generate_uuid(tb.uuid.value);
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