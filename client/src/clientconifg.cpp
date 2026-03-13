#include "clientconfig.h"
#include "log.h"

#include <QDir>
#include <QStandardPaths>
#include <QSettings>
#include <QCoreApplication>

ClientConfig::ClientConfig()
{
    // Ensure settings ini exists
    QString path = settingsFilePath();
    if (!QFile::exists(path))
    {
        LOGW("ClientConfig", "Settings file not found at %s; creating default settings.ini", path.toStdString().c_str());
        QSettings s(path, QSettings::IniFormat);

        // Fill out sync data with default values
        s.beginGroup("Sync");
        s.setValue("enabled", true);
        s.setValue("server_address", "http://localhost:8001/sync");
        s.setValue("server_port", 8001);
        s.setValue("client_id", "mcal2-client");

        // Defalt cert values
        s.setValue("client_cert", "certs/client.crt");
        s.setValue("client_key", "certs/client.key");
        s.setValue("server_ca", "certs/server_ca.crt");
        s.endGroup();

        // Create a default score weight profile
        QString g = "Profile:Eat the Frog";
        s.beginGroup(g);
        s.setValue("due_date_weight", 0.4);
        s.setValue("priority_weight", 0.4);
        s.setValue("scope_weight", 0.2);
        s.endGroup();
    }
}

/* -------------------------------------------------------------------------- */
/*                                  Get files                                 */
/* -------------------------------------------------------------------------- */

QString ClientConfig::settingsFilePath()
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

/* -------------------------------------------------------------------------- */
/*                                  Get data                                  */
/* -------------------------------------------------------------------------- */

bool ClientConfig::syncEnabled()
{
    QString path = settingsFilePath();
    QSettings s(path, QSettings::IniFormat);
    return s.value("Sync/enabled", false).toBool();
}

QString ClientConfig::serverAddress()
{
    QString path = settingsFilePath();
    QSettings s(path, QSettings::IniFormat);
    return s.value("Sync/server_address", "http://localhost:8000").toString();
}

int ClientConfig::serverPort()
{
    QString path = settingsFilePath();
    QSettings s(path, QSettings::IniFormat);
    return s.value("Sync/server_port", 8000).toInt();
}

QString ClientConfig::clientId()
{
    QString path = settingsFilePath();
    QSettings s(path, QSettings::IniFormat);
    return s.value("Sync/client_id", "mcal2-client").toString();
}

QString ClientConfig::clientCertPath()
{
    QString path = settingsFilePath();
    QSettings s(path, QSettings::IniFormat);
    return s.value("Sync/client_cert", "certs/client.crt").toString();
}

QString ClientConfig::clientKeyPath()
{
    QString path = settingsFilePath();
    QSettings s(path, QSettings::IniFormat);
    return s.value("Sync/client_key", "certs/client.key").toString();
}
QString ClientConfig::serverCaPath()
{
    QString path = settingsFilePath();
    QSettings s(path, QSettings::IniFormat);
    return s.value("Sync/server_ca", "certs/server_ca.crt").toString();
}