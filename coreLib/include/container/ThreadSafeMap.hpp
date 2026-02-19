//
// Created by 赵子墨 on 2026/2/16.
//

#ifndef MYEVENTBUS_THREADSAFEMAP_HPP
#define MYEVENTBUS_THREADSAFEMAP_HPP


#include <map>
#include <mutex>
#include <shared_mutex>
#include <optional>
#include <functional>



template <typename K, typename V>
class ThreadSafeMap {
public:

    ThreadSafeMap() = default;

    ~ThreadSafeMap() = default;

    ThreadSafeMap(const ThreadSafeMap &rhs) {
        std::shared_lock<std::shared_mutex> lock(rhs.mtx_);
        map_ = rhs.map_;
    }

    ThreadSafeMap &operator=(const ThreadSafeMap &rhs) {
        if (&rhs != this) {
            std::unique_lock<std::shared_mutex> lock1(mtx_, std::defer_lock);
            std::unique_lock<std::shared_mutex> lock2(rhs.mtx_, std::defer_lock);
            std::lock(lock1, lock2);
            map_ = rhs.map_;
        }
        return *this;
    }

    V operator[](const K &key) {
        std::unique_lock<std::shared_mutex> lock(mtx_);
        return map_[key];
    }

    int size() {
        std::shared_lock<std::shared_mutex> lock(mtx_);
        return map_.size();
    }

    bool isEmpty() {
        std::shared_lock<std::shared_mutex> lock(mtx_);
        return map_.empty();
    }

    bool insert(const K &key, const V &value) {
        std::unique_lock<std::shared_mutex> lock(mtx_);
        auto ret = map_.insert(std::pair<K, V>(key, value));
        return ret.second;
    }

    void ensureInsert(const K &key, const V &value) {
        std::unique_lock<std::shared_mutex> lock(mtx_);
        map_[key] = value;
    }

    bool find(const K &key, V &value) {
        bool ret = false;
        std::shared_lock<std::shared_mutex> lock(mtx_);

        auto iter = map_.find(key);
        if (iter != map_.end()) {
            value = iter->second;
            ret = true;
        }

        return ret;
    }

    bool findOldAndSetNew(const K &key, V &oldValue, const V &newValue) {
        bool ret = false;
        std::unique_lock<std::shared_mutex> lock(mtx_);
        if (map_.size() > 0) {
            auto iter = map_.find(key);
            if (iter != map_.end()) {
                oldValue = iter->second;
                iter->second = newValue;
                ret = true;
            }
        }

        return ret;
    }

    int erase(const K &key) {
        std::unique_lock<std::shared_mutex> lock(mtx_);
        return map_.erase(key);
    }

    int eraseUnLocking(const K &key) {
        return map_.erase(key);
    }

    void clear() {
        std::unique_lock<std::shared_mutex> lock(mtx_);
        map_.clear();
    }

    std::optional<V> beginValue() {
        std::shared_lock<std::shared_mutex> lock(mtx_);
        if (map_.empty()) return std::nullopt;
        return map_.begin()->second;
    }

    std::optional<K> beginKey() {
        std::shared_lock<std::shared_mutex> lock(mtx_);
        if (map_.empty()) return std::nullopt;
        return map_.begin()->first;
    }

    std::optional<V> endValue() {
        std::shared_lock<std::shared_mutex> lock(mtx_);
        if (map_.empty()) return std::nullopt;
        return map_.rbegin()->second;
    }

    std::optional<K> endKey() {
        std::shared_lock<std::shared_mutex> lock(mtx_);
        if (map_.empty()) return std::nullopt;
        return map_.rbegin()->first;
    }

    void foreachMap(std::function<bool(K, V)> iterator) {
        std::shared_lock<std::shared_mutex> lock(mtx_);
        for (auto it = map_.begin(); it != map_.end(); it++) {
            if (iterator(it->first, it->second)) {
                break;
            }
        }
    }

private:
    mutable std::shared_mutex mtx_;
    std::map<K, V> map_;

};



#endif //MYEVENTBUS_THREADSAFEMAP_HPP
