#pragma once
#include <cmath>
#include <csignal>
#include <iostream>
#include <stdexcept>
#include <string>

namespace go2_sim {
inline volatile std::sig_atomic_t stop_requested = 0;
inline void SignalStop(int) { stop_requested = 1; }
inline void InstallSignals() { std::signal(SIGINT, SignalStop); std::signal(SIGTERM, SignalStop); }
struct Options {
    int domain = 1;
    std::string network = "lo", api = "lowcmd", action = "sequence";
    double duration = 0;
    bool automatic = false, help = false;
};
inline Options ParseOptions(int argc, char** argv, bool adapter = false) {
    Options o;
    bool domain_set = false, network_set = false, positional = false;
    for (int i = 1; i < argc; ++i) {
        std::string key = argv[i];
        if (key == "--help" || key == "-h") { o.help = true; continue; }
        if (key == "--auto" && !adapter) { o.automatic = true; continue; }
        if (key.rfind("-", 0) != 0 && !adapter && !positional && !network_set) {
            o.network = key; positional = true;
            if (!domain_set) o.domain = 0;
            continue;
        }
        if (i + 1 >= argc) throw std::invalid_argument("Missing value for " + key);
        const std::string value = argv[++i];
        if (key == "--domain") {
            size_t end = 0; o.domain = std::stoi(value, &end); domain_set = true;
            if (end != value.size() || o.domain < 0 || o.domain > 232) throw std::invalid_argument("Invalid DDS domain");
        } else if (key == "--network" && !positional) {
            o.network = value; network_set = true;
            if (value.empty()) throw std::invalid_argument("Empty network");
        } else if (key == "--duration") {
            size_t end = 0; o.duration = std::stod(value, &end);
            if (end != value.size() || !std::isfinite(o.duration) || o.duration <= 0)
                throw std::invalid_argument("Duration must be finite and positive");
        } else if (key == "--api" && !adapter) o.api = value;
        else if (key == "--action" && !adapter) o.action = value;
        else throw std::invalid_argument("Unknown option: " + key);
    }
    if (o.api != "lowcmd" && o.api != "sport") throw std::invalid_argument("API must be lowcmd or sport");
    if (o.action != "standup" && o.action != "standdown" && o.action != "sequence")
        throw std::invalid_argument("Action must be standup, standdown or sequence");
    return o;
}
inline void PrintOptions(bool adapter = false) {
    std::cout << "[--domain ID] [--network IFACE] [--duration SECONDS]";
    if (!adapter) std::cout << " [network] [--api lowcmd|sport] [--action standup|standdown|sequence] [--auto]";
    std::cout << std::endl;
}
}  // namespace go2_sim
