#include "nezha_util.h"
#include <windows.h>

namespace nezha{
    std::shared_ptr<spdlog::logger> Util::logger;

    std::wstring Util::stringToWstring(const std::string &src) {
        int size_needed = MultiByteToWideChar(CP_ACP, 0, src.c_str(), (int)src.size(), nullptr, 0);
        std::wstring dest_str(size_needed, 0);
        MultiByteToWideChar(CP_ACP, 0, src.c_str(), (int)src.size(), &dest_str[0], size_needed);
        return dest_str;
    }

    std::string Util::wstringToString(const std::wstring &src) {
        int size_needed = WideCharToMultiByte(CP_ACP, 0, src.c_str(), (int)src.size(), nullptr, 0, nullptr, nullptr);
        std::string dest_str(size_needed, 0);
        WideCharToMultiByte(CP_ACP, 0, src.c_str(), (int)src.size(), &dest_str[0], size_needed, nullptr, nullptr);
        return dest_str;
    }

    std::wstring Util::utf8ToUtf16(const std::string &src) {
        int size_needed = MultiByteToWideChar(CP_UTF8, 0, src.c_str(), (int)src.size(), nullptr, 0);
        std::wstring dest_str(size_needed, 0);
        MultiByteToWideChar(CP_UTF8, 0, src.c_str(), (int)src.size(), &dest_str[0], size_needed);
        return dest_str;
    }

    std::string Util::utf16ToUtf8(const std::wstring &src) {
        int size_needed = WideCharToMultiByte(CP_UTF8, 0, src.c_str(), (int)src.size(), nullptr, 0, nullptr, nullptr);
        std::string dest_str(size_needed, 0);
        WideCharToMultiByte(CP_UTF8, 0, src.c_str(), (int)src.size(), &dest_str[0], size_needed, nullptr, nullptr);
        return dest_str;
    }

    void Util::setLogger(const std::string &logger_name) {
        if(!logger_name.empty())logger = spdlog::get(logger_name);
    }
}