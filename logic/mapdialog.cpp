#include "mapdialog.h"
#include "ui_mapdialog.h"

MapDialog::MapDialog(std::shared_ptr<map_service::GetMapResponse> mapPtr,
                     QWidget *parent) :
    QDialog(parent),
    ui(new Ui::MapDialog),
    mapPtr(mapPtr),
    mapWidget(new MapWidget(this)),
    timer(new QTimer(this)),
    previousWidth(0),
    previousHeight(0)
{
    ui->setupUi(this);

    // Добавляем MapWidget в макет диалога
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->addWidget(mapWidget);
    setLayout(layout);

    // Инициализируем предыдущие данные как пустые
    previousData.clear();
    previousWidth = 0;
    previousHeight = 0;

    // Подключаем сигнал таймера к слоту обновления карты
    connect(timer, &QTimer::timeout, this, &MapDialog::onMapUpdated);

    // Запускаем таймер с интервалом 1000 мс (1 секунда)
    timer->start(42);
}

MapDialog::~MapDialog()
{
    delete ui;
}

bool MapDialog::hasMapChanged() const
{
    if (!mapPtr) {
        return false;
    }

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

bool MapDialog::hasPoseChanged() const
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

PoseQuaternion MapDialog::poseToQuaternion(map_service::Pose pose) const
{
    // Извлекаем данные позы робота
    return {pose.orientation_x(),
            pose.orientation_y(),
            pose.orientation_z(),
            pose.orientation_w()};

}

void MapDialog::onMapUpdated()
{
    if (!mapPtr) {
        return;
    }

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

    mapWidget->update();
}
