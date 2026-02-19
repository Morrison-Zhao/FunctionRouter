//
// Created by 赵子墨 on 2026/2/16.
//

#ifndef MYEVENTBUS_THREADSAFEQUEUE_HPP
#define MYEVENTBUS_THREADSAFEQUEUE_HPP


#include <deque>
#include <condition_variable>
#include <shared_mutex>



template <typename T, typename Container = std::deque<T> >
class ThreadSafeQueue {
public:
    ThreadSafeQueue() = default;


    ~ThreadSafeQueue() = default;

    
    template<typename ...Args>
    void emplace(Args &&...args) {
        std::unique_lock<std::shared_mutex> lock{mtx_};
        queue_.emplace_back(std::forward<Args>(args)...);
        cond_.notify_one();
    }


    void push(const T& val) {
        emplace(val);
    }


    void push(T && val) {
        emplace(std::move(val));
    }


    // 阻塞取数据
    T pop() {
        std::unique_lock<std::shared_mutex> lock(mtx_);
        cond_.wait(lock, [this] { return !queue_.empty(); });//如果队列不为空就继续执行，否则阻塞
        T ret{std::move_if_noexcept(queue_.front())};
        queue_.pop_front();
        return ret;
    }


    // 非阻塞取数据
    std::optional<T> try_pop() {
        std::unique_lock<std::shared_mutex> lock{mtx_};
        if (queue_.empty()) {
            return {};
        }
        std::optional<T> ret{std::move_if_noexcept(queue_.front())};
        queue_.pop_front();
        return ret;
    }


    // 获取大小
    int size() const {
        std::shared_lock<std::shared_mutex> lock{mtx_};
        return queue_.size();
    }


    // 判断是否为空
    bool empty() const {
        std::shared_lock<std::shared_mutex> lock{mtx_};
        return queue_.empty();
    }


    // 清空
    void clear() {
        std::unique_lock<std::shared_mutex> lock{mtx_};
        std::deque<T> empty;
        queue_.swap(empty);
    }


private:
    std::condition_variable_any cond_;
    mutable std::shared_mutex mtx_;
    Container queue_ = {};

};


#endif //MYEVENTBUS_THREADSAFEQUEUE_HPP
