#ifndef ROBOTINFODIALOG_H
#define ROBOTINFODIALOG_H

#include <QDialog>
#include <QPlainTextEdit>
#include "network/protobuf/rc_api_v2.pb.h"

namespace Ui {
class RobotInfoDialog;
}

class RobotInfoDialog : public QDialog
{
    Q_OBJECT

public:
    explicit RobotInfoDialog(QWidget *parent = nullptr);
    ~RobotInfoDialog();

    /// @brief Display advertised capabilities without exposing ROS endpoint details.
    void setRobotInfo(const rc::api::v2::RobotInfo &info);

private slots:
    void on_pushButton_clicked();

private:
    Ui::RobotInfoDialog *ui;
    QPlainTextEdit *capabilitiesView = nullptr;
};

#endif // ROBOTINFODIALOG_H
