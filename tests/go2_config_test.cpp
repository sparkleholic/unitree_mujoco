#include "param.h"
#include <iostream>
#include <stdexcept>

void check(bool v, const char* why) { if (!v) throw std::runtime_error(why); }
bool rejected(const std::string& mode, const std::string& robot, int idl, int joints) {
    try { param::ValidateControl(mode, robot, idl, joints); return false; }
    catch (const std::invalid_argument&) { return true; }
}
int main() {
    check(param::config.control == "lowcmd", "Default compatibility");
    check(!rejected("lowcmd", "g1", 1, 29), "HG lowcmd compatibility");
    check(!rejected("sport", "go2", 0, 12), "Go2 sport valid");
    check(rejected("other", "go2", 0, 12), "Invalid mode");
    check(rejected("sport", "g1", 1, 29), "Reject HG sport");
    check(rejected("sport", "b2", 0, 12), "Reject non-Go2");
    check(rejected("sport", "go2", 0, 11), "Reject wrong joint count");
    param::config.load_from_yaml(TEST_CONFIG_PATH);
    check(param::config.control == "lowcmd", "YAML backward compatibility");
    std::cout << "PASS control validation and YAML compatibility\n";
}
