#ifndef NAVDIALOG_H
#define NAVDIALOG_H

#include <QButtonGroup>
#include <QCheckBox>
#include <QDialog>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QPushButton>
#include <QRadioButton>
#include <QElapsedTimer>
#include <QShowEvent>
#include <QHideEvent>
#include <QTimer>
#include <QVBoxLayout>
#include <cstdint>
#include "mapwidget.h"
#include "network/networkcontroller.h"
#include "network/protobuf/robot.pb.h"
#include <memory>
#include <vector>

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

protected:
    void showEvent(QShowEvent *event) override;
    void hideEvent(QHideEvent *event) override;

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
    std::shared_ptr<NetworkController> networkController;

    bool isFollowWaypoints {true};
    bool isPatrolMode {false};
    QGroupBox *waypointsGroupBox;
    QGroupBox *commandsGroupBox;
    QListWidget *goalListWidget;
    QCheckBox *patrolCheckBox {nullptr};
    QPushButton *sendGoalsButton {nullptr};
    QLabel *statusTaskLabel {nullptr};
    Navigation::CommandStatus lastNavStatus {Navigation::CommandStatus::SUCCESS};
    bool awaitingNavAck {false};

    // Предыдущие данные карты для проверки изменений
    std::vector<int8_t> previousData;
    int previousWidth;
    int previousHeight;
    std::uint64_t previousZoneSignature {0};
    QElapsedTimer lastUpdateTimer;

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
    void stopNavigation();
    void refreshGoalList();
    void recordGoalsSnapshot();
    void updateStateFromGoals();
    void updateStateFromStatus();
    void setTaskState(NavTaskState state);
    void startUpdates();
    void stopUpdates();

    std::shared_ptr<std::vector<QPointF>> navigationGoalsList;
    QString taskStatusAsString();
    NavTaskState navTaskState {NavTaskState::Idle};
};

#endif // NAVDIALOG_H
