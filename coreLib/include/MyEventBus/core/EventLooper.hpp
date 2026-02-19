//
// Created by 赵子墨 on 2026/2/18.
//

#ifndef MYEVENTBUS_EVENTLOOPER_HPP
#define MYEVENTBUS_EVENTLOOPER_HPP


#include "../container/ThreadSafeQueue.hpp"

#define STOP_ID 202602180118



class EventLooper {
public:
    EventLooper() = default;

    ~EventLooper() = default;

    void push(const int64_t& event_id) {
        queue_.emplace(event_id);
    }

    void start(const std::function<void(int64_t)>& cb) {
        if (isLooping_) {
            return;
        }
        isLooping_ = true;

        while(true) {
            int64_t event_id = queue_.pop();
            if (event_id == STOP_ID) {
                break;
            }
            cb(event_id);
        }
    }

    void stop() {
        if (!isLooping_) {
            return;
        }
        queue_.clear();
        queue_.emplace(STOP_ID);
    }

    bool isLooping() const {
        return isLooping_;
    }

private:
    bool isLooping_ = false;
    ThreadSafeQueue<int64_t> queue_ = {};

};

#endif //MYEVENTBUS_EVENTLOOPER_HPP
