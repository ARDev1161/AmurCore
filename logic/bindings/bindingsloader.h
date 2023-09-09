#ifndef BINDINGSLOADER_H
#define BINDINGSLOADER_H

#include <QJsonObject>
#include <QJsonDocument>
#include <QFile>

class BindingsLoader
{
    QHash<QString, QVariant> bindTable;
    QString configPath;

    void parseJson(const QJsonObject& obj, const QString& parentPath = "");
    void saveToJson(QJsonObject& obj, const QString& parentPath = "");
public:
    BindingsLoader(QString configPath);

    void loadBindings();
    void saveBindings();

    QHash<QString, QVariant> getBindTable() const;
};

#endif // BINDINGSLOADER_H
