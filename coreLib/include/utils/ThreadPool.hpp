//
// Created by 赵子墨 on 2026/2/18.
//

#ifndef MYEVENTBUS_THREADPOOL_HPP
#define MYEVENTBUS_THREADPOOL_HPP



#include <vector>
#include <deque>
#include <mutex>
#include <thread>
#include <atomic>
#include <condition_variable>
#include <functional>
#include <future>
#include <memory>
#include <chrono>



class ThreadPool {
public:
    explicit ThreadPool() = default;

    ~ThreadPool() {
        stop();
    }

    // 启动线程池
    void start(size_t thread_nums = std::thread::hardware_concurrency()) {
        if (thread_nums == 0) {
            thread_nums = 1;
        }
        
        // 1. 先创建所有 Worker 对象，确保 workers_ 容器稳定
        workers_.reserve(thread_nums);
        for (size_t i = 0; i < thread_nums; ++i) {
            workers_.emplace_back(std::make_unique<Worker>(i, this));
        }
        
        isRunning_ = true;

        // 2. 再启动所有线程
        for (auto& worker : workers_) {
            worker->thread = std::thread([w = worker.get()] { w->run(); });
        }
    }

    // 停止线程池
    void stop() {
        bool expected = true;
        if (!isRunning_.compare_exchange_strong(expected, false)) {
            return; // 已经停止了
        }

        for (auto& worker : workers_) {
            {
                std::lock_guard<std::mutex> lock(worker->mtx);
                worker->running = false;
            }
            worker->cond.notify_one();
        }

        for (auto& worker : workers_) {
            if (worker->thread.joinable()) {
                worker->thread.join();
            }
        }
        
        workers_.clear();
    }

    template <typename F, typename... Args>
    void execute(F&& f, Args&&... args) {
        // 包装任务
        auto task = std::bind(std::forward<F>(f), std::forward<Args>(args)...);
        submit(std::make_shared<std::function<void()>>(task));
    }

    size_t size() const {
        return workers_.size();
    }

    // 统计所有队列的任务总数 (近似值，无锁读取)
    size_t taskNums() const {
        size_t total = 0;
        for (const auto& worker : workers_) {
            std::lock_guard<std::mutex> lock(worker->mtx);
            total += worker->tasks.size();
        }
        return total;
    }

    size_t activeNums() const {
        return activeThreads_.load(std::memory_order_relaxed);
    }

private:
    void submit(std::shared_ptr<std::function<void()>> task) {
        if (!isRunning_) {
            return;
        }

        // Round-Robin 分发策略：均匀分配任务到各个 Worker
        size_t idx = (submit_idx_++) % workers_.size();
        auto& worker = workers_[idx];

        {
            std::lock_guard<std::mutex> lock(worker->mtx);
            worker->tasks.emplace_back(std::move(task));
        }
        worker->cond.notify_one();
    }

    struct Worker {
        size_t id;
        ThreadPool* pool;
        std::thread thread;
        std::deque<std::shared_ptr<std::function<void()>>> tasks;
        mutable std::mutex mtx;
        std::condition_variable cond;
        bool running = true;

   Worker(size_t id, ThreadPool* pool) : id(id), pool(pool) {}

        // Worker 主循环
        void run() {
            printf("Worker %zu started\n", id);
            while (running) {
                std::shared_ptr<std::function<void()>> task;

                // 1. 尝试从自己的队列取任务
                {
                    std::unique_lock<std::mutex> lock(mtx);
                    
                    // 等待条件：有任务 或者 停止运行 (超时唤醒以尝试窃取任务)
                    cond.wait_for(lock, std::chrono::milliseconds(10), [this] {
                        return !tasks.empty() || !running;
                    });

                    if (!running && tasks.empty()) {
                        return;
                    }

                    if (!tasks.empty()) {
                        task = std::move(tasks.front());
                        tasks.pop_front();
                    }
                }

                // 2. 如果自己没任务，尝试窃取 (Work Stealing)
                if (!task) {
                    task = steal_task();
                }

                // 3. 执行任务
                if (task) {
                    pool->activeThreads_++;
                    (*task)();
                    pool->activeThreads_--;
                }
            }
        }

        std::shared_ptr<std::function<void()>> steal_task() const {
            for (size_t i = 0; i < pool->workers_.size(); ++i) {
                if (i == id) {
                    continue; // 不偷自己
                }

                auto& victim = pool->workers_[i];
                if (victim->tasks.empty()) {
                    continue; // 快速检查 (非线程安全但高效)
                }

                std::unique_lock<std::mutex> lock(victim->mtx, std::try_to_lock);
                if (lock.owns_lock() && !victim->tasks.empty()) {
                    // 偷走队尾的任务 (从后面偷，减少与 victim 线程从前面取任务的冲突)
                    auto task = std::move(victim->tasks.back());
                    victim->tasks.pop_back();
                    return task;
                }
            }
            return nullptr;
        }
    };

private:
    std::vector<std::unique_ptr<Worker>> workers_;
    std::atomic<bool> isRunning_ = false;
    std::atomic<size_t> submit_idx_ = 0; // 用于 Round-Robin 分发
    std::atomic<size_t> activeThreads_ = 0;

};

#endif //MYEVENTBUS_THREADPOOL_HPP
