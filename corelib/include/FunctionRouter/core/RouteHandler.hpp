//
// Created by 赵子墨 on 2026/2/17.
//

#ifndef FUNCTIONROUTER_ROUTEHANDLER_HPP
#define FUNCTIONROUTER_ROUTEHANDLER_HPP


#include <any>
#include <tuple>


// 事件处理器，存储函数
class RouteHandlerBase {
public:
    virtual ~RouteHandlerBase() = default;

    virtual void invoke(std::vector<std::any> args) = 0;
};



// 普通函数，无参
template <typename Func>
class RouteHandlerWithoutArgs : public RouteHandlerBase {
public:
    explicit RouteHandlerWithoutArgs(Func&& func) : func_(std::forward<Func>(func)) {}

    void invoke(std::vector<std::any> args) override {
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
class RouteHandlerWithArgs : public RouteHandlerBase {
public:
    explicit RouteHandlerWithArgs(Func&& func) : func_(std::forward<Func>(func)) {}

    void invoke(std::vector<std::any> args) override {
        if (args.size() != sizeof...(Args)) {
            printf("[FunctionRouter] [ERROR] args quantity mismatch, declared: %zu, invoked: %zu\n", sizeof...(Args), args.size());
            return;
        }

        invokeImpl(args, std::index_sequence_for<Args...>{});
    }

private:
    using ArgsTuple = std::tuple<Args...>;

    template <size_t... I>
    void invokeImpl(std::vector<std::any>& args, std::index_sequence<I...>) {
        try {
            func_(std::any_cast<typename std::tuple_element<I, ArgsTuple>::type>(args[I])...);
        } catch (const std::bad_any_cast &e) {
            printf("[FunctionRouter] [ERROR] %s, check event params is match ?\n", e.what());
        }
    }

private:
    Func func_;

};



// 成员函数，无参
template <typename Class, typename Ret>
class RouteHandlerWithoutArgsMember : public RouteHandlerBase {
public:
    using MemberFunc = Ret(Class::*)();

    explicit RouteHandlerWithoutArgsMember(Class* instance, MemberFunc func) : instance_(instance), func_(func) {
    }

    void invoke(std::vector<std::any> args) override {
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
class RouteHandlerWithArgsMember : public RouteHandlerBase {
public:
    using MemberFunc = Ret(Class::*)(Args...);

    explicit RouteHandlerWithArgsMember(Class* instance, MemberFunc func) : instance_(instance), func_(func) {
    }

    void invoke(std::vector<std::any> args) override{
        if (args.size() != sizeof...(Args)) {
            printf("[FunctionRouter] [ERROR] memberFunc args quantity mismatch, declared: %zu, invoked: %zu\n", sizeof...(Args), args.size());
            return;
        }

        invokeImpl(args, std::index_sequence_for<Args...>{});
    }

private:
    using ArgsTuple = std::tuple<Args...>;

    template <size_t... I>
    void invokeImpl(std::vector<std::any>& args, std::index_sequence<I...>) {
        try {
            (instance_->*func_)(std::any_cast<typename std::tuple_element<I, ArgsTuple>::type>(args[I])...);
        } catch (const std::bad_any_cast &e) {
            printf("[FunctionRouter] [ERROR] %s, check event params is match ?\n", e.what());
        }
    }

private:
    Class* instance_ = nullptr;
    MemberFunc func_;

};



// 成员函数，无参const
template <typename Class, typename Ret>
class RouteHandlerWithoutArgsMemberConst : public RouteHandlerBase {
public:
    using MemberFunc = Ret(Class::*)() const;

    explicit RouteHandlerWithoutArgsMemberConst(const Class* instance, MemberFunc func) : instance_(instance), func_(func) {
    }

    void invoke(std::vector<std::any> args) override {
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
class RouteHandlerWithArgsMemberConst : public RouteHandlerBase {
public:
    using MemberFunc = Ret(Class::*)(Args...) const;

    explicit RouteHandlerWithArgsMemberConst(const Class* instance, MemberFunc func) : instance_(instance), func_(func) {
    }

    void invoke(std::vector<std::any> args) override {
        if (args.size() != sizeof...(Args)) {
            printf("[FunctionRouter] [ERROR] memberFunc args quantity mismatch, declared: %zu, invoked: %zu\n", sizeof...(Args), args.size());
            return;
        }

        invokeImpl(args, std::index_sequence_for<Args...>{});
    }

private:
    using ArgsTuple = std::tuple<Args...>;

    template <size_t... I>
    void invokeImpl(std::vector<std::any>& args, std::index_sequence<I...>) {
        try {
            (instance_->*func_)(std::any_cast<typename std::tuple_element<I, ArgsTuple>::type>(args[I])...);
        } catch (const std::bad_any_cast &e) {
            printf("[FunctionRouter] [ERROR] %s, check event params is match ?\n", e.what());
        }
    }

private:
    const Class* instance_ = nullptr;
    MemberFunc func_;

};



#endif //FUNCTIONROUTER_ROUTEHANDLER_HPP
