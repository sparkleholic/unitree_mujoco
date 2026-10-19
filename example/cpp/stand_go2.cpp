#include <iostream>
#include <atomic>
#include "go2_options.h"
#include <unitree/robot/go2/sport/sport_client.hpp>
#include <chrono>
#include <thread>
#include <stdio.h>
#include <stdint.h>
#include <math.h>
#include <unitree/robot/channel/channel_publisher.hpp>
#include <unitree/robot/channel/channel_subscriber.hpp>
#include <unitree/idl/go2/LowState_.hpp>
#include <unitree/idl/go2/LowCmd_.hpp>
#include <unitree/common/time/time_tool.hpp>
#include <unitree/common/thread/thread.hpp>

using namespace unitree::common;
using namespace unitree::robot;

#define TOPIC_LOWCMD "rt/lowcmd"
#define TOPIC_LOWSTATE "rt/lowstate"

constexpr double PosStopF = (2.146E+9f);
constexpr double VelStopF = (16000.0f);

class Custom
{
public:
    Custom(){};
    void Init();
    ~Custom() { Stop(); }
    void Stop() {
        stop_ = true;
        if (lowCmdWriteThread.joinable()) lowCmdWriteThread.join();
        if (lowstate_subscriber) lowstate_subscriber->CloseChannel();
    }
    std::string action = "sequence";

private:
    void InitLowCmd();
    void LowStateMessageHandler(const void *messages);
    void LowCmdWrite();

private:
    double stand_up_joint_pos[12] = {0.00571868, 0.608813, -1.21763, -0.00571868, 0.608813, -1.21763,
                                     0.00571868, 0.608813, -1.21763, -0.00571868, 0.608813, -1.21763};
    double stand_down_joint_pos[12] = {0.0473455, 1.22187, -2.44375, -0.0473455, 1.22187, -2.44375, 0.0473455,
                                       1.22187, -2.44375, -0.0473455, 1.22187, -2.44375};
    double dt = 0.002;
    double runing_time = 0.0;
    double phase = 0.0;

    unitree_go::msg::dds_::LowCmd_ low_cmd{};     // default init
    unitree_go::msg::dds_::LowState_ low_state{}; // default init

    /*publisher*/
    ChannelPublisherPtr<unitree_go::msg::dds_::LowCmd_> lowcmd_publisher;
    /*subscriber*/
    ChannelSubscriberPtr<unitree_go::msg::dds_::LowState_> lowstate_subscriber;

    /*LowCmd write thread*/
    std::thread lowCmdWriteThread;
    std::atomic<bool> stop_{false};
};

uint32_t crc32_core(uint32_t *ptr, uint32_t len)
{
    unsigned int xbit = 0;
    unsigned int data = 0;
    unsigned int CRC32 = 0xFFFFFFFF;
    const unsigned int dwPolynomial = 0x04c11db7;

    for (unsigned int i = 0; i < len; i++)
    {
        xbit = 1 << 31;
        data = ptr[i];
        for (unsigned int bits = 0; bits < 32; bits++)
        {
            if (CRC32 & 0x80000000)
            {
                CRC32 <<= 1;
                CRC32 ^= dwPolynomial;
            }
            else
            {
                CRC32 <<= 1;
            }

            if (data & xbit)
                CRC32 ^= dwPolynomial;
            xbit >>= 1;
        }
    }

    return CRC32;
}

void Custom::Init()
{
    InitLowCmd();
    /*create publisher*/
    lowcmd_publisher.reset(new ChannelPublisher<unitree_go::msg::dds_::LowCmd_>(TOPIC_LOWCMD));
    lowcmd_publisher->InitChannel();

    /*create subscriber*/
    lowstate_subscriber.reset(new ChannelSubscriber<unitree_go::msg::dds_::LowState_>(TOPIC_LOWSTATE));
    lowstate_subscriber->InitChannel(std::bind(&Custom::LowStateMessageHandler, this, std::placeholders::_1), 1);

    /*loop publishing thread*/
    lowCmdWriteThread = std::thread([this] {
        auto deadline = std::chrono::steady_clock::now();
        while (!stop_) {
            LowCmdWrite();
            deadline += std::chrono::milliseconds(2);
            std::this_thread::sleep_until(deadline);
        }
    });
}

void Custom::InitLowCmd()
{
    low_cmd.head()[0] = 0xFE;
    low_cmd.head()[1] = 0xEF;
    low_cmd.level_flag() = 0xFF;
    low_cmd.gpio() = 0;

    for (int i = 0; i < 20; i++)
    {
        low_cmd.motor_cmd()[i].mode() = (0x01); // motor switch to servo (PMSM) mode
        low_cmd.motor_cmd()[i].q() = (PosStopF);
        low_cmd.motor_cmd()[i].kp() = (0);
        low_cmd.motor_cmd()[i].dq() = (VelStopF);
        low_cmd.motor_cmd()[i].kd() = (0);
        low_cmd.motor_cmd()[i].tau() = (0);
    }
}

void Custom::LowStateMessageHandler(const void *message)
{
    (void)message;
}

void Custom::LowCmdWrite()
{

    runing_time += dt;
    if (action == "standup" || (action == "sequence" && runing_time < 3.0))
    {
        // Stand up in first 3 second

        // Total time for standing up or standing down is about 1.2s
        phase = tanh(runing_time / 1.2);
        for (int i = 0; i < 12; i++)
        {
            low_cmd.motor_cmd()[i].q() = phase * stand_up_joint_pos[i] + (1 - phase) * stand_down_joint_pos[i];
            low_cmd.motor_cmd()[i].dq() = 0;
            low_cmd.motor_cmd()[i].kp() = phase * 50.0 + (1 - phase) * 20.0;
            low_cmd.motor_cmd()[i].kd() = 3.5;
            low_cmd.motor_cmd()[i].tau() = 0;
        }
    }
    else
    {
        // Then stand down
        phase = tanh((runing_time - (action == "sequence" ? 3.0 : 0.0)) / 1.2);
        for (int i = 0; i < 12; i++)
        {
            low_cmd.motor_cmd()[i].q() = phase * stand_down_joint_pos[i] + (1 - phase) * stand_up_joint_pos[i];
            low_cmd.motor_cmd()[i].dq() = 0;
            low_cmd.motor_cmd()[i].kp() = 50;
            low_cmd.motor_cmd()[i].kd() = 3.5;
            low_cmd.motor_cmd()[i].tau() = 0;
        }
    }

    low_cmd.crc() = crc32_core((uint32_t *)&low_cmd, (sizeof(unitree_go::msg::dds_::LowCmd_) >> 2) - 1);
    lowcmd_publisher->Write(low_cmd);
}

int main(int argc, char** argv)
{
    using namespace go2_sim;
    try {
        const auto options = ParseOptions(argc, argv);
        if (options.help) { PrintOptions(); return 0; }
        InstallSignals();
        ChannelFactory::Instance()->Init(options.domain, options.network);
        if (!options.automatic) { std::cout << "Press enter to start" << std::endl; std::cin.get(); }
        if (options.api == "sport") {
            unitree::robot::go2::SportClient client;
            client.SetTimeout(1.0f);
            client.Init();
            std::this_thread::sleep_for(std::chrono::milliseconds(400));
            auto issue = [&](bool up) {
                const int code = up ? client.StandUp() : client.StandDown();
                std::cout << (up ? "StandUp" : "StandDown") << " result=" << code << std::endl;
                return code == 0;
            };
            if (!issue(options.action != "standdown")) return 1;
            const auto start = std::chrono::steady_clock::now();
            bool down = false;
            while (!stop_requested) {
                const double elapsed = std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
                if (options.action == "sequence" && !down && elapsed >= 6) {
                    if (!issue(false)) return 1;
                    down = true;
                }
                if (options.duration && elapsed >= options.duration) break;
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
        } else {
            Custom custom;
            custom.action = options.action;
            custom.Init();
            const auto start = std::chrono::steady_clock::now();
            while (!stop_requested && (!options.duration ||
                std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count() < options.duration))
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            custom.Stop();
        }
        return 0;
    } catch (const std::exception& e) { std::cerr << e.what() << std::endl; return 2; }
}
