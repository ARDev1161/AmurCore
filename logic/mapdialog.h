#ifndef MAPDIALOG_H
#define MAPDIALOG_H

#include <QDialog>
#include <QTimer>
#include <QVBoxLayout>
#include "mapwidget.h"
#include "network/protobuf/map.grpc.pb.h"

namespace Ui {
class MapDialog;
}

class MapDialog : public QDialog
{
    Q_OBJECT

public:
    explicit MapDialog(std::shared_ptr<map_service::GetMapResponse> mapPtr,
                       QWidget *parent = nullptr);
    ~MapDialog();

private slots:
    /**
     * @brief Слот для обновления отображаемой карты.
     */
    void onMapUpdated();

private:
    Ui::MapDialog *ui;
    MapWidget *mapWidget;
    QTimer *timer; ///< Таймер для периодической проверки обновлений

    std::shared_ptr<map_service::GetMapResponse> mapPtr;

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
};

#endif // MAPDIALOG_H
