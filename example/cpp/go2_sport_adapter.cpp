#include "go2_control_session.h"
#include "go2_lowcmd.h"
#include "go2_options.h"
#include "go2_sport_server.h"
#include <unitree/idl/go2/LowState_.hpp>
#include <unitree/robot/channel/channel_subscriber.hpp>
#include <unitree/robot/channel/channel_publisher.hpp>
#include <thread>

int main(int argc, char** argv) {
    using namespace go2_sim;
    try {
        const auto options = ParseOptions(argc, argv, true);
        if (options.help) { PrintOptions(true); return 0; }
        InstallSignals();
        unitree::robot::ChannelFactory::Instance()->Init(options.domain, options.network);
        Go2ControlSession session;
        unitree::robot::ChannelSubscriber<unitree_go::msg::dds_::LowState_> state("rt/lowstate");
        state.InitChannel([&](const void* raw) {
            const auto& msg = *static_cast<const unitree_go::msg::dds_::LowState_*>(raw);
            Posture measured{};
            for (size_t i = 0; i < measured.size(); ++i) measured[i] = msg.motor_state()[i].q();
            session.Feedback(measured, SteadySeconds());
        });
        unitree::robot::ChannelPublisher<unitree_go::msg::dds_::LowCmd_> publisher("rt/lowcmd");
        publisher.InitChannel();
        Go2SportServer server([&](StandPose pose) { return session.Request(pose, SteadySeconds()); });
        server.Init();
        server.Start();
        std::cout << "Adapter ready; waiting for feedback and a sport command" << std::endl;
        const double start = SteadySeconds();
        auto deadline = std::chrono::steady_clock::now();
        bool active = false;
        while (!stop_requested && (!options.duration || SteadySeconds() - start < options.duration)) {
            const double now = SteadySeconds();
            auto target = session.Tick(now, now);
            if (target) publisher.Write(MakeLowCmd(*target));
            if (active && !target) std::cout << "Control stopped: feedback unavailable; new command required" << std::endl;
            active = target.has_value();
            deadline += std::chrono::milliseconds(2);
            std::this_thread::sleep_until(deadline);
        }
        server.Stop();
        state.CloseChannel();
        std::cout << "Adapter stopped" << std::endl;
    } catch (const std::exception& e) { std::cerr << e.what() << std::endl; return 2; }
}
