#include "go2_control_session.h"
#include <cmath>
#include <iostream>
#include <stdexcept>

void check(bool value, const char* why) { if (!value) throw std::runtime_error(why); }
int main() {
    using namespace go2_sim;
    Go2ControlSession s;
    Posture m{};
    check(s.Request(StandPose::Up, 0) == 3202, "Reject before feedback");
    check(!s.Tick(0, 0), "No output before feedback");
    s.Feedback(m, 0);
    check(!s.Tick(0, 0), "No output before command");
    check(s.Request(StandPose::Up, 0) == 0, "Accept fresh command");
    check(s.Tick(10, 0).has_value(), "Start command");
    s.Feedback(m, 0.1);
    check(std::abs(s.Tick(11, 0.1).value()[1] - 0.3044065) < 1e-8, "Advance by motion clock");
    check(s.Request(StandPose::Down, 1) == 3202, "Reject stale feedback");
    check(!s.Tick(12, 1), "Stop output on feedback loss");
    s.Feedback(m, 1.1);
    check(!s.Tick(13, 1.1), "Reconnect must not resurrect command");
    check(s.Request(StandPose::Down, 1.1) == 0, "New command after reconnect");
    check(s.Tick(13, 1.1).has_value(), "Restart");
    m.fill(0.4);
    s.Feedback(m, 1.2);
    s.Request(StandPose::Up, 1.2);
    check(std::abs(s.Tick(13.1, 1.2).value()[1] - 0.4) < 1e-8, "Reversal starts from measured posture");
    check(!s.Tick(0, 1.2), "Time reset invalidates active command");
    check(!s.Tick(0.1, 1.2), "Reset does not resume old command");
    s.Request(StandPose::Up, 1.2);
    auto paused = s.Tick(0.1, 1.2);
    s.Feedback(m, 1.3);
    check(s.Tick(0.1, 1.3) == paused, "Paused motion clock must freeze trajectory");
    s.Feedback(m, 3.0);  // Reconnect arrives before the next control tick.
    check(!s.Tick(0.2, 3.0), "A feedback gap must invalidate old control even without a stale tick");
    s.Reset();
    check(s.Request(StandPose::Up, 1.3) == 3202, "Explicit reset requires fresh feedback");
    std::cout << "PASS readiness, stale state, reconnect, reversal, pause and reset\n";
}
