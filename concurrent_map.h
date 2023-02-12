#ifndef CONCURRENT_MAP_H
#define CONCURRENT_MAP_H

#include <map>
#include <mutex>
#include <thread>
#include <utility>

template<typename Key, typename Value>
class ConcurrentUnorderedMap {
public:
    Value &operator[](const Key &key) {
        std::lock_guard<std::mutex> lock(mutex);
        return map[key];
    }

    void erase(const Key &key) {
        std::lock_guard<std::mutex> lock(mutex);
        map.erase(key);
    }

    bool contains(const Key &key) {
        std::lock_guard<std::mutex> lock(mutex);
        return !(map.find(key) == map.end());
    }

    bool empty() {
        std::lock_guard<std::mutex> lock(mutex);
        return map.empty();
    }

    bool check_and_change(const Key &key, Value from, Value to) {
        std::lock_guard<std::mutex> lock(mutex);
        if (map.find(key) == map.end()) {
            return false;
        }

        if (map.at(key) == from) {
            map[key] = to;
            return true;
        }

        return false;
    }


//    auto begin() {
//        std::lock_guard<std::mutex> lock(mutex);
//        return map.begin();
//    }
//
//    auto end() {
//        std::lock_guard<std::mutex> lock(mutex);
//        return map.end();
//    }

private:
    std::map<Key, Value> map;
    std::mutex mutex;
};

#endif //CONCURRENT_MAP_H
