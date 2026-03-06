#include "syncronize.h"

#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QUrl>
#include <QDebug>
#include <QEventLoop>

Synchronizer::Synchronizer(Database& db, QObject* parent)
    : QObject(parent), db(db), networkManager(new QNetworkAccessManager(this)) {
    connect(networkManager, &QNetworkAccessManager::finished, this, &Synchronizer::onSyncReply);
    lastServerVersion = getLastServerVersion();
}

void Synchronizer::sync() {
    QJsonArray changes = collectLocalChanges();

    QJsonObject payload;
    payload["client_id"] = clientId;
    payload["last_server_version"] = lastServerVersion;
    payload["entries"] = changes;

    QJsonDocument doc(payload);
    QByteArray data = doc.toJson();

    QNetworkRequest request = QNetworkRequest(QUrl(serverUrl));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    networkManager->post(request, data);
}

void Synchronizer::onSyncReply(QNetworkReply* reply) {
    if (reply->error() != QNetworkReply::NoError) {
        qWarning() << "Sync request failed:" << reply->errorString();
        reply->deleteLater();
        return;
    }

    QByteArray responseData = reply->readAll();
    QJsonDocument responseDoc = QJsonDocument::fromJson(responseData);
    QJsonObject responseObj = responseDoc.object();

    int newServerVersion = responseObj["new_server_version"].toInt();
    QJsonArray entries = responseObj["entries"].toArray();

    applyServerChanges(entries, newServerVersion);

    reply->deleteLater();

    emit syncCompleted();
}

QJsonArray Synchronizer::collectLocalChanges() {
    QJsonArray entries;

    // Collect timeblock changes
    {
        const char* sql = "SELECT t.uuid, t.status, t.name, t.description, t.day_frequency, t.duration, t.start, t.day_start, t.completed_datetime, r.modified_at, r.deleted_at "
                          "FROM timeblocks t RIGHT JOIN timeblock_change_receipts r ON t.uuid = r.uuid WHERE r.modified_at > ?";
        sqlite3_stmt* stmt;
        if (sqlite3_prepare_v2(db.db, sql, -1, &stmt, 0) == SQLITE_OK) {
            sqlite3_bind_int64(stmt, 1, lastServerVersion);
            while (sqlite3_step(stmt) == SQLITE_ROW) {
                QJsonObject data;
                data["uuid"] = QString::fromUtf8((const char*)sqlite3_column_text(stmt, 0));
                if (sqlite3_column_type(stmt, 0) != SQLITE_NULL) {  // Not deleted
                    data["status"] = sqlite3_column_int(stmt, 1);
                    data["name"] = QString::fromUtf8((const char*)sqlite3_column_text(stmt, 2));
                    data["description"] = QString::fromUtf8((const char*)sqlite3_column_text(stmt, 3));
                    data["day_frequency"] = sqlite3_column_int(stmt, 4);
                    data["duration"] = (qint64)sqlite3_column_int64(stmt, 5);
                    data["start"] = (qint64)sqlite3_column_int64(stmt, 6);
                    data["day_start"] = (qint64)sqlite3_column_int64(stmt, 7);
                    data["completed_datetime"] = (qint64)sqlite3_column_int64(stmt, 8);
                }
                data["modified_at"] = (qint64)sqlite3_column_int64(stmt, 9);
                data["deleted_at"] = sqlite3_column_type(stmt, 10) == SQLITE_NULL ? QJsonValue::Null : (qint64)sqlite3_column_int64(stmt, 10);

                QJsonObject entry;
                entry["table"] = "timeblocks";
                entry["data"] = data;
                entries.append(entry);
            }
            sqlite3_finalize(stmt);
        }
    }

    // Collect task changes
    {
        const char* sql = "SELECT t.uuid, t.timeblock_uuid, t.name, t.description, t.due_date, t.priority, t.scope, t.status, t.goal_spec, t.completed_datetime, r.modified_at, r.deleted_at "
                          "FROM tasks t RIGHT JOIN task_change_receipts r ON t.uuid = r.uuid WHERE r.modified_at > ?";
        sqlite3_stmt* stmt;
        if (sqlite3_prepare_v2(db.db, sql, -1, &stmt, 0) == SQLITE_OK) {
            sqlite3_bind_int64(stmt, 1, lastServerVersion);
            while (sqlite3_step(stmt) == SQLITE_ROW) {
                QJsonObject data;
                data["uuid"] = QString::fromUtf8((const char*)sqlite3_column_text(stmt, 0));
                if (sqlite3_column_type(stmt, 0) != SQLITE_NULL) {
                    data["timeblock_uuid"] = QString::fromUtf8((const char*)sqlite3_column_text(stmt, 1));
                    data["name"] = QString::fromUtf8((const char*)sqlite3_column_text(stmt, 2));
                    data["description"] = QString::fromUtf8((const char*)sqlite3_column_text(stmt, 3));
                    data["due_date"] = (qint64)sqlite3_column_int64(stmt, 4);
                    data["priority"] = sqlite3_column_int(stmt, 5);
                    data["scope"] = sqlite3_column_int(stmt, 6);
                    data["status"] = sqlite3_column_int(stmt, 7);
                    data["goal_spec"] = sqlite3_column_int(stmt, 8);
                    data["completed_datetime"] = (qint64)sqlite3_column_int64(stmt, 9);
                }
                data["modified_at"] = (qint64)sqlite3_column_int64(stmt, 10);
                data["deleted_at"] = sqlite3_column_type(stmt, 11) == SQLITE_NULL ? QJsonValue::Null : (qint64)sqlite3_column_int64(stmt, 11);

                QJsonObject entry;
                entry["table"] = "tasks";
                entry["data"] = data;
                entries.append(entry);
            }
            sqlite3_finalize(stmt);
        }
    }

    // Collect habit entry changes
    {
        const char* sql = "SELECT h.task_uuid, h.date, r.modified_at, r.deleted_at "
                          "FROM habit_entries h RIGHT JOIN habit_entry_change_receipts r ON h.task_uuid = r.task_uuid AND h.date = r.date WHERE r.modified_at > ?";
        sqlite3_stmt* stmt;
        if (sqlite3_prepare_v2(db.db, sql, -1, &stmt, 0) == SQLITE_OK) {
            sqlite3_bind_int64(stmt, 1, lastServerVersion);
            while (sqlite3_step(stmt) == SQLITE_ROW) {
                QJsonObject data;
                data["task_uuid"] = QString::fromUtf8((const char*)sqlite3_column_text(stmt, 0));
                data["date"] = QString::fromUtf8((const char*)sqlite3_column_text(stmt, 1));
                data["modified_at"] = (qint64)sqlite3_column_int64(stmt, 2);
                data["deleted_at"] = sqlite3_column_type(stmt, 3) == SQLITE_NULL ? QJsonValue::Null : (qint64)sqlite3_column_int64(stmt, 3);

                QJsonObject entry;
                entry["table"] = "habit_entries";
                entry["data"] = data;
                entries.append(entry);
            }
            sqlite3_finalize(stmt);
        }
    }

    // Collect entry link changes
    {
        const char* sql = "SELECT l.parent_uuid, l.child_uuid, l.link_type, r.modified_at, r.deleted_at "
                          "FROM entry_links l RIGHT JOIN entry_link_change_receipts r ON l.parent_uuid = r.parent_uuid AND l.child_uuid = r.child_uuid WHERE r.modified_at > ?";
        sqlite3_stmt* stmt;
        if (sqlite3_prepare_v2(db.db, sql, -1, &stmt, 0) == SQLITE_OK) {
            sqlite3_bind_int64(stmt, 1, lastServerVersion);
            while (sqlite3_step(stmt) == SQLITE_ROW) {
                QJsonObject data;
                data["parent_uuid"] = QString::fromUtf8((const char*)sqlite3_column_text(stmt, 0));
                data["child_uuid"] = QString::fromUtf8((const char*)sqlite3_column_text(stmt, 1));
                if (sqlite3_column_type(stmt, 0) != SQLITE_NULL) {
                    data["link_type"] = sqlite3_column_int(stmt, 2);
                }
                data["modified_at"] = (qint64)sqlite3_column_int64(stmt, 3);
                data["deleted_at"] = sqlite3_column_type(stmt, 4) == SQLITE_NULL ? QJsonValue::Null : (qint64)sqlite3_column_int64(stmt, 4);

                QJsonObject entry;
                entry["table"] = "entry_links";
                entry["data"] = data;
                entries.append(entry);
            }
            sqlite3_finalize(stmt);
        }
    }

    return entries;
}

void Synchronizer::applyServerChanges(const QJsonArray& entries, int newServerVersion) {
    for (const QJsonValue& value : entries) {
        QJsonObject entry = value.toObject();
        QString table = entry["table"].toString();
        QJsonObject data = entry["data"].toObject();

        if (table == "timeblocks") {
            QString uuid = data["uuid"].toString();
            if (data.contains("deleted_at") && !data["deleted_at"].isNull()) {
                // Delete
                db.delete_timeblock(uuid.toUtf8().constData());
            } else {
                // Insert or update
                Timeblock tb;
                strncpy(tb.uuid.value, uuid.toUtf8().constData(), UUID_LEN);
                tb.status = static_cast<TimeblockStatus>(data["status"].toInt());
                tb.name = strdup(data["name"].toString().toUtf8().constData());
                tb.desc = strdup(data["description"].toString().toUtf8().constData());
                tb.day_frequency = GoalSpec::from_sql(data["day_frequency"].toInt());
                tb.duration = data["duration"].toVariant().toLongLong();
                tb.start = data["start"].toVariant().toLongLong();
                tb.day_start = data["day_start"].toVariant().toLongLong();
                tb.completed_datetime = data["completed_datetime"].toVariant().toLongLong();
                try {
                    db.update_timeblock(tb);
                } catch (...) {
                    db.insert_timeblock(tb);
                }
            }
        } else if (table == "tasks") {
            QString uuid = data["uuid"].toString();
            if (data.contains("deleted_at") && !data["deleted_at"].isNull()) {
                db.delete_task(uuid.toUtf8().constData());
            } else {
                Task task;
                strncpy(task.uuid.value, uuid.toUtf8().constData(), UUID_LEN);
                strncpy(task.timeblock_uuid.value, data["timeblock_uuid"].toString().toUtf8().constData(), UUID_LEN);
                task.name = strdup(data["name"].toString().toUtf8().constData());
                task.desc = strdup(data["description"].toString().toUtf8().constData());
                task.due_date = data["due_date"].toVariant().toLongLong();
                task.priority = static_cast<Priority>(data["priority"].toInt());
                task.scope = static_cast<Scope>(data["scope"].toInt());
                task.status = static_cast<TaskStatus>(data["status"].toInt());
                task.goal_spec = GoalSpec::from_sql(data["goal_spec"].toInt());
                task.completed_datetime = data["completed_datetime"].toVariant().toLongLong();
                try {
                    db.update_task(task);
                } catch (...) {
                    db.insert_task(task);
                }
            }
        } else if (table == "habit_entries") {
            QString taskUuid = data["task_uuid"].toString();
            QString date = data["date"].toString();
            if (data.contains("deleted_at") && !data["deleted_at"].isNull()) {
                db.remove_habit_entry(taskUuid.toUtf8().constData(), date.toUtf8().constData());
            } else {
                db.add_habit_entry(taskUuid.toUtf8().constData(), date.toUtf8().constData());
            }
        } else if (table == "entry_links") {
            QString parentUuid = data["parent_uuid"].toString();
            QString childUuid = data["child_uuid"].toString();
            LinkType linkType = static_cast<LinkType>(data["link_type"].toInt());
            if (data.contains("deleted_at") && !data["deleted_at"].isNull()) {
                db.remove_entry_link(parentUuid.toUtf8().constData(), childUuid.toUtf8().constData(), linkType);
            } else {
                db.add_entry_link(parentUuid.toUtf8().constData(), childUuid.toUtf8().constData(), linkType);
            }
        }
    }

    setLastServerVersion(newServerVersion);
}

int Synchronizer::getLastServerVersion() {
    const char* sql = "SELECT last_server_version FROM client_sync_state WHERE id = 1;";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db.db, sql, -1, &stmt, 0) != SQLITE_OK) return 0;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        int version = sqlite3_column_int(stmt, 0);
        sqlite3_finalize(stmt);
        return version;
    }
    sqlite3_finalize(stmt);
    return 0;
}

void Synchronizer::setLastServerVersion(int version) {
    const char* sql = "UPDATE client_sync_state SET last_server_version = ? WHERE id = 1;";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db.db, sql, -1, &stmt, 0) == SQLITE_OK) {
        sqlite3_bind_int(stmt, 1, version);
        sqlite3_step(stmt);
        sqlite3_finalize(stmt);
    }
    lastServerVersion = version;
}