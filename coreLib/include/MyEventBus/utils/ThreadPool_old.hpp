//
// Created by 赵子墨 on 2025/4/8.
//

#ifndef MYEVENTLOOPER_THREADPOOL_HPP
#define MYEVENTLOOPER_THREADPOOL_HPP

#include <vector>
#include <queue>
#include <memory>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <future>
#include <functional>
#include <stdexcept>
#include <atomic>
#include <type_traits>


class ThreadPool {
public:
    explicit ThreadPool() = default;

    ~ThreadPool() {
        {
            std::unique_lock<std::mutex> lock(mtx_);
            isRunning_ = false;
        }
        cond_.notify_all();
        for (auto& worker: workers_) { worker.join(); }
    }

    template<typename F, typename ...Args>
    auto addTask(F &&f, Args &&... args) -> std::future<typename std::invoke_result<F, Args...>::type> {
        using return_type = typename std::invoke_result<F, Args...>::type;

        auto task = std::make_shared<std::packaged_task<return_type()>>(
                std::bind(std::forward<F>(f), std::forward<Args>(args)...));

        auto result = task->get_future();

        {
            std::unique_lock<std::mutex> lock(mtx_);
            if (!isRunning_) { throw std::runtime_error("error threadPool stopped\n");}
            tasks_.emplace([task]{ (*task)();});
        }

        cond_.notify_one();
        return result;

    }

    size_t size() const {
        return workers_.size();
    }

    size_t taskNums() const {
        std::unique_lock<std::mutex> lock(mtx_);
        return tasks_.size();
    }


    void start(size_t thread_nums = std::thread::hardware_concurrency()) {
        {
            std::unique_lock<std::mutex> lock(mtx_);
            for (int i = 0; i < thread_nums; ++i) { workers_.emplace_back([this]{ worker_thread(); });}
            isRunning_ = true;
        }
    }

    void stop() {
        {
            std::unique_lock<std::mutex> lock(mtx_);
            for (auto&& worker : workers_) {
                worker.join();
            }
            workers_.clear();
            isRunning_ = false;
        }
    }

private:
    void worker_thread() {
        while (true) {
            std::function<void()> task;

            {
                std::unique_lock<std::mutex> lock(mtx_);
                cond_.wait(lock, [this]{ return (!this->isRunning_) || (!this->tasks_.empty());});
                if (!isRunning_ && tasks_.empty()) { return; }
                task = std::move(tasks_.front());
                tasks_.pop();
            }

            activeThreads_++;
            task();
            activeThreads_--;
        }
    }

    std::vector<std::thread> workers_ = {};
    std::queue<std::function<void()> > tasks_ = {};
    mutable std::mutex mtx_;
    std::condition_variable cond_;
    std::atomic<bool> isRunning_ = true;
    std::atomic<int> activeThreads_ = 0;
};

#endif //MYEVENTLOOPER_THREADPOOL_HPP
