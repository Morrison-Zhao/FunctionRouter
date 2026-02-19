//
// Created by 赵子墨 on 2026/2/18.
//

#ifndef FUNCTIONROUTER_ROUTEWRAPPER_HPP
#define FUNCTIONROUTER_ROUTEWRAPPER_HPP


#include "RouteHandler.hpp"
#include "ThreadMode.hpp"
#include "../container/ThreadSafeQueue.hpp"



class RouteWrapper {
public:
    ThreadMode mode;
    std::shared_ptr<RouteHandlerBase> handler = nullptr;
    ThreadSafeQueue<std::vector<std::any>> params = {};
};

#endif //FUNCTIONROUTER_ROUTEWRAPPER_HPP
