#ifndef NEZHA_AGENT_CPP_NEZHA_UTIL_H
#define NEZHA_AGENT_CPP_NEZHA_UTIL_H
#include <spdlog/spdlog.h>
#include "common.h"

namespace nezha{
    class Util {
    public:
        template<typename T, typename V>
        static T readRegedit(const std::wstring& name, const std::wstring& path, unsigned long dstValueType){
            HKEY hKey;
            T value;
            DWORD valueType;
            V* valueData;
            DWORD valueSize;
            if constexpr (std::is_same_v<T, std::string> ||
                          std::is_same_v<T, std::wstring>){
                valueData = new V[MAX_PATH];
                valueSize = MAX_PATH;
            }else{
                valueData = new V;
                valueSize = sizeof(*valueData);
            }

            // 打开注册表键
            LONG result = RegOpenKeyEx(HKEY_LOCAL_MACHINE, path.c_str(), 0, KEY_READ, &hKey);
            if (result != ERROR_SUCCESS)
            {
                LOG_ERROR("Failed to open registry key. Error code:{}", result);
                goto end;
            }

            // 读取注册表值
            result = RegQueryValueEx(hKey, name.c_str(), nullptr, &valueType, reinterpret_cast<LPBYTE>(valueData), &valueSize);
            if (result != ERROR_SUCCESS)
            {
                LOG_ERROR("Failed to read registry value. Error code:{}", result);
                RegCloseKey(hKey);
                goto end;
            }
            // 根据值类型进行适当的处理
            if (valueType == dstValueType)
            {
                if constexpr(std::is_same_v<T, std::wstring>) {
                    value = T(valueData);
                }else if constexpr (std::is_same_v<T, std::string>){
                    value = Util::utf16ToUtf8(std::wstring(valueData));
                }
                else{
                    value = *valueData;
                }

                if constexpr(std::is_same_v<T, std::wstring>) {
                    LOG_INFO("Registry value:{}", Util::utf16ToUtf8(value));
                }
                else{
                    LOG_INFO("Registry value:{}", value);
                }
            }else{
                LOG_ERROR("Registry value has unsupported type.");
            }
            RegCloseKey(hKey);
            end:
            if constexpr (std::is_same_v<T, std::string> ||
                          std::is_same_v<T, std::wstring>){
                delete[] valueData;
            }else{
                delete valueData;
            }
            return value;
        }
    public:
        static void setLogger(const std::string& logger_name);
        static std::wstring stringToWstring(const std::string& src);
        static std::string wstringToString(const std::wstring& src);
        static std::wstring utf8ToUtf16(const std::string& src);
        static std::string utf16ToUtf8(const std::wstring& src);
    private:
        Util() = default;
        static std::shared_ptr<spdlog::logger> logger;
    };
}


#endif //NEZHA_AGENT_CPP_NEZHA_UTIL_H
