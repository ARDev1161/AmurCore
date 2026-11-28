#pragma once
#include <chrono>

// Local only, guarded by the same mutex as Controls and Sensors.
struct TeleopIntent {
    using Clock = std::chrono::steady_clock;
    Clock::time_point sample_time{};
    bool deadman_released = false;
    static constexpr auto max_age = std::chrono::milliseconds(250);
};
