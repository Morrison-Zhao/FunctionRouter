#include "FunctionRouter/core/FunctionRouter.hpp"
#include <iostream>
#include <thread>
#include <string>
#include <cassert>
#include <atomic>


struct ComplexData {
    int id;
    std::string payload;
};


enum EventID {
    // 基础功能测试

    // ------------------------------ 可调用对象兼容性 --------------------------------
    FREE_FUNC_WITHOUT_ARGS = 100000,
    FREE_FUNC_WITH_ARGS,
    FREE_FUNC_WITH_ARGS_REF,
    FREE_FUNC_STATIC,

    LAMBDA_WITHOUT_ARGS,
    LAMBDA_FUNC_WITH_ARGS,
    LAMBDA_FUNC_WITH_ARGS_REF,

    MEMBER_FUNC_WITHOUT_ARGS ,
    MEMBER_FUNC_WITHOUT_ARGS_CONST,
    MEMBER_FUNC_WITH_ARGS,
    MEMBER_FUNC_WITH_ARGS_REF,
    MEMBER_FUNC_WITH_ARGS_CONST,

    // ------------------------------ 功能测试 --------------------------------
    UNREGISTER_TEST,

    PERSISTENT_TEST,

    // ------------------------------ 性能测试 ---------------------------------
    PERF_SYNC,
    PERF_ASYNC,
    PERF_MAIN,
    PERF_LOOP,
};


void free_without_args() {
    std::cout << "[Pass] Free Function Without Args" << std::endl;
}

int free_with_args(const int& num, const std::string& str)  {
    std::cout << "[Pass] Free Function With Args" << std::endl;
    return num;
}

void free_with_args_ref(int& num)  {
    std::cout << "[Pass] Free Function With Args Ref (Note: value semantics)" << std::endl;
    num += 1;
}

static void static_without_args() {
    std::cout << "[Pass] Static Function." << std::endl;
}



class MemberTest {
public:
    void member_test_without_args() {
        std::cout << name_ << " [Pass] member without Args" << std::endl;
    }

    int member_test_with_args(const int& num, const std::string& str) {
        std::cout << name_ << " [Pass] member with Args " << " num: " << num << " str: " << str << std::endl;
        return num;
    }

    int member_test_without_args_const() const {
        std::cout << name_ << " [Pass] member without Args const " << "size: " << size_ << std::endl;
        return size_;
    }

    void member_test_with_args_const(const int& num, const std::string& str) const {
        std::cout << name_ << " [Pass] member with Args const " << " num: " << num << " str: " << str << std::endl;
    }

    void member_test_with_args_ref(int& num) {
        num += 1;
        std::cout << name_ << " [Pass] member with Args const " << " num: " << num << std::endl;
    }

private:
    std::string name_ = "TEST_MEMBER";
    int size_ = 0;
};



void TEST_FUNCTIONS() {
    std::cout << "\n ================ Functional Test ==================" << std::endl;
    auto& router = FunctionRouter::getInstance();

    // change mode here
    ThreadMode mode = ThreadMode::ASYNC;
    std::string current_test_mode;
    if (mode == ThreadMode::MAIN) {
        current_test_mode = "MAIN";
    } else if (mode == ThreadMode::SYNC) {
        current_test_mode = "SYNC";
    } else if (mode == ThreadMode::ASYNC) {
        current_test_mode = "ASYNC";
    } else if (mode == ThreadMode::LOOP) {
        current_test_mode = "LOOP";
    }

    std::cout << "current test mode: " << current_test_mode << std::endl;

    // 普通函数，无参
    router.registerEvent<void()>(FREE_FUNC_WITHOUT_ARGS, mode, &free_without_args);
    router.postEvent(FREE_FUNC_WITHOUT_ARGS);

    // 普通函数，有参
    router.registerEvent<int(const int&, const std::string&)>(FREE_FUNC_WITH_ARGS, mode, &free_with_args);
    router.postEvent(FREE_FUNC_WITH_ARGS, 1, std::string("test"));

    // 普通函数，有参，左值引用
    int test_num = 0;
    router.registerEvent<void(int&)>(FREE_FUNC_WITH_ARGS_REF, mode, &free_with_args_ref);
    router.postEvent(FREE_FUNC_WITH_ARGS_REF, std::ref(test_num));
    std::cout << "new test_num: " << test_num;


    // 静态函数
    router.registerEvent<void()>(FREE_FUNC_STATIC, mode, &static_without_args);
    router.postEvent(FREE_FUNC_STATIC);

    // lambda, 无参
    router.registerEvent<void()>(LAMBDA_WITHOUT_ARGS, mode, []{
        std::cout << "[Pass] Lambda Without Args" << std::endl;
    });
    router.postEvent(LAMBDA_WITHOUT_ARGS);

    // lambda, 有参
    router.registerEvent<void(const int& val)>(LAMBDA_FUNC_WITH_ARGS, mode, [](const int& val){
        std::cout << "[Pass] Lambda With Args" << std::endl;
    });
    router.postEvent(LAMBDA_FUNC_WITH_ARGS, 1);

    // lambda，有参左值引用
    router.registerEvent<void(int&)>(LAMBDA_FUNC_WITH_ARGS_REF, mode, [](int& val){
        std::cout << "[Pass] lambda With Args Ref." << "test_val: " << val << std::endl;
        val += 1;
    });
    int test_val = 0;
    router.postEvent(LAMBDA_FUNC_WITH_ARGS_REF, test_val);


    MemberTest member;

    // 成员函数，无参
    router.registerEvent<MemberTest>(MEMBER_FUNC_WITHOUT_ARGS, mode, &member, &MemberTest::member_test_without_args);
    router.postEvent(MEMBER_FUNC_WITHOUT_ARGS);

    // 成员函数，有参
    router.registerEvent<MemberTest, int, const int&, const std::string&>(MEMBER_FUNC_WITH_ARGS, mode, &member, &MemberTest::member_test_with_args);
    router.postEvent(MEMBER_FUNC_WITH_ARGS, 2, std::string("test_member1"));

    // 成员函数，无参const
    router.registerEvent<MemberTest>(MEMBER_FUNC_WITHOUT_ARGS_CONST, mode, &member, &MemberTest::member_test_without_args_const);
    router.postEvent(MEMBER_FUNC_WITHOUT_ARGS_CONST);

    // 成员函数，有参const
    router.registerEvent<MemberTest, void, const int&, const std::string&>(MEMBER_FUNC_WITH_ARGS_CONST, mode, &member, &MemberTest::member_test_with_args_const);
    router.postEvent(MEMBER_FUNC_WITH_ARGS_CONST, 3, std::string("test_member2"));

    // 成员函数，有参，左值引用
    router.registerEvent<MemberTest, void, int&>(MEMBER_FUNC_WITH_ARGS_REF, mode, &member, &MemberTest::member_test_with_args_ref);
    router.postEvent(MEMBER_FUNC_WITH_ARGS_REF, 3, std::string("test_member2"));
    std::cout << "You Can find a mistake here. line 158" << std::endl;

    // 注销测试
    std::atomic<int> call_count{0};
    router.registerEvent<void()>(UNREGISTER_TEST, ThreadMode::SYNC, [&] { call_count++; });
    router.postEvent(UNREGISTER_TEST);
    assert(call_count == 1);

    router.unregisterEvent(UNREGISTER_TEST);
    router.postEvent(UNREGISTER_TEST);
    assert(call_count == 1);
    std::cout << "[Pass] Unregister Event" << std::endl;


    // 持久化测试
    call_count = 0;
    router.registerEvent<void()>(PERSISTENT_TEST, ThreadMode::SYNC, [&] { call_count++; }, false);
    router.postEvent(PERSISTENT_TEST);

    assert(call_count == 1);
    router.postEvent(PERSISTENT_TEST);
    assert(call_count == 1);
    std::cout << "[Pass] Persistent Event" << std::endl;
}


void TEST_DIRECT() {
    std::cout << "\n========== Direct Test ==========" << std::endl;
    auto &router = FunctionRouter::getInstance();

    // 1. SYNC
    bool sync_called = false;
    router.postEvent([&] { sync_called = true; }, ThreadMode::SYNC);
    assert(sync_called);
    std::cout << "[Pass] Direct Post SYNC" << std::endl;

    // 2. ASYNC
    std::atomic<bool> async_called{false};
    router.postEvent([&] { async_called = true; }, ThreadMode::ASYNC);
    while (!async_called) std::this_thread::yield();
    std::cout << "[Pass] Direct Post ASYNC" << std::endl;

    // 3. LOOP (子线程)
    std::atomic<bool> loop_called{false};
    std::thread::id loop_thread_id;
    router.postEvent([&] {
        loop_called = true;
        loop_thread_id = std::this_thread::get_id();
        // std::cout << "[Debug] LOOP task executed" << std::endl;
    }, ThreadMode::LOOP);

    int timeout = 0;
    while (!loop_called && timeout++ < 1000) std::this_thread::sleep_for(std::chrono::milliseconds(1));
    if (!loop_called) std::cout << "[Fail] Direct Post LOOP Timeout" << std::endl;
    else {
        assert(loop_thread_id != std::this_thread::get_id());
        std::cout << "[Pass] Direct Post LOOP" << std::endl;
    }

    // 4. MAIN (主线程)
    std::atomic<bool> main_called{false};
    router.postEvent([&] { main_called = true; }, ThreadMode::MAIN);

    timeout = 0;
    while (!main_called && timeout++ < 1000) std::this_thread::sleep_for(std::chrono::milliseconds(1));
    if (!main_called) std::cout << "[Fail] Direct Post MAIN Timeout" << std::endl;
    else std::cout << "[Pass] Direct Post MAIN" << std::endl;

    // 5. 带参数 (通过 lambda 捕获)
    int val = 123;
    std::atomic<int> result{0};
    router.postEvent([=, &result] { result = val * 2; }, ThreadMode::ASYNC);
    while (result == 0) std::this_thread::yield();
    assert(result == 246);
    std::cout << "[Pass] Direct Post with Capture Args" << std::endl;
}


void TEST_PERF() {
    std::cout << "\n========== Performance Tests ==========" << std::endl;
    auto& router = FunctionRouter::getInstance();
    const int COUNT = 100000;

    // SYNC 模式
    {
        std::atomic<int> counter{0};
        router.registerEvent<void()>(PERF_SYNC, ThreadMode::SYNC, [&]{ counter++; });

        auto start = std::chrono::high_resolution_clock::now();
        for(int i=0; i<COUNT; ++i) router.postEvent(PERF_SYNC);
        auto end = std::chrono::high_resolution_clock::now();

        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
        std::cout << "SYNC  Mode: " << COUNT << " events in " << ms << " ms ("
                  << (double)COUNT/ms*1000 << " ops/sec)" << std::endl;
    }

    // ASYNC 模式
    {
        std::atomic<int> counter{0};
        router.registerEvent<void()>(PERF_ASYNC, ThreadMode::ASYNC, [&]{ counter++; });

        auto start = std::chrono::high_resolution_clock::now();
        for(int i=0; i<COUNT; ++i) router.postEvent(PERF_ASYNC);

        // 等待完成
        while(counter < COUNT) std::this_thread::sleep_for(std::chrono::milliseconds(1));

        auto end = std::chrono::high_resolution_clock::now();
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
        std::cout << "ASYNC Mode: " << COUNT << " events in " << ms << " ms ("
                  << (double)COUNT/ms*1000 << " ops/sec)" << std::endl;
    }

    // MAIN 模式
    {
        std::atomic<int> counter{0};
        router.registerEvent<void()>(PERF_MAIN, ThreadMode::MAIN, [&]{ counter++; });

        auto start = std::chrono::high_resolution_clock::now();
        for(int i=0; i<COUNT; ++i) router.postEvent(PERF_MAIN);

        // 等待完成
        while(counter < COUNT) std::this_thread::sleep_for(std::chrono::milliseconds(1));

        auto end = std::chrono::high_resolution_clock::now();
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
        std::cout << "MAIN Mode: " << COUNT << " events in " << ms << " ms ("
                  << (double)COUNT/ms*1000 << " ops/sec)" << std::endl;
    }

    // LOOP 模式
    {
        std::atomic<int> counter{0};
        router.registerEvent<void()>(PERF_LOOP, ThreadMode::LOOP, [&]{ counter++; });

        auto start = std::chrono::high_resolution_clock::now();
        for(int i=0; i<COUNT; ++i) router.postEvent(PERF_LOOP);

        // 等待完成
        while(counter < COUNT) std::this_thread::sleep_for(std::chrono::milliseconds(1));

        auto end = std::chrono::high_resolution_clock::now();
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
        std::cout << "LOOP Mode: " << COUNT << " events in " << ms << " ms ("
                  << (double)COUNT/ms*1000 << " ops/sec)" << std::endl;
    }

}


void run_all_tests() {
    TEST_FUNCTIONS();
    TEST_DIRECT();
    TEST_PERF();

    std::cout << "\n>>> All tests finished. Quitting..." << std::endl;
    std::this_thread::sleep_for(std::chrono::milliseconds(200)); // 让日志飞一会儿
    FunctionRouter::getInstance().quit();
}

int main() {
    auto& dispatcher = FunctionRouter::getInstance();
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
