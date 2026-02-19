#include "MyEventBus/extensions/AsyncLogger.hpp"
#include <iostream>
#include <deque>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <atomic>

namespace MyEventBus {
namespace Extensions {

struct AsyncLogger::Impl {
    std::deque<std::string> queue_;
    std::mutex mtx_;
    std::condition_variable cv_;
    std::thread thread_;
    std::atomic<bool> running_{false};

    void worker_thread() {
        while (true) {
            std::unique_lock<std::mutex> lock(mtx_);
            cv_.wait(lock, [this] { return !queue_.empty() || !running_; });

            if (!running_ && queue_.empty()) {
                return;
            }

            std::deque<std::string> batch;
            batch.swap(queue_);
            lock.unlock();

            for (const auto& msg : batch) {
                std::cout << "[AsyncLogger] " << msg << std::endl;
            }
        }
    }
};

AsyncLogger::AsyncLogger() : pImpl_(std::make_unique<Impl>()) {}

AsyncLogger::~AsyncLogger() {
    stop();
}

void AsyncLogger::start() {
    if (pImpl_->running_) return;
    pImpl_->running_ = true;
    pImpl_->thread_ = std::thread(&Impl::worker_thread, pImpl_.get());
}

void AsyncLogger::stop() {
    if (!pImpl_->running_) return;
    
    {
        std::lock_guard<std::mutex> lock(pImpl_->mtx_);
        pImpl_->running_ = false;
    }
    pImpl_->cv_.notify_one();
    
    if (pImpl_->thread_.joinable()) {
        pImpl_->thread_.join();
    }
}

void AsyncLogger::log(const std::string& message) {
    if (!pImpl_->running_) return;
    
    {
        std::lock_guard<std::mutex> lock(pImpl_->mtx_);
        pImpl_->queue_.push_back(message);
    }
    pImpl_->cv_.notify_one();
}

} // namespace Extensions
} // namespace MyEventBus
