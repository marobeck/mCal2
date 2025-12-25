#pragma once

// --- Data ---
#include <vector>

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
#include "entrydetailsview.h"
#include "dayscheduleview.h"

/**
 * Scenes in the application
 */
enum class Scene
{
    TodoList,
    DaySchedule,
    NewEntry,
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
    MainWindow(QWidget *parent = nullptr);

public slots:
    // Scene managament
    void switchRightPanel(Scene scene, QVariant data = {});
    // Left panel scene management
    void switchLeftPanel(Scene scene, QVariant data = {});

    // Run updates
    void onTaskCreated(Task *task, int timeblockIndex);

public:
    // Timeblocks loaded from database
    std::vector<Timeblock> timeblocks;
    Database db;

    // --- Scenes ---
    DayScheduleView *scheduleView;

    /** Left panel scenes
     * - Schedule view
     * - New entry view
     * - Entry details view
     */
    QStackedWidget *leftStack;
    NewEntryView *newEntryView;
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