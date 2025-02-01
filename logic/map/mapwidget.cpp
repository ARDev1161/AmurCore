#include "mapwidget.h"
#include <QPainter>
#include <QDebug>

MapWidget::MapWidget(QWidget* parent)
    : QWidget(parent) {
}

void MapWidget::setMapData(const std::vector<int8_t>& data, int width, int height) {
    if (width <= 0 || height <= 0 || data.empty()) {
        qWarning() << "Invalid map data.";
        m_mapImage = QImage(); // пустая
        update();
        return;
    }

    // Создаём QImage c 8-битным изображением (один байт на пиксель, GRAY-кодировка)
    // Можно также QImage::Format_RGBA8888, если хочется цвет.
    QImage newImage(width, height, QImage::Format_ARGB32);

    // Проходим по данным occupancy grid
    // Обычно:
    // -1 => неизвестная область (рисуем, например, серым)
    // 0 => свободная область (белый)
    // 100 => занятая область (чёрный)
    // Остальные значения (1..99) – шкала от белого к чёрному
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            int index = y * width + x;
            int8_t val = data[index];

            QColor color;
            if (val < 0) {
                // неизвестная область
                color = QColor(128, 128, 128); // серый
            } else {
                // val от 0 до 100
                // Можно сделать простую линейную интерполяцию
                // 0 => белый (255, 255, 255)
                // 100 => чёрный (0, 0, 0)
                int shade = 255 - static_cast<int>(2.55 * val);
                color = QColor(shade, shade, shade);
            }
            newImage.setPixelColor(x, y, color);
        }
    }

    m_mapImage = newImage;
    update(); // перерисовать виджет
}

void MapWidget::paintEvent(QPaintEvent* /*event*/) {
    QPainter painter(this);

    if (!m_mapImage.isNull()) {
        // Рисуем картинку так, чтобы она вписалась в размер виджета
        // Можно по-разному масштабировать.
        QRect targetRect(0, 0, width(), height());
        QRect sourceRect = m_mapImage.rect();
        painter.drawImage(targetRect, m_mapImage, sourceRect);
    }
}
