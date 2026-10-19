#include <unitree/robot/channel/channel_subscriber.hpp>
#include <unitree/idl/go2/LowState_.hpp>
#include <unitree/idl/go2/SportModeState_.hpp>
#include <chrono>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <thread>

int main(int argc, char** argv) {
    if (argc != 5) { std::cerr << "DOMAIN DURATION OUTPUT NETWORK\n"; return 2; }
    const double duration = std::stod(argv[2]);
    unitree::robot::ChannelFactory::Instance()->Init(std::stoi(argv[1]), argv[4]);
    std::mutex mutex;
    unitree_go::msg::dds_::LowState_ low{};
    double height = 0;
    auto low_time = std::chrono::steady_clock::time_point{};
    auto high_time = low_time;
    unitree::robot::ChannelSubscriber<unitree_go::msg::dds_::LowState_> ls("rt/lowstate");
    unitree::robot::ChannelSubscriber<unitree_go::msg::dds_::SportModeState_> hs("rt/sportmodestate");
    ls.InitChannel([&](const void* ptr) {
        std::lock_guard<std::mutex> lock(mutex);
        low = *static_cast<const unitree_go::msg::dds_::LowState_*>(ptr);
        low_time = std::chrono::steady_clock::now();
    });
    hs.InitChannel([&](const void* ptr) {
        std::lock_guard<std::mutex> lock(mutex);
        height = static_cast<const unitree_go::msg::dds_::SportModeState_*>(ptr)->position()[2];
        high_time = std::chrono::steady_clock::now();
    });
    std::ofstream out(argv[3]);
    if (!out) return 2;
    out << std::setprecision(12) << "time,tick,height,roll,pitch";
    for (int i = 0; i < 12; ++i) out << ",q" << i;
    out << '\n';
    const auto start = std::chrono::steady_clock::now();
    int samples = 0;
    bool invalid = false;
    while (std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count() < duration) {
        {
            std::lock_guard<std::mutex> lock(mutex);
            const auto now = std::chrono::steady_clock::now();
            if (now - low_time < std::chrono::milliseconds(100) && now - high_time < std::chrono::milliseconds(100)) {
                out << std::chrono::duration<double>(now - start).count() << ',' << low.tick() << ',' << height
                    << ',' << low.imu_state().rpy()[0] << ',' << low.imu_state().rpy()[1];
                for (int i = 0; i < 12; ++i) {
                    const auto q = low.motor_state()[i].q();
                    out << ',' << q;
                    invalid = invalid || !std::isfinite(q);
                }
                out << '\n'; ++samples;
                invalid = invalid || !std::isfinite(height);
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    ls.CloseChannel(); hs.CloseChannel();
    std::cout << "Collected " << samples << " sensor samples\n";
    return samples > 0 && !invalid ? 0 : 1;
}
