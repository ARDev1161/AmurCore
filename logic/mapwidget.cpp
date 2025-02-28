#include "mapwidget.h"
#include <QPainter>
#include <QDebug>
#include <QWheelEvent>
#include <QMouseEvent>
#include <algorithm>
#include <QtMath>

MapWidget::MapWidget(std::shared_ptr<std::vector<QPointF>> navListGoals, QWidget* parent)
    : m_goalsWorld(navListGoals),
      QWidget(parent),
      m_scaleFactor(1.0),
      m_minScale(0.1),
      m_maxScale(10.0),
      m_offset(0, 0),
      m_panning(false),
      m_robot_x(0.0f),
      m_robot_y(0.0f),
      m_robot_theta(0.0),
      m_mapResolution(0.05) // Примерная начальная разрешающая способность (метр/пиксель)
{
    setMouseTracking(true);
    setBackgroundRole(QPalette::Base);
    setAutoFillBackground(true);

    // Загрузка изображения робота из ресурсов
    if(!m_robotPixmap.load(":/images/robot.png")) {
        qWarning() << "Couldn't load image of robot. Creating default triangle";
        // Создаём треугольник по умолчанию
        m_robotPixmap = QPixmap(20, 20);
        m_robotPixmap.fill(Qt::transparent);
        QPainter painter(&m_robotPixmap);
        painter.setBrush(Qt::green);
        QPoint points[3] = { QPoint(10, 0), QPoint(0, 20), QPoint(20, 20) };
        painter.drawPolygon(points, 3);
    }
}


MapWidget::~MapWidget()
{
    // Очистка ресурсов, если необходимо
}

void MapWidget::setMapData(const std::vector<int8_t>& data,
                           int width, int height,
                           double resolution,
                           double originX, double originY)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    // Проверка валидности
    if (width <= 0 || height <= 0 || data.empty()) {
        m_mapImage = QImage();
        update();
        return;
    }

    // Сохраняем параметры
    m_mapResolution = resolution;
    m_originX = originX;
    m_originY = originY;

    // Создаём QImage с форматом ARGB32
    QImage newImage(width, height, QImage::Format_ARGB32);
    newImage.fill(Qt::white); // Заполняем белым фоном

    // Обработка данных карты
    for(int y = 0; y < height; ++y) {
        for(int x = 0; x < width; ++x) {
            int index = y * width + x;
            int8_t val = data[index];

            QColor color;
            if(val < 0) {
                // Неизвестная область
                color = QColor(128, 128, 128); // Серый
            }
            else {
                // val от 0 до 100
                // Линейная интерполяция: 0 -> белый, 100 -> чёрный
                int shade = 255 - static_cast<int>(2.55 * val);
                color = QColor(shade, shade, shade);
            }

            newImage.setPixelColor(x, y, color);
        }
    }

    // Отражаем изображение по вертикали, чтобы соответствовать системе координат Qt
    m_mapImage = newImage.mirrored(false, true);

    // --- Вызываем автоподгонку ТОЛЬКО при первом отображении ---
    if (!m_firstMapLoaded) {
        // Вписать карту в размеры виджета один раз
        QSize widgetSize = size();
        updateScaleOnResize(widgetSize.width(), widgetSize.height());
        m_firstMapLoaded = true;
    }

    // Инициируем перерисовку виджета
    update();
}

void MapWidget::fitMapToWidget()
{
    if (m_mapImage.isNull()) {
        return; // Нет карты — нечего вписывать
    }
    QSize widgetSize = size();
    updateScaleOnResize(widgetSize.width(), widgetSize.height());
    update();
}

void MapWidget::setRobotLocation(double posX, double posY) {
    m_robot_x = posX;
    m_robot_y = posY;
}

void MapWidget::setRobotOrientation(PoseQuaternion quaternion) {
    // Конвертация кватерниона в угол поворота (yaw)
    double siny_cosp = 2.0 * (quaternion.w * quaternion.z + quaternion.x * quaternion.y);
    double cosy_cosp = 1.0 - 2.0 * (quaternion.y * quaternion.y + quaternion.z * quaternion.z);
    double yaw = std::atan2(siny_cosp, cosy_cosp);
    m_robot_theta = -yaw + M_PI / 2.0;
}

void MapWidget::setRobotOrientation(double yaw)
{
    m_robot_theta = yaw;
}

void MapWidget::setRobotPose(double posX, double posY, PoseQuaternion quaternion)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    setRobotLocation(posX, posY);
    setRobotOrientation(quaternion);
}

const std::shared_ptr<std::vector<QPointF> > &MapWidget::getGoals() const
{
    return m_goalsWorld;
}

void MapWidget::drawGrid(QPainter &painter)
{
    // Настройка "косметического" пера,
    // чтобы толщина линий оставалась 1 пиксель независимо от scale
    QPen gridPen(Qt::gray, 1.0, Qt::SolidLine);
    gridPen.setCosmetic(true);
    painter.setPen(gridPen);

    // Ширина/высота карты в метрах
    double mapWidthM  = m_mapImage.width()  * m_mapResolution;
    double mapHeightM = m_mapImage.height() * m_mapResolution;

    // Границы мира, которые покрывает карта
    double minX = m_originX;
    double maxX = m_originX + mapWidthM;
    double minY = m_originY;
    double maxY = m_originY + mapHeightM;

    // Целочисленные границы сетки
    int iMin = static_cast<int>(std::floor(minX));
    int iMax = static_cast<int>(std::ceil(maxX));
    int jMin = static_cast<int>(std::floor(minY));
    int jMax = static_cast<int>(std::ceil(maxY));

    // Рисуем вертикальные линии x = i
    for (int i = iMin; i <= iMax; ++i) {
        // Переводим "i метров" → пиксели карты
        double px = (i - m_originX) / m_mapResolution;

        // Пропускаем, если px за границей картинки:
        if (px < 0.0 || px > m_mapImage.width())
            continue;

        // Линия сверху вниз (учитывая, что в QImage(0,0) – верх)
        painter.drawLine(QPointF(px, 0),
                         QPointF(px, m_mapImage.height()));
    }

    // Рисуем горизонтальные линии y = j
    for (int j = jMin; j <= jMax; ++j) {
        // Если карта "перевёрнута" через mirrored(false, true),
        // то для Y обычно используем (height - 1 - (...))
        double py = (m_mapImage.height() - 1)
                    - ((j - m_originY) / m_mapResolution);

        // Пропускаем, если py за границей картинки:
        if (py < 0.0 || py > m_mapImage.height())
            continue;

        painter.drawLine(QPointF(0, py),
                         QPointF(m_mapImage.width(), py));
    }
}

void MapWidget::drawAxis(QPainter &painter)
{
    // Предположим, что m_offset содержит экранную позицию начала координат карты.
    QPointF origin = m_offset;

    // Рисуем ось X (красная линия)
    QPen penX(Qt::red);
    penX.setWidth(2);
    painter.setPen(penX);
    painter.drawLine(origin, origin + QPoint(20, 0));

    // Рисуем ось Y (зелёная линия)
    QPen penY(Qt::green);
    penY.setWidth(2);
    painter.setPen(penY);
    // В экранной системе координат Y растет вниз, поэтому для оси Y вверх отнимаем значение по Y.
    painter.drawLine(origin, origin - QPoint(0, 20));
}

void MapWidget::drawRobot(QPainter &painter)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_mapImage.isNull()) {
        return; // Нет карты — пока не рисуем робота
    }

    // Преобразование положения робота из метров в пиксели с учетом происхождения карты
    double robotPixelX = (m_robot_x - m_originX) / m_mapResolution;
    // Инверсия Y: высота - 1 - (мировая_коорд / разрешение)
    double robotPixelY = m_mapImage.height() - (m_robot_y - m_originY) / m_mapResolution;

    // "Экрание" координаты = offset + scaleFactor * (координаты на карте)
    double screenX = m_offset.x() + m_scaleFactor * robotPixelX;
    double screenY = m_offset.y() + m_scaleFactor * robotPixelY;

    // Определяем размер робота в пикселях
    double robot_size = 20.0;
    QPixmap scaledPixmap = m_robotPixmap.scaled(robot_size, robot_size,
                                                Qt::KeepAspectRatio,
                                                Qt::SmoothTransformation);

    // Создаём трансформацию для робота
    QTransform transform;
    transform.translate(screenX, screenY);
    transform.rotateRadians(m_robot_theta);

    painter.save();
    // Применяем трансформацию
    painter.setTransform(transform, true); // Сохраняем существующие трансформации

    QPointF topLeft(-robot_size/2, -robot_size/2);
    painter.drawPixmap(topLeft, scaledPixmap);

    painter.restore();
}

void MapWidget::drawWaypoints(QPainter &painter)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    // Если нет целей, выходим
    if (!m_goalsWorld || m_goalsWorld->empty()) {
        return;
    }

    // Настройка пера и кисти для маркера
    QPen markerPen(Qt::red);
    markerPen.setWidth(2);  // толщина контура
    painter.setPen(markerPen);

    QBrush markerBrush(Qt::yellow);
    painter.setBrush(markerBrush);

    // Для подписей используем другой цвет пера
    QPen textPen(Qt::black);

    // Радиус маркера в пикселях (на экране)
    int markerRadius = 7;

    // Нумерация целей
    int goalIndex = 1;

    // Проходим по всем целям
    for (const QPointF &goal : *m_goalsWorld)
    {
        // 1) Переводим из мировых координат (ROS) -> пиксели карты
        double mapPixelX = (goal.x() - m_originX) / m_mapResolution;
        double mapPixelY = m_mapImage.height() - (goal.y() - m_originY) / m_mapResolution;

        // 2) Переводим пиксели карты -> экран
        double screenX = m_offset.x() + m_scaleFactor * mapPixelX;
        double screenY = m_offset.y() + m_scaleFactor * mapPixelY;
        QPointF screenPoint(screenX, screenY);

        // 3) Рисуем маркер (кружок) постоянного радиуса
        painter.drawEllipse(screenPoint, markerRadius, markerRadius);

        // 4) Подписываем индекс (в центре или чуть смещён)
        painter.setPen(textPen);

        // Сместим текст, чтобы он был виден:
        double shiftX = -markerRadius / 2.0;
        double shiftY = markerRadius / 2.0;
        // Если двузначные индексы – можно сместить сильнее.

        painter.drawText(screenPoint + QPointF(shiftX, shiftY),
                         QString::number(goalIndex));

        // Восстанавливаем перо для контура кружка (если оно сбилось)
        painter.setPen(markerPen);

        ++goalIndex;
    }
}

void MapWidget::paintEvent(QPaintEvent* /*event*/)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::SmoothPixmapTransform, true);

    if(!m_mapImage.isNull()) {
        painter.save();
        // Применяем масштабирование и сдвиг
        painter.translate(m_offset);
        painter.scale(m_scaleFactor, m_scaleFactor);
        // 1) Рисуем карту
        painter.drawImage(0, 0, m_mapImage);

        // 2) Рисуем сетку поверх карты
        if (m_showGrid) {
            drawGrid(painter);
        }

        painter.restore(); // scale=1

        // 3) Рисуем оси поверх карты
        if (m_showAxis) {
            drawAxis(painter);
        }

        // 4) Рисуем робота фиксированного размера (не масштабируя)
        drawRobot(painter);

        // 5) Отрисовываем цели (не масштабируемые)
        drawWaypoints(painter);
    }
}

void MapWidget::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);

    QSize widgetSize = event->size();
    updateScaleOnResize(widgetSize.width(), widgetSize.height());
    update();
}

QPointF MapWidget::widgetToWorld(const QPointF &point) const {
    // Переводим координаты виджета в координаты карты (пиксели)
    QPointF pixel = (point - m_offset) / m_scaleFactor;
    // Переводим пиксельные координаты в мировые (в метрах)
    double world_x = pixel.x() * m_mapResolution + m_originX;
    double world_y = ( (m_mapImage.height() - 1) - pixel.y() )
            * m_mapResolution + m_originY;

    return QPointF(world_x, world_y);
}

QPointF MapWidget::widgetToMap(const QPointF &p) const
{
    // Перевод из координат виджета в координаты карты (пиксели)
    return (p - m_offset) / m_scaleFactor;
}

QPointF MapWidget::mapToWidget(const QPointF &mapPt) const
{
    // Обратное преобразование: из пикселей карты в координаты виджета
    return mapPt * m_scaleFactor + m_offset;
}

void MapWidget::centerOnRobot()
{
    if (m_mapImage.isNull())
        return;

    // 1) Робот в координатах карты (пиксели) до масштабирования
    double robotMapX = (m_robot_x - m_originX) / m_mapResolution;
    double robotMapY = (m_mapImage.height() - 1)
                       - (m_robot_y - m_originY) / m_mapResolution;

    // 2) Центр виджета
    QPointF widgetCenter(width()/2.0, height()/2.0);

    // 3) При рисовании мы делаем:
    //    screenXY = m_offset + ( mapXY * m_scaleFactor )
    // Хотим, чтобы screenXY = widgetCenter
    // => m_offset = widgetCenter - mapXY * m_scaleFactor

    QPointF desiredOffset = widgetCenter
                            - QPointF(robotMapX * m_scaleFactor,
                                      robotMapY * m_scaleFactor);
    m_offset = desiredOffset.toPoint();

    update();
}

void MapWidget::setShowGrid(bool enable)
{
    m_showGrid = enable;
    update();
}

void MapWidget::setShowAxis(bool enable)
{
    m_showAxis = enable;
    update();
}

void MapWidget::wheelEvent(QWheelEvent* event)
{
    if(m_mapImage.isNull()) {
        event->ignore();
        return;
    }

    QPointF mousePos = event->position();

    // 2) Переводим её в "координаты карты" (до изменения масштаба)
    QPointF beforeScaleMapPoint = widgetToMap(mousePos);

    // Определяем направление прокрутки
    QPoint numDegrees = event->angleDelta() / 8;
    if(numDegrees.y() == 0) {
        event->ignore();
        return;
    }

    double factor = 1.15;
    if(numDegrees.y() > 0) {
        // Zoom in
        m_scaleFactor *= factor;
    }
    else {
        // Zoom out
        m_scaleFactor /= factor;
    }

    // Ограничиваем масштабирование
    m_scaleFactor = std::clamp(m_scaleFactor, m_minScale, m_maxScale);
    QPointF afterScaleScreenPoint = mapToWidget(beforeScaleMapPoint);
    QPointF delta = mousePos - afterScaleScreenPoint;
    m_offset += delta.toPoint();

    update();
}

void MapWidget::mousePressEvent(QMouseEvent* event)
{
    if(!m_mapImage.isNull()) {
        // Панорамирование
        if(event->button() == Qt::LeftButton) {
            m_panning = true;
            m_lastMousePos = event->pos();
            setCursor(Qt::ClosedHandCursor);
        }

        // Если нажата правая кнопка — добавляем цель
        if(event->button() == Qt::RightButton){
            QPointF worldPoint = widgetToWorld(event->pos());
            {
                std::lock_guard<std::mutex> lock(m_mutex);
                m_goalsWorld->push_back(worldPoint);
                int goalIndex = static_cast<int>(m_goalsWorld->size());
                qDebug() << "Добавлена цель" << goalIndex << "в" << worldPoint;
                emit goalAdded(worldPoint, goalIndex);
            }
            update();
            // Если же это начало панорамирования, можно обрабатывать отдельно
            // Но если вы хотите отличать клик от панорамирования, возможно, стоит добавить небольшой таймер или порог перемещения.
        }
    }
}

void MapWidget::mouseMoveEvent(QMouseEvent* event)
{
    // панорамирование при зажатой левой кнопке
    if(m_panning) {
        QPoint delta = event->pos() - m_lastMousePos;
        m_lastMousePos = event->pos();
        m_offset += delta;
        update();
    }

    // Получаем координаты в системе ROS:
    QPointF worldPt = widgetToWorld(event->pos());
    double rosX = worldPt.x();
    double rosY = worldPt.y();

    emit mouseMoved(rosX, rosY); // Посылаем сигнал
}

void MapWidget::mouseReleaseEvent(QMouseEvent* event)
{
    if(event->button() == Qt::LeftButton && m_panning) {
        m_panning = false;
        setCursor(Qt::ArrowCursor);
    }
}

void MapWidget::updateScaleOnResize(int widgetWidth, int widgetHeight)
{
    if(m_mapImage.isNull()) {
        return;
    }

    double scaleX = static_cast<double>(widgetWidth) / m_mapImage.width();
    double scaleY = static_cast<double>(widgetHeight) / m_mapImage.height();

    // Выбираем минимальный масштаб, чтобы вся карта помещалась
    m_scaleFactor = std::min(scaleX, scaleY);

    // Ограничиваем масштабирование
    m_scaleFactor = std::clamp(m_scaleFactor, m_minScale, m_maxScale);

    // Центрируем карту
    double scaledWidth = m_mapImage.width() * m_scaleFactor;
    double scaledHeight = m_mapImage.height() * m_scaleFactor;

    m_offset = QPoint(static_cast<int>((widgetWidth - scaledWidth) / 2),
                      static_cast<int>((widgetHeight - scaledHeight) / 2));
}
