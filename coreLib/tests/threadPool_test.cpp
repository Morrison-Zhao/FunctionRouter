#include "FunctionRouter/utils/ThreadPool.hpp"
#include <iostream>
#include <vector>
#include <chrono>
#include <cassert>
#include <atomic>
#include <cmath>

// 辅助函数：模拟耗时任务
int heavy_task(int id, int duration_ms) {
    std::this_thread::sleep_for(std::chrono::milliseconds(duration_ms));
    return id * id;
}

void test_basic_task() {
    std::cout << "[Test] Basic Task Submission..." << std::endl;
    ThreadPool pool;
    pool.start(2);

    pool.execute([]{ return 42; });
    pool.execute([](int x){ return x + 1; }, 10);
    pool.execute(heavy_task, 5, 100);

    std::cout << "[Pass] Basic Task Submission" << std::endl;
}

void test_fire_and_forget() {
    std::cout << "[Test] Fire-and-Forget Task..." << std::endl;
    ThreadPool pool;
    pool.start(4);
    
    std::atomic<int> counter{0};
    const int TARGET = 1000;
    
    for(int i=0; i<TARGET; ++i) {
        pool.execute([&counter]{
            counter++;
        });
    }
    

    int retry = 0;
    while(counter < TARGET && retry < 100) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        retry++;
    }
    
    assert(counter == TARGET);
    std::cout << "[Pass] Fire-and-Forget Task (" << TARGET << " tasks)" << std::endl;
}

void test_performance() {
    std::cout << "\n[Test] Performance Benchmark..." << std::endl;
    
    const int TASK_COUNT = 100000;
    const int THREAD_NUM = std::thread::hardware_concurrency();
    
    std::cout << "Tasks: " << TASK_COUNT << ", Threads: " << THREAD_NUM << std::endl;

    // 1. 轻量任务测试 (add_task with Future)
    {
        std::cout << "--- Light Tasks (add_task with Future) ---" << std::endl;
        ThreadPool pool;
        pool.start(THREAD_NUM);
        

        auto start = std::chrono::high_resolution_clock::now();
        
        for(int i=0; i<TASK_COUNT; ++i) {
           pool.execute([i]{
                int val = i * i;
                (void)val;
            });
        }
        

        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
        
        std::cout << "Time: " << duration << " ms" << std::endl;
        std::cout << "Throughput: " << (double)TASK_COUNT / duration * 1000 << " tasks/sec" << std::endl;
    }

    // 2. 轻量任务测试 (execute Fire-and-forget)
    {
        std::cout << "\n--- Light Tasks (execute Fire-and-forget) ---" << std::endl;
        ThreadPool pool;
        pool.start(THREAD_NUM);
        
        std::atomic<int> completed{0};
        auto start = std::chrono::high_resolution_clock::now();
        
        for(int i=0; i<TASK_COUNT; ++i) {
            pool.execute([&completed]{
                completed++;
            });
        }
        
        // 等待完成
        while(completed < TASK_COUNT) {
            std::this_thread::yield();
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
        
        std::cout << "Time: " << duration << " ms" << std::endl;
        std::cout << "Throughput: " << (double)TASK_COUNT / duration * 1000 << " tasks/sec" << std::endl;
        std::cout << "Note: Fire-and-forget should be faster due to no Future overhead." << std::endl;
    }

    // 3. 重型任务测试
    {
        std::cout << "\n--- Heavy Tasks (Parallel Speedup Test) ---" << std::endl;
        const int HEAVY_TASK_COUNT = 1000; // Fewer tasks
        
        auto heavy_calc = [](int n) {
            double result = 0;
            for(int i=0; i<200000; ++i) result += std::sqrt(n + i); // More work per task
            return result;
        };

        // A. 串行基准
        volatile double serial_total = 0;
        auto start_serial = std::chrono::high_resolution_clock::now();
        for(int i=0; i<HEAVY_TASK_COUNT; ++i) {
            serial_total = serial_total + heavy_calc(i);
        }
        auto end_serial = std::chrono::high_resolution_clock::now();
        auto serial_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end_serial - start_serial).count();
        std::cout << "Serial Time: " << serial_ms << " ms (Check: " << serial_total << ")" << std::endl;

        // B. 并行测试
        ThreadPool pool;
        pool.start(THREAD_NUM);
        std::atomic<int> completed{0};
        
        auto start_pool = std::chrono::high_resolution_clock::now();
        for(int i=0; i<HEAVY_TASK_COUNT; ++i) {
            pool.execute([&completed, i, heavy_calc] {
                volatile double val = heavy_calc(i);
                (void)val;
                completed.fetch_add(1, std::memory_order_relaxed);
            });
        }
        
        while(completed < HEAVY_TASK_COUNT) {
            std::this_thread::yield();
        }
        
        auto end_pool = std::chrono::high_resolution_clock::now();
        
        auto pool_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end_pool - start_pool).count();
        std::cout << "ThreadPool Time: " << pool_ms << " ms" << std::endl;
        
        if (pool_ms > 0) {
            double speedup = (double)serial_ms / pool_ms;
            std::cout << "Speedup: " << speedup << "x" << std::endl;
        }
    }
}

int main() {
    try {
        test_basic_task();
        test_fire_and_forget();
        test_performance();
        
        std::cout << "\nAll tests passed!" << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Test failed with exception: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}
