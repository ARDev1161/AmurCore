// mapwidget.h
#pragma once

#include <QWidget>
#include <QImage>

class MapWidget : public QWidget {
  Q_OBJECT
public:
  explicit MapWidget(QWidget* parent = nullptr);

  // Метод для обновления карты
  // data – это массив, где -1 обычно неизвестная область, 0–100 – уровень занятости
  // width, height – размеры карты
  void setMapData(const std::vector<int8_t>& data, int width, int height);

protected:
  void paintEvent(QPaintEvent* event) override;

private:
  QImage m_mapImage; // здесь будем хранить карту как картинку
};
