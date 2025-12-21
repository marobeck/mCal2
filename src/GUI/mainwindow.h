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

public:
    // Scenes
    DayScheduleView *scheduleView;
    QStackedWidget *leftStack;
    NewEntryView *leftNewEntryView;
    EntryDetailsView *leftEntryDetailsView;
    QStackedWidget *rightStack;
    TodoListView *todoListView;
    NewEntryView *newEntryView;
    EntryDetailsView *entryDetailsView;
};