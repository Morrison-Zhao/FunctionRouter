#pragma once
#include <iostream>
#include <string>

namespace FunctionRouter {
namespace Extensions {

class RouterLogger {
public:
    static void log(const std::string& message) {
        std::cout << "[RouterLogger] " << message << std::endl;
    }
};

} // namespace Extensions
} // namespace FunctionRouter
