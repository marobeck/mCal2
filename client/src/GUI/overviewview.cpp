#include "overviewview.h"

#include "log.h"
#include "taskitemwidget.h"

#define TASKS_TO_DISPLAY 5

OverviewView::OverviewView(QWidget *parent, CalendarRepository *dataRepo)
    : QWidget(parent), repo(dataRepo)
{
    const char *TAG = "OverviewView::Constructor";
    LOGI(TAG, "Initializing OverviewView...");

    if (!repo)
    {
        LOGE(TAG, "No CalendarRepository provided to OverviewView!");
        throw std::runtime_error("No CalendarRepository provided to OverviewView");
    }

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(8, 8, 8, 8);
    layout->setSpacing(10);

    QLabel *title = new QLabel("Top Tasks", this);
    title->setStyleSheet("font-weight: bold; font-size: 18px;");
    layout->addWidget(title);

    // -- Urgent tasks list --

    m_urgentTasksList = new QListWidget(this);
    m_urgentTasksList->setStyleSheet("QListWidget { background: transparent; border: none; }");
    m_urgentTasksList->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    // m_urgentTasksList->setMaximumWidth(400); // Limit width
    layout->addWidget(m_urgentTasksList);

    // --- Ledger of tasks completed today ---

    m_completedTasksList = new QListWidget(this);
    m_completedTasksList->setStyleSheet("QListWidget { background: darkGrey; border: none; }");
    m_completedTasksList->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    layout->addWidget(m_completedTasksList);

    updateOverview();
}

void OverviewView::updateOverview()
{
    const char *TAG = "OverviewView::updateOverview";
    LOGI(TAG, "Updating overview");

    std::vector<Task *> tasksToDisplay;
    std::vector<Task *> completedToday;
    time_t now = std::time(nullptr);


    // Iterate through tasks to get the most urgent ones and tasks completed today.
    for (auto &[uuid, taskPtr] : repo->tasks())
    {
        if (!taskPtr)
        {
            LOGW(TAG, "Skipping invalid pointer in memory");
            continue;
        }

        // Iterate through all ongoing tasks
        if (taskPtr->status == TaskStatus::INCOMPLETE || taskPtr->status == TaskStatus::HABIT)
        {
            tasksToDisplay.push_back(taskPtr.get());
        }
        else if (taskPtr->status == TaskStatus::COMPLETE)
        {
            // Check if task was completed today
            time_t startOfDay = now - (now % 86400); // Get start of current day
            if (taskPtr->completed_datetime >= startOfDay)
            {
                completedToday.push_back(taskPtr.get());
            }
        }
    }

    // --- Update list of top tasks ---

    // Sort tasks by urgency and take the top ones
    std::sort(tasksToDisplay.begin(), tasksToDisplay.end(), [](const Task *a, const Task *b)
              { return a->get_urgency() > b->get_urgency(); });
    if (tasksToDisplay.size() > (int)TASKS_TO_DISPLAY)
    {
        tasksToDisplay.resize((int)TASKS_TO_DISPLAY);
    }

    // Update the list widget with the top tasks
    m_urgentTasksList->clear();
    for (auto *task : tasksToDisplay)
    {
        QListWidgetItem *item = new QListWidgetItem(m_urgentTasksList);
        TaskItemWidget *widget = new TaskItemWidget(task, repo, this, TaskItemWidget::Mode::COMPACT);
        item->setSizeHint(widget->sizeHint());
        m_urgentTasksList->addItem(item);
        m_urgentTasksList->setItemWidget(item, widget);
    }

    // --- Update ledger of tasks completed today ---
    if (completedToday.empty())
    {
        m_completedTasksList->clear();
        QListWidgetItem *item = new QListWidgetItem("No tasks completed today.", m_completedTasksList);
        item->setTextAlignment(Qt::AlignVCenter | Qt::AlignHCenter); // Center the text
        item->setForeground(Qt::darkGray);                           // Set text color to dark red
        item->setFont(QFont("Helvetica", 18, QFont::Bold));          // Set font to bold
        item->setFlags(Qt::NoItemFlags);                             // Make item non-interactive
        item->setSizeHint(QSize(m_completedTasksList->width(), 50)); // Set item height
        m_completedTasksList->addItem(item);
        return;
    }

    // Sort completed tasks by completion time (most recent first)
    std::sort(completedToday.begin(), completedToday.end(), [](const Task *a, const Task *b)
              { return a->completed_datetime > b->completed_datetime; });

    // Update the completed tasks list widget
    m_completedTasksList->clear();
    for (auto *task : completedToday)
    {
        QListWidgetItem *item = new QListWidgetItem(m_completedTasksList);
        TaskItemWidget *widget = new TaskItemWidget(task, repo, this, TaskItemWidget::Mode::COMPACT);
        item->setSizeHint(widget->sizeHint());
        m_completedTasksList->addItem(item);
        m_completedTasksList->setItemWidget(item, widget);
    }
}