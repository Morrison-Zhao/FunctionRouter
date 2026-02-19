#include "../include/event_bus/EventDispatcher.hpp"
#include <iostream>
#include <thread>
#include <string>
#include <cassert>
#include <atomic>
#include <chrono>
#include <vector>

// ==========================================
// 辅助类和函数
// ==========================================

struct ComplexData {
    int id;
    std::string payload;
};

void free_function(int val) {
    // std::cout << "Free function called with " << val << std::endl;
}

class TestService {
public:
    static void static_func(int val) {
        // std::cout << "Static function called with " << val << std::endl;
    }

    void member_func(const std::string& msg) {
        // std::cout << "Member function called with " << msg << std::endl;
        last_msg = msg;
    }

    void const_member_func(int val) const {
        // std::cout << "Const member function called with " << val << std::endl;
    }

    void ref_member_func(int& val) {
        val += 1;
    }

    std::string last_msg;
};

// ==========================================
// 测试用例 ID
// ==========================================
enum EventID {
    EVT_FREE_FUNC = 100,
    EVT_STATIC_FUNC,
    EVT_MEMBER_FUNC,
    EVT_MEMBER_FUNC_REF,
    EVT_CONST_MEMBER_FUNC,
    EVT_LAMBDA,
    EVT_COMPLEX_DATA,
    EVT_UNREGISTER_TEST,
    
    EVT_PERF_SYNC = 1000,
    EVT_PERF_ASYNC,
    EVT_PERF_MAIN
};

// ==========================================
// 测试逻辑
// ==========================================

void test_functional() {
    std::cout << "\n========== [Functional Tests] ==========" << std::endl;
    auto& dispatcher = EventDispatcher::getInstance();
    TestService service;

    // 1. 普通函数
    dispatcher.registerEvent<void(int)>(EVT_FREE_FUNC, ThreadMode::SYNC, free_function);
    dispatcher.postEvent(EVT_FREE_FUNC, 1);
    std::cout << "[Pass] Free Function" << std::endl;

    // 2. 静态成员函数
    dispatcher.registerEvent<void(int)>(EVT_STATIC_FUNC, ThreadMode::SYNC, &TestService::static_func);
    dispatcher.postEvent(EVT_STATIC_FUNC, 2);
    std::cout << "[Pass] Static Member Function" << std::endl;

    // 3. 成员函数 (引用传参)
    dispatcher.registerEvent<TestService, void, const std::string&>(EVT_MEMBER_FUNC, ThreadMode::SYNC, &service, &TestService::member_func);
    dispatcher.postEvent(EVT_MEMBER_FUNC, std::string("Hello"));
    assert(service.last_msg == "Hello");
    std::cout << "[Pass] Member Function (Reference Args)" << std::endl;

//    int test_val = 3;
//    dispatcher.registerEvent<TestService, void, int&>(EVT_MEMBER_FUNC_REF, ThreadMode::SYNC, &service, &TestService::ref_member_func);
//    dispatcher.postEvent(EVT_MEMBER_FUNC, test_val);
//    std::cout << "[Pass] Member Function (Ref 2)" << std::endl;


    // 4. Const 成员函数
    dispatcher.registerEvent<TestService, void, int>(EVT_CONST_MEMBER_FUNC, ThreadMode::SYNC, &service, &TestService::const_member_func);
    dispatcher.postEvent(EVT_CONST_MEMBER_FUNC, 3);
    std::cout << "[Pass] Const Member Function" << std::endl;

    // 5. Lambda
    int lambda_val = 0;
    dispatcher.registerEvent<void(int)>(EVT_LAMBDA, ThreadMode::SYNC, [&](int v){ lambda_val = v; });
    dispatcher.postEvent(EVT_LAMBDA, 42);
    assert(lambda_val == 42);
    std::cout << "[Pass] Lambda" << std::endl;

    // 6. 复杂对象传参
    dispatcher.registerEvent<void(const ComplexData&)>(EVT_COMPLEX_DATA, ThreadMode::SYNC, [](const ComplexData& d){
        assert(d.id == 99 && d.payload == "Complex");
    });
    dispatcher.postEvent(EVT_COMPLEX_DATA, ComplexData{99, "Complex"});
    std::cout << "[Pass] Complex Data Struct" << std::endl;

    // 7. 注销测试
    std::atomic<int> call_count{0};
    dispatcher.registerEvent<void()>(EVT_UNREGISTER_TEST, ThreadMode::SYNC, [&]{ call_count++; });
    dispatcher.postEvent(EVT_UNREGISTER_TEST);
    assert(call_count == 1);
    
    dispatcher.unregisterEvent(EVT_UNREGISTER_TEST);
    dispatcher.postEvent(EVT_UNREGISTER_TEST);
    assert(call_count == 1); // 应该不再增加
    std::cout << "[Pass] Unregister Event" << std::endl;
}

void test_performance() {
    std::cout << "\n========== [Performance Tests] ==========" << std::endl;
    auto& dispatcher = EventDispatcher::getInstance();
    const int COUNT = 100000;

    // 1. SYNC 模式
    {
        std::atomic<int> counter{0};
        dispatcher.registerEvent<void()>(EVT_PERF_SYNC, ThreadMode::SYNC, [&]{ counter++; });
        
        auto start = std::chrono::high_resolution_clock::now();
        for(int i=0; i<COUNT; ++i) dispatcher.postEvent(EVT_PERF_SYNC);
        auto end = std::chrono::high_resolution_clock::now();
        
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
        std::cout << "SYNC  Mode: " << COUNT << " events in " << ms << " ms (" 
                  << (double)COUNT/ms*1000 << " ops/sec)" << std::endl;
    }

    // 2. ASYNC 模式
    {
        std::atomic<int> counter{0};
        dispatcher.registerEvent<void()>(EVT_PERF_ASYNC, ThreadMode::ASYNC, [&]{ counter++; });
        
        auto start = std::chrono::high_resolution_clock::now();
        for(int i=0; i<COUNT; ++i) dispatcher.postEvent(EVT_PERF_ASYNC);
        
        // 等待完成
        while(counter < COUNT) std::this_thread::sleep_for(std::chrono::milliseconds(1));
        
        auto end = std::chrono::high_resolution_clock::now();
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
        std::cout << "ASYNC Mode: " << COUNT << " events in " << ms << " ms (" 
                  << (double)COUNT/ms*1000 << " ops/sec)" << std::endl;
    }
}

void run_all_tests() {
    test_functional();
    test_performance();
    
    std::cout << "\n>>> All tests finished. Quitting..." << std::endl;
    std::this_thread::sleep_for(std::chrono::milliseconds(200)); // 让日志飞一会儿
    EventDispatcher::getInstance().quit();
}

int main() {
    auto& dispatcher = EventDispatcher::getInstance();
    std::cout << "Main Thread ID: " << std::this_thread::get_id() << std::endl;

    // 独立线程运行测试，防止阻塞 Looper
    std::thread test_thread([&]{
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        run_all_tests();
    });

    dispatcher.execute(); // 阻塞在此
    
    if (test_thread.joinable()) test_thread.join();
    return 0;
}
