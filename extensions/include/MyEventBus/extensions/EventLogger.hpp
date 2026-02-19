#pragma once
#include <iostream>
#include <string>

namespace MyEventBus {
namespace Extensions {

class EventLogger {
public:
    static void log(const std::string& message) {
        std::cout << "[EventLogger] " << message << std::endl;
    }
};

} // namespace Extensions
} // namespace MyEventBus
