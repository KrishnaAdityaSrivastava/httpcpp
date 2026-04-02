#include "cache.hpp"
std::string HTTP::Cache::get(std::string key){
    auto it = cache.find(key);
    if (it == cache.end()) return "";
    return it->second;
}

HTTP::Cache::Cache(){
    cache["reqcnt"] = "0";
}

void HTTP::Cache::put(std::string key,std::string& value){
    cache[key] = value;
}

void HTTP::Cache::inc(std::string key,int inc){

    cache[key] = std::to_string(std::stoi(cache[key]) + inc);
}