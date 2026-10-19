#pragma once
#include "go2_posture_controller.h"
#include <unitree/idl/go2/LowCmd_.hpp>
#include <unitree/dds_wrapper/common/crc.h>

namespace go2_sim {
inline unitree_go::msg::dds_::LowCmd_ MakeLowCmd(const Posture& q, double kp = 50) {
    unitree_go::msg::dds_::LowCmd_ cmd{};
    cmd.head()[0] = 0xFE;
    cmd.head()[1] = 0xEF;
    cmd.level_flag() = 0xFF;
    for (auto& motor : cmd.motor_cmd()) motor.mode() = 1;
    for (size_t i = 0; i < q.size(); ++i) {
        auto& m = cmd.motor_cmd()[i];
        m.q() = q[i];
        m.kp() = kp;
        m.kd() = 3.5;
    }
    cmd.crc() = crc32_core(reinterpret_cast<uint32_t*>(&cmd), (sizeof(cmd) >> 2) - 1);
    return cmd;
}
}  // namespace go2_sim
