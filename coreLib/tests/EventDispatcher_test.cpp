#include "../include/event_bus/EventDispatcher.hpp"
#include <iostream>
#include <thread>
#include <string>
#include <cassert>
#include <atomic>
#include <chrono>
#include <vector>

// 定义一些测试用的 Event ID
const int64_t EVENT_TEST_START = 1;
const int64_t EVENT_SYNC = 100;
const int64_t EVENT_MAIN = 200;
const int64_t EVENT_ASYNC = 300;
const int64_t EVENT_LOOP = 400;
const int64_t EVENT_PERF = 500;
const int64_t EVENT_MEMBER = 600;

class TestComponent {
public:
    void onMemberEvent(const std::string& msg) {
        std::cout << "[TestComponent] Member event received: " << msg << std::endl;
        received = true;
    }
    bool received = false;
};

void run_all_tests() {
    auto& dispatcher = EventDispatcher::getInstance();
    std::cout << ">>> Starting Tests in thread: " << std::this_thread::get_id() << std::endl;

    // 1. 测试 SYNC (当前线程立即执行)
    {
        std::cout << "[Test] SYNC Event..." << std::endl;
        bool sync_executed = false;
        dispatcher.registerEvent<void()>(EVENT_SYNC, ThreadMode::SYNC, [&]{
            std::cout << "  -> SYNC event executed in " << std::this_thread::get_id() << std::endl;
            sync_executed = true;
        });
        dispatcher.postEvent(EVENT_SYNC);
        assert(sync_executed); // 必须立即为真
        std::cout << "[Pass] SYNC Event" << std::endl;
    }

    // 2. 测试 MAIN (主线程执行)
    {
        std::cout << "[Test] MAIN Event..." << std::endl;
        // 注意：我们在子线程里 post，期望在主线程执行
        dispatcher.registerEvent<void(int)>(EVENT_MAIN, ThreadMode::MAIN, [](int val){
            std::cout << "  -> MAIN event executed in " << std::this_thread::get_id() << " with value " << val << std::endl;
        });
        dispatcher.postEvent(EVENT_MAIN, 123);
        // 结果无法立即断言，只能通过日志观察，因为 MAIN 事件需要等待主线程 Looper 调度
    }

    // 3. 测试 LOOP (独立子线程串行执行)
    {
        std::cout << "[Test] LOOP Event..." << std::endl;
        dispatcher.registerEvent<void()>(EVENT_LOOP, ThreadMode::LOOP, []{
            std::cout << "  -> LOOP event executed in " << std::this_thread::get_id() << std::endl;
        });
        dispatcher.postEvent(EVENT_LOOP);
    }

    // 4. 测试 Member Function
    {
        std::cout << "[Test] Member Function..." << std::endl;
        static TestComponent comp; // static 保证生命周期
        dispatcher.registerEvent<TestComponent, void, const std::string&>(EVENT_MEMBER, ThreadMode::SYNC, &comp, &TestComponent::onMemberEvent);
        dispatcher.postEvent(EVENT_MEMBER, std::string("Hello Member"));
        assert(comp.received);
        std::cout << "[Pass] Member Function" << std::endl;
    }

    // 5. 性能测试 (ASYNC)
    {
        std::cout << "[Test] Performance Benchmark (ASYNC)..." << std::endl;
        const int TASK_COUNT = 100;
        int counter{0};
        
        dispatcher.registerEvent<void()>(EVENT_PERF, ThreadMode::ASYNC, [&]{
            counter++;
            printf("count: %i\n", counter);
        });

        auto start = std::chrono::high_resolution_clock::now();
        for(int i=0; i<TASK_COUNT; ++i) {
            dispatcher.postEvent(EVENT_PERF);
        }
        
        // 等待完成
        while(counter < TASK_COUNT) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
        
        std::cout << "  -> Processed " << TASK_COUNT << " events in " << ms << " ms" << std::endl;
        std::cout << "  -> Throughput: " << (double)TASK_COUNT / ms * 1000 << " events/sec" << std::endl;
    }

    std::cout << "<<< All Tests Completed. Quitting..." << std::endl;
    
    // 给一点时间让 MAIN 事件打印出来
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    
    // 退出主循环
    dispatcher.quit();
}

int main() {
    auto& dispatcher = EventDispatcher::getInstance();

    std::cout << "Main Thread ID: " << std::this_thread::get_id() << std::endl;

    // 启动一个独立的测试线程，避免占用线程池资源导致死锁
    std::thread test_thread([&]{
        // 等待调度器启动
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        run_all_tests();
    });

    // 启动调度器 (这是阻塞的！)
    std::cout << "Dispatcher starting..." << std::endl;
    dispatcher.execute();
    
    if (test_thread.joinable()) {
        test_thread.join();
    }
    
    std::cout << "Dispatcher stopped." << std::endl;

    return 0;
}
