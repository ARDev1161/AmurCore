#include "navdialog.h"
#include "ui_navdialog.h"
#include "navigation_adapter.h"
#include <vector>
#include <mutex>

class NavCommandBuilder
{
public:
    explicit NavCommandBuilder(std::shared_ptr<Controls> controls) : controls_(std::move(controls)) {}

    void buildFollowWaypoints(const std::vector<QPointF> &goals) const
    {
        auto* navCmd = resetNavCommand();
        if(!navCmd)
            return;

        Navigation::FollowWaypoints* followWaypointsCmd = navCmd->mutable_follow_waypoints();
        auto* waypoints = followWaypointsCmd->mutable_waypoints();
        waypoints->Clear();
        waypoints->Reserve(static_cast<int>(goals.size()));

        for (const QPointF &point : goals) {
            NavigationAdapter::fromPoint(point, followWaypointsCmd->add_waypoints());
        }

        navCmd->set_request_feedback(true);
    }

    void buildGoThroughPoses(const std::vector<QPointF> &goals) const
    {
        auto* navCmd = resetNavCommand();
        if(!navCmd)
            return;

        Navigation::GoThroughPoses* goThroughPosesCmd = navCmd->mutable_go_through_poses();
        auto* poses = goThroughPosesCmd->mutable_poses();
        poses->Clear();
        poses->Reserve(static_cast<int>(goals.size()));

        for (const QPointF &point : goals) {
            NavigationAdapter::fromPoint(point, goThroughPosesCmd->add_poses());
        }

        navCmd->set_request_feedback(true);
    }

    void buildCancelTask() const
    {
        auto* navCmd = resetNavCommand();
        if(!navCmd)
            return;

        navCmd->mutable_cancel_task();
        navCmd->set_request_feedback(false);
    }

private:
    Navigation::NavCommandRequest* resetNavCommand() const
    {
        if(!controls_)
            return nullptr;

        auto* navCmd = controls_->mutable_navcontrol();
        navCmd->Clear();
        return navCmd;
    }

    std::shared_ptr<Controls> controls_;
};

class NavGoalsHistory
{
public:
    explicit NavGoalsHistory(std::shared_ptr<std::vector<QPointF>> goals)
        : goals_(std::move(goals))
    {
        if(goals_)
            history_.push_back(*goals_);
    }

    void record()
    {
        if(!goals_)
            return;

        if(currentIndex_ + 1 < history_.size())
            history_.erase(history_.begin() + static_cast<long>(currentIndex_ + 1), history_.end());

        history_.push_back(*goals_);
        currentIndex_ = history_.size() - 1;
    }

    bool canUndo() const { return currentIndex_ > 0; }
    bool canRedo() const { return currentIndex_ + 1 < history_.size(); }

    bool undo()
    {
        if(!canUndo())
            return false;
        --currentIndex_;
        restore();
        return true;
    }

    bool redo()
    {
        if(!canRedo())
            return false;
        ++currentIndex_;
        restore();
        return true;
    }

private:
    void restore()
    {
        if(goals_ && currentIndex_ < history_.size())
            *goals_ = history_[currentIndex_];
    }

    std::shared_ptr<std::vector<QPointF>> goals_;
    std::vector<std::vector<QPointF>> history_;
    std::size_t currentIndex_ {0};
};

NavDialog::NavDialog(std::shared_ptr<Controls> controlsPtr,
                     std::shared_ptr<Sensors> sensorsPtr,
                     std::shared_ptr<map_service::GetMapResponse> mapPtr,
                     std::mutex &grpcMutex,
                     std::mutex &mapMutex,
                     QWidget *parent) :
    QDialog(parent),
    ui(new Ui::NavDialog),
    controls(controlsPtr),
    sensors(sensorsPtr),
    mapPtr(mapPtr),
    mapMutex_(mapMutex),
    grpcMutex_(grpcMutex),
    networkController(std::make_shared<NetworkController>(controlsPtr, sensorsPtr, mapPtr)),
    navCmdBuilder(std::make_unique<NavCommandBuilder>(controlsPtr)),
    timer(new QTimer(this)),
    previousWidth(0),
    previousHeight(0)
{
    ui->setupUi(this);

    // 1) Корневой лейаут для всего диалога
    QVBoxLayout *dialogLayout = new QVBoxLayout(this);
    setLayout(dialogLayout);

    // Создаём список целей и историю изменений
    navigationGoalsList = std::make_shared<std::vector<QPointF>>();
    navGoalsHistory = std::make_unique<NavGoalsHistory>(navigationGoalsList);

    // 2) Создаём контейнер, который можно скрывать/задизейблить.
    navContainer = new QWidget(this);
    navContainer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    navContainer->setEnabled(false);

    // 3) Добавляем navContainer в диалоговый лейаут
    // Он занимает всё доступное пространство.
    dialogLayout->addWidget(navContainer, /*stretch=*/1);

    // Создаём горизонтальный макет для карты и списка целей
    QHBoxLayout *hLayout = new QHBoxLayout(navContainer);

    QVBoxLayout *mapLayout = new QVBoxLayout;
    mapWidget = std::make_shared<MapWidget>(navigationGoalsList, this);
    mapLayout->addWidget(mapWidget.get());

    QHBoxLayout *hLayoutMapControl = new QHBoxLayout;
    QLabel *labelCoords = new QLabel("X=..., Y=...");
    hLayoutMapControl->addWidget(labelCoords);

    QCheckBox *drawGridCheckBox = new QCheckBox("Draw grid");
    drawGridCheckBox->setChecked(true);
    hLayoutMapControl->addWidget(drawGridCheckBox);

    QCheckBox *drawAxisCheckBox = new QCheckBox("Draw axis");
    drawAxisCheckBox->setChecked(true);
    hLayoutMapControl->addWidget(drawAxisCheckBox);

    QPushButton *buttonCenter = new QPushButton("Go to robot");
    hLayoutMapControl->addWidget(buttonCenter);

    mapLayout->addLayout(hLayoutMapControl);

    connect(buttonCenter, &QPushButton::clicked,
                mapWidget.get(), &MapWidget::centerOnRobot);

    connect(drawAxisCheckBox, &QCheckBox::toggled,
            mapWidget.get(), &MapWidget::setShowAxis);

    connect(drawGridCheckBox, &QCheckBox::toggled,
            mapWidget.get(), &MapWidget::setShowGrid);

    connect(mapWidget.get(), &MapWidget::goalAdded,
            this, [this](const QPointF &, int){
                recordGoalsSnapshot();
                refreshGoalList();
                updateStateFromGoals();
            });

    connect(mapWidget.get(), &MapWidget::mouseMoved,
                this, [labelCoords](double x, double y){
                    // Показываем, что это координаты ROS
                    QString text = QString("X=%1, Y=%2")
                            .arg(x, 0, 'f', 3)
                            .arg(y, 0, 'f', 3);
                    labelCoords->setText(text);
                });
    hLayout->addLayout(mapLayout, 3);  // карта занимает 3/4 пространства

    goalListWidget = new QListWidget(this);

    waypointsGroupBox = new QGroupBox(tr("Waypoints list"), this);
    // Создаем вертикальный layout для group box
    QVBoxLayout *groupLayout = new QVBoxLayout;
    groupLayout->addWidget(goalListWidget);
    waypointsGroupBox->setLayout(groupLayout);

    QPushButton *clearWaypoints = new QPushButton("Clear waypoints", this);
    groupLayout->addWidget(clearWaypoints);
    connect(clearWaypoints, &QPushButton::clicked, this, &NavDialog::clearGoals);

    QHBoxLayout *historyLayout = new QHBoxLayout;
    QPushButton *undoGoalsButton = new QPushButton("Undo", this);
    QPushButton *redoGoalsButton = new QPushButton("Redo", this);
    historyLayout->addWidget(undoGoalsButton);
    historyLayout->addWidget(redoGoalsButton);
    groupLayout->addLayout(historyLayout);
    connect(undoGoalsButton, &QPushButton::clicked, this, &NavDialog::undoGoals);
    connect(redoGoalsButton, &QPushButton::clicked, this, &NavDialog::redoGoals);


    commandsGroupBox = new QGroupBox(tr("Commands"), this);
    QVBoxLayout *commandsLayout = new QVBoxLayout;

    // Добавляем 2 radiobutton для переключения isFollowWaypoints
    QRadioButton *radioFollowWaypoints = new QRadioButton(tr("Follow Waypoints"), this);
    QRadioButton *radioGoThroughPoses = new QRadioButton(tr("Go Through Poses"), this);
    // Объединяем в группу, чтобы они были взаимоисключающими
    QButtonGroup *followWaypointsGroup = new QButtonGroup(this);
    followWaypointsGroup->addButton(radioFollowWaypoints);
    followWaypointsGroup->addButton(radioGoThroughPoses);
    // Устанавливаем значение по умолчанию (при необходимости)
    radioFollowWaypoints->setChecked(true);

    // Добавляем кнопки в layout
    commandsLayout->addWidget(radioFollowWaypoints);
    commandsLayout->addWidget(radioGoThroughPoses);

    // Подключаем переключение флага isFollowWaypoints
    connect(radioFollowWaypoints, &QRadioButton::toggled,
            this, [this](bool checked) {
                if (checked) {
                    isFollowWaypoints = true;
                }
            });
    connect(radioGoThroughPoses, &QRadioButton::toggled,
            this, [this](bool checked) {
                if (checked) {
                    isFollowWaypoints = false;
                }
            });

    // Patrol mode
    QCheckBox *isPatrolCheckBox = new QCheckBox("Patrol");  ///< Сlosed route of waypoints
    commandsLayout->addWidget(isPatrolCheckBox);

    sendGoalsButton = new QPushButton("Send goals", this);
    commandsLayout->addWidget(sendGoalsButton);
    connect(sendGoalsButton, &QPushButton::clicked, this, &NavDialog::sendGoals);

    statusTaskLabel = new QLabel("", this);
    commandsLayout->addWidget(statusTaskLabel);
    connect(timer, &QTimer::timeout, this, [=]() {
        QString newData = taskStatusAsString();
        statusTaskLabel->setText("Status: " + newData);
        updateStateFromStatus();
    });

    commandsGroupBox->setLayout(commandsLayout);

    // Создаем вертикальный макет для правой части, в котором разместим оба group box
    QVBoxLayout *rightLayout = new QVBoxLayout;
    rightLayout->addWidget(waypointsGroupBox);
    rightLayout->addWidget(commandsGroupBox);

    // Чтобы добавить vertical layout в горизонтальный, создаем контейнерный виджет
    QWidget *rightWidget = new QWidget(this);
    rightWidget->setLayout(rightLayout);

    hLayout->addWidget(rightWidget, 1);  // список целей занимает 1/4

    // Инициализируем предыдущие данные как пустые
    previousData.clear();
    previousWidth = 0;
    previousHeight = 0;

    // Подключаем сигнал таймера к слоту обновления карты
    connect(timer, &QTimer::timeout, this, &NavDialog::onMapUpdated);

    // Реагируем на обновления от сети
    connect(networkController.get(), &NetworkController::mapUpdated,
            this, &NavDialog::onMapUpdated);

    // Запускаем таймер для проверки обновлений буферов
    startUpdates();
    lastUpdateTimer.start();

    refreshGoalList();
    updateStateFromGoals();
}

NavDialog::~NavDialog()
{
    delete ui;
}

void NavDialog::showEvent(QShowEvent *event)
{
    QDialog::showEvent(event);
    startUpdates();
}

void NavDialog::hideEvent(QHideEvent *event)
{
    QDialog::hideEvent(event);
    stopUpdates();
}

bool NavDialog::hasMapChanged() const
{
    if (!mapPtr)
        return false;

    // Проверяем изменения в размерах карты
    if (mapPtr->map().width() != previousWidth ||
        mapPtr->map().height() != previousHeight) {
        return true;
    }

    // Проверяем изменения в данных карты
    const auto& currentData = mapPtr->map().data();
    if (currentData.size() != previousData.size()) {
        return true;
    }

    // Сравниваем данные карты
    return !std::equal(currentData.begin(), currentData.end(), previousData.begin());
}

bool NavDialog::hasPoseChanged() const
{
    if (!mapPtr)
        return false;

    bool result = false;

    // Первичная инициализация. +1 для того чтобы данные отличались при первом запуске
    static int prevPoseX = mapPtr->robotpose().position_x() + 1;
    static int prevPoseY = mapPtr->robotpose().position_y() + 1;
    static PoseQuaternion prevQ = {0,0,0,0};

    if(prevPoseX != mapPtr->robotpose().position_x() ||
       prevPoseY != mapPtr->robotpose().position_y() ||
       prevQ != poseToQuaternion(mapPtr->robotpose()))
        result = true;

    prevPoseX = mapPtr->robotpose().position_x();
    prevPoseY = mapPtr->robotpose().position_y();
    prevQ = poseToQuaternion(mapPtr->robotpose());

    return result;
}

PoseQuaternion NavDialog::poseToQuaternion(map_service::Pose pose) const
{
    // Извлекаем данные позы робота
    return {pose.orientation_x(),
            pose.orientation_y(),
            pose.orientation_z(),
            pose.orientation_w()};

}

void NavDialog::clearGoals()
{
    navigationGoalsList->clear();
    recordGoalsSnapshot();
    refreshGoalList();
    mapWidget->update();
    updateStateFromGoals();
}

void NavDialog::sendGoals()
{
    if(navTaskState == NavTaskState::Busy) {
        stopNavigation();
    } else {
        if(isFollowWaypoints)
            followWaypoints();
        else
            goThroughPoses();
        setTaskState(NavTaskState::Busy);
    }
}

void NavDialog::undoGoals()
{
    if(navGoalsHistory && navGoalsHistory->undo()) {
        refreshGoalList();
        mapWidget->update();
        updateStateFromGoals();
    }
}

void NavDialog::redoGoals()
{
    if(navGoalsHistory && navGoalsHistory->redo()) {
        refreshGoalList();
        mapWidget->update();
        updateStateFromGoals();
    }
}

void NavDialog::followWaypoints()
{
    if(navCmdBuilder)
        navCmdBuilder->buildFollowWaypoints(*navigationGoalsList);
}

void NavDialog::goThroughPoses()
{
    if(navCmdBuilder)
        navCmdBuilder->buildGoThroughPoses(*navigationGoalsList);
}

void NavDialog::stopNavigation()
{
    if(navCmdBuilder)
        navCmdBuilder->buildCancelTask();
}

QString NavDialog::taskStatusAsString()
{
    int statusValue = sensors->mutable_navcontrolstatus()->status();
    const google::protobuf::EnumDescriptor* descriptor = Navigation::CommandStatus_descriptor();
    if (descriptor) {
        const google::protobuf::EnumValueDescriptor* enumValue = descriptor->FindValueByNumber(statusValue);
        if (enumValue)
        {
            absl::string_view nameView = enumValue->name();
            return QString::fromLatin1(nameView.data(), static_cast<int>(nameView.size()));
        }
    }
    return QString("Unknown status");
}

void NavDialog::recordGoalsSnapshot()
{
    if(navGoalsHistory)
        navGoalsHistory->record();
}

void NavDialog::refreshGoalList()
{
    if(!goalListWidget)
        return;

    goalListWidget->clear();

    if(!navigationGoalsList)
        return;

    int index = 1;
    for (const QPointF &point : *navigationGoalsList) {
        goalListWidget->addItem(QString("%1: X=%2 Y=%3")
                                .arg(index++)
                                .arg(point.x(), 0, 'f', 3)
                                .arg(point.y(), 0, 'f', 3));
    }
}

void NavDialog::updateStateFromGoals()
{
    if(navTaskState == NavTaskState::Busy)
        return;

    if(!navigationGoalsList || navigationGoalsList->empty()) {
        setTaskState(NavTaskState::Idle);
    } else {
        setTaskState(NavTaskState::Ready);
    }
}

void NavDialog::updateStateFromStatus()
{
    if(!sensors)
        return;

    const auto status = static_cast<Navigation::CommandStatus>(sensors->navcontrolstatus().status());
    switch (status) {
    case Navigation::CommandStatus::IN_PROGRESS:
        setTaskState(NavTaskState::Busy);
        break;
    case Navigation::CommandStatus::SUCCESS:
    case Navigation::CommandStatus::FAILURE:
    case Navigation::CommandStatus::CANCELED:
        if(navigationGoalsList && !navigationGoalsList->empty())
            setTaskState(NavTaskState::Ready);
        else
            setTaskState(NavTaskState::Idle);
        break;
    default:
        updateStateFromGoals();
        break;
    }
}

void NavDialog::setTaskState(NavTaskState state)
{
    navTaskState = state;
    if(sendGoalsButton) {
        switch (state) {
        case NavTaskState::Busy:
            sendGoalsButton->setText(tr("Stop"));
            sendGoalsButton->setEnabled(true);
            break;
        case NavTaskState::Ready:
            sendGoalsButton->setText(tr("Send goals"));
            sendGoalsButton->setEnabled(true);
            break;
        case NavTaskState::Idle:
        default:
            sendGoalsButton->setText(tr("Send goals"));
            sendGoalsButton->setEnabled(false);
            break;
        }
    }
}

void NavDialog::onMapUpdated()
{
    if (!isVisible()) {
        return;
    }
    if (lastUpdateTimer.isValid() && lastUpdateTimer.elapsed() < 100) {
        return;
    }
    lastUpdateTimer.restart();

    std::vector<int8_t> dataCopy;
    int width = 0;
    int height = 0;
    double resolution = 0.0;
    double originX = 0.0;
    double originY = 0.0;
    double robotX = 0.0;
    double robotY = 0.0;
    PoseQuaternion robotQuat {};
    bool mapChanged = false;
    bool poseChanged = false;

    {
        std::scoped_lock lock(mapMutex_, grpcMutex_);

        if (!mapPtr) {
            navContainer->setEnabled(false);
            return;
        }
        if(!navContainer->isEnabled() && !(mapPtr->map().data().empty()))
            navContainer->setEnabled(true);

        mapChanged = hasMapChanged();
        poseChanged = hasPoseChanged();
        if (!mapChanged && !poseChanged) {
            return;
        }

        if (mapChanged) {
            dataCopy.assign(mapPtr->map().data().begin(), mapPtr->map().data().end());
            width = mapPtr->map().width();
            height = mapPtr->map().height();

            resolution = mapPtr->map().resolution();

            originX = mapPtr->map().origin().position_x();
            originY = mapPtr->map().origin().position_y();

            // Обновляем предыдущие данные
            previousData = dataCopy;
            previousWidth = width;
            previousHeight = height;
        }

        if (poseChanged) {
            robotX = mapPtr->robotpose().position_x();
            robotY = mapPtr->robotpose().position_y();
            robotQuat = poseToQuaternion(mapPtr->robotpose());
        }
    }

    if (mapChanged) {
        // Обновляем данные в MapWidget
        mapWidget->setMapData(dataCopy, width, height, resolution, originX, originY);
    }

    if (poseChanged) {
        // Обновляем позицию робота в MapWidget
        mapWidget->setRobotPose(robotX, robotY, robotQuat);
    }

    mapWidget->update();
}

void NavDialog::startUpdates()
{
    if(timer && !timer->isActive())
        timer->start(42);
}

void NavDialog::stopUpdates()
{
    if(timer && timer->isActive())
        timer->stop();
}
