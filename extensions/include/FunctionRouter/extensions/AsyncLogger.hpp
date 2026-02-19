#pragma once
#include <string>
#include <memory>
#include <thread>
#include <vector>
#include <mutex>
#include <condition_variable>
#include <atomic>

namespace FunctionRouter {
namespace Extensions {

/**
 * @brief 一个异步日志记录器模块
 * 展示了如何将实现细节隐藏在 .cpp 中，只暴露必要的接口
 */
class AsyncLogger {
public:
    AsyncLogger();
    ~AsyncLogger();

    // 禁用拷贝，防止资源管理混乱
    AsyncLogger(const AsyncLogger&) = delete;
    AsyncLogger& operator=(const AsyncLogger&) = delete;

    /**
     * @brief 启动日志线程
     */
    void start();

    /**
     * @brief 停止日志线程
     */
    void stop();

    /**
     * @brief 提交日志（非阻塞）
     */
    void log(const std::string& message);

private:
    // Pimpl 模式或者直接私有成员
    struct Impl;
    std::unique_ptr<Impl> pImpl_; 
};

} // namespace Extensions
} // namespace FunctionRouter
