#ifndef ROBOTINFODIALOG_H
#define ROBOTINFODIALOG_H

#include <QDialog>

namespace Ui {
class RobotInfoDialog;
}

class RobotInfoDialog : public QDialog
{
    Q_OBJECT

public:
    explicit RobotInfoDialog(QWidget *parent = nullptr);
    ~RobotInfoDialog();

private slots:
    void on_pushButton_clicked();

private:
    Ui::RobotInfoDialog *ui;
};

#endif // ROBOTINFODIALOG_H
