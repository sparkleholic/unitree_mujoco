#include "go2_posture_controller.h"
#include <cmath>
#include <iostream>
#include <stdexcept>

void check(bool value, const char* why) {
    if (!value) throw std::runtime_error(why);
}
int main() {
    using namespace go2_sim;
    Go2PostureController c;
    Posture measured{};
    check(!c.Active(), "No command must mean no control");
    c.Begin(StandPose::Up, measured, 10);
    check(c.Active(), "Begin activates control");
    check(std::abs(c.Sample(10)[1]) < 1e-12, "Transition must start from measured joints");
    check(std::abs(c.Sample(11)[1] - 0.3044065) < 1e-8, "Smoothstep midpoint");
    check(std::abs(c.Sample(12)[1] - 0.608813) < 1e-8, "Reach up target");
    check(std::abs(c.Sample(20)[1] - 0.608813) < 1e-8, "Hold target");
    measured.fill(0.3);
    c.Begin(StandPose::Down, measured, 20);
    check(std::abs(c.Sample(20)[1] - 0.3) < 1e-12, "Reversal starts from current measured state");
    check(std::abs(c.Sample(22)[2] + 2.44375) < 1e-8, "Reach down target");
    c.Reset();
    check(!c.Active(), "Reset invalidates old command");
    std::cout << "PASS posture interpolation, hold, reversal and reset\n";
}
