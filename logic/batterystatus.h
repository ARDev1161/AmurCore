#pragma once

#include <cmath>
#include <QString>
#include "network/protobuf/power.pb.h"

// Pure presentation policy. Never confuse absent telemetry with an empty pack.
// Keep it independent of the Qt widget lifecycle for offscreen regression tests.
inline QString batteryStatusText(const Power::BatteryState &battery)
{
    if (!battery.present()) {
        return {};
    }
    QString result = QStringLiteral("Battery: unknown");
    const double percentage = static_cast<double>(battery.percentage());
    if (std::isfinite(percentage) && percentage >= 0.0 && percentage <= 1.0) {
        result = QStringLiteral("Battery: %1%").arg(qRound(percentage * 100.0));
    }
    if (battery.power_supply_status() == Power::BatteryState::CHARGING) {
        result += QStringLiteral(" (charging)");
    }
    return result;
}
