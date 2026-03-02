#include "settingsview.h"
#include "log.h"
#include "task.h"

#include <QVBoxLayout>
#include <QLabel>
#include <QComboBox>
#include <QSettings>
#include <QStandardPaths>
#include <QInputDialog>
#include <QCoreApplication>
#include <QDir>

ScoreWeights g_score_weights; // Global variable to hold current score weights

static QString settingsFilePath()
{
    // Use asset folder
    QString assetPath = QCoreApplication::applicationDirPath() + "/assets";
    if (QDir(assetPath).exists())
        return assetPath + "/settings.ini";
    else
    {
        // Fallback to standard config location
        QString configDir = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
        QDir().mkpath(configDir); // Ensure directory exists
        return configDir + "/settings.ini";
    }
}

void SettingsView::FindScoreWeights()
{
    const char *TAG = "SettingsView::FindScoreWeights";

    QString path = settingsFilePath();
    QSettings s(path, QSettings::IniFormat);

    // Clear combo and fill with groups named "Profile:<name>"
    m_sortWeightCombo->blockSignals(true);
    m_sortWeightCombo->clear();

    const QStringList groups = s.childGroups();
    // Expect groups like "Profile:Default"
    for (const QString &g : groups)
    {
        if (g.startsWith("Profile:"))
            m_sortWeightCombo->addItem(g.mid(QString("Profile:").size()), g);
    }

    // If no profiles, create a default
    if (m_sortWeightCombo->count() == 0)
    {
        LOGW(TAG, "No score weight profiles found in settings; creating default (Eat the Frog).");

        QString g = "Profile:Eat the Frog";
        s.beginGroup(g);
        s.setValue("due_date_weight", 0.4);
        s.setValue("priority_weight", 0.4);
        s.setValue("scope_weight", 0.2);
        s.endGroup();
        m_sortWeightCombo->addItem("Eat the Frog", g);
    }

    // Read current_profile value from INI (root key)
    QString cur = s.value("current_profile", QString()).toString();
    if (!cur.isEmpty())
    {
        // find the index of the matching group data
        int found = -1;
        for (int i = 0; i < m_sortWeightCombo->count(); ++i)
        {
            if (m_sortWeightCombo->itemData(i).toString() == cur)
            {
                found = i;
                break;
            }
        }
        if (found >= 0)
            m_sortWeightCombo->setCurrentIndex(found);
        else
            m_sortWeightCombo->setCurrentIndex(0);
    }
    else
    {
        m_sortWeightCombo->setCurrentIndex(0);
    }

    m_sortWeightCombo->blockSignals(false);

    // Connect the combobox signal (only once)
    connect(m_sortWeightCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &SettingsView::onProfileChanged);

    // Apply the currently selected profile to g_score_weights
    onProfileChanged(m_sortWeightCombo->currentIndex());
}

SettingsView::SettingsView(QWidget *parent) : QWidget(parent)
{
    // Build UI
    QVBoxLayout *layout = new QVBoxLayout(this);

    // Score weight profile selector
    QLabel *sortWeightLabel = new QLabel("Score Weight Profile:", this);
    m_sortWeightCombo = new QComboBox(this);
    layout->addWidget(sortWeightLabel);
    layout->addWidget(m_sortWeightCombo);

    // TODO: App themes
    // // Theme selector (placeholder, no functionality yet)
    // QLabel *themeLabel = new QLabel("Theme:", this);
    // m_themeCombo = new QComboBox(this);
    // m_themeCombo->addItem("Light");
    // m_themeCombo->addItem("Dark");
    // layout->addWidget(themeLabel);
    // layout->addWidget(m_themeCombo);

    // Move all widges to top
    layout->addStretch();

    // Load score weight profiles from settings
    FindScoreWeights();
}

void SettingsView::onProfileChanged(int index)
{
    const char *TAG = "SettingsView::onProfileChanged";

    if (index < 0 || !m_sortWeightCombo)
        return;

    QString groupKey = m_sortWeightCombo->itemData(index).toString();
    if (groupKey.isEmpty())
        return;

    QString path = settingsFilePath();
    QSettings s(path, QSettings::IniFormat);

    s.beginGroup(groupKey);
    double due = s.value("due_date_weight", 1.0).toDouble();
    double pri = s.value("priority_weight", 1.0).toDouble();
    double sc = s.value("scope_weight", 1.0).toDouble();
    double c = s.value("undated_pressure_constant", 0.3).toDouble();
    s.endGroup();

    // Update global weights so Task::get_urgency() uses them
    ScoreWeights w;
    w.due_date_weight = static_cast<float>(due);
    w.priority_weight = static_cast<float>(pri);
    w.scope_weight = static_cast<float>(sc);
    w.undated_pressure_constant = static_cast<float>(c);
    g_score_weights = w;

    LOGI(TAG, "Profile '%s' selected: due=%f pri=%f scope=%f undated_pressure_constant=%f",
         groupKey.toUtf8().constData(), due, pri, sc, c);

    // Persist current_profile at root of INI
    s.setValue("current_profile", groupKey);

    // Remember locally
    m_currentProfile = groupKey;

    // Optionally notify other components: you can emit a custom signal here if desired.
    // For now, repository / UI refresh will pick up next time modelChanged() is called.
}