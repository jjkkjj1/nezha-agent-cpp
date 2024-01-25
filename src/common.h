#ifndef NEZHA_AGENT_CPP_COMMON_H
#define NEZHA_AGENT_CPP_COMMON_H

#define LOG_INFO(x, ...) if(logger.get())logger->info("[" __FUNCTION__ "]"x, __VA_ARGS__);else spdlog::info("[" __FUNCTION__ "]"x, __VA_ARGS__)
#define LOG_ERROR(x, ...) if(logger.get())logger->error("[" __FUNCTION__ "]"x, __VA_ARGS__);else spdlog::error("[RiotCaptchaLoginApi] ""[" __FUNCTION__ "]"x, __VA_ARGS__)
#ifdef _DEBUG
#define LOG_DEBUG(x, ...) if(logger.get())logger->debug("[" __FUNCTION__ "]"x, __VA_ARGS__);else spdlog::debug("[RiotCaptchaLoginApi] ""[" __FUNCTION__ "]"x, __VA_ARGS__)
#else
#define LOG_DEBUG(x, ...)
#endif

#define TOSTRING(x) #x
#define MAKE_VERSION(major, minor, patch)  TOSTRING(cpp_##major##.##minor##.##patch)
#define VERSION MAKE_VERSION(0, 1, 0)
#endif //NEZHA_AGENT_CPP_COMMON_H
