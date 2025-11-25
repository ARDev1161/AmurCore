#include "navdialog.h"
#include "ui_navdialog.h"

namespace {

Navigation::Pose* fillPose(Navigation::Pose* pose, const QPointF& point)
{
    if (!pose)
        return nullptr;

    pose->set_x(point.x());
    pose->set_y(point.y());
    pose->set_z(0.0);

    pose->set_orientation_w(1.0);
    pose->set_orientation_x(0.0);
    pose->set_orientation_y(0.0);
    pose->set_orientation_z(0.0);

    return pose;
}

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
            fillPose(followWaypointsCmd->add_waypoints(), point);
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
            fillPose(goThroughPosesCmd->add_poses(), point);
        }

        navCmd->set_request_feedback(true);
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

} // namespace

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
    navCmdBuilder(std::make_unique<NavCommandBuilder>(controlsPtr)),
    timer(new QTimer(this)),
    previousWidth(0),
    previousHeight(0)
{
    ui->setupUi(this);

    // 1) Корневой лейаут для всего диалога
    QVBoxLayout *dialogLayout = new QVBoxLayout(this);
    setLayout(dialogLayout);

    // Создаём список целей
    navigationGoalsList = std::make_shared<std::vector<QPointF>>();

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

    QPushButton *sendGoalsButton = new QPushButton("Send goals", this);
    commandsLayout->addWidget(sendGoalsButton);
    connect(sendGoalsButton, &QPushButton::clicked, this, &NavDialog::sendGoals);

    QLabel *statusTask = new QLabel("", this);
    commandsLayout->addWidget(statusTask);
    connect(timer, &QTimer::timeout, this, [=]() {
        QString newData = taskStatusAsString();
        statusTask->setText("Status: " + newData);
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

    // Запускаем таймер для проверки обновлений буферов
    timer->start(42);
}

NavDialog::~NavDialog()
{
    delete ui;
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
}

void NavDialog::sendGoals()
{
    if(isFollowWaypoints)
        followWaypoints();
    else
        goThroughPoses();
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

void NavDialog::onMapUpdated()
{
    if (!mapPtr) {
        navContainer->setEnabled(false);
        return;
    }
    if(!navContainer->isEnabled() && !(mapPtr->map().data().empty()))
        navContainer->setEnabled(true);

    // Проверяем, изменились ли данные карты
    if (hasMapChanged()) {
        // Извлекаем данные карты
        std::vector<int8_t> data(mapPtr->map().data().begin(), mapPtr->map().data().end());
        int width = mapPtr->map().width();
        int height = mapPtr->map().height();

        double resolution = mapPtr->map().resolution();

        double originX = mapPtr->map().origin().position_x();
        double originY = mapPtr->map().origin().position_y();

        // Обновляем данные в MapWidget
        mapWidget->setMapData(data, width, height, resolution, originX, originY);

        // Обновляем предыдущие данные
        previousData = data;
        previousWidth = width;
        previousHeight = height;
    }

    if (hasPoseChanged()) {
        // Обновляем позицию робота в MapWidget

        mapWidget->setRobotPose(mapPtr->robotpose().position_x(),
                                mapPtr->robotpose().position_y(),
                                poseToQuaternion(mapPtr->robotpose()));
    }

    // Обновление списка целей (если MapWidget отправляет сигнал goalAdded, можно подключить этот сигнал)
    // Здесь, например, можно получить список целей из mapWidget и обновить goalListWidget.
    auto goals = mapWidget->getGoals();
    goalListWidget->clear();
    int index = 1;

    if(goals)
        for (auto goal : *goals) {
            goalListWidget->addItem(QString("Goal %1: (%2, %3)").arg(index).arg(goal.x(), 0, 'f', 2).arg(goal.y(), 0, 'f', 2));
            index++;
        }

    mapWidget->update();
}
