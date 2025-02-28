#ifndef NAVDIALOG_H
#define NAVDIALOG_H

#include <QDialog>
#include <QTimer>
#include <QVBoxLayout>
#include <QGroupBox>
#include <QCheckBox>
#include <QLabel>
#include <QPushButton>
#include <QListWidget>
#include "mapwidget.h"
#include "network/protobuf/robot.pb.h"

using namespace Robot;

namespace Ui {
class NavDialog;
}

class NavDialog : public QDialog
{
    Q_OBJECT

public:
    explicit NavDialog(std::shared_ptr<Controls> controlsPtr,
                       std::shared_ptr<Sensors> sensorsPtr,
                       std::shared_ptr<map_service::GetMapResponse> mapPtr,
                       std::mutex &grpcMutex,
                       std::mutex &mapMutex,
                       QWidget *parent = nullptr);
    ~NavDialog();

signals:
    void mouseMoved(double x, double y);

private slots:
    /**
     * @brief Слот для обновления отображаемой карты.
     */
    void onMapUpdated();
    void clearWaypoints();
    void followWaypoints();

private:
    Ui::NavDialog *ui;
    QWidget *navContainer; ///< Главный контейнер всего интерфейса
    std::shared_ptr<MapWidget> mapWidget;
    QTimer *timer; ///< Таймер для периодической проверки обновлений

    std::shared_ptr<Controls> controls;
    std::shared_ptr<Sensors> sensors;
    std::shared_ptr<map_service::GetMapResponse> mapPtr;
    std::mutex &grpcMutex_;
    std::mutex &mapMutex_;

    QGroupBox *waypointsGroupBox;
    QGroupBox *commandsGroupBox;
    QListWidget *goalListWidget;

    // Предыдущие данные карты для проверки изменений
    std::vector<int8_t> previousData;
    int previousWidth;
    int previousHeight;

    /**
     * @brief Проверяет, изменились ли данные карты.
     * @return true, если карта изменилась; иначе false.
     */
    bool hasMapChanged() const;
    /**
     * @brief Проверяет, изменились ли данные положения робота.
     * @return true, если положение изменилось; иначе false.
     */
    bool hasPoseChanged() const;

    PoseQuaternion poseToQuaternion(map_service::Pose pose) const;

    std::shared_ptr<std::vector<QPointF>> navigationGoalsList;
};

#endif // NAVDIALOG_H
