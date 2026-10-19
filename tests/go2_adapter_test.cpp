// Integration test with the real adapter process and SDK DDS/RPC transports.
#include <unitree/robot/go2/sport/sport_client.hpp>
#include <unitree/robot/channel/channel_publisher.hpp>
#include <unitree/robot/channel/channel_subscriber.hpp>
#include <unitree/idl/go2/LowState_.hpp>
#include <unitree/idl/go2/LowCmd_.hpp>
#include <atomic>
#include <chrono>
#include <iostream>
#include <stdexcept>
#include <thread>

void check(bool b, const char* s) { if (!b) throw std::runtime_error(s); }
int main(int argc, char** argv) {
    if (argc != 2) return 2;
    unitree::robot::ChannelFactory::Instance()->Init(std::stoi(argv[1]), "lo");
    std::atomic<int> commands{0};
    std::atomic<bool> feedback{false}, stop{false};
    unitree::robot::ChannelSubscriber<unitree_go::msg::dds_::LowCmd_> sub("rt/lowcmd");
    sub.InitChannel([&](const void*) { ++commands; });
    unitree::robot::ChannelPublisher<unitree_go::msg::dds_::LowState_> pub("rt/lowstate");
    pub.InitChannel();
    unitree::robot::go2::SportClient client;
    client.SetTimeout(1.0f); client.Init();
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    check(client.StandUp() == 3202, "No feedback: reject command");
    check(commands == 0, "No feedback: no LowCmd publication");
    std::thread source([&] {
        unitree_go::msg::dds_::LowState_ state{};
        while (!stop) {
            if (feedback) pub.Write(state);
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
    });
    try {
        feedback = true;
        std::this_thread::sleep_for(std::chrono::milliseconds(300));
        check(commands == 0, "Feedback alone must not activate control");
        check(client.StandUp() == 0, "Fresh feedback: accept command");
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        check(commands > 20, "Accepted command must produce LowCmd");
        feedback = false;
        std::this_thread::sleep_for(std::chrono::milliseconds(800));
        const int count = commands;
        std::this_thread::sleep_for(std::chrono::milliseconds(150));
        check(commands == count, "Stale feedback must stop LowCmd publication");
        check(client.StandDown() == 3202, "Stale feedback must reject command");
        feedback = true;
        std::this_thread::sleep_for(std::chrono::milliseconds(300));
        check(commands == count, "Reconnect must not resurrect the old command");
        check(client.StandDown() == 0, "Reconnect plus new command can restart");
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        check(commands > count + 20, "New command must resume publication");
    } catch (...) { stop = true; source.join(); sub.CloseChannel(); throw; }
    stop = true; source.join(); sub.CloseChannel();
    std::cout << "PASS adapter readiness, actual LowCmd publication, timeout and reconnect\n";
}
