/** CalendarRepository.h
 * Handles memory access and storage of timeblocks and tasks.
 * Sorts timeblocks and tasks in memory
 */
#pragma once
#include <vector>
#include <functional>
#include <QObject>
#include <unordered_map>
#include <memory>

#include "syncronize.h"

class CalendarRepository : public QObject
{
    Q_OBJECT

public:
    CalendarRepository();
    ~CalendarRepository();

    /* ---------------------------------- Accessors --------------------------------- */
    // Must be const since they are used by views to read data without modifying it
    const std::vector<Timeblock> &timeblocks() const;
    const TaskHash &tasks() const;
    /* ---------------------------- In memory access ---------------------------- */
    void sortTimeblocks();                                                                 // sorts timeblocks in memory
    void sortTasks(std::vector<Task *> &tasks);                                            // sorts tasks within each timeblock in memory (not timeblocks)
    Task *findTaskByUuid(const char *uuid);                                                // Recovers pointer to repository task by UUID
    void findTasksByList(const std::vector<char *> &uuids, std::vector<Task *> &outTasks); // Recovers pointers to repository tasks by list of UUIDs
    Timeblock *findTimeblockByUuid(const char *uuid);                                      // Recovers pointer to repository timeblock by UUID

    /* ------------------------------ Load from DB ------------------------------ */
    // Load everything from DB into memory
    void loadAll();
    void sync(); // Sync with server
    std::vector<Task *> getTasksForTimeblock(const UUID &timeblockUuid); // Load tasks for a specific timeblock into provided vector
    // --- Getters ---
    void habitCompletionPreview(Task &task); // fills task.completed_days with recent completions
    void habitCompletionStats(const char *taskUuid, std::vector<time_t> &completionDates);

    /* ------------------ Modifiers (update both memory and DB) ----------------- */
    // Tasks
    bool addTask(Task &task, size_t timeblockIndex);                // returns success
    bool removeTask(const char *taskUuid);                          // Delete task by UUID
    bool updateTask(const Task &task);                                     // persist updated task
    bool moveTask(const char *taskUuid, const char *timeblockUuid); // move task to different timeblock
    // Habits
    bool addHabitEntry(const char *taskUuid, const char *dateIso8601);
    bool addHabitEntry(const char *taskUuid, time_t date);
    bool removeHabitEntry(const char *taskUuid, const char *dateIso8601);
    bool removeHabitEntry(const char *taskUuid, time_t date);
    bool habitEntryExists(const char *taskUuid, const char *dateIso8601);
    bool habitEntryExists(const char *taskUuid, time_t date);
    // Timeblocks
    bool addTimeblock(Timeblock &tb); // returns success
    bool removeTimeblock(const char *timeblockUuid);
    bool updateTimeblock(const Timeblock &tb); // persist updated timeblock
    // Entry links
    bool addEntryLink(Task *parentTask, Task *childTask, LinkType linkType = LinkType::DEPENDENCY);    // Update database and in-memory model
    bool removeEntryLink(Task *parentTask, Task *childTask, LinkType linkType = LinkType::DEPENDENCY); // Update database and in-memory model
    void getLinkedEntries(Task *task);                                                                 // Get linked tasks for a given task
    bool removeAllLinksForTask(Task *task);                                                            // Remove all links for a given task

signals:
    // Notify listeners that the model has changed
    void modelChanged();

private:
    Database m_db; //  DB interface
    Synchronizer* m_synchronizer; // Sync interface

    // All tasks are stored in hash map for O(1) access by UUID, timeblocks store pointers to their tasks for organization
    TaskHash m_tasks;                    // In-memory model of tasks, keyed by UUID for fast lookup
    std::vector<Timeblock> m_timeblocks; // In-memory model of timeblocks (does not own tasks, just organizes them)
};