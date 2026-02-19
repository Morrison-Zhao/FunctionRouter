#include "FunctionRouter/core/FunctionRouter.hpp"
#include <iostream>
#include <string>
#include <thread>

// Simple event ID
enum EventID {
    EVT_SAY_HELLO = 1
};

void say_hello(const std::string& name) {
    std::cout << "Hello, " << name << "! (from thread " << std::this_thread::get_id() << ")" << std::endl;
}

int main() {
    auto& dispatcher = FunctionRouter::getInstance();
    
    std::cout << "Main thread: " << std::this_thread::get_id() << std::endl;

    // Register a simple event
    dispatcher.registerEvent<void(const std::string&)>(EVT_SAY_HELLO, ThreadMode::SYNC, say_hello);

    // Post the event
    dispatcher.postEvent(EVT_SAY_HELLO, std::string("World"));

    // Start the dispatcher (this will block in this simple example, or we can run it in a thread)
    // For this demo, we just used SYNC mode which runs immediately on postEvent if called from a supported context.
    // But FunctionRouter usually needs execute() to be running for ASYNC/MAIN modes.
    
    // Let's try an ASYNC event
    dispatcher.registerEvent<void(const std::string&)>(2, ThreadMode::ASYNC, [](const std::string& msg){
        std::cout << "Async message: " << msg << " (from thread " << std::this_thread::get_id() << ")" << std::endl;
    });

    // We need to start the dispatcher loop in a separate thread or call execute() at the end
    std::thread runner([&](){
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        dispatcher.postEvent(2, std::string("This is async"));
        
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        dispatcher.quit();
    });

    dispatcher.execute(); // Blocks here until quit() is called
    
    if(runner.joinable()) runner.join();

    return 0;
}
