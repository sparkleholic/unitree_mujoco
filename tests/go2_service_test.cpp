#include "go2_sport_server.h"
#include <unitree/robot/go2/sport/sport_client.hpp>
#include <atomic>
#include <chrono>
#include <iostream>
#include <stdexcept>
#include <thread>

void check(bool v, const char* m) { if (!v) throw std::runtime_error(m); }
int main() {
    using namespace go2_sim;
    unitree::robot::ChannelFactory::Instance()->Init(72, "lo");
    std::atomic<bool> ready{false};
    std::atomic<int> last{0};
    Go2SportServer server([&](StandPose pose) {
        if (!ready) return int32_t(3202);
        last = pose == StandPose::Up ? 1 : 2;
        return int32_t(0);
    });
    server.Init();
    server.Start();
    unitree::robot::go2::SportClient client;
    client.SetTimeout(1.0f);
    client.Init();
    std::this_thread::sleep_for(std::chrono::milliseconds(400));
    check(client.StandUp() == 3202, "Unready must fail");
    ready = true;
    check(client.StandUp() == 0 && last == 1, "StandUp dispatch");
    check(client.StandDown() == 0 && last == 2, "StandDown dispatch");
    check(client.Sit() == 3203, "Unsupported API must fail");
    check(client.GetServerApiVersion() == "1.0.0.1", "Version contract");
    server.Stop();
    check(client.StandUp() != 0, "Stopped server must not respond successfully");
    std::cout << "PASS actual SDK RPC dispatch, readiness, version, unsupported API and shutdown\n";
}
