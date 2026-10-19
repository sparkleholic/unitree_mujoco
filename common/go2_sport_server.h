#pragma once
#include "go2_posture_controller.h"
#include <unitree/robot/server/server.hpp>
#include <unitree/robot/go2/sport/sport_api.hpp>
#include <iostream>

namespace go2_sim {
class Go2SportServer : public unitree::robot::Server {
public:
    explicit Go2SportServer(std::function<int32_t(StandPose)> accept)
        : Server(unitree::robot::go2::ROBOT_SPORT_SERVICE_NAME), accept_(std::move(accept)) {}
    ~Go2SportServer() override { Stop(); }
    void Init() override {
        using namespace unitree::robot::go2;
        SetApiVersion(ROBOT_SPORT_API_VERSION);
        RegistHandler(ROBOT_SPORT_API_ID_STANDUP, [this](const std::string&, std::string&) {
            return Accept(StandPose::Up);
        });
        RegistHandler(ROBOT_SPORT_API_ID_STANDDOWN, [this](const std::string&, std::string&) {
            return Accept(StandPose::Down);
        });
    }
    // Join the SDK request worker before destroying callback-owned state.
    void Stop() { mServerStubPtr.reset(); }
private:
    int32_t Accept(StandPose pose) {
        const int32_t code = accept_(pose);
        std::cout << "sport " << (pose == StandPose::Up ? "StandUp" : "StandDown")
                  << " accepted=" << (code == 0) << " code=" << code << std::endl;
        return code;
    }
    std::function<int32_t(StandPose)> accept_;
};
}  // namespace go2_sim
