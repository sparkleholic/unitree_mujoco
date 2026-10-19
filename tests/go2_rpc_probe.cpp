#include <unitree/robot/go2/sport/sport_client.hpp>
#include <unitree/robot/channel/channel_factory.hpp>
#include <chrono>
#include <iostream>
#include <thread>

int main(int argc, char** argv) {
    if (argc != 3) return 2;
    unitree::robot::ChannelFactory::Instance()->Init(std::stoi(argv[1]), "lo");
    unitree::robot::go2::SportClient client;
    client.SetTimeout(1.0f);
    client.Init();
    std::this_thread::sleep_for(std::chrono::milliseconds(400));
    std::string action = argv[2];
    int code = action == "up" ? client.StandUp() :
               action == "down" ? client.StandDown() : client.Sit();
    std::cout << "RPC " << action << " result=" << code << std::endl;
    return code == 0 ? 0 : 1;
}
