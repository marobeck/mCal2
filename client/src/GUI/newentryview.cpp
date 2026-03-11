#include "newentryview.h"
#include "uuid.h"
#include "log.h"

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
    m_title = new QLabel("New Task / Habit", this);
    m_title->setStyleSheet("font-weight: bold; font-size: 18px;");
    layout->addWidget(m_title);

    // --- Form ---
    QFormLayout *form = new QFormLayout;

    // Name input
    m_nameEdit = new QLineEdit(this);
    form->addRow("Name:", m_nameEdit);

    // Description input
    m_descEdit = new QTextEdit(this);
    m_descEdit->setFixedHeight(160);
    form->addRow("Description:", m_descEdit);

    // Timeblock dropdown
    m_timeblockCombo = new QComboBox(this);
    form->addRow("Timeblock:", m_timeblockCombo);

    // Type dropdown (Task, Habit - Frequency, Habit - Weekday)
    m_typeCombo = new QComboBox(this);
    m_typeCombo->addItem("Task");              // Index 0 = Task
    m_typeCombo->addItem("Habit (Frequency)"); // Index 1 = Habit (frequency)
    m_typeCombo->addItem("Habit (Weekday)");   // Index 2 = Habit (weekday)
    connect(m_typeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &NewEntryView::onTypeChanged);
    form->addRow("Type:", m_typeCombo);

    // --- Urgency and importance dropdowns ---
    QHBoxLayout *urgRow = new QHBoxLayout;

    // Priority dropdown
    m_priorityCombo = new QComboBox(this);
    m_priorityCombo->addItem("Very Low", static_cast<int>(Priority::VERY_LOW));
    m_priorityCombo->addItem("Low", static_cast<int>(Priority::LOW));
    m_priorityCombo->addItem("Medium", static_cast<int>(Priority::MEDIUM));
    m_priorityCombo->addItem("High", static_cast<int>(Priority::HIGH));
    m_priorityCombo->addItem("Very High", static_cast<int>(Priority::VERY_HIGH));
    m_priorityCombo->setCurrentIndex(2); // Default to Medium
    // Label
    QLabel *priorityLabel = new QLabel("Priority:");
    priorityLabel->setToolTip("Estimate of how important this task is (Very Low = not important at all, Very High = extremely important)");
    urgRow->addWidget(priorityLabel, 0);
    urgRow->addWidget(m_priorityCombo, 1);

    // Scope dropdown
    m_scopeCombo = new QComboBox(this);
    m_scopeCombo->addItem("XS", static_cast<int>(Scope::XS));
    m_scopeCombo->addItem("S", static_cast<int>(Scope::S));
    m_scopeCombo->addItem("M", static_cast<int>(Scope::M));
    m_scopeCombo->addItem("L", static_cast<int>(Scope::L));
    m_scopeCombo->addItem("XL", static_cast<int>(Scope::XL));
    m_scopeCombo->setCurrentIndex(2); // Default to M
    // Label
    QLabel *scopeLabel = new QLabel("Scope:");
    scopeLabel->setToolTip("Estimate of how much effort/time would be required to get this task done (XS = very little effort, XL = a lot of effort)");
    urgRow->addWidget(scopeLabel, 0);
    urgRow->addWidget(m_scopeCombo, 1);

    form->addRow(urgRow);

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
    connect(m_undatedCheck, &QCheckBox::toggled, this, [this](bool checked)
            { m_dueEdit->setEnabled(!checked); });

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

    // --- Prerequisites box (only for tasks) ---
    m_prereqBox = new QWidget(this);
    QHBoxLayout *prereqLayout = new QHBoxLayout(m_prereqBox);
    prereqLayout->setContentsMargins(0, 0, 0, 0);
    QLabel *prereqLabel = new QLabel("Prerequisites:", m_prereqBox);
    m_prereqPlusBtn = new QPushButton("+", m_prereqBox);
    m_prereqOkBtn = new QPushButton("OK", m_prereqBox);
    m_prereqCancelBtn = new QPushButton("Cancel", m_prereqBox);
    m_prereqOkBtn->setVisible(false);
    m_prereqCancelBtn->setVisible(false);
    m_prereqPreviewArea = new QWidget(m_prereqBox);
    QHBoxLayout *previewLayout = new QHBoxLayout(m_prereqPreviewArea);
    previewLayout->setContentsMargins(0, 0, 0, 0);
    m_prereqPreviewArea->setVisible(false);

    prereqLayout->addWidget(prereqLabel, 1);
    prereqLayout->addWidget(m_prereqPreviewArea, 2);
    prereqLayout->addWidget(m_prereqPlusBtn, 0);
    prereqLayout->addWidget(m_prereqOkBtn, 0);
    prereqLayout->addWidget(m_prereqCancelBtn, 0);

    completionLayout->addWidget(m_prereqBox);

    // Connect prerequisite buttons
    connect(m_prereqPlusBtn, &QPushButton::clicked, this, &NewEntryView::onPrereqPlusClicked);
    connect(m_prereqOkBtn, &QPushButton::clicked, this, &NewEntryView::onPrereqOkClicked);
    connect(m_prereqCancelBtn, &QPushButton::clicked, this, &NewEntryView::onPrereqCancelClicked);

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

void NewEntryView::clearFields()
{
    m_editMode = false;
    m_title->setText("New Task / Habit");

    m_nameEdit->clear();
    m_descEdit->clear();
    m_priorityCombo->setCurrentIndex(2); // Medium
    m_typeCombo->setCurrentIndex(0);     // Task
    m_dueEdit->setDateTime(QDateTime::currentDateTime());
    m_undatedCheck->setChecked(false);
    m_timeblockCombo->setEnabled(true);
}

void NewEntryView::onPrereqPlusClicked()
{
    // Enter link-selection mode: show OK/Cancel, hide plus, notify MainWindow
    m_prereqPlusBtn->setVisible(false);
    m_prereqOkBtn->setVisible(true);
    m_prereqCancelBtn->setVisible(true);
    m_prereqPreviewArea->setVisible(true);
    m_linkModeActive = true;
    m_prereqUuid.clear();
    if (m_prereqPreviewWidget)
    {
        delete m_prereqPreviewWidget;
        m_prereqPreviewWidget = nullptr;
    }
    emit requestStartPrereqLink();
}

void NewEntryView::onPrereqOkClicked()
{
    // Confirm selection and leave link mode
    m_prereqPlusBtn->setVisible(true);
    m_prereqOkBtn->setVisible(false);
    m_prereqCancelBtn->setVisible(false);
    m_linkModeActive = false;
    emit requestEndPrereqLink();
}

void NewEntryView::onPrereqCancelClicked()
{
    // Cancel selection and clear preview
    m_prereqPlusBtn->setVisible(true);
    m_prereqOkBtn->setVisible(false);
    m_prereqCancelBtn->setVisible(false);
    m_linkModeActive = false;
    m_prereqUuid.clear();
    if (m_prereqPreviewWidget)
    {
        delete m_prereqPreviewWidget;
        m_prereqPreviewWidget = nullptr;
    }
    m_prereqPreviewArea->setVisible(false);
    emit requestEndPrereqLink();
}

void NewEntryView::previewPrerequisite(const Task *task)
{
    // If nullptr -> cancel
    if (!task)
    {
        // act like cancel
        onPrereqCancelClicked();
        return;
    }

    // Show preview widget inside preview area
    if (m_prereqPreviewWidget)
    {
        delete m_prereqPreviewWidget;
        m_prereqPreviewWidget = nullptr;
    }
    m_prereqPreviewWidget = new TaskItemWidget(task); // Use preview constructor
    QHBoxLayout *layout = qobject_cast<QHBoxLayout *>(m_prereqPreviewArea->layout());
    if (!layout)
    {
        layout = new QHBoxLayout(m_prereqPreviewArea);
        layout->setContentsMargins(0, 0, 0, 0);
    }
    layout->addWidget(m_prereqPreviewWidget);
    m_prereqPreviewArea->setVisible(true);
    // Store uuid for later retrieval
    m_prereqUuid = task->uuid.value;
}

std::string NewEntryView::takePrereqUuid()
{
    return m_prereqUuid;
}

void NewEntryView::loadTaskForEditing(Task *task)
{
    const char *TAG = "NewEntryView::loadTaskForEditing";

    LOGI(TAG, "Loading task for editing: %s", task->name ? task->name : "(null)");

    if (!task)
    {
        LOGW(TAG, "Null task provided to loadTaskForEditing; clearing inputs.");
        clearFields();
        return;
    }

    // Note that scene is in edit mode
    m_editMode = true;
    m_editingTask = task;

    m_title->setText("Edit Task / Habit");

    // Populate fields
    QString name = task->name ? QString::fromUtf8(task->name) : QString("");
    m_nameEdit->setText(name);

    QString desc = task->desc ? QString::fromUtf8(task->desc) : QString("");
    m_descEdit->setText(desc);

    // Disable timeblock selection during edit
    m_timeblockCombo->setEnabled(false);

    // Priority
    int priorityIndex = 2; // Default to Medium
    switch (task->priority)
    {
    case Priority::VERY_LOW:
        priorityIndex = 0;
        break;
    case Priority::LOW:
        priorityIndex = 1;
        break;
    case Priority::MEDIUM:
        priorityIndex = 2;
        break;
    case Priority::HIGH:
        priorityIndex = 3;
        break;
    case Priority::VERY_HIGH:
        priorityIndex = 4;
        break;
    }
    m_priorityCombo->setCurrentIndex(priorityIndex);

    // Scope
    int scopeIndex = 2; // Default to M
    switch (task->scope)
    {
    case Scope::XS:
        scopeIndex = 0;
        break;
    case Scope::S:
        scopeIndex = 1;
        break;
    case Scope::M:
        scopeIndex = 2;
        break;
    case Scope::L:
        scopeIndex = 3;
        break;
    case Scope::XL:
        scopeIndex = 4;
        break;
    }
    m_scopeCombo->setCurrentIndex(scopeIndex);

    // Type and related fields
    if (task->status == TaskStatus::HABIT)
    {
        // Determine if frequency-based or weekday-based habit
        if (task->goal_spec.mode() == GoalSpec::Mode::Frequency)
        {
            m_typeCombo->setCurrentIndex(1); // Habit (Frequency)
            m_frequencySpin->setValue(task->goal_spec.frequency());
        }
        else if (task->goal_spec.mode() == GoalSpec::Mode::DayFrequency)
        {
            m_typeCombo->setCurrentIndex(2); // Habit (Weekday)
            // Set weekday checkboxes
            for (int i = 0; i < 7; ++i)
            {
                Weekday dayFlag = static_cast<Weekday>(1 << i);
                m_weekdayChecks[i]->setChecked(task->goal_spec.has_day(dayFlag));
            }
        }
    }
    else
    {
        m_typeCombo->setCurrentIndex(0); // Task
        // Due date
        if (task->due_date == 0)
        {
            m_undatedCheck->setChecked(true);
        }
        else
        {
            QDateTime dueDate = QDateTime::fromSecsSinceEpoch(static_cast<qint64>(task->due_date));
            m_dueEdit->setDateTime(dueDate);
            m_undatedCheck->setChecked(false);
        }
    }

    // Prerequisite preview
    LOGI(TAG, "Task has %zu prerequisites", task->prerequisites.size());
    if (!task->prerequisites.empty())
    { // For simplicity, only preview the first prerequisite if multiple are present
        const char *prereqName = task->prerequisites[0]->name ? task->prerequisites[0]->name : "(null)";
        LOGI(TAG, "Previewing prerequisite task: %s", prereqName);
        previewPrerequisite(task->prerequisites[0]);
    }
}

void NewEntryView::onTypeChanged(int index)
{
    // 0 = Task, 1 = Habit (Frequency), 2 = Habit (Weekday)
    if (index == 0)
    {
        // Task: show due date row
        if (m_dueRowWidget)
            m_dueRowWidget->setVisible(true);
        if (m_freqWidget)
            m_freqWidget->setVisible(false);
        if (m_weekdayWidget)
            m_weekdayWidget->setVisible(false);
    }
    else if (index == 1)
    {
        // Habit (Frequency)
        if (m_dueRowWidget)
            m_dueRowWidget->setVisible(false);
        if (m_freqWidget)
            m_freqWidget->setVisible(true);
        if (m_weekdayWidget)
            m_weekdayWidget->setVisible(false);
    }
    else
    {
        // Habit (Weekday)
        if (m_dueRowWidget)
            m_dueRowWidget->setVisible(false);
        if (m_freqWidget)
            m_freqWidget->setVisible(false);
        if (m_weekdayWidget)
            m_weekdayWidget->setVisible(true);
    }
}

void NewEntryView::onCreateClicked()
{
    // Build Task object
    Task *t = new Task();

    // Set UUID
    if (m_editMode)
    {
        memcpy(t->uuid.value, m_editingTask->uuid.value, UUID_LEN);
        memcpy(t->timeblock_uuid.value, m_editingTask->timeblock_uuid.value, UUID_LEN);
    }

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

    // Set scope
    int rawScope = m_scopeCombo->currentData().toInt();
    t->scope = static_cast<Scope>(rawScope);

    // Set due date or goal spec based on type
    int typeIndex = m_typeCombo->currentIndex();
    if (typeIndex == 0)
    {
        if (m_editMode && m_editingTask->status != TaskStatus::HABIT)
        {
            t->status = m_editingTask->status; // Preserve existing status
        }
        else
        {
            t->status = TaskStatus::INCOMPLETE; // New task starts as incomplete
        }

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
    if (m_editMode)
    {
        emit taskEdited(t); // Run update signal
    }
    else
    {
        emit taskCreated(t, tbIndex); // Run add signal
    }

    // Clear inputs
    m_nameEdit->clear();
    m_descEdit->clear();
}