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

    // Type dropdown
    m_typeCombo = new QComboBox(this);
    m_typeCombo->addItem("Task");  // Index 0 = Task
    m_typeCombo->addItem("Habit"); // Index 1 = Habit
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

    // Due date input
    m_dueEdit = new QDateTimeEdit(QDateTime::currentDateTime(), this);
    m_dueEdit->setCalendarPopup(true);
    form->addRow("Due date:", m_dueEdit);

    // Habit options group
    QGroupBox *habitBox = new QGroupBox("Habit Options", this);
    QVBoxLayout *habitLayout = new QVBoxLayout(habitBox);

    m_habitModeCombo = new QComboBox(this);
    m_habitModeCombo->addItem("Frequency");
    m_habitModeCombo->addItem("Weekday");
    connect(m_habitModeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &NewEntryView::onHabitModeChanged);
    habitLayout->addWidget(m_habitModeCombo);

    // Frequency input
    QHBoxLayout *freqRow = new QHBoxLayout;
    m_frequencySpin = new QSpinBox(this);
    m_frequencySpin->setRange(0, 127);
    m_frequencySpin->setValue(1);
    freqRow->addWidget(new QLabel("Times per week:"));
    freqRow->addWidget(m_frequencySpin);
    habitLayout->addLayout(freqRow);

    // Weekday checkboxes
    QHBoxLayout *weekdayRow = new QHBoxLayout;
    const char *days[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
    for (int i = 0; i < 7; ++i)
    {
        m_weekdayChecks[i] = new QCheckBox(days[i], this);
        weekdayRow->addWidget(m_weekdayChecks[i]);
    }
    habitLayout->addLayout(weekdayRow);

    form->addRow(habitBox);

    layout->addLayout(form);

    // Create button
    m_createBtn = new QPushButton("Create", this);
    connect(m_createBtn, &QPushButton::clicked, this, &NewEntryView::onCreateClicked);
    layout->addWidget(m_createBtn, 0, Qt::AlignRight);

    // Initialize UI state
    onTypeChanged(m_typeCombo->currentIndex());
    onHabitModeChanged(m_habitModeCombo->currentIndex());
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
    // 0 = Task, 1 = Habit
    bool isHabit = (index == 1);
    m_dueEdit->setVisible(!isHabit);
    m_habitModeCombo->setVisible(isHabit);
    // also adjust the group of habit controls
    for (int i = 0; i < 7; ++i)
        m_weekdayChecks[i]->setVisible(isHabit);
    m_frequencySpin->setVisible(isHabit);
}

void NewEntryView::onHabitModeChanged(int index)
{
    // 0 = Frequency, 1 = Weekday
    bool freq = (index == 0);
    m_frequencySpin->setVisible(freq);
    for (int i = 0; i < 7; ++i)
        m_weekdayChecks[i]->setVisible(!freq);
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
        // Task: set due date
        qint64 secs = m_dueEdit->dateTime().toSecsSinceEpoch();
        t->due_date = static_cast<time_t>(secs);
    }
    else
    {
        // Habit: build GoalSpec
        int mode = m_habitModeCombo->currentIndex();
        if (mode == 0)
        {
            // Frequency
            uint8_t times = static_cast<uint8_t>(m_frequencySpin->value());
            t->goal_spec = GoalSpec::frequency(times);
        }
        else
        {
            // weekday flags: GoalSpec weekday flags use mapping in goalspec.h
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
}