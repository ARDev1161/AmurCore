#ifndef MAPWIDGET_H
#define MAPWIDGET_H

#include <QWidget>
#include <QImage>
#include <QPixmap>
#include <QPoint>
#include <QScrollBar>
#include <vector>
#include <mutex>

struct PoseQuaternion {
    double x;
    double y;
    double z;
    double w;
};

static bool operator!=(const PoseQuaternion& lhs, const PoseQuaternion& rhs) {
    return (lhs.x != rhs.x) ||
           (lhs.y != rhs.y) ||
           (lhs.z != rhs.z) ||
           (lhs.w != rhs.w);
}

static bool operator==(const PoseQuaternion& lhs, const PoseQuaternion& rhs) {
    return (lhs.x == rhs.x) &&
           (lhs.y == rhs.y) &&
           (lhs.z == rhs.z) &&
           (lhs.w == rhs.w);
}


class MapWidget : public QWidget {
  Q_OBJECT
public:
  explicit MapWidget(std::shared_ptr<std::vector<QPointF>> navListGoals, QWidget* parent = nullptr);
    ~MapWidget();

    /**
     * @brief Устанавливает данные карты и инициирует перерисовку.
     * @param data Вектор данных карты (OccupancyGrid.data).
     * @param width Ширина карты.
     * @param height Высота карты.
     * @param resolution Разрешение карты. Метр/пиксел
     * @param originX Координата X начала координат.
     * @param originY Координата Y начала координат.
     */
    void setMapData(const std::vector<int8_t>& data,
                    int width, int height,
                    double resolution,
                    double originX, double originY);

    void setRobotLocation(double posX, double posY);
    void setRobotOrientation(PoseQuaternion quaternion);
    void setRobotOrientation(double yaw);

    void setRobotPose(double posX, double posY, PoseQuaternion quaternion);

    // Возвращает список целей (в мировых координатах)
    const std::shared_ptr<std::vector<QPointF> > &getGoals() const;

signals:
    // Можно, например, сигнализировать о добавлении цели
    void goalAdded(const QPointF &goalWorld, int goalIndex);    
    void mouseMoved(double x, double y);

public slots:
    void centerOnRobot();
    void setShowGrid(bool enable);
    void setShowAxis(bool enable);

protected:
    /**
     * @brief Обработчик события перерисовки виджета.
     * @param event Событие перерисовки.
     */
    void paintEvent(QPaintEvent* event) override;

    /**
     * @brief Обработчик события изменения размера виджета.
     * @param event Событие изменения размера.
     */
    void resizeEvent(QResizeEvent* event) override;

    /**
     * @brief Обработчик события прокрутки колесиком мыши.
     * @param event Событие прокрутки колесиком.
     */
    void wheelEvent(QWheelEvent* event) override;

    /**
     * @brief Обработчик события нажатия кнопки мыши.
     * @param event Событие нажатия кнопки мыши.
     */
    void mousePressEvent(QMouseEvent* event) override;

    /**
     * @brief Обработчик события перемещения мыши.
     * @param event Событие перемещения мыши.
     */
    void mouseMoveEvent(QMouseEvent* event) override;

    /**
     * @brief Обработчик события отпускания кнопки мыши.
     * @param event Событие отпускания кнопки мыши.
     */
    void mouseReleaseEvent(QMouseEvent* event) override;

private:    
    /**
     * @brief Обновляет масштаб и сдвиг при изменении размера виджета.
     */
    void updateScaleOnResize(int widgetWidth, int widgetHeight);

    QPointF widgetToWorld(const QPointF &point) const;
    QPointF widgetToMap(const QPointF &p) const;
    QPointF mapToWidget(const QPointF &mapPt) const;
    bool m_showGrid = true;
    bool m_showAxis = true;

    void drawGrid(QPainter &painter);
    void drawAxis(QPainter &painter);
    void drawRobot(QPainter &painter);
    void drawWaypoints(QPainter &painter);

    void fitMapToWidget();

    QImage m_mapImage;      // Изображение карты
    std::mutex m_mutex;     // Мьютекс для защиты данных карты
    bool m_firstMapLoaded = false;  // Флаг, была ли карта уже загружена

    double m_originX;
    double m_originY;
    double m_mapResolution; // Метр/пиксель

    // Переменные для масштабирования и панорамирования
    double m_scaleFactor;   // Текущий масштаб
    double m_minScale;      // Минимальный масштаб
    double m_maxScale;      // Максимальный масштаб
    QPoint m_lastMousePos;  // Последняя позиция мыши для панорамирования
    QPoint m_offset;        // Текущий сдвиг (панорамирование)
    bool m_panning;         // Флаг, указывающий на активное панорамирование

    // Переменные для положения робота
    QPixmap m_robotPixmap; // Изображение робота (треугольник)
    float m_robot_x;
    float m_robot_y;
    double m_robot_theta; // Ориентация в радианах

    // Список целей (world coordinates)
    std::shared_ptr<std::vector<QPointF>> m_goalsWorld;
};

#endif // MAPWIDGET_H
