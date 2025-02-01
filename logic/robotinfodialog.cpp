#include "robotinfodialog.h"
#include "ui_robotinfodialog.h"

RobotInfoDialog::RobotInfoDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::RobotInfoDialog)
{
    ui->setupUi(this);
}

RobotInfoDialog::~RobotInfoDialog()
{
    delete ui;
}

void RobotInfoDialog::on_pushButton_clicked()
{

}

