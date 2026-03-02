#pragma once

#include <QWidget>
#include <QLabel>
#include <QComboBox>

class SettingsView : public QWidget
{
    Q_OBJECT
public:
    explicit SettingsView(QWidget *parent = nullptr);
    void FindScoreWeights(); // Review the .ini file for score weight settings and update sortWeightCombo accordingly

private:
    QComboBox *m_sortWeightCombo = nullptr; // Combo box for selecting sort weight
    QComboBox *m_themeCombo = nullptr;      // Combo box for selecting theme
    QString m_currentProfile; // Member to track current profile

private slots:
    void onProfileChanged(int index); // Slot to handle combobox changes
};