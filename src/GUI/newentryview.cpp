#include "newentryview.h"

#include <QVBoxLayout>
#include <QLabel>

#include <QFormLayout>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QTextEdit>
#include <QComboBox>
#include <QDateTimeEdit>
#include <QSpinBox>
#include <QCheckBox>
#include <QPushButton>
#include <QGroupBox>
#include <QDateTime>
#include "timeblock.h"

NewEntryView::NewEntryView(QWidget *parent)
    : QWidget(parent)
{
    QVBoxLayout *layout = new QVBoxLayout(this);

    // Title
    QLabel *title = new QLabel("New Task / Habit", this);
    title->setStyleSheet("font-weight: bold; font-size: 18px;");
    layout->addWidget(title);

    // --- Form ---
    QFormLayout *form = new QFormLayout;

    // Name input
    m_nameEdit = new QLineEdit(this);
    form->addRow("Name:", m_nameEdit);

    // Description input
    m_descEdit = new QTextEdit(this);
    m_descEdit->setFixedHeight(80);
    form->addRow("Description:", m_descEdit);

    // Timeblock dropdown
    m_timeblockCombo = new QComboBox(this);
    form->addRow("Timeblock:", m_timeblockCombo);

    // Type dropdown (Task, Habit - Frequency, Habit - Weekday)
    m_typeCombo = new QComboBox(this);
    m_typeCombo->addItem("Task");                 // Index 0 = Task
    m_typeCombo->addItem("Habit (Frequency)");    // Index 1 = Habit (frequency)
    m_typeCombo->addItem("Habit (Weekday)");      // Index 2 = Habit (weekday)
    connect(m_typeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &NewEntryView::onTypeChanged);
    form->addRow("Type:", m_typeCombo);

    // Priority dropdown
    m_priorityCombo = new QComboBox(this);
    m_priorityCombo->addItem("Very Low", static_cast<int>(Priority::VERY_LOW));
    m_priorityCombo->addItem("Low", static_cast<int>(Priority::LOW));
    m_priorityCombo->addItem("Medium", static_cast<int>(Priority::MEDIUM));
    m_priorityCombo->addItem("High", static_cast<int>(Priority::HIGH));
    m_priorityCombo->addItem("Very High", static_cast<int>(Priority::VERY_HIGH));
    form->addRow("Priority:", m_priorityCombo);

    // Completion parameters group - contains due date (for Task), frequency, or weekday controls
    QGroupBox *completionBox = new QGroupBox("Completion Parameters", this);
    QVBoxLayout *completionLayout = new QVBoxLayout(completionBox);

    // Due date row (for Task)
    m_dueEdit = new QDateTimeEdit(QDateTime::currentDateTime(), this);
    m_dueEdit->setCalendarPopup(true);
    m_dueRowWidget = new QWidget(this);
    QHBoxLayout *dueLayout = new QHBoxLayout(m_dueRowWidget);
    dueLayout->setContentsMargins(0, 0, 0, 0);
    dueLayout->addWidget(m_dueEdit, 1);
    m_undatedCheck = new QCheckBox("Undated", this);
    m_undatedCheck->setToolTip("If set, this task will have no due date");
    m_undatedCheck->setChecked(false);
    dueLayout->addWidget(m_undatedCheck, 0, Qt::AlignVCenter);
    completionLayout->addWidget(m_dueRowWidget);

    // When 'Undated' is checked, disable the date picker and later store due_date=0
    connect(m_undatedCheck, &QCheckBox::toggled, this, [this](bool checked) { m_dueEdit->setEnabled(!checked); });

    // Frequency input (for Habit - Frequency)
    m_freqWidget = new QWidget(this);
    QHBoxLayout *freqWidgetLayout = new QHBoxLayout(m_freqWidget);
    freqWidgetLayout->setContentsMargins(0, 0, 0, 0);
    m_frequencySpin = new QSpinBox(this);
    m_frequencySpin->setRange(0, 127);
    m_frequencySpin->setValue(1);
    freqWidgetLayout->addWidget(new QLabel("Times per week:"));
    freqWidgetLayout->addWidget(m_frequencySpin);
    completionLayout->addWidget(m_freqWidget);

    // Weekday checkboxes (for Habit - Weekday)
    m_weekdayWidget = new QWidget(this);
    QHBoxLayout *weekdayRow = new QHBoxLayout(m_weekdayWidget);
    const char *days[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
    for (int i = 0; i < 7; ++i)
    {
        m_weekdayChecks[i] = new QCheckBox(days[i], this);
        weekdayRow->addWidget(m_weekdayChecks[i]);
    }
    completionLayout->addWidget(m_weekdayWidget);

    form->addRow(completionBox);

    layout->addLayout(form);

    // Create button
    m_createBtn = new QPushButton("Create", this);
    connect(m_createBtn, &QPushButton::clicked, this, &NewEntryView::onCreateClicked);
    layout->addWidget(m_createBtn, 0, Qt::AlignRight);

    // Initialize UI state
    onTypeChanged(m_typeCombo->currentIndex());
}

void NewEntryView::populateTimeblocks(const std::vector<Timeblock> &timeblocks)
{
    m_timeblockCombo->clear();
    for (const Timeblock &tb : timeblocks)
    {
        QString name = tb.name ? QString::fromUtf8(tb.name) : QString("(Unnamed)");
        m_timeblockCombo->addItem(name);
    }
}

void NewEntryView::onTypeChanged(int index)
{
    // 0 = Task, 1 = Habit (Frequency), 2 = Habit (Weekday)
    if (index == 0)
    {
        // Task: show due date row
        if (m_dueRowWidget) m_dueRowWidget->setVisible(true);
        if (m_freqWidget) m_freqWidget->setVisible(false);
        if (m_weekdayWidget) m_weekdayWidget->setVisible(false);
    }
    else if (index == 1)
    {
        // Habit (Frequency)
        if (m_dueRowWidget) m_dueRowWidget->setVisible(false);
        if (m_freqWidget) m_freqWidget->setVisible(true);
        if (m_weekdayWidget) m_weekdayWidget->setVisible(false);
    }
    else
    {
        // Habit (Weekday)
        if (m_dueRowWidget) m_dueRowWidget->setVisible(false);
        if (m_freqWidget) m_freqWidget->setVisible(false);
        if (m_weekdayWidget) m_weekdayWidget->setVisible(true);
    }
}

void NewEntryView::onCreateClicked()
{
    // Build Task object
    Task *t = new Task();

    // Set name and description
    QString name = m_nameEdit->text().trimmed();
    if (!name.isEmpty())
        t->name = strdup(name.toUtf8().constData());
    else
        t->name = strdup("(untitled)");

    QString desc = m_descEdit->toPlainText();
    t->desc = strdup(desc.toUtf8().constData());

    // Set priority
    int rawPriority = m_priorityCombo->currentData().toInt();
    t->priority = static_cast<Priority>(rawPriority);

    // Set due date or goal spec based on type
    int typeIndex = m_typeCombo->currentIndex();
    if (typeIndex == 0)
    {
        t->status = TaskStatus::INCOMPLETE; // Notify that this is a task

        // Task: set due date
        if (m_undatedCheck && m_undatedCheck->isChecked())
        {
            t->due_date = 0;
        }
        else
        {
            qint64 secs = m_dueEdit->dateTime().toSecsSinceEpoch();
            t->due_date = static_cast<time_t>(secs);
        }
    }
    else
    {
        t->status = TaskStatus::HABIT; // Notify that this is a habit

        // Habit: build GoalSpec based on which type was selected
        if (typeIndex == 1)
        {
            // Frequency
            uint8_t times = static_cast<uint8_t>(m_frequencySpin->value());
            t->goal_spec = GoalSpec::frequency(times);
        }
        else
        {
            // Weekday-based
            uint8_t flags = 0;
            // Weekday checkboxes array: 0=Sun .. 6=Sat
            const Weekday mapping[7] = {Weekday::Sunday, Weekday::Monday, Weekday::Tuesday, Weekday::Wednesday, Weekday::Thursday, Weekday::Friday, Weekday::Saturday};
            for (int i = 0; i < 7; ++i)
            {
                if (m_weekdayChecks[i]->isChecked())
                    flags |= static_cast<uint8_t>(mapping[i]);
            }
            t->goal_spec = GoalSpec::day_frequency(flags);
        }
    }

    int tbIndex = m_timeblockCombo->currentIndex();
    emit taskCreated(t, tbIndex);

    // Clear inputs
    m_nameEdit->clear();
    m_descEdit->clear();
}