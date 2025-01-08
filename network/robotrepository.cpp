#include "robotrepository.h"

RobotRepository::RobotRepository(const QString &dbName)
    : m_dbName(dbName)
{
    // Можно автоматически вызывать openDatabase() здесь
    // но иногда удобнее делать это вручную снаружи
}

RobotRepository::~RobotRepository()
{
    if (m_db.isOpen()) {
        m_db.close();
    }
}

bool RobotRepository::openDatabase()
{
    // Чтобы можно было открыть несколько раз или переоткрыть:
    if (QSqlDatabase::contains("RobotConnection")) {
        m_db = QSqlDatabase::database("RobotConnection");
    } else {
        m_db = QSqlDatabase::addDatabase("QSQLITE", "RobotConnection");
        m_db.setDatabaseName(m_dbName);
    }

    if (!m_db.open()) {
        qDebug() << "Failed to open database:" << m_db.lastError().text();
        return false;
    }

    // Создаём таблицу, если её нет
    if (!createTableIfNotExists()) {
        m_db.close();
        return false;
    }

    return true;
}

bool RobotRepository::createTableIfNotExists()
{
    if (!m_db.isOpen()) {
        qDebug() << "Database not open!";
        return false;
    }

    QSqlQuery query(m_db);
    QString createTableSQL = R"(
        CREATE TABLE IF NOT EXISTS robots (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            machineID TEXT,
            name TEXT,
            address TEXT,
            port INTEGER,
            portForAnswer INTEGER,
            macAddress TEXT
        )
    )";

    if (!query.exec(createTableSQL)) {
        qDebug() << "Failed to create table 'robots':" << query.lastError().text();
        return false;
    }

    return true;
}

bool RobotRepository::robotExists(const QString &machineID)
{
    if (!m_db.isOpen()) {
        qDebug() << "Database not open!";
        return false;
    }

    QSqlQuery query(m_db);
    query.prepare("SELECT COUNT(*) FROM robots WHERE machineID = :machineID");
    query.bindValue(":machineID", machineID);

    if (!query.exec()) {
        qDebug() << "Failed to check robot existence:" << query.lastError().text();
        return false;
    }

    if (query.next()) {
        return query.value(0).toInt() > 0;
    }

    return false;
}

bool RobotRepository::deleteRobot(const RobotEntryPtr &robot)
{
    if (!m_db.isOpen()) {
        qDebug() << "Database is not open!";
        return false;
    }

    QSqlQuery query(m_db);
    query.prepare("DELETE FROM robots WHERE machineID = :machineID");
    query.bindValue(":machineID", robot->machineID());

    if (!query.exec()) {
        qDebug() << "Failed to delete robot:" << query.lastError().text();
        return false;
    }

    qDebug() << "Robot with machineID" << robot->machineID() << "deleted successfully.";
    return true;
}

bool RobotRepository::updateRobot(const RobotEntryPtr &robot)
{
    if (!m_db.isOpen()) {
        qDebug() << "Database is not open!";
        return false;
    }

    QSqlQuery query(m_db);
    query.prepare(R"(
        UPDATE robots
        SET name = :name,
            address = :address,
            port = :port,
            portForAnswer = :portForAnswer
        WHERE machineID = :machineID
    )");

    query.bindValue(":name", robot->name());
    query.bindValue(":address", robot->address().toString());
    query.bindValue(":port", robot->port());
    query.bindValue(":portForAnswer", robot->getPortForAnswer());
    query.bindValue(":machineID", robot->machineID());

    if (!query.exec()) {
        qDebug() << "Failed to update robot:" << query.lastError().text();
        return false;
    }

    qDebug() << "Robot with machineID" << robot->machineID() << "updated successfully.";
    return true;
}

bool RobotRepository::addRobot(const RobotEntryPtr robot)
{
    if (!m_db.isOpen()) {
        qDebug() << "Database not open!";
        return false;
    }

    if (robotExists(robot->machineID())) {
        qDebug() << "Robot with machineID" << robot->machineID() << "already exists. Skipping insertion.";
        return false;
    }

    QSqlQuery query(m_db);
    query.prepare(R"(
        INSERT INTO robots
            (machineID, name, address, port, portForAnswer, macAddress)
        VALUES
            (:machineID, :name, :address, :port, :portForAnswer, :macAddress)
    )");

    query.bindValue(":machineID",     robot->machineID());
    query.bindValue(":name",          robot->name());
    query.bindValue(":address",       robot->address().toString());
    query.bindValue(":port",          robot->port());
    query.bindValue(":portForAnswer", robot->getPortForAnswer());
    query.bindValue(":macAddress",    robot->macAddress());

    if (!query.exec()) {
        qDebug() << "Failed to insert robot:" << query.lastError().text();
        return false;
    }
    return true;
}

bool RobotRepository::addRobots(const RobotList &robots)
{
    if (!m_db.isOpen()) {
        qDebug() << "Database not open!";
        return false;
    }

    QSqlQuery query(m_db);

    // Начинаем транзакцию для ускорения вставки
    if (!m_db.transaction()) {
        qDebug() << "Failed to begin transaction:" << m_db.lastError().text();
        return false;
    }

    query.prepare(R"(
        INSERT INTO robots
            (machineID, name, address, port, portForAnswer, macAddress)
        VALUES
            (:machineID, :name, :address, :port, :portForAnswer, :macAddress)
    )");

    for (RobotEntryPtr robot : robots) {
        if (!robot) continue; // пропускаем, если указатель нулевой

        if (robotExists(robot->machineID())) {
            qDebug() << "Robot with machineID" << robot->machineID() << "already exists. Skipping insertion.";
            continue;
        }

        query.bindValue(":machineID",     robot->machineID());
        query.bindValue(":name",          robot->name());
        query.bindValue(":address",       robot->address().toString());
        query.bindValue(":port",          robot->port());
        query.bindValue(":portForAnswer", robot->getPortForAnswer());
        query.bindValue(":macAddress",    robot->macAddress());

        if (!query.exec()) {
            qDebug() << "Failed to insert robot:" << query.lastError().text();
        }
    }

    if (!m_db.commit()) {
        qDebug() << "Failed to commit transaction:" << m_db.lastError().text();
        return false;
    }

    return true;
}

RobotList RobotRepository::loadRobots(QUdpSocket *socket)
{
    RobotList result;
    if (!m_db.isOpen()) {
        qDebug() << "Database not open!";
        return result;
    }

    QSqlQuery query(m_db);
    if (!query.exec(R"(
        SELECT machineID, name, address, port, portForAnswer, macAddress
        FROM robots
    )")) {
        qDebug() << "Failed to load robots:" << query.lastError().text();
        return result;
    }

    while (query.next()) {
        QString machineID     = query.value("machineID").toString();
        QString name          = query.value("name").toString();
        QString addressString = query.value("address").toString();
        quint16 port          = query.value("port").toUInt();
        quint16 portForAnswer = query.value("portForAnswer").toUInt();
        QString macAddress    = query.value("macAddress").toString();

        RobotEntryPtr robot = std::make_shared<RobotEntry>(socket,
                                           QHostAddress(addressString),
                                           port,
                                           machineID,
                                           name,
                                           macAddress);
        robot->setPortForAnswer(portForAnswer);

        result.append(robot);
    }

    return result;
}

RobotList RobotRepository::searchRobots(
    QUdpSocket *socket,
    const QString &machineID,
    const QString &name,
    const QString &address,
    const QString &macAddress
) {
    RobotList result;
    if (!m_db.isOpen()) {
        qDebug() << "Database not open!";
        return result;
    }

    QString baseQuery = R"(
        SELECT machineID, name, address, port, portForAnswer, macAddress
        FROM robots
    )";

    // Формируем WHERE в зависимости от непустых полей
    QStringList conditions;
    if (!machineID.isEmpty()) {
        conditions << "machineID = :machineID";
    }
    if (!name.isEmpty()) {
        conditions << "name = :name";
    }
    if (!address.isEmpty()) {
        conditions << "address = :address";
    }
    if (!macAddress.isEmpty()) {
        conditions << "macAddress = :macAddress";
    }

    if (!conditions.isEmpty()) {
        baseQuery += " WHERE " + conditions.join(" AND ");
    }

    QSqlQuery query(m_db);
    query.prepare(baseQuery);

    if (!machineID.isEmpty()) {
        query.bindValue(":machineID", machineID);
    }
    if (!name.isEmpty()) {
        query.bindValue(":name", name);
    }
    if (!address.isEmpty()) {
        query.bindValue(":address", address);
    }
    if (!macAddress.isEmpty()) {
        query.bindValue(":macAddress", macAddress);
    }

    if (!query.exec()) {
        qDebug() << "Failed to search robots:" << query.lastError().text();
        return result;
    }

    while (query.next()) {
        QString machineID_     = query.value("machineID").toString();
        QString name_          = query.value("name").toString();
        QString addressString  = query.value("address").toString();
        quint16 port           = static_cast<quint16>(query.value("port").toUInt());
        quint16 portForAnswer  = static_cast<quint16>(query.value("portForAnswer").toUInt());
        QString macAddress_    = query.value("macAddress").toString();

        RobotEntryPtr robot = std::make_shared<RobotEntry>(
            socket,
            QHostAddress(addressString),
            port,
            machineID_,
            name_,
            macAddress_
            );
        robot->setPortForAnswer(portForAnswer);

        result.append(robot);
    }

    return result;
}

bool RobotRepository::deleteRobots(
    const QString &machineID,
    const QString &name,
    const QString &address,
    const QString &macAddress
) {
    if (!m_db.isOpen()) {
        qDebug() << "Database not open!";
        return false;
    }

    QString baseQuery = "DELETE FROM robots";

    QStringList conditions;
    if (!machineID.isEmpty()) {
        conditions << "machineID = :machineID";
    }
    if (!name.isEmpty()) {
        conditions << "name = :name";
    }
    if (!address.isEmpty()) {
        conditions << "address = :address";
    }
    if (!macAddress.isEmpty()) {
        conditions << "macAddress = :macAddress";
    }

    if (!conditions.isEmpty()) {
        baseQuery += " WHERE " + conditions.join(" AND ");
    }

    QSqlQuery query(m_db);
    query.prepare(baseQuery);

    if (!machineID.isEmpty()) {
        query.bindValue(":machineID", machineID);
    }
    if (!name.isEmpty()) {
        query.bindValue(":name", name);
    }
    if (!address.isEmpty()) {
        query.bindValue(":address", address);
    }
    if (!macAddress.isEmpty()) {
        query.bindValue(":macAddress", macAddress);
    }

    if (!query.exec()) {
        qDebug() << "Failed to delete robots:" << query.lastError().text();
        return false;
    }

    return true;
}
