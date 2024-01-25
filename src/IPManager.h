#ifndef NEZHA_AGENT_CPP_IPMANAGER_H
#define NEZHA_AGENT_CPP_IPMANAGER_H
#include <string>
#include <mutex>
#include <vector>
#include <json/json.h>
#include "spdlog/spdlog.h"
template<typename T>
class Singleton
{
protected: // 不能直接构造，同时子类可访问，这是protected的优势
    Singleton() = default;

public: // 不提供拷贝构造函数和赋值函数
    Singleton(const Singleton& one) = delete;
    Singleton& operator=(const Singleton& one) = delete;

public: // 全局访问函数
    static T& GetInstance()
    {
        static T instance;
        return instance;
    }
};

namespace nezha{
    class IPManager: public Singleton<IPManager>{
    public:
        friend class Singleton<IPManager>;
    private:
        IPManager();
    public:
        void setLogger(const std::string& logger_name);
        bool getIpInfo();
        std::string request(const std::string& url, bool is_v6);
        bool parseIp(const Json::Value& v4_json, const Json::Value& v6_json);
        bool parseCountryCode(const Json::Value& v4_json, const Json::Value& v6_json);
        void run();
        std::string getIp();
        std::string getCountryCode();
    private:
        std::shared_ptr<spdlog::logger> logger;
        std::string ip;
        std::string country_code;
        std::mutex mtx;
        std::vector<std::string> ip_keys = {
                "ip",
                "query"
        };
        std::vector<std::string> country_code_keys = {
                "country_code",
                "countryCode"
        };
        std::vector<std::string> url_list = {
                "https://api.ip.sb/geoip",
                "http://api.myip.la/en?json",
                "http://ip-api.com/json/"
        };
        std::string UA = "Mozilla/5.0 (Macintosh; Intel Mac OS X 10_15_7) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/112.0.0.0 Safari/537.36";
    };
}


#endif //NEZHA_AGENT_CPP_IPMANAGER_H
