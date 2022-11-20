#ifndef JSONWORKER_H
#define JSONWORKER_H

#include <QObject>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>

class JSONWorker : public QObject
{
    Q_OBJECT
public:
    explicit JSONWorker(QObject *parent = nullptr);

    int readJSON(QString filename, const QJsonObject &json);
    int writeJSON(QString filename, QJsonObject &json) const;
};

#endif // JSONWORKER_H
