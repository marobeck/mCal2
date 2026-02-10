#include "overviewview.h"

#include "log.h"
#include "taskitemwidget.h"

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
    updateOverview(repo->timeblocks());
}

void OverviewView::updateOverview(const std::vector<Timeblock> &timeblocks)
{
    const char *TAG = "OverviewView::updateOverview";
    LOGI(TAG, "Updating overview with %zu timeblocks.", timeblocks.size());

    size_t tasksDisplayed = 5; // Max number of tasks to display in overview

    // Iterate through all timeblocks and tasks to find the most urgent tasks
    std::vector<const Task *> tasksToDisplay;
    for (const auto &tb : timeblocks)
    {
        // Grab all incomplete tasks
        for (const auto &task : tb.tasks)
        {
            tasksToDisplay.push_back(&task);
        }
    }

    // Sort tasks by urgency and take the top ones
    std::sort(tasksToDisplay.begin(), tasksToDisplay.end(), [](const Task *a, const Task *b)
              { return a->get_urgency() > b->get_urgency(); });
    if (tasksToDisplay.size() > tasksDisplayed)
    {
        tasksToDisplay.resize(tasksDisplayed);
    }

    // Update the list widget with the top tasks
    m_urgentTasksList->clear();
    for (const auto *task : tasksToDisplay)
    {
        QListWidgetItem *item = new QListWidgetItem(m_urgentTasksList);
        TaskItemWidget *widget = new TaskItemWidget(*task, repo, this, TaskItemWidget::Mode::COMPACT);
        item->setSizeHint(widget->sizeHint());
        m_urgentTasksList->addItem(item);
        m_urgentTasksList->setItemWidget(item, widget);
    }
}