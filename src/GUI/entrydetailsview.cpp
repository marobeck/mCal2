#include "entrydetailsview.h"

#include "log.h"

#include <QVBoxLayout>
#include <QString>

EntryDetailsView::EntryDetailsView(QWidget *parent)
    : QWidget(parent)
{
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(8, 4, 8, 4);
    layout->setSpacing(6);

    m_nameLabel = new QLabel("", this);
    m_nameLabel->setStyleSheet("font-weight: bold; font-size: 18px;");
    layout->addWidget(m_nameLabel);

    m_dueLabel = new QLabel("", this);
    layout->addWidget(m_dueLabel);

    m_priorityLabel = new QLabel("", this);
    layout->addWidget(m_priorityLabel);

    m_urgencyLabel = new QLabel("", this);
    layout->addWidget(m_urgencyLabel);

    m_prereqLabel = new QLabel("", this);
    layout->addWidget(m_prereqLabel);

    m_descLabel = new QLabel("", this);
    m_descLabel->setWordWrap(true);
    layout->addWidget(m_descLabel);

    // Force everything to the top
    layout->addStretch();
}

void EntryDetailsView::loadTask(const Task *task)
{
    const char *TAG = "EntryDetailsView::loadTask";

    LOGI(TAG, "Loading task details view for task \"%s\" (%p)", task->name, task);

    if (!task)
    {
        LOGW(TAG, "Null task provided to loadTask; clearing view.");

        // Clear all fields
        m_nameLabel->setText("");
        m_descLabel->setText("");
        m_dueLabel->setText("");
        m_priorityLabel->setText("");
        m_urgencyLabel->setText("");
        m_prereqLabel->setText("");
        return;
    }

    // Name (may be null)
    QString name = task->name ? QString::fromUtf8(task->name) : QString("(untitled)");
    m_nameLabel->setText(name);

    // Description (may be null)
    QString desc = task->desc ? QString::fromUtf8(task->desc) : QString("");
    m_descLabel->setText(desc);

    // Due date string via Task API
    try
    {
        std::string due = task->due_date_full_string();
        m_dueLabel->setText(QString::fromStdString("Due: " + due));
    }
    catch (...)
    {
        m_dueLabel->setText("Due: (n/a)");
    }

    // Priority
    std::string priorityStr;
    try
    {
        priorityStr = task->priority_string();
        m_priorityLabel->setText(QString::fromStdString("Priority: " + priorityStr));
    }
    catch (...)
    {
        m_priorityLabel->setText("Priority: (n/a)");
    }

    // Urgency via Task API
    float urg = 0.0f;
    try
    {
        urg = task->get_urgency();
        m_urgencyLabel->setText(QString::fromStdString("Urgency: " + std::to_string(urg)));
    }
    catch (...)
    {
        m_urgencyLabel->setText("Urgency: (n/a)");
    }

    // Prerequisite task title if present
    if (task->prereq && task->prereq->name)
        m_prereqLabel->setText(QString::fromUtf8(task->prereq->name));
    else
        m_prereqLabel->setText("");
}