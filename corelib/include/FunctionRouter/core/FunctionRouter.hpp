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
 *  支持多种类型可调用对象， 有参、无参 | 成员、非成员
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

        std::thread route_thread = std::thread([=]() {
            routeLooper_->start([=](int64_t eventId) {
                auto wrapper = std::make_shared<RouteWrapper>();
                bool find = routeEvents_.find(eventId, wrapper);
                if (find) {
                    auto task = [wrapper] { wrapper->handler->invoke(wrapper->params.pop());};
                    if (wrapper->mode == ThreadMode::LOOP) {
                        task();

                        // 非持久化移除
                        if (!wrapper->persistent) {
                            routeEvents_.erase(eventId);
                        }
                    }
                }
            });
        });
        route_thread.detach();

        mainLooper_->start([=](int64_t eventId) {
            auto wrapper = std::make_shared<RouteWrapper>();
            bool find = mainEvents_.find(eventId, wrapper);
            if (find) {
                auto task = [wrapper] { wrapper->handler->invoke(wrapper->params.pop());};
                if (wrapper->mode == ThreadMode::MAIN) {
                    task();

                    // 非持久化移除
                    if (!wrapper->persistent) {
                        mainEvents_.erase(eventId);
                    }
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
    // 普通函数无参
    template<typename Signature>
    void registerEvent(int64_t eventId, ThreadMode mode,
                       std::function<Signature> func, bool persistent = true) {
        registerEventHelper(eventId, mode, func, (Signature *) nullptr, persistent);
    }


    template<typename Ret = void>
    void registerEventHelper(int64_t eventId, ThreadMode mode,
                             std::function<Ret()> func, Ret(*)(), bool persistent = true) {
        auto handler = std::make_shared<RouteHandlerWithoutArgs<std::function<Ret()>>>(std::move(func));
        registerEventWrapper(eventId, mode, handler, persistent);
    }


    // 普通函数有参
    template<typename Ret = void, typename... Args>
    void registerEventHelper(int64_t eventId, ThreadMode mode,
                             std::function<Ret(Args...)> func, Ret(*)(Args...), bool persistent = true) {
        auto handler = std::make_shared<RouteHandlerWithArgs<std::function<Ret(Args...)>, Args...>>(std::move(func));
        registerEventWrapper(eventId, mode, handler, persistent);
    }


    // 成员函数无参
    template<typename Class, typename Ret = void>
    void registerEvent(int64_t eventId, ThreadMode mode,
                       Class *instance, Ret(Class::*func)(), bool persistent = true) {
        auto handler = std::make_shared<RouteHandlerWithoutArgsMember<Class, Ret>>(instance, func);
        registerEventWrapper(eventId, mode, handler, persistent);
    }


    // 成员函数有参
    template<typename Class, typename Ret = void, typename... Args>
    void registerEvent(int64_t eventId, ThreadMode mode,
                       Class *instance, Ret(Class::*func)(Args...), bool persistent = true) {
        auto handler = std::make_shared<RouteHandlerWithArgsMember<Class, Ret, Args...>>(instance, func);
        registerEventWrapper(eventId, mode, handler, persistent);
    }


    // 成员函数无参const
    template<typename Class, typename Ret = void>
    void registerEvent(int64_t eventId, ThreadMode mode,
                       Class *instance, Ret(Class::*func)() const, bool persistent = true) {
        auto handler = std::make_shared<RouteHandlerWithoutArgsMemberConst<Class, Ret>>(instance, func);
        registerEventWrapper(eventId, mode, handler, persistent);
    }


    // 成员函数有参const
    template<typename Class, typename Ret = void, typename... Args>
    void registerEvent(int64_t eventId, ThreadMode mode,
                       Class *instance, Ret(Class::*func)(Args...) const, bool persistent = true) {
        auto handler = std::make_shared<RouteHandlerWithArgsMemberConst<Class, Ret, Args...>>(instance, func);
        registerEventWrapper(eventId, mode, handler, persistent);
    }


    // 注销任务
    int unregisterEvent(int64_t eventId) {
        bool result = routeEvents_.erase(eventId) > 0;
        result = mainEvents_.erase(eventId) > 0 || result;
        return result;
    }


    // 调用-id
    template<typename... Args>
    void postEvent(int64_t eventId, Args &&... args) {
        std::shared_ptr<RouteWrapper> wrapper;

        bool eventExists = false;
        eventExists = (routeEvents_.find(eventId, wrapper) || mainEvents_.find(eventId, wrapper));
        if (!eventExists) {
            return;
        }

        // 打包参数
        std::vector<std::any> anyArgs;
        anyArgs.reserve(sizeof...(Args));
        (anyArgs.push_back(std::make_any<typename std::remove_cv<typename std::remove_reference<Args>::type>::type>
                                   (std::forward<Args>(args))), ...);

        bool find = routeEvents_.find(eventId, wrapper);
        find = (find || mainEvents_.find(eventId, wrapper));
        if (find) {
            auto mode = wrapper->mode;
            wrapper->params.push(anyArgs);
            if (mode == ThreadMode::SYNC) {
                auto task = [wrapper]() { wrapper->handler->invoke(wrapper->params.pop()); };
                task();

                if (!wrapper->persistent) {
                    routeEvents_.erase(eventId);
                }
            } else if (mode == ThreadMode::ASYNC) {
                auto task = [wrapper]() { wrapper->handler->invoke(wrapper->params.pop()); };
                threadPool_->execute(task);

                if (!wrapper->persistent) {
                    routeEvents_.erase(eventId);
                }
            } else if (mode == ThreadMode::MAIN) {
                mainLooper_->push(eventId);
            } else {
                routeLooper_->push(eventId);
            }
        }
    }


    // 调用-直接
    void postEvent(std::function<void()> func, ThreadMode mode = ThreadMode::ASYNC) {
        if (mode == ThreadMode::SYNC) {
            func();
        } else if (mode == ThreadMode::ASYNC) {
            threadPool_->execute(func);
        } else {
            auto wrapper = std::make_shared<RouteWrapper>();
            wrapper->mode = mode;
            wrapper->handler = std::make_shared<RouteHandlerWithoutArgs<std::function<void()>>>(std::move(func));
            wrapper->persistent = false;
            wrapper->params.push({});


            // 生成临时ID (负数，避免与用户ID冲突)
            int64_t tempId = tempIdCounter_--;


            if (mode == ThreadMode::LOOP) {
                routeEvents_.insert(tempId, wrapper);
                routeLooper_->push(tempId);
            } else if (mode == ThreadMode::MAIN) {
                mainEvents_.insert(tempId, wrapper);
                mainLooper_->push(tempId);
            }
        }
    }

private:
    FunctionRouter() = default;


    void registerEventWrapper(int64_t eventId, ThreadMode mode,
                              const std::shared_ptr<RouteHandlerBase> &handler, bool persistent = true) {
        auto wrapper = std::make_shared<RouteWrapper>();
        wrapper->mode = mode;
        wrapper->handler = handler;
        wrapper->persistent = persistent;

        if (mode == ThreadMode::MAIN) {
            mainEvents_.insert(eventId, wrapper);
        } else {
            routeEvents_.insert(eventId, wrapper);
        }
    }

private:
    std::unique_ptr<ThreadPool> threadPool_ = std::make_unique<ThreadPool>();
    std::unique_ptr<RouteLooper> routeLooper_ = std::make_unique<RouteLooper>();
    std::unique_ptr<RouteLooper> mainLooper_ = std::make_unique<RouteLooper>();

    std::atomic<bool> isRunning_ = false;
    ThreadSafeMap<int64_t, std::shared_ptr<RouteWrapper>> routeEvents_ = {};
    ThreadSafeMap<int64_t, std::shared_ptr<RouteWrapper>> mainEvents_ = {};
    std::atomic<int64_t> tempIdCounter_ = -1;

};


#endif //FUNCTIONROUTER_FUNCTIONROUTER_HPP
