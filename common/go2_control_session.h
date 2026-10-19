#pragma once
#include "go2_posture_controller.h"
#include <chrono>
#include <cstdint>
#include <cmath>
#include <mutex>
#include <optional>

namespace go2_sim {
inline double SteadySeconds() {
    return std::chrono::duration<double>(std::chrono::steady_clock::now().time_since_epoch()).count();
}

// The feedback callback, RPC callback and control loop share only this state.
class Go2ControlSession {
public:
    void Feedback(const Posture& measured, double now) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (ready_ && !Fresh(now)) Clear();
        for (double q : measured) if (!std::isfinite(q)) { Clear(); return; }
        measured_ = measured;
        received_ = now;
        ready_ = true;
    }
    int32_t Request(StandPose pose, double now) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!Fresh(now)) { Clear(); return 3202; }
        pending_ = pose;  // Last accepted request wins before the next control tick.
        return 0;
    }
    std::optional<Posture> Tick(double motion_time, double now) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!Fresh(now)) { Clear(); return std::nullopt; }
        if (last_time_ && motion_time < *last_time_) {
            controller_.Reset();
            pending_.reset();
        }
        last_time_ = motion_time;
        if (pending_) {
            controller_.Begin(*pending_, measured_, motion_time);
            pending_.reset();
        }
        if (!controller_.Active()) return std::nullopt;
        return controller_.Sample(motion_time);
    }
    void Reset() {
        std::lock_guard<std::mutex> lock(mutex_);
        Clear();
    }
private:
    bool Fresh(double now) const { return ready_ && now >= received_ && now - received_ < 0.5; }
    void Clear() {
        ready_ = false;
        pending_.reset();
        last_time_.reset();
        controller_.Reset();
    }
    std::mutex mutex_;
    Posture measured_{};
    double received_ = 0;
    bool ready_ = false;
    std::optional<double> last_time_;
    std::optional<StandPose> pending_;
    Go2PostureController controller_;
};
}  // namespace go2_sim
