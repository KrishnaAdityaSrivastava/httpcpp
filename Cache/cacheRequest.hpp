#pragma once
#include <string>

namespace HTTP {
class CacheRequest{
    public:
        std::string type;
        std::string key;
        std::string value;

        void GET(std::string key) {
            //std::string type; // inc put get
            type = "GET";
            this->key = key;
            //int val;
        };
        void PUT(std::string key,std::string value) {
            //std::string type; // inc put get
            type = "PUT";

            this->key = key;
            this->value = value;
            //int val;
        };
        void INC(std::string key,std::string value) {
            //std::string type; // inc put get
            type = "INC";

            this->key = key;
            this->value = value;
            //int val;
        };
};


}