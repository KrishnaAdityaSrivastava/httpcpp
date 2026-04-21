#ifndef Cache_h
#define Cache_h

#include <unordered_map>
#include <iostream>
#include <queue>
#include <string>
#include <mutex>
#include <atomic>

namespace HTTP{
    class Cache{
        private:
            std::unordered_map<std::string, std::atomic<int>> cache;
            std::mutex map_mtx;
        public:
            Cache();
            int get(std::string key);
            void put(std::string key, int value);
            void inc(const std::string& key, int val);
    };
}
#endif