#ifndef Cache_h
#define Cache_h

#include <unordered_map>
#include <iostream>
#include <queue>
#include <string>


namespace HTTP{
    class Cache{
        private:
            std::unordered_map<std::string, std::string> cache;
            //std::queue<HTTP::CacheRequest> cacheQueue;
        public:
            Cache();
            std::string get(std::string key);
            void put(std::string key,std::string& value);
            void inc(std::string key,int inc);
    };
}
#endif