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
    battery.set_percentage(std::numeric_limits<float>::infinity());
    if (apply() != QStringLiteral("Battery: unknown (charging)")) return 13;
    battery.set_percentage(-std::numeric_limits<float>::infinity());
    if (apply() != QStringLiteral("Battery: unknown (charging)")) return 14;
    battery.set_present(false);
    if (!apply().isEmpty()) return 7;
    battery.Clear(); // equivalent to absent Sensors.batteryState
    if (!apply().isEmpty()) return 8;
    // Reconnect: the next snapshot must restore a real percentage, and the
    // old charging status must not leak across the missing battery.
    battery.set_present(true);
    battery.set_percentage(0.76f);
    if (apply() != QStringLiteral("Battery: 76%")) return 9;
    battery.set_percentage(0.0f);
    if (apply() != QStringLiteral("Battery: 0%")) return 10;
    battery.set_percentage(1.0f);
    if (apply() != QStringLiteral("Battery: 100%")) return 11;
    battery.Clear();
    if (!apply().isEmpty()) return 12;
    return 0;
}
