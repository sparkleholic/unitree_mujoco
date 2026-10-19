#pragma once

#include <algorithm>
#include <array>

namespace go2_sim {
using Posture = std::array<double, 12>;
enum class StandPose { Up, Down };
inline constexpr Posture kStandUp = {
    0.00571868, 0.608813, -1.21763, -0.00571868, 0.608813, -1.21763,
    0.00571868, 0.608813, -1.21763, -0.00571868, 0.608813, -1.21763};
inline constexpr Posture kStandDown = {
    0.0473455, 1.22187, -2.44375, -0.0473455, 1.22187, -2.44375,
    0.0473455, 1.22187, -2.44375, -0.0473455, 1.22187, -2.44375};

class Go2PostureController {
public:
    void Begin(StandPose pose, const Posture& measured, double time_seconds) {
        from_ = measured;
        to_ = pose == StandPose::Up ? kStandUp : kStandDown;
        start_ = time_seconds;
        active_ = true;
    }
    Posture Sample(double time_seconds) const {
        const double u = std::clamp((time_seconds - start_) / 2.0, 0.0, 1.0);
        const double phase = u * u * (3.0 - 2.0 * u);
        Posture target{};
        for (size_t i = 0; i < target.size(); ++i)
            target[i] = from_[i] + phase * (to_[i] - from_[i]);
        return target;
    }
    void Reset() { active_ = false; }
    bool Active() const { return active_; }
private:
    Posture from_{}, to_{};
    double start_ = 0;
    bool active_ = false;
};
}  // namespace go2_sim
