#include "cache.hpp"

HTTP::Cache::Cache(){
    cache["reqcnt"] = 0;
}

// std::string HTTP::Cache::get(std::string key){
//     auto it = cache.find(key);
//     if (it == cache.end()) return "";
//     return it->second;
    
// }

// void HTTP::Cache::put(std::string key,std::string& value){
//     cache[key] = value;
// }

int HTTP::Cache::get(std::string key){
    std::lock_guard<std::mutex> lock(map_mtx);
    auto it = cache.find(key);
    if (it == cache.end()) return 0;
    return it->second.load(std::memory_order_relaxed);
}

void HTTP::Cache::put(std::string key, int value){
    std::lock_guard<std::mutex> lock(map_mtx);
    cache[key].store(value, std::memory_order_relaxed);
}

void HTTP::Cache::inc(const std::string& key, int val){
    std::lock_guard<std::mutex> lock(map_mtx);
    cache[key].fetch_add(val, std::memory_order_relaxed);
}
// void HTTP::Cache::inc(const std::string& key, int val){
//     std::atomic<int>* counter;

//     {
//         std::lock_guard<std::mutex> lock(map_mtx);
//         counter = &cache[key];
//     }

//     counter->fetch_add(val, std::memory_order_relaxed);
// }

