/** CalendarRepository.h
 * Handles memory access and storage of timeblocks and tasks.
 * Sorts timeblocks and tasks in memory
 */
#pragma once
#include <vector>
#include <functional>
#include <QObject>

#include "database.h"
#include "timeblock.h"
#include "task.h"

class CalendarRepository : public QObject
{
    Q_OBJECT

public:
    CalendarRepository();
    ~CalendarRepository();

    /* ---------------------------- In memory access ---------------------------- */
    void sortTimeblocks(); // sorts timeblocks in memory
    void sortTasks();      // sorts tasks within each timeblock in memory (not timeblocks)
    const std::vector<Timeblock> &timeblocks() const;

    /* ------------------------------ Load from DB ------------------------------ */
    // Load everything from DB into memory
    void loadAll();
    // --- Getters ---
    void habitCompletionPreview(Task &task); // fills task.completed_days with recent completions
    void habitCompletionStats(const char *taskUuid, std::vector<time_t> &completionDates);

    /* ------------------ Modifiers (update both memory and DB) ----------------- */
    // Tasks
    bool addTask(Task &task, size_t timeblockIndex);                // returns success
    bool removeTask(const char *taskUuid);                          // Delete task by UUID
    bool updateTask(const Task &task);                              // persist updated task
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

signals:
    // Notify listeners that the model has changed
    void modelChanged();

private:
    Database m_db;                       //  DB interface
    std::vector<Timeblock> m_timeblocks; // in-memory model
};