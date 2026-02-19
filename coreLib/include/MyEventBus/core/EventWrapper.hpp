//
// Created by 赵子墨 on 2026/2/18.
//

#ifndef MYEVENTBUS_EVENTWRAPPER_HPP
#define MYEVENTBUS_EVENTWRAPPER_HPP


#include "EventHandler.hpp"
#include "ThreadMode.hpp"
#include "../container/ThreadSafeQueue.hpp"



class EventWrapper {
public:
    ThreadMode mode;
    std::shared_ptr<EventHandlerBase> handler = nullptr;
    ThreadSafeQueue<std::vector<std::any>> params = {};
};

#endif //MYEVENTBUS_EVENTWRAPPER_HPP
