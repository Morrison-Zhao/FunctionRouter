//
// Created by 赵子墨 on 2026/2/17.
//

#ifndef FUNCTIONROUTER_ROUTEHANDLER_HPP
#define FUNCTIONROUTER_ROUTEHANDLER_HPP


#include <any>
#include <tuple>
#include <type_traits>
#include <functional>


// 事件处理器，存储函数
class RouteHandlerBase {
public:
    virtual ~RouteHandlerBase() = default;

    // 1. const 版本 (只读)
    virtual void invoke(const std::vector<std::any>& args) = 0;
    
    // 2. 非 const 左值引用版本 (可修改)
    virtual void invoke(std::vector<std::any>& args) = 0;

    // 3. 右值引用版本 (可修改，用于临时对象)
    virtual void invoke(std::vector<std::any>&& args) = 0;

protected:
    // 智能转换：尝试解包 reference_wrapper，如果失败则尝试直接转换值
    template<typename WantedType>
    static decltype(auto) any_cast_smart(std::any& operand) {
        using DecayType = typename std::remove_reference<WantedType>::type;
        
        // 1. 尝试作为 reference_wrapper<DecayType> 解包
        if (auto* p = std::any_cast<std::reference_wrapper<DecayType>>(&operand)) {
            return p->get();
        }
        
        // 2. 尝试作为 reference_wrapper<std::remove_const_t<DecayType>> 解包 (处理 const T& 绑定到 T&)
        if constexpr (std::is_const_v<DecayType>) {
             using NonConstDecayType = std::remove_const_t<DecayType>;
             if (auto* p = std::any_cast<std::reference_wrapper<NonConstDecayType>>(&operand)) {
                 return static_cast<WantedType>(p->get());
             }
        }

        // 3. 尝试直接作为 DecayType 解包 (值拷贝)
        if (auto* p = std::any_cast<DecayType>(&operand)) {
            return static_cast<WantedType>(*p);
        }
        
        // 4. 尝试作为 DecayType 的非 const 版本解包 (处理 const T& 绑定到 any<T>)
        if constexpr (std::is_const_v<DecayType>) {
            using NonConstDecayType = std::remove_const_t<DecayType>;
             if (auto* p = std::any_cast<NonConstDecayType>(&operand)) {
                 return static_cast<WantedType>(*p);
             }
        }
        
        throw std::bad_any_cast();
    }
};



// 普通函数，无参
template <typename Func>
class RouteHandlerWithoutArgs : public RouteHandlerBase {
public:
    explicit RouteHandlerWithoutArgs(Func&& func) : func_(std::forward<Func>(func)) {}

    void invoke(const std::vector<std::any>& args) override { invokeImpl(); }
    void invoke(std::vector<std::any>& args) override { invokeImpl(); }
    void invoke(std::vector<std::any>&& args) override { invokeImpl(); }

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

    // 1. const 版本：必须确保 Args 中的引用被移除或变为 const，否则 any_cast<int&>(const any&) 会失败
    void invoke(const std::vector<std::any>& args) override {
        if (checkArgs(args)) {
            invokeImplConst(args, std::index_sequence_for<Args...>{});
        }
    }

    // 2. 非 const 版本：完整支持引用语义
    void invoke(std::vector<std::any>& args) override {
        if (checkArgs(args)) {
            invokeImpl(args, std::index_sequence_for<Args...>{});
        }
    }

    // 3. 右值版本：完整支持引用语义
    void invoke(std::vector<std::any>&& args) override {
        if (checkArgs(args)) {
            invokeImpl(args, std::index_sequence_for<Args...>{});
        }
    }

private:
    using ArgsTuple = std::tuple<Args...>;

    bool checkArgs(const std::vector<std::any>& args) {
        if (args.size() != sizeof...(Args)) {
            printf("[FunctionRouter] [ERROR] args quantity mismatch, declared: %zu, invoked: %zu\n", sizeof...(Args), args.size());
            return false;
        }
        return true;
    }

    // 可修改的实现
    template <size_t... I>
    void invokeImpl(std::vector<std::any>& args, std::index_sequence<I...>) {
        try {
            func_(any_cast_smart<typename std::tuple_element<I, ArgsTuple>::type>(args[I])...);
        } catch (const std::bad_any_cast &e) {
            printf("[FunctionRouter] [ERROR] %s, check event params is match ?\n", e.what());
        }
    }

    // 只读的实现：将 Args 中的 T& 转换为 const T& 或 T，以适配 const any
    template <size_t... I>
    void invokeImplConst(const std::vector<std::any>& args, std::index_sequence<I...>) {
        try {
            // 强转去 const，为了适配 func(int&) 这种非 const 引用参数
            // 注意：这是为了兼容 const invoke 接口的妥协，调用者需保证 args 实际上可修改（例如是临时对象的 const 引用）
            func_(any_cast_smart<typename std::tuple_element<I, ArgsTuple>::type>(const_cast<std::any&>(args[I]))...);
        } catch (const std::bad_any_cast &e) {
            printf("[FunctionRouter] [ERROR] const invoke: %s, check event params is match ?\n", e.what());
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

    void invoke(const std::vector<std::any>& args) override { invokeImpl(); }
    void invoke(std::vector<std::any>& args) override { invokeImpl(); }
    void invoke(std::vector<std::any>&& args) override { invokeImpl(); }

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

    void invoke(const std::vector<std::any>& args) override {
        if (checkArgs(args)) invokeImplConst(args, std::index_sequence_for<Args...>{});
    }

    void invoke(std::vector<std::any>& args) override {
        if (checkArgs(args)) invokeImpl(args, std::index_sequence_for<Args...>{});
    }

    void invoke(std::vector<std::any>&& args) override {
        if (checkArgs(args)) invokeImpl(args, std::index_sequence_for<Args...>{});
    }

private:
    using ArgsTuple = std::tuple<Args...>;

    bool checkArgs(const std::vector<std::any>& args) {
        if (args.size() != sizeof...(Args)) {
            printf("[FunctionRouter] [ERROR] memberFunc args quantity mismatch, declared: %zu, invoked: %zu\n", sizeof...(Args), args.size());
            return false;
        }
        return true;
    }

    template <size_t... I>
    void invokeImpl(std::vector<std::any>& args, std::index_sequence<I...>) {
        try {
            (instance_->*func_)(any_cast_smart<typename std::tuple_element<I, ArgsTuple>::type>(args[I])...);
        } catch (const std::bad_any_cast &e) {
            printf("[FunctionRouter] [ERROR] %s, check event params is match ?\n", e.what());
        }
    }

    template <size_t... I>
    void invokeImplConst(const std::vector<std::any>& args, std::index_sequence<I...>) {
        try {
            (instance_->*func_)(any_cast_smart<typename std::tuple_element<I, ArgsTuple>::type>(const_cast<std::any&>(args[I]))...);
        } catch (const std::bad_any_cast &e) {
            printf("[FunctionRouter] [ERROR] const invoke: %s, check event params is match ?\n", e.what());
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

    void invoke(const std::vector<std::any>& args) override { invokeImpl(); }
    void invoke(std::vector<std::any>& args) override { invokeImpl(); }
    void invoke(std::vector<std::any>&& args) override { invokeImpl(); }

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

    void invoke(const std::vector<std::any>& args) override {
        if (checkArgs(args)) invokeImplConst(args, std::index_sequence_for<Args...>{});
    }

    void invoke(std::vector<std::any>& args) override {
        if (checkArgs(args)) invokeImpl(args, std::index_sequence_for<Args...>{});
    }

    void invoke(std::vector<std::any>&& args) override {
        if (checkArgs(args)) invokeImpl(args, std::index_sequence_for<Args...>{});
    }

private:
    using ArgsTuple = std::tuple<Args...>;

    bool checkArgs(const std::vector<std::any>& args) {
        if (args.size() != sizeof...(Args)) {
            printf("[FunctionRouter] [ERROR] memberFunc args quantity mismatch, declared: %zu, invoked: %zu\n", sizeof...(Args), args.size());
            return false;
        }
        return true;
    }

    template <size_t... I>
    void invokeImpl(std::vector<std::any>& args, std::index_sequence<I...>) {
        try {
            (instance_->*func_)(any_cast_smart<typename std::tuple_element<I, ArgsTuple>::type>(args[I])...);
        } catch (const std::bad_any_cast &e) {
            printf("[FunctionRouter] [ERROR] %s, check event params is match ?\n", e.what());
        }
    }

    template <size_t... I>
    void invokeImplConst(const std::vector<std::any>& args, std::index_sequence<I...>) {
        try {
            (instance_->*func_)(any_cast_smart<typename std::tuple_element<I, ArgsTuple>::type>(const_cast<std::any&>(args[I]))...);
        } catch (const std::bad_any_cast &e) {
            printf("[FunctionRouter] [ERROR] const invoke: %s, check event params is match ?\n", e.what());
        }
    }

private:
    const Class* instance_ = nullptr;
    MemberFunc func_;

};



#endif //FUNCTIONROUTER_ROUTEHANDLER_HPP
