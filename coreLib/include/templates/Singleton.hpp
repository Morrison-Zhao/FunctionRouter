//
// Created by 赵子墨 on 2026/2/16.
//

#ifndef MYEVENTBUS_SINGLETON_HPP
#define MYEVENTBUS_SINGLETON_HPP



/**
 *
 * 单例基类 CRTP
 *
 * class Derived : public Singleton<Derived> {
 *      friend Singleton<Derived>;
 * private:
 *      Derived() = default;
 * }
 *
 * */
template <typename T>
class Singleton {
public:
    static T& getInstance() {
        static T instance;
        return instance;
    }

    Singleton(const Singleton&) = delete;

    Singleton& operator=(const Singleton&) = delete;

protected:
    Singleton() = default;

    ~Singleton() = default;

};



#endif //MYEVENTBUS_SINGLETON_HPP
