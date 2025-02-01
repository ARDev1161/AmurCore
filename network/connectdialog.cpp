#include "connectdialog.h"
#include "ui_connectdialog.h"

ConnectDialog::ConnectDialog(QWidget *parent, std::shared_ptr<NetworkController> networkController, std::shared_ptr<RobotRepository> repo) :
    QDialog(parent),
    ui(new Ui::ConnectDialog),
    updateTimer(new QTimer(this)),
    networkController(networkController),
    repo(repo)
{
    ui->setupUi(this);

    // Настройка таймера
    updateTimer->setInterval(500); // Интервал 500 мс (2 раза в секунду)
    connect(updateTimer, &QTimer::timeout, this, &ConnectDialog::updateTable);

    connect(ui->robotsTableWidget,
            &QTableWidget::itemChanged,
            this,
            &ConnectDialog::onRobotNameChanged);
    connect(ui->robotsTableWidget, &QTableWidget::cellDoubleClicked, this, &ConnectDialog::onTableRowDoubleClicked);

    // Запуск таймера
    updateTimer->start();
}

ConnectDialog::~ConnectDialog()
{
    delete ui;
}

void ConnectDialog::on_buttonBox_accepted()
{

}

void ConnectDialog::updateTable()
{
    static RobotList activeUnknown, knownInactive, activeKnown;

    /// Get robot list & split it to categories
    bool changed = categorizeRobots(repo->loadRobots(), networkController->getRobots(), activeUnknown, knownInactive, activeKnown);

    if(!changed)
        return;

    // Очищаем таблицу перед добавлением
    ui->robotsTableWidget->clearContents();
    ui->robotsTableWidget->setRowCount(0);

    // Устанавливаем заголовки (если нужно)
    setupTableHeaders(ui->robotsTableWidget);

    int rowIndex = 0;

    static QColor defaultTextColor = ui->robotsTableWidget->palette().color(QPalette::Text);
    static QColor defaultBackgroundColor = ui->robotsTableWidget->palette().color(QPalette::Base);

    rowIndex = fillTableWithRobots(ui->robotsTableWidget, activeKnown, QColor(127, 255, 0), QColor(Qt::black), rowIndex);
    rowIndex = fillTableWithRobots(ui->robotsTableWidget, activeUnknown, defaultBackgroundColor, defaultTextColor, rowIndex);
    fillTableWithRobots(ui->robotsTableWidget, knownInactive, QColor(128, 128, 128), defaultTextColor, rowIndex);
}

void ConnectDialog::onRobotNameChanged(QTableWidgetItem *item)
{
    // Проверяем, что пользователь менял именно колонку Name.
    if (item->column() != 0) {
        // Иначе, игнорируем
        return;
    }

    // Получаем новое имя (то, что пользователь ввёл)
    QString newName = item->text();

    // Извлекаем указатель на RobotEntry из данных ячейки
    QVariant dataVar = item->data(Qt::UserRole + 1);
    if (!dataVar.isValid()) {
        return;
    }

    // "Распаковываем" shared_ptr
    RobotEntryPtr robotPtr = dataVar.value<RobotEntryPtr>();
    if (!robotPtr) {
        return;
    }

    // Считываем из UserRole «старое» имя
    const QVariant oldNameVariant = item->data(Qt::UserRole);
    QString oldName = oldNameVariant.isValid() ? oldNameVariant.toString()
                                               : QString();

    // Сравниваем
    if (oldName == newName) {
        // Ничего не изменилось (возможно пользователь нажал Enter, но не менял текст)
        return;
    }

    // Случай 1: Старое имя пустое, новое не пустое -> это новая запись
    if (oldName.isEmpty() && !newName.isEmpty()) {
        // Добавляем в RobotRepository
        robotPtr->setName(newName);
        // Никаких безымянных роботов в базе!!!
        if(repo->robotExists(robotPtr->machineID()))
            repo->deleteRobot(robotPtr);

        repo->addRobot(robotPtr);
        qDebug() << "Add new robot:" << newName;
    }
    // Случай 2: Старое имя непустое, новое тоже непустое -> переименовываем
    else if (!oldName.isEmpty() && !newName.isEmpty()) {
        // Обновляем робота в RobotRepository
        repo->updateRobot(robotPtr);
        qDebug() << "Rename robot from" << oldName << "to" << newName;
    }
    // Случай 3: Новое имя пустое (пользователь стёр) -> подтверждение удаления
    else if (!newName.isEmpty() == false) {
        // Показываем окно "Do you really want to delete the robot: oldName?"
        QMessageBox::StandardButton reply = QMessageBox::question(
            this,
            tr("Delete Robot"),
            tr("Do you really want to delete the robot: %1?").arg(oldName),
            QMessageBox::Yes | QMessageBox::No
            );

        if (reply == QMessageBox::Yes) {
            repo->deleteRobot(robotPtr);
            qDebug() << "Removing robot:" << oldName;
        } else {
            // Откатываем, т.е. восстанавливаем старое имя
            item->setText(oldName);
        }
    }

    // В любом случае, после «успешного» изменения нужно сохранить текущее значение в UserRole
    // Чтобы при следующем изменении у нас было актуальное "старое" имя
    item->setData(Qt::UserRole, newName);
}

void ConnectDialog::onTableRowDoubleClicked(int row, int column)
{
    Q_UNUSED(column); // Column is not used here

    // Assuming the address is in the third column (index 2)
    QTableWidgetItem *addressItem = ui->robotsTableWidget->item(row, 2); // Column for address
    QTableWidgetItem *portItem = ui->robotsTableWidget->item(row, 3);    // Column for port

    if (addressItem && portItem) {
        QString address = addressItem->text(); // Get the address from the cell
        QString port = portItem->text();       // Get the port from the cell

        // Format the string as: address:port
        QString fullAddress = QString("%1:%2").arg(address, port);
        ui->lineHostName->setText(fullAddress); // Set the full address in the QLineEdit
    }
}

bool ConnectDialog::categorizeRobots(
    const RobotList &knownRobots,  // List of known robots from the database
    const RobotList &activeRobots, // List of currently active robots in the network
    RobotList &activeUnknown,     // Result: active but unknown robots
    RobotList &knownInactive,     // Result: known but inactive robots
    RobotList &activeKnown        // Result: active and known robots
    ) {
    // Use QHash for fast lookup of known robots by machineID
    QHash<QString, RobotEntryPtr> knownRobotsMap;
    QSet<QString> activeRobotIDs;

    // Populate the knownRobotsMap for quick access to known robots
    for (const RobotEntryPtr &robot : knownRobots) {
        if (robot) {
            knownRobotsMap.insert(robot->machineID(), robot);
        }
    }

    // Populate activeRobotIDs and classify active robots into activeUnknownNew and activeKnownNew
    RobotList activeUnknownNew, activeKnownNew;

    for (const RobotEntryPtr &robot : activeRobots) {
        if (robot) {
            activeRobotIDs.insert(robot->machineID());

            // If the robot is known
            if (knownRobotsMap.contains(robot->machineID())) {
                activeKnownNew.append(knownRobotsMap.value(robot->machineID())); // Active and known robot
            } else {
                activeUnknownNew.append(robot); // Active but unknown robot
            }
        }
    }

    // Identify known robots that are inactive
    RobotList knownInactiveNew;
    for (const QString &machineID : knownRobotsMap.keys()) {
        if (!activeRobotIDs.contains(machineID)) {
            knownInactiveNew.append(knownRobotsMap.value(machineID)); // Known but inactive robot
        }
    }

    // Helper function to extract machineIDs as a QSet
    auto extractMachineIDs = [](const RobotList &robots) {
        QSet<QString> machineIDs;
        for (const RobotEntryPtr &robot : robots) {
            if (robot) {
                machineIDs.insert(robot->machineID());
            }
        }
        return machineIDs;
    };

    // Compare machineIDs of new and existing lists
    bool isChanged = (extractMachineIDs(activeUnknown) != extractMachineIDs(activeUnknownNew)) ||
                     (extractMachineIDs(knownInactive) != extractMachineIDs(knownInactiveNew)) ||
                     (extractMachineIDs(activeKnown) != extractMachineIDs(activeKnownNew));

    // Update the result lists only if there are changes
    if (isChanged) {
        activeUnknown = std::move(activeUnknownNew);
        knownInactive = std::move(knownInactiveNew);
        activeKnown = std::move(activeKnownNew);
    }

    return isChanged; // Return true if the lists were updated, false otherwise
}

void ConnectDialog::setupTableHeaders(QTableWidget *tableWidget)
{
    tableWidget->setColumnCount(4);
    QStringList headers;
    headers << "Name" << "Machine ID" << "Address" << "Port" << "MAC";
    tableWidget->setHorizontalHeaderLabels(headers);
}

int ConnectDialog::fillTableWithRobots(QTableWidget *tableWidget, const RobotList &robots, const QColor &backgroundColor, const QColor &textColor, int startRow)
{
    int rowIndex = startRow;

    for (RobotEntryPtr robot : robots) {
        if (!robot) continue;

        // Устанавливаем новую строку в таблице
        tableWidget->insertRow(rowIndex);

        QStringList robotData = {
            robot->name(),
            robot->machineID(),
            robot->address().toString(),
            QString::number(robot->port()),
            robot->macAddress()
        };

        // Заполняем ячейки строки
        for (int col = 0; col < robotData.size(); ++col) {
            QTableWidgetItem *item = new QTableWidgetItem(robotData[col]); // Создаём QTableWidgetItem для каждого столбца

            // Устанавливаем цвет фона и текста
            item->setBackground(backgroundColor);
            item->setForeground(textColor);

            // Флаги по умолчанию (не редактируется)
            Qt::ItemFlags flags = Qt::ItemIsEnabled | Qt::ItemIsSelectable;
            // Разрешаем редактировать только в колонке 0 (Name)
            if (col == 0) {
                flags |= Qt::ItemIsEditable;

                // В data(Qt::UserRole) будем хранить «текущее/старое» имя.
                item->setData(Qt::UserRole, robot->name());
            }
            item->setFlags(flags);

            // *** Сохраняем указатель на объект RobotEntry ***
            item->setData(Qt::UserRole + 1, QVariant::fromValue(robot));

            tableWidget->setItem(rowIndex, col, item); // Вставляем ячейки
        }

        ++rowIndex; // Переход к следующей строке
    }

    return rowIndex; // Возвращаем текущий индекс строки
}

RobotEntryPtr ConnectDialog::findRobotByMachineId(const RobotList &robots, const QString &machineId)
{
    auto it = std::find_if(robots.begin(), robots.end(),
                           [&machineId](const std::shared_ptr<RobotEntry> &robot) {
                               return robot && robot->machineID() == machineId;
                           });
    return (it != robots.end()) ? *it : nullptr; // Если найден, возвращаем указатель, иначе nullptr
}


void ConnectDialog::on_connectButton_clicked()
{

}

