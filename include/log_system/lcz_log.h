/* 全局接口：可选包含。muduo 工程请优先使用上层 include/Logger.hpp，避免重复定义 L_* 宏。 */
#pragma once
#include "utility.hpp"
#include "level.hpp"
#include "message.hpp"
#include "Logformat.hpp"
#include "sink.hpp"
#include "Logger.hpp"
#include "buffer.hpp"
#include "looper.hpp"

namespace lcz {

inline Logger::ptr getLogger(const std::string& name = "root_logger") {
    Logger::ptr logger = LoggerManager::getInstance().getLogger(name);
    if (logger == nullptr) {
        logger = LoggerManager::getInstance().rootLogger();
    }
    return logger;
}

inline Logger::ptr getrootLogger() {
    return LoggerManager::getInstance().rootLogger();
}

}  // namespace lcz

#define LCZ_DEBUG(fmt, ...) lcz::getrootLogger()->Debug(__FILE__, static_cast<size_t>(__LINE__), fmt, ##__VA_ARGS__)
#define LCZ_INFO(fmt, ...)  lcz::getrootLogger()->Info(__FILE__, static_cast<size_t>(__LINE__), fmt, ##__VA_ARGS__)
#define LCZ_WARN(fmt, ...)  lcz::getrootLogger()->Warn(__FILE__, static_cast<size_t>(__LINE__), fmt, ##__VA_ARGS__)
#define LCZ_ERROR(fmt, ...) lcz::getrootLogger()->Error(__FILE__, static_cast<size_t>(__LINE__), fmt, ##__VA_ARGS__)
#define LCZ_FATAL(fmt, ...) lcz::getrootLogger()->Fatal(__FILE__, static_cast<size_t>(__LINE__), fmt, ##__VA_ARGS__)
