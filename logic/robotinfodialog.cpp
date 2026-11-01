#include "robotinfodialog.h"
#include "ui_robotinfodialog.h"
#include <QVBoxLayout>

RobotInfoDialog::RobotInfoDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::RobotInfoDialog)
{
    ui->setupUi(this);
    ui->groupBox_2->setGeometry(20, 105, 350, 130);
    capabilitiesView = new QPlainTextEdit(ui->groupBox_2);
    capabilitiesView->setReadOnly(true);
    auto *layout = new QVBoxLayout(ui->groupBox_2);
    layout->addWidget(capabilitiesView);
    capabilitiesView->setPlainText(tr("No v2 robot descriptor received"));
}

RobotInfoDialog::~RobotInfoDialog()
{
    delete ui;
}

void RobotInfoDialog::setRobotInfo(const rc::api::v2::RobotInfo &info)
{
    if (!capabilitiesView) {
        return;
    }
    if (info.robot_id().empty()) {
        capabilitiesView->setPlainText(tr("No v2 robot descriptor received"));
        return;
    }
    QStringList lines;
    lines << tr("Robot: %1 (%2)")
                 .arg(QString::fromStdString(info.display_name()),
                      QString::fromStdString(info.robot_id()));
    lines << tr("RC API: %1.%2").arg(info.api_major()).arg(info.api_minor());
    for (const auto &capability : info.capabilities()) {
        QString label = QString::fromStdString(capability.id());
        if (capability.velocity_axes_size() > 0) {
            QStringList axes;
            for (const auto &axis : capability.velocity_axes()) {
                axes << QString::fromStdString(axis);
            }
            label += QString(" (%1)").arg(axes.join(", "));
        }
        lines << QString("  - %1").arg(label);
    }
    capabilitiesView->setPlainText(lines.join("\n"));
}

void RobotInfoDialog::on_pushButton_clicked()
{

}

