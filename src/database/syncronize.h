#pragma once

#include <QtNetwork/QNetworkAccessManager>
#include <QObject>
#include <QJsonArray>

#include "database.h"

class Synchronizer : public QObject {
    Q_OBJECT

private:
    Database& db;
    QString serverUrl = "http://127.0.0.1:8000/sync";
    QString clientId = "mcal2-client";
    QNetworkAccessManager* networkManager;
    int lastServerVersion;

public:
    Synchronizer(Database& db, QObject* parent = nullptr);

    void sync();

signals:
    void syncCompleted();

private slots:
    void onSyncReply(QNetworkReply* reply);

private:
    QJsonArray collectLocalChanges();
    void applyServerChanges(const QJsonArray& entries, int newServerVersion);
    int getLastServerVersion();
    void setLastServerVersion(int version);
};
