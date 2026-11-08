#include <QApplication>
#include <QLabel>
#include <QByteArray>
#include <limits>
#include "logic/batterystatus.h"

int main(int argc, char **argv)
{
    QApplication app(argc, argv);
    QLabel label;
    Power::BatteryState battery;
    auto apply = [&]() {
        const QString text = batteryStatusText(battery);
        label.setText(text);
        label.setVisible(!text.isEmpty());
        return text;
    };
    if (!apply().isEmpty()) return 1; // default absent
    battery.set_present(true);
    battery.set_percentage(0.42f);
    if (apply() != QStringLiteral("Battery: 42%")) return 2;
    battery.set_power_supply_status(Power::BatteryState::CHARGING);
    if (apply() != QStringLiteral("Battery: 42% (charging)")) return 3;
    battery.set_percentage(std::numeric_limits<float>::quiet_NaN());
    if (apply() != QStringLiteral("Battery: unknown (charging)")) return 4;
    battery.set_percentage(-1.0f);
    if (apply() != QStringLiteral("Battery: unknown (charging)")) return 5;
    battery.set_percentage(1.5f);
    if (apply() != QStringLiteral("Battery: unknown (charging)")) return 6;
    battery.set_present(false);
    if (!apply().isEmpty()) return 7;
    battery.Clear(); // equivalent to absent Sensors.batteryState
    if (!apply().isEmpty()) return 8;
    return 0;
}
