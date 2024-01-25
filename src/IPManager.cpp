#include "IPManager.h"
#include <curl/curl.h>
#include "common.h"

namespace nezha{

    IPManager::IPManager() = default;

    bool IPManager::getIpInfo() {
        bool need_ip = true;
        bool need_country_code = true;
        for(auto& url: url_list){
            LOG_DEBUG("request url:{}", url);
            auto v4_ret = request(url, false);
            auto v6_ret = request(url, true);
            Json::Value v4_json;
            Json::Value v6_json;
            Json::Reader jsonReader;
            jsonReader.parse(v4_ret, v4_json);
            jsonReader.parse(v6_ret, v6_json);
            if(need_ip){
                auto parse_ip_ret = parseIp(v4_json, v6_json);
                if  (parse_ip_ret) need_ip=false;
            }
            if(need_country_code){
                auto parse_country_code_ret = parseCountryCode(v4_json, v6_json);
                if (parse_country_code_ret) need_country_code=false;
            }
            if(!need_ip && !need_country_code) break;
        }
        return !need_ip;
    }

    static size_t WriteFunction(void* input, size_t uSize, size_t uCount, void* avg)
    {
        size_t uLen = uSize * uCount;
        std::string* pVec = (std::string*)(avg);
        int old_size = (int)pVec->size();
        pVec->resize(old_size + uLen);
        auto start_pos = pVec->begin() + old_size;
        memcpy_s(&(pVec->at(old_size)), uLen, input, uLen);
        return uLen;
    }
    std::string IPManager::request(const std::string &url, bool is_v6) {
        CURL *curl;
        CURLcode res;
        std::string readBuffer;
        curl = curl_easy_init();
        if(curl) {
            curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
            curl_easy_setopt(curl, CURLOPT_USERAGENT, UA.c_str());
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteFunction);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
            if(is_v6) curl_easy_setopt(curl, CURLOPT_IPRESOLVE, CURL_IPRESOLVE_V6);
            else curl_easy_setopt(curl, CURLOPT_IPRESOLVE, CURL_IPRESOLVE_V4);
            res = curl_easy_perform(curl);
            curl_easy_cleanup(curl);
        }
        return readBuffer;
    }

    std::string traverseJson(const Json::Value& jsonValue, const std::string& explore_key) {
        std::string ret;
        // 遍历对象的字段
        for (auto it = jsonValue.begin(); it != jsonValue.end(); ++it) {

            const Json::Value& value = *it;
            if (value.isObject() || value.isArray()) {
                traverseJson(value, explore_key);  // 递归遍历对象或数组
            }
            if(jsonValue.isObject()){
                std::string key = it.key().asString();
                if(key == explore_key){
                    ret = value.asString();
                    break;
                }
            }

        }
        return ret;
    }

    bool IPManager::parseIp(const Json::Value &v4_json, const Json::Value &v6_json) {
        bool ret = false;
        std::string v4_ip;
        std::string v6_ip;
        for(auto& key: ip_keys){
            v4_ip = traverseJson(v4_json, key);
            v6_ip = traverseJson(v6_json, key);
            if((!v4_ip.empty() && v4_ip.find('.')!=std::string::npos) ||
                (!v6_ip.empty() && v6_ip.find(':')!=std::string::npos)){
                ret = true;
                break;
            }
        }
        if(ret){
            std::lock_guard<std::mutex> lock(mtx);
            if(!v4_ip.empty() && !v6_ip.empty()) ip = v4_ip+"/"+v6_ip;
            else if(!v4_ip.empty()) ip = v4_ip;
            else if(!v6_ip.empty()) ip = v6_ip;
            LOG_DEBUG("ip:{}", ip);
        }
        return ret;
    }

    bool IPManager::parseCountryCode(const Json::Value &v4_json, const Json::Value &v6_json) {
        bool ret = false;
        std::string country_code_;
        for(auto& key: country_code_keys){
            country_code_ = traverseJson(v4_json, key);
            if(!country_code_.empty()){
                ret = true;
                break;
            }
            country_code_ = traverseJson(v6_json, key);
            if(!country_code_.empty()){
                ret = true;
                break;
            }
        }
        if(ret){
            for (char& c : country_code_) {
                c = std::tolower(c);
            }
            std::lock_guard<std::mutex> lock(mtx);
            country_code = country_code_;
            LOG_DEBUG("country_code:{}", country_code);
        }
        return ret;
    }

    std::string IPManager::getIp() {
        std::lock_guard<std::mutex> lock(mtx);
        return ip;
    }

    std::string IPManager::getCountryCode() {
        std::lock_guard<std::mutex> lock(mtx);
        return country_code;
    }

    void IPManager::run() {
        std::thread([this](){
            while(true){
                auto check_ret = getIpInfo();
                if(!check_ret){
                    LOG_ERROR("get ip info fail, sleep 60s");
                    std::this_thread::sleep_for(std::chrono::seconds(60));
                    continue;
                }
                std::this_thread::sleep_for(std::chrono::minutes(30));
            }
        }).detach();
    }

    void IPManager::setLogger(const std::string &logger_name) {
        logger = spdlog::get(logger_name);
    }
}