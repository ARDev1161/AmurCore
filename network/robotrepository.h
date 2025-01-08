#ifndef ROBOTREPOSITORY_H
#define ROBOTREPOSITORY_H

#include <QSqlDatabase>
#include <QString>
#include <QList>
#include <QVariant>
#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>

#include "robotentry.h"

class RobotRepository
{
public:
    // Конструктор принимает имя файла БД
    explicit RobotRepository(const QString &dbName);

    // Деструктор на случай, если нужно закрыть БД
    ~RobotRepository();

    // Открытие/создание БД + создание таблицы при необходимости
    bool openDatabase();

    // Проверка существования робота по machineID
    bool robotExists(const QString &machineID);

    // Удаление робота по shared_ptr
    bool deleteRobot(const RobotEntryPtr &robot);
    // Обновление данных робота по shared_ptr
    bool updateRobot(const RobotEntryPtr &robot);
    // Добавить одного робота в таблицу (INSERT)
    bool addRobot(const RobotEntryPtr robot);
    // Добавить несколько роботов разом (транзакция)
    bool addRobots(const RobotList &robots);

    // Загрузить всех роботов из таблицы (SELECT *).
    // При желании можно передать socket, если нужно.
    RobotList loadRobots(QUdpSocket *socket = nullptr);

    // Найти роботов по указанным критериям (SELECT с WHERE).
    // Неуказанные поля (пустые строки) игнорируются.
    RobotList searchRobots(
        QUdpSocket *socket = nullptr,
        const QString &machineID = QString(),
        const QString &name = QString(),
        const QString &address = QString(),
        const QString &macAddress = QString()
    );

    // Удалить роботов по указанным критериям (DELETE с WHERE).
    // Неуказанные поля (пустые строки) игнорируются.
    bool deleteRobots(
        const QString &machineID = QString(),
        const QString &name = QString(),
        const QString &address = QString(),
        const QString &macAddress = QString()
    );

private:
    // Метод для создания таблицы, если нет
    bool createTableIfNotExists();

private:
    QSqlDatabase m_db;
    QString m_dbName;
};

#endif // ROBOTREPOSITORY_H
