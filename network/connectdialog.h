#ifndef CONNECTDIALOG_H
#define CONNECTDIALOG_H

#include <QTimer>
#include <QDialog>
#include <QTableWidget>
#include <QMessageBox>
#include <QMetaType>
#include "networkcontroller.h"
#include "robotrepository.h"

namespace Ui {
class ConnectDialog;
}

Q_DECLARE_METATYPE(RobotEntryPtr)

class ConnectDialog : public QDialog
{
    Q_OBJECT

    std::shared_ptr<NetworkController> networkController;
    std::shared_ptr<RobotRepository> repo;
    RobotList knownRobots;

    bool categorizeRobots(
        const RobotList &knownRobots,  // Список известных роботов из БД
        const RobotList &activeRobots, // Список активных роботов в сети
        RobotList &activeUnknown,     // Результат: активные неизвестные
        RobotList &knownInactive,     // Результат: известные, но не активные
        RobotList &activeKnown        // Результат: активные известные
        );
    void setupTableHeaders(QTableWidget *tableWidget);
    int fillTableWithRobots(QTableWidget *tableWidget, const RobotList &robots, const QColor &backgroundColor, const QColor &textColor, int startRow);
    RobotEntryPtr findRobotByMachineId(const RobotList &robots, const QString &machineId);
public:
    explicit ConnectDialog(QWidget *parent, std::shared_ptr<NetworkController> networkController, std::shared_ptr<RobotRepository> repo);
    ~ConnectDialog();

private slots:
    void updateTable();
    void onRobotNameChanged(QTableWidgetItem *item);
    void onTableRowDoubleClicked(int row, int column);
    void on_buttonBox_accepted();

    void on_connectButton_clicked();

private:
    Ui::ConnectDialog *ui;
    QTimer *updateTimer; // Таймер для периодического вызова updateTable
};

#endif // CONNECTDIALOG_H
