//
// Created by 赵子墨 on 2026/2/17.
//

#ifndef MYEVENTBUS_EVENTHANDLER_HPP
#define MYEVENTBUS_EVENTHANDLER_HPP


#include <any>


// 事件处理器，存储函数
class EventHandlerBase {
public:
    virtual ~EventHandlerBase() = default;

    virtual void invoke(const std::vector<std::any>& args) = 0;
};



// 普通函数，无参
template <typename Func>
class EventHandlerWithoutArgs : public EventHandlerBase {
public:
    explicit EventHandlerWithoutArgs(Func&& func) : func_(std::forward<Func>(func)) {}

    void invoke(const std::vector<std::any>& args) override {
        invokeImpl();
    }

private:
    void invokeImpl() {
        func_();
    }

private:
    Func func_;

};



// 普通函数，有参
template <typename Func, typename ...Args>
class EventHandlerWithArgs : public EventHandlerBase {
public:
    explicit EventHandlerWithArgs(Func&& func) : func_(std::forward<Func>(func)) {}

    void invoke(const std::vector<std::any>& args) override {
        if (args.size() != sizeof...(Args)) {
            printf("[EventBus] [ERROR] args quantity mismatch, declared: %zu, invoked: %zu\n", sizeof...(Args), args.size());
            return;
        }

        invokeImpl(args, std::index_sequence_for<Args...>{});
    }

private:
    template <size_t... I>
    void invokeImpl(const std::vector<std::any>& args, std::index_sequence<I...>) {
        try {
//            func_(std::any_cast<
//                    typename std::remove_cv<
//                            typename std::remove_reference<Args...>::type
//                            >::type
//                    >(args[I]...));
            func_(std::any_cast<Args...>(args[I]...));
        } catch (const std::bad_any_cast &e) {
            printf("[EventBus] [ERROR] %s, check event params is match ?\n", e.what());
        }
    }

private:
    Func func_;

};



// 成员函数，无参
template <typename Class, typename Ret>
class EventHandlerWithoutArgsMember : public EventHandlerBase {
public:
    using MemberFunc = Ret(Class::*)();

    explicit EventHandlerWithoutArgsMember(Class* instance, MemberFunc func) : instance_(instance), func_(func) {
    }

    void invoke(const std::vector<std::any>& args) override {
        invokeImpl();
    }

private:
    void invokeImpl() {
        (instance_->*func_)();
    }

private:
    Class* instance_ = nullptr;
    MemberFunc func_;

};



// 成员函数，有参
template <typename Class, typename Ret, typename... Args>
class EventHandlerWithArgsMember : public EventHandlerBase {
public:
    using MemberFunc = Ret(Class::*)(Args...);

    explicit EventHandlerWithArgsMember(Class* instance, MemberFunc func) : instance_(instance), func_(func) {
    }

    void invoke(const std::vector<std::any>& args) override{
        if (args.size() != sizeof...(Args)) {
            printf("[EventBus] [ERROR] memberFunc args quantity mismatch, declared: %zu, invoked: %zu\n", sizeof...(Args), args.size());
            return;
        }

        invokeImpl(args, std::index_sequence_for<Args...>{});
    }

private:
    template <size_t... I>
    void invokeImpl(const std::vector<std::any>& args, std::index_sequence<I...>) {
//        (instance_->*func_)(std::any_cast<
//                                typename std::remove_cv<
//                                        typename std::remove_reference<Args...>::type
//                                        >::type
//                                >(args[I]...));

        (instance_->*func_)(std::any_cast<Args...>(args[I]...));
    }

private:
    Class* instance_ = nullptr;
    MemberFunc func_;

};



// 成员函数，无参const
template <typename Class, typename Ret>
class EventHandlerWithoutArgsMemberConst : public EventHandlerBase {
public:
    using MemberFunc = Ret(Class::*)() const;

    explicit EventHandlerWithoutArgsMemberConst(const Class* instance, MemberFunc func) : instance_(instance), func_(func) {
    }

    void invoke(const std::vector<std::any>& args) override {
        invokeImpl();
    }

private:
    void invokeImpl() {
        (instance_->*func_)();
    }

private:
    const Class* instance_ = nullptr;
    MemberFunc func_;

};



// 成员函数，有参const
template <typename Class, typename Ret, typename... Args>
class EventHandlerWithArgsMemberConst : public EventHandlerBase {
public:
    using MemberFunc = Ret(Class::*)(Args...) const;

    explicit EventHandlerWithArgsMemberConst(const Class* instance, MemberFunc func) : instance_(instance), func_(func) {
    }

    void invoke(const std::vector<std::any>& args) override {
        if (args.size() != sizeof...(Args)) {
            printf("[EventBus] [ERROR] memberFunc args quantity mismatch, declared: %zu, invoked: %zu\n", sizeof...(Args), args.size());
            return;
        }

        invokeImpl(args, std::index_sequence_for<Args...>{});
    }

private:
    template <size_t... I>
    void invokeImpl(const std::vector<std::any>& args, std::index_sequence<I...>) {
//        (instance_->*func_)(std::any_cast<
//                typename std::remove_cv<
//                        typename std::remove_reference<Args...>::type
//                >::type
//        >(args[I]...));
        (instance_->*func_)(std::any_cast<Args...>(args[I]...));
    }

private:
    const Class* instance_ = nullptr;
    MemberFunc func_;

};



#endif //MYEVENTBUS_EVENTHANDLER_HPP
