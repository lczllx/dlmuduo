#pragma once
#include <cstdarg>
#include <cstdio>
#include <thread>
#include <utility>
#include <functional>
#include <mutex>
#include <atomic>
#include <unordered_map>
#include <cassert>
#include <iostream>
#include <sstream>
#include <memory>
#include <vector>
#include <string>
#include "utility.hpp"
#include "level.hpp"
#include "Logformat.hpp"
#include "sink.hpp"
#include "looper.hpp"

namespace lcz
{

// 日志器基类
class Logger
{
public:
    using ptr = std::shared_ptr<Logger>;

    Logger(const std::string& logger_name, Formatter::ptr formatter, LogLevel::value level, const std::vector<LogSink::ptr>& logsink)
        : _logger_name(logger_name), _formatter(formatter), _limit_level(level), _logsink(logsink)
    {}

    std::vector<LogSink::ptr> getSinks() const {
        return _logsink;
    }

    std::string getLogger() const { 
        std::unique_lock<std::mutex> lock(_mtx);
        return _logger_name; 
    }

    // 新增：运行时修改日志级别（线程安全）
    void setLevel(lcz::LogLevel::value level) {
        _limit_level.store(level);
    }

    LogLevel::value level() const {
        return _limit_level.load();
    }

    void Debug(const std::string& file, size_t line, const std::string& fmt, ...)
    {
        if (LogLevel::value::DEBUG < _limit_level.load()) return;

        va_list vl;
        va_start(vl, fmt);
        char* res = nullptr;
        int ret = vasprintf(&res, fmt.c_str(), vl);
        va_end(vl);

        if (ret < 0) {
            std::cerr << "vasprintf failed: memory allocation error" << std::endl;
            return;
        }

        Logmsg msg(LogLevel::value::DEBUG, file, line, getLogger(), res);
        std::stringstream ss;
        _formatter->format(ss, msg);
        log(ss.str().c_str(), ss.str().size());
        free(res);
    }

    void Info(const std::string& file, size_t line, const std::string& fmt, ...)
    {
        if (LogLevel::value::INFO < _limit_level.load()) return;

        va_list vl;
        va_start(vl, fmt);
        char* res = nullptr;
        int ret = vasprintf(&res, fmt.c_str(), vl);
        va_end(vl);

        if (ret < 0) {
            std::cerr << "vasprintf failed: memory allocation error" << std::endl;
            return;
        }

        Logmsg msg(LogLevel::value::INFO, file, line, getLogger(), res);
        std::stringstream ss;
        _formatter->format(ss, msg);
        log(ss.str().c_str(), ss.str().size());
        free(res);
    }

    void Warn(const std::string& file, size_t line, const std::string& fmt, ...)
    {
        if (LogLevel::value::WARN < _limit_level.load()) return;

        va_list vl;
        va_start(vl, fmt);
        char* res = nullptr;
        int ret = vasprintf(&res, fmt.c_str(), vl);
        va_end(vl);

        if (ret < 0) {
            std::cerr << "vasprintf failed: memory allocation error" << std::endl;
            return;
        }

        Logmsg msg(LogLevel::value::WARN, file, line, getLogger(), res);
        std::stringstream ss;
        _formatter->format(ss, msg);
        log(ss.str().c_str(), ss.str().size());
        free(res);
    }

    void Error(const std::string& file, size_t line, const std::string& fmt, ...)
    {
        if (LogLevel::value::ERROR < _limit_level.load()) return;

        va_list vl;
        va_start(vl, fmt);
        char* res = nullptr;
        int ret = vasprintf(&res, fmt.c_str(), vl);
        va_end(vl);

        if (ret < 0) {
            std::cerr << "vasprintf failed: memory allocation error" << std::endl;
            return;
        }

        Logmsg msg(LogLevel::value::ERROR, file, line, getLogger(), res);
        std::stringstream ss;
        _formatter->format(ss, msg);
        log(ss.str().c_str(), ss.str().size());
        free(res);
    }

    void Fatal(const std::string& file, size_t line, const std::string& fmt, ...)
    {
        if (LogLevel::value::FATAL < _limit_level.load()) return;

        va_list vl;
        va_start(vl, fmt);
        char* res = nullptr;
        int ret = vasprintf(&res, fmt.c_str(), vl);
        va_end(vl);

        if (ret < 0) {
            std::cerr << "vasprintf failed: memory allocation error" << std::endl;
            return;
        }

        Logmsg msg(LogLevel::value::FATAL, file, line, getLogger(), res);
        std::stringstream ss;
        _formatter->format(ss, msg);
        log(ss.str().c_str(), ss.str().size());
        free(res);
    }

protected:
    mutable std::mutex _mtx;                  // 互斥锁，保证线程安全
    std::string _logger_name;                 // 日志器名称
    Formatter::ptr _formatter;                // 格式化模块智能指针
    std::atomic<LogLevel::value> _limit_level;// 原子操作的日志级别限制
    std::vector<LogSink::ptr> _logsink;       // 日志落地器智能指针数组

    virtual void log(const char* data, size_t len) = 0;

    // 仅允许 LoggerManager 调整名称，避免与 name->logger 映射不一致
    void setLoggerNameUnsafe(const std::string& new_name) {
        std::unique_lock<std::mutex> lock(_mtx);
        _logger_name = new_name;
    }
    friend class LoggerManager;
};


// 同步输出日志器
class SyncLogger : public Logger
{
public:
    SyncLogger(const std::string& logger_name, Formatter::ptr formatter, LogLevel::value level, const std::vector<LogSink::ptr>& logsink)
        : Logger(logger_name, formatter, level, logsink)
    {}

    virtual void log(const char* data, size_t len) override
    {
        std::unique_lock<std::mutex> lock(_mtx);
        if (_logsink.empty()) return;
        for (auto& s : _logsink)
        {
            s->log(data, len);
        }
    }
};


// 异步输出日志器
class AsyncLogger : public Logger
{
public:
    AsyncLogger(const std::string& logger_name, Formatter::ptr formatter, LogLevel::value level,
                const std::vector<LogSink::ptr>& logsink, AsyncType looper_type)
        : Logger(logger_name, formatter, level, logsink),
          _looper(std::make_shared<AsyncLooper>(
              std::bind(&AsyncLogger::reallog, this, std::placeholders::_1),
              looper_type))
    {}

    virtual void log(const char* data, size_t len) override
    {
        _looper->push(data, len); // 已线程安全
    }

    void reallog(Buffer& buf)
    {
        if (_logsink.empty()) return;
        for (auto& s : _logsink)
        {
            s->log(buf.begin(), buf.abletoreadlen());
        }
    }

private:
    AsyncLooper::ptr _looper; // 管理异步日志器
};


// 日志器类型枚举
enum class LoggerType
{
    LOGGER_SYNC,
    LOGGER_ASYNC
};


// 日志器建造者基类
class LoggerBuilder
{
public:
    LoggerBuilder()
        : _looper_type(AsyncType::ASYNC_SAFE),
          _logger_type(LoggerType::LOGGER_ASYNC),
          _level(LogLevel::value::DEBUG)
    {}

    void buildloggertype(LoggerType type) { _logger_type = type; }
    void buildunsafeASYNC() { _looper_type = lcz::AsyncType::ASYNC_UNSAFE; }
    void buildloggername(const std::string& loggername) { _logger_name = loggername; }
    void buildloggerlevel(lcz::LogLevel::value level) { _level = level; }
    void buildloggerformatter(const std::string& pattern) { _formatter = std::make_shared<Formatter>(pattern); }

    template <typename Sinktype, typename... Args>
    void buildloggersink(Args&&... args)
    {
        _sinks.push_back(std::make_shared<Sinktype>(std::forward<Args>(args)...));
    }

    virtual Logger::ptr build() = 0;

protected:
    lcz::AsyncType _looper_type;
    LoggerType _logger_type;
    std::string _logger_name;
    lcz::LogLevel::value _level;
    lcz::Formatter::ptr _formatter;
    std::vector<lcz::LogSink::ptr> _sinks;
};


// 局部日志器建造者
class LocalLoggerBuilder : public LoggerBuilder
{
public:
    virtual Logger::ptr build() override
    {
        assert(!_logger_name.empty());
        if (!_formatter) {
            _formatter = std::make_shared<Formatter>();
        }
        if (_sinks.empty()) {
            buildloggersink<StdoutSink>();
        }
        if (_logger_type == LoggerType::LOGGER_SYNC) {
            return std::make_shared<SyncLogger>(_logger_name, _formatter, _level, _sinks);
        } else {
            return std::make_shared<AsyncLogger>(_logger_name, _formatter, _level, _sinks, _looper_type);
        }
    }
};


// 日志器管理器（单例）
class LoggerManager
{
public:
    void replaceRootLogger(Logger::ptr newRootLogger)
    {
        if (!newRootLogger) return;
        std::unique_lock<std::mutex> lock(_mutex);
        _loggers.erase(_root_logger->getLogger());
        _root_logger = newRootLogger;
        _loggers.emplace(newRootLogger->getLogger(), newRootLogger);
    }

    static LoggerManager& getInstance()
    {
        static LoggerManager eton;
        return eton;
    }

    void addLogger(Logger::ptr& logger)
    {
        if (!logger) return;
        if (hasLogger(logger->getLogger())) return;
        std::unique_lock<std::mutex> lock(_mutex);
        _loggers.insert(std::make_pair(logger->getLogger(), logger));
    }

    bool hasLogger(const std::string& name)
    {
        std::unique_lock<std::mutex> lock(_mutex);
        return _loggers.find(name) != _loggers.end();
    }

    Logger::ptr getLogger(const std::string& name)
    {
        std::unique_lock<std::mutex> lock(_mutex);
        auto it = _loggers.find(name);
        if (it != _loggers.end())
            return it->second;
        return nullptr;
    }

    Logger::ptr rootLogger() const
    {
        return _root_logger;
    }

    void initRootLogger(Logger::ptr logger)
    {
        if (!logger) return;
        std::unique_lock<std::mutex> lock(_mutex);
        if (_root_logger) {
            _loggers.erase(_root_logger->getLogger());
        }
        _root_logger = logger;
        _loggers[_root_logger->getLogger()] = _root_logger;
    }

    // 新增：按名设置日志级别（运行时）
    bool setLoggerLevel(const std::string& name, LogLevel::value level)
    {
        std::unique_lock<std::mutex> lock(_mutex);
        auto it = _loggers.find(name);
        if (it == _loggers.end()) return false;
        it->second->setLevel(level);
        return true;
    }

    // 新增：安全重命名日志器（更新映射并修改内部名称）
    bool renameLogger(const std::string& oldName, const std::string& newName)
    {
        if (oldName == newName) return true;
        std::unique_lock<std::mutex> lock(_mutex);
        auto it = _loggers.find(oldName);
        if (it == _loggers.end()) return false;
        if (_loggers.find(newName) != _loggers.end()) return false; // 新名已存在
        Logger::ptr lg = it->second;
        _loggers.erase(it);
        lg->setLoggerNameUnsafe(newName);
        _loggers.emplace(newName, lg);
        if (_root_logger == lg) {
            _root_logger = lg; // 根指针不变，名称已更新
        }
        return true;
    }

private:
    LoggerManager()
    {
        std::unique_ptr<lcz::LoggerBuilder> builder(new lcz::LocalLoggerBuilder());
        builder->buildloggername("root_logger");
        _root_logger = builder->build();
        _loggers.insert(std::make_pair("root_logger", _root_logger));
    }

    mutable std::mutex _mutex;
    Logger::ptr _root_logger;
    std::unordered_map<std::string, Logger::ptr> _loggers;
};


// 全局日志器建造者
class GlobalLoggerBuilder : public LoggerBuilder
{
public:
    virtual Logger::ptr build() override
    {
        assert(!_logger_name.empty());
        if (!_formatter) {
            _formatter = std::make_shared<Formatter>();
        }
        if (_sinks.empty()) {
            buildloggersink<StdoutSink>();
        }
        Logger::ptr logger;
        if (_logger_type == LoggerType::LOGGER_SYNC) {
            logger = std::make_shared<SyncLogger>(_logger_name, _formatter, _level, _sinks);
        } else {
            logger = std::make_shared<AsyncLogger>(_logger_name, _formatter, _level, _sinks, _looper_type);
        }
        LoggerManager::getInstance().addLogger(logger);
        return logger;
    }
};

} // namespace lcz