//
// Created by 赵子墨 on 2026/2/18.
//

#ifndef FUNCTIONROUTER_FUNCTIONROUTER_HPP
#define FUNCTIONROUTER_FUNCTIONROUTER_HPP


#include "RouteWrapper.hpp"
#include "RouteLooper.hpp"


#include "../container/ThreadSafeMap.hpp"
#include "../container/ThreadSafeQueue.hpp"
#include "../templates/Singleton.hpp"
#include "../utils/ThreadPool.hpp"


/**
 *  实现事件总线调度器，跨组件、解耦模块间交互
 *  支持四种调度方式，当前线程、主线程、线程池、子线程
 *  支持多种类型函数， 有参、无参 | 成员、非成员
 *
 *  使用方法：使用统一的方法注册，后续只需要根据id和参数直接跨组件调用
 */
class FunctionRouter : public Singleton<FunctionRouter> {
    friend class Singleton<FunctionRouter>;

public:
    void execute() {
        if (isRunning_) {
            return;
        }
        threadPool_->start();
        isRunning_ = true;

        std::thread dispatcher_thread = std::thread([=]() {
            routeLooper_->start([=](int64_t eventId) {
                auto wrapper = std::make_shared<RouteWrapper>();
                bool find = dispatchEvents_.find(eventId, wrapper);
                if (find) {
                    auto task = [wrapper] {
                        wrapper->handler->invoke(wrapper->params.pop());
                    };

                    if (wrapper->mode == ThreadMode::LOOP) {
                        task();
                    }
//                    else if (wrapper->mode == ThreadMode::ASYNC) {
//                        threadPool_->execute(task);
//                    }
                }
            });
        });
        dispatcher_thread.detach();

        mainLooper_->start([=](int64_t eventId) {
            auto wrapper = std::make_shared<RouteWrapper>();
            bool find = mainEvents_.find(eventId, wrapper);

            if (find) {
                auto task = [wrapper] {
                    wrapper->handler->invoke(wrapper->params.pop());
                };

                if (wrapper->mode == ThreadMode::MAIN) {
                    task();
                }
            }
        });
        printf("execute quit.\n");
    }

    void quit() {
        if (!isRunning_) {
            return;
        }

        if (routeLooper_->isLooping()) {
            routeLooper_->stop();
        }

        if (mainLooper_->isLooping()) {
            mainLooper_->stop();
        }
    }


public:
    template<typename Signature>
    void registerEvent(int64_t eventId, ThreadMode mode, std::function<Signature> func) {
        registerEventHelper(eventId, mode, func, (Signature *) nullptr);
    }


    template<typename Ret>
    void registerEventHelper(int64_t eventId, ThreadMode mode, std::function<Ret()> func, Ret(*)()) {
        auto handler = std::make_shared<RouteHandlerWithoutArgs<std::function<Ret()>>>(std::move(func));
        registerRouteWrapper(eventId, mode, handler);
    }


    template<typename Ret, typename... Args>
    void registerEventHelper(int64_t eventId, ThreadMode mode, std::function<Ret(Args...)> func, Ret(*)(Args...)) {
        auto handler = std::make_shared<RouteHandlerWithArgs<std::function<Ret(Args...)>, Args...>>(std::move(func));
        registerRouteWrapper(eventId, mode, handler);
    }


    template<typename Class, typename Ret>
    void registerEvent(int64_t eventId, ThreadMode mode, Class *instance, Ret(Class::*func)()) {
        auto handler = std::make_shared<RouteHandlerWithoutArgsMember<Class, Ret>>(instance, func);
        registerRouteWrapper(eventId, mode, handler);
    }


    template<typename Class, typename Ret, typename... Args>
    void registerEvent(int64_t eventId, ThreadMode mode, Class *instance, Ret(Class::*func)(Args...)) {
        auto handler = std::make_shared<RouteHandlerWithArgsMember<Class, Ret, Args...>>(instance, func);
        registerRouteWrapper(eventId, mode, handler);
    }


    template<typename Class, typename Ret>
    void registerEvent(int64_t eventId, ThreadMode mode, Class *instance, Ret(Class::*func)()) const {
        auto handler = std::make_shared<RouteHandlerWithoutArgsMemberConst<Class, Ret>>(instance, func);
        registerRouteWrapper(eventId, mode, handler);
    }


    template<typename Class, typename Ret, typename... Args>
    void registerEvent(int64_t eventId, ThreadMode mode, Class *instance, Ret(Class::*func)(Args...) const) {
        auto handler = std::make_shared<RouteHandlerWithArgsMemberConst<Class, Ret, Args...>>(instance, func);
        registerRouteWrapper(eventId, mode, handler);
    }


    int unregisterEvent(int64_t eventId) {
        bool result = dispatchEvents_.erase(eventId) > 0;
        result = mainEvents_.erase(eventId) > 0 || result;
        return result;
    }


    template<typename... Args>
    void postEvent(int64_t eventId, Args &&... args) {
        // 避免不必要的参数打包
        std::shared_ptr<RouteWrapper> wrapper;
        bool eventExists = false;
        eventExists = (dispatchEvents_.find(eventId, wrapper) || mainEvents_.find(eventId, wrapper));

        if (!eventExists) {
            return;
        }

        // 打包参数
        std::vector<std::any> anyArgs;
        anyArgs.reserve(sizeof...(Args));
        (anyArgs.push_back(std::make_any<typename std::remove_cv<typename std::remove_reference<Args>::type>::type>
                                   (std::forward<Args>(args))), ...);

        bool find = dispatchEvents_.find(eventId, wrapper);
        find = (find || mainEvents_.find(eventId, wrapper));
        if (find) {
            auto mode = wrapper->mode;
            wrapper->params.push(anyArgs);
            // 当前线程调用
            if (mode == ThreadMode::SYNC) {
                auto task = [wrapper]() { wrapper->handler->invoke(wrapper->params.pop()); };
                task();
            } else if (mode == ThreadMode::MAIN) {
                mainLooper_->push(eventId);
            } else if (mode == ThreadMode::ASYNC) {
                auto task = [wrapper] {
                    wrapper->handler->invoke(wrapper->params.pop());
                };
                threadPool_->execute(task);
            } else {
                routeLooper_->push(eventId);
            }
        }
    }


private:
    FunctionRouter() = default;

    void registerRouteWrapper(int64_t eventId, ThreadMode mode, const std::shared_ptr<RouteHandlerBase> &handler) {
        auto wrapper = std::make_shared<RouteWrapper>();
        wrapper->mode = mode;
        wrapper->handler = handler;

        if (mode == ThreadMode::MAIN) {
            mainEvents_.insert(eventId, wrapper);
        } else {
            dispatchEvents_.insert(eventId, wrapper);
        }
    }

private:
    std::unique_ptr<ThreadPool> threadPool_ = std::make_unique<ThreadPool>();
    std::unique_ptr<RouteLooper> routeLooper_ = std::make_unique<RouteLooper>();
    std::unique_ptr<RouteLooper> mainLooper_ = std::make_unique<RouteLooper>();

    std::atomic<bool> isRunning_ = false;
    ThreadSafeMap<int64_t, std::shared_ptr<RouteWrapper>> dispatchEvents_ = {};
    ThreadSafeMap<int64_t, std::shared_ptr<RouteWrapper>> mainEvents_ = {};
};










#endif //FUNCTIONROUTER_FUNCTIONROUTER_HPP
