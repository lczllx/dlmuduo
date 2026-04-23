#ifndef MUDUO_LOGGER_H
#define MUDUO_LOGGER_H

#include <cstdio>
#include <cstdlib>
#include <string>

#include "log_system/Logger.hpp"

namespace muduo {

enum LogLevel {
    DEBUG = 0,
    INFO,
    WARN,
    ERROR,
    FATAL
};

// 与原有 L_DEBUG("fmt", ...) 用法兼容；底层走 lcz 异步/同步日志器
class Logger {
private:
    LogLevel _level;
    const char* _file;
    int _line;
    std::string _message;

public:
    Logger(LogLevel level, const char* file, int line)
        : _level(level), _file(file), _line(line) {}

    ~Logger() {
        lcz::Logger::ptr root = lcz::LoggerManager::getInstance().rootLogger();
        if (!root) {
            return;
        }
        const std::string f = _file ? _file : "";
        const size_t line = static_cast<size_t>(_line > 0 ? _line : 0);
        switch (_level) {
        case DEBUG:
            root->Debug(f, line, "%s", _message.c_str());
            break;
        case INFO:
            root->Info(f, line, "%s", _message.c_str());
            break;
        case WARN:
            root->Warn(f, line, "%s", _message.c_str());
            break;
        case ERROR:
            root->Error(f, line, "%s", _message.c_str());
            break;
        case FATAL:
            root->Fatal(f, line, "%s", _message.c_str());
            std::abort();
            break;
        default:
            break;
        }
    }

    template <typename... Args>
    void operator()(const char* format, Args... args) {
        char buffer[4096];
        snprintf(buffer, sizeof(buffer), format, args...);
        _message = buffer;
    }

    void operator()(const char* msg) { _message = msg ? msg : ""; }

    void operator()(const std::string& msg) { _message = msg; }
};

}  // namespace muduo

#define L_DEBUG muduo::Logger(muduo::DEBUG, __FILE__, __LINE__)
#define L_INFO  muduo::Logger(muduo::INFO, __FILE__, __LINE__)
#define L_WARN  muduo::Logger(muduo::WARN, __FILE__, __LINE__)
#define L_ERROR muduo::Logger(muduo::ERROR, __FILE__, __LINE__)
#define L_FATAL muduo::Logger(muduo::FATAL, __FILE__, __LINE__)

#endif
