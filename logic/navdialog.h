#ifndef NAVDIALOG_H
#define NAVDIALOG_H

#include <QDialog>
#include <QTimer>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QLabel>
#include <QCheckBox>
#include <QRadioButton>
#include <QButtonGroup>
#include <QPushButton>
#include <QListWidget>
#include <memory>
#include <vector>
#include "mapwidget.h"
#include "network/protobuf/robot.pb.h"

using namespace Robot;

namespace Ui {
    class NavDialog;
}

class NavCommandBuilder;
class NavGoalsHistory;

class NavDialog : public QDialog
{
    Q_OBJECT

    enum class NavTaskState
    {
        Idle,
        Ready,
        Busy
    };

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
    void clearGoals();
    void sendGoals();
    void undoGoals();
    void redoGoals();

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
    std::unique_ptr<NavCommandBuilder> navCmdBuilder;
    std::unique_ptr<NavGoalsHistory> navGoalsHistory;

    bool isFollowWaypoints {true};
    QGroupBox *waypointsGroupBox;
    QGroupBox *commandsGroupBox;
    QListWidget *goalListWidget;
    QPushButton *sendGoalsButton {nullptr};
    QLabel *statusTaskLabel {nullptr};

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

    void followWaypoints();
    void goThroughPoses();
    void refreshGoalList();
    void recordGoalsSnapshot();
    void updateStateFromGoals();
    void setTaskState(NavTaskState state);

    std::shared_ptr<std::vector<QPointF>> navigationGoalsList;
    QString taskStatusAsString();
    NavTaskState navTaskState {NavTaskState::Idle};
};

#endif // NAVDIALOG_H
