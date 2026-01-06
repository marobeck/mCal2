#pragma once

// --- Data ---
#include "calendarrepository.h"

// --- UI ---
#include <QMainWindow>
#include <QLabel>
#include <QVector>
#include <QListWidget>
#include <QHBoxLayout>
#include <QStackedWidget>
#include <QPushButton>
#include <QIcon>
#include <QVBoxLayout>

// --- Scenes ---
#include "todolistview.h"
#include "newentryview.h"
#include "newtimeblockview.h"
#include "entrydetailsview.h"
#include "dayscheduleview.h"

Q_DECLARE_METATYPE(const Task *)

/**
 * Scenes in the application
 */
enum class Scene
{
    TodoList,
    DaySchedule,
    NewEntry,
    NewTimeblock,
    EntryDetails,
    Settings
};

class MainWindow : public QMainWindow
{
    Q_OBJECT

private:
    Scene currentLeftScene = Scene::DaySchedule;
    Scene currentRightScene = Scene::TodoList;

public:
    MainWindow(QWidget *parent = nullptr, CalendarRepository *dataPtr = nullptr);

public slots:
    // Scene managament
    void switchRightPanel(Scene scene, QVariant data = {});
    // Left panel scene management
    void switchLeftPanel(Scene scene, QVariant data = {});

    // Run updates
    void onTaskCreated(Task *task, int timeblockIndex);
    void onTimeblockCreated(Timeblock *timeblock);
    void modelChanged();
    // Entry details actions
    void onDeleteTaskRequested(const QString &taskUuid);
    void onMoveTaskRequested(const QString &taskUuid);
    void onEditTaskRequested(const QString &taskUuid);

public:
    // Access to memory and database
    CalendarRepository *repo = nullptr;

    // --- Scenes ---
    DayScheduleView *scheduleView;

    /** Left panel scenes
     * - Schedule view
     * - New entry view
     * - Entry details view
     */
    QStackedWidget *leftStack;
    NewEntryView *newEntryView;
    NewTimeblockView *newTimeblockView;
    EntryDetailsView *entryDetailsView;

    /** Right panel scenes
     * - Timeblock todo list view
     */
    QStackedWidget *rightStack;
    TodoListView *todoListView;

    // Thin edge panels for quick scene buttons
    QWidget *leftEdgePanel = nullptr;
    QWidget *rightEdgePanel = nullptr;
    QVBoxLayout *leftEdgeLayout = nullptr;
    QVBoxLayout *rightEdgeLayout = nullptr;

    // Helpers to add icon buttons to the thin edge panels. The button will
    // call switchLeftPanel/switchRightPanel with the provided scene and data.
    void addLeftEdgeButton(const QIcon &icon, Scene scene, QVariant data = {});
    void addRightEdgeButton(const QIcon &icon, Scene scene, QVariant data = {});
};