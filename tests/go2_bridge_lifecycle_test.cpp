#include "unitree_sdk2_bridge.h"
#include <unitree/robot/go2/sport/sport_client.hpp>
#include <chrono>
#include <iostream>
#include <stdexcept>
#include <thread>

void check(bool b, const char* why) { if (!b) throw std::runtime_error(why); }
int main() {
    param::config.robot = "go2";
    param::config.control = "sport";
    param::config.robot_scene = TEST_GO2_SCENE;
    param::config.print_scene_information = 0;
    param::config.use_joystick = 0;
    unitree::robot::ChannelFactory::Instance()->Init(74, "lo");
    char error[1024]{};
    std::unique_ptr<mjModel, decltype(&mj_deleteModel)> model(mj_loadXML(TEST_GO2_SCENE, nullptr, error, sizeof(error)), mj_deleteModel);
    check(bool(model), error);
    std::unique_ptr<mjData, decltype(&mj_deleteData)> data(mj_makeData(model.get()), mj_deleteData);
    mj_forward(model.get(), data.get());
    // A twelve-actuator XML is not usable unless it provides motor feedback.
    std::string xml = "<mujoco><worldbody>";
    for (int i = 0; i < 12; ++i)
        xml += "<body><joint name='j" + std::to_string(i) + "'/><geom size='.01' mass='1'/></body>";
    xml += "</worldbody><actuator>";
    for (int i = 0; i < 12; ++i) xml += "<motor joint='j" + std::to_string(i) + "'/>";
    xml += "</actuator></mujoco>";
    mjVFS vfs;
    mj_defaultVFS(&vfs);
    mj_addBufferVFS(&vfs, "no_sensors.xml", xml.data(), static_cast<int>(xml.size()));
    std::unique_ptr<mjModel, decltype(&mj_deleteModel)> invalid(
        mj_loadXML("no_sensors.xml", &vfs, error, sizeof(error)), mj_deleteModel);
    mj_deleteVFS(&vfs);
    check(bool(invalid), error);
    std::unique_ptr<mjData, decltype(&mj_deleteData)> invalid_data(mj_makeData(invalid.get()), mj_deleteData);
    bool rejected = false;
    try { auto unused = CreateUnitreeBridge(invalid.get(), invalid_data.get()); }
    catch (const std::invalid_argument&) { rejected = true; }
    check(rejected, "Reject a sensorless model before accessing motor feedback");
    auto bridge = CreateUnitreeBridge(model.get(), data.get());
    unitree::robot::go2::SportClient client;
    client.SetTimeout(1.0f); client.Init();
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    bridge->run();
    check(client.StandUp() == 0, "Request must reach internal server");
    bridge->run();
    data->time = 1;
    bridge->run();
    const double moving_ctrl = data->ctrl[1];
    check(std::abs(moving_ctrl) > 1, "Active trajectory must apply torque");
    bridge->run();
    check(data->ctrl[1] == moving_ctrl, "Paused simulation must freeze trajectory");
    mj_resetData(model.get(), data.get());
    mj_forward(model.get(), data.get());
    bridge->run();
    for (int i = 0; i < 12; ++i) check(data->ctrl[i] == 0, "Reset must invalidate old torque");
    bridge->run();
    for (int i = 0; i < 12; ++i) check(data->ctrl[i] == 0, "Reset must not restart old command");
    check(client.StandDown() == 0, "New command after reset");
    bridge->run();
    data->time = 1;
    bridge->run();
    check(std::abs(data->ctrl[1]) > 1, "New command must run");
    bridge.reset();
    data.reset();
    model.reset(mj_loadXML(TEST_GO2_SCENE, nullptr, error, sizeof(error)));
    data.reset(mj_makeData(model.get()));
    mj_forward(model.get(), data.get());
    bridge = CreateUnitreeBridge(model.get(), data.get());
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    bridge->run();
    for (int i = 0; i < 12; ++i) check(data->ctrl[i] == 0, "Reload starts without an old command");
    check(client.StandUp() == 0, "Reloaded server must accept fresh commands");
    bridge.reset();
    std::cout << "PASS actual bridge RPC, pause, time reset, reload and shutdown\n";
}
