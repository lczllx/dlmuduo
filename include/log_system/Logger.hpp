// /*
// 日志器模块
// 1.管理格式化模块对象，落地模块对象数组（一个日志器可能向多个位置进行输出）
// 2.默认的日志输出限制
// 3.互斥锁（保证线程安全，不会查询交叉日志）
// 4.日志器名称（日志的标识，以便后续查询）
// ***抽象logger基类，派生出同步日志器类，异步日志器类

// */
// #pragma once
// #include <cstdarg>      // 用于 va_list, va_start, va_end
// #include <cstdio>       // 用于 vasprintf
// #include<thread>
// #include <utility> 
// #include<functional>
// #include<mutex>
// #include<atomic>
// #include<unordered_map>
// #include<stdarg.h>
// #include "utility.hpp"
// #include "level.hpp"
// #include "Logformat.hpp"
// #include "sink.hpp"
// #include "looper.hpp"

// namespace lcz
// {
    
//     class Logger
//     {
//         public:
//         using ptr = std::shared_ptr<Logger>;//logger基类智能指针
//         Logger(std::string &logger_name,Formatter::ptr formatter,LogLevel::value level,std::vector<LogSink::ptr>& logsink)
//         :_logger_name(logger_name),_formatter(formatter),_limit_level(level),_logsink(logsink.begin(),logsink.end())
//         {}
//         std::vector<LogSink::ptr> getSinks() const {
//         return _logsink;
//     }
//         std::string getLogger(){return _logger_name;}
//         //完成构造日志消息对象过程并格式化，得到格式化后的日志消息字符串
//         void Debug(const std::string &file,const size_t line,const std::string &fmt,...)
//         {
//             //通过传入的参数构造一个日志消息对象，对日志进行格式化
//             //先判断日志等级
//             if(LogLevel::value::DEBUG<_limit_level)return;

//             //对fmt格式化字符串和不定参进行组织，得到日志消息字符串
//             // 创建可变参数列表对象，用于访问可变参数
//             va_list vl;
//             // 初始化可变参数列表，vl指向fmt之后的第一个参数
//             va_start(vl, fmt);
//             // 声明指针，用于接收vasprintf动态分配的内存地址
//             char* res = nullptr;
//             // 使用vasprintf格式化字符串：
//             // - 自动分配足够的内存来存储格式化后的字符串
//             // - 返回格式化后的字符串长度（不包括结尾的null字符）
//             // - 返回值小于0表示分配失败
//             int ret = vasprintf(&res, fmt.c_str(), vl);
//             if (ret < 0) {
//                 // vasprintf失败，可能是内存分配失败
//                 std::cout << "vasprintf failed: memory allocation error" << std::endl;               
//                 // 清理可变参数列表
//                 va_end(vl);
//                 return;
//             }
//             // 清理可变参数列表，结束对可变参数的访问
//             va_end(vl);

//             //构建logmsg对象
//             Logmsg msg(LogLevel::value::DEBUG,file,line,_logger_name,res);

//             //通过格式化工具对msg进行格式化
//             std::stringstream ss;
//             _formatter->format(ss,msg);

//             //进行日志落地
//             log(ss.str().c_str(),ss.str().size());



//             free(res);//res指向动态分配的内存，使用后需要手动释放

//         }
//         void Info(const std::string & file,const size_t line,const std::string &fmt,...)
//         {
//             //通过传入的参数构造一个日志消息对象，对日志进行格式化
//             //先判断日志等级
//             if(LogLevel::value::INFO<_limit_level)return;

//             //对fmt格式化字符串和不定参进行组织，得到日志消息字符串
//             // 创建可变参数列表对象，用于访问可变参数
//             va_list vl;
//             // 初始化可变参数列表，vl指向fmt之后的第一个参数
//             va_start(vl, fmt);
//             // 声明指针，用于接收vasprintf动态分配的内存地址
//             char* res = nullptr;
//             // 使用vasprintf格式化字符串：
//             // - 自动分配足够的内存来存储格式化后的字符串
//             // - 返回格式化后的字符串长度（不包括结尾的null字符）
//             // - 返回值小于0表示分配失败
//             int ret = vasprintf(&res, fmt.c_str(), vl);
//             if (ret < 0) {
//                 // vasprintf失败，可能是内存分配失败
//                 std::cout << "vasprintf failed: memory allocation error" << std::endl;               
//                 // 清理可变参数列表
//                 va_end(vl);
//                 return;
//             }
//             // 清理可变参数列表，结束对可变参数的访问
//             va_end(vl);

//             //构建logmsg对象
//             Logmsg msg(LogLevel::value::INFO,file,line,_logger_name,res);

//             //通过格式化工具对msg进行格式化
//             std::stringstream ss;
//             _formatter->format(ss,msg);

//             //进行日志落地
//             log(ss.str().c_str(),ss.str().size());

//             free(res);//res指向动态分配的内存，使用后需要手动释放
//         }
//         void Warn(const std::string & file,const size_t line,const std::string &fmt,...)
//         {
//             //通过传入的参数构造一个日志消息对象，对日志进行格式化
//             //先判断日志等级
//             if(LogLevel::value::WARN<_limit_level)return;

//             //对fmt格式化字符串和不定参进行组织，得到日志消息字符串
//             // 创建可变参数列表对象，用于访问可变参数
//             va_list vl;
//             // 初始化可变参数列表，vl指向fmt之后的第一个参数
//             va_start(vl, fmt);
//             // 声明指针，用于接收vasprintf动态分配的内存地址
//             char* res = nullptr;
//             // 使用vasprintf格式化字符串：
//             // - 自动分配足够的内存来存储格式化后的字符串
//             // - 返回格式化后的字符串长度（不包括结尾的null字符）
//             // - 返回值小于0表示分配失败
//             int ret = vasprintf(&res, fmt.c_str(), vl);
//             if (ret < 0) {
//                 // vasprintf失败，可能是内存分配失败
//                 std::cout << "vasprintf failed: memory allocation error" << std::endl;               
//                 // 清理可变参数列表
//                 va_end(vl);
//                 return;
//             }
//             // 清理可变参数列表，结束对可变参数的访问
//             va_end(vl);

//             //构建logmsg对象
//             Logmsg msg(LogLevel::value::WARN,file,line,_logger_name,res);

//             //通过格式化工具对msg进行格式化
//             std::stringstream ss;
//             _formatter->format(ss,msg);

//             //进行日志落地
//             log(ss.str().c_str(),ss.str().size());

//             free(res);//res指向动态分配的内存，使用后需要手动释放
//         }
//         void Error(const std::string & file,const size_t line,const std::string &fmt,...)
//         {
//             //通过传入的参数构造一个日志消息对象，对日志进行格式化
//             //先判断日志等级
//             if(LogLevel::value::ERROR<_limit_level)return;

//             //对fmt格式化字符串和不定参进行组织，得到日志消息字符串
//             // 创建可变参数列表对象，用于访问可变参数
//             va_list vl;
//             // 初始化可变参数列表，vl指向fmt之后的第一个参数
//             va_start(vl, fmt);
//             // 声明指针，用于接收vasprintf动态分配的内存地址
//             char* res = nullptr;
//             // 使用vasprintf格式化字符串：
//             // - 自动分配足够的内存来存储格式化后的字符串
//             // - 返回格式化后的字符串长度（不包括结尾的null字符）
//             // - 返回值小于0表示分配失败
//             int ret = vasprintf(&res, fmt.c_str(), vl);
//             if (ret < 0) {
//                 // vasprintf失败，可能是内存分配失败
//                 std::cout << "vasprintf failed: memory allocation error" << std::endl;               
//                 // 清理可变参数列表
//                 va_end(vl);
//                 return;
//             }
//             // 清理可变参数列表，结束对可变参数的访问
//             va_end(vl);

//             //构建logmsg对象
//             Logmsg msg(LogLevel::value::ERROR,file,line,_logger_name,res);

//             //通过格式化工具对msg进行格式化
//             std::stringstream ss;
//             _formatter->format(ss,msg);

//             //进行日志落地
//           log(ss.str().c_str(),ss.str().size());

//             free(res);//res指向动态分配的内存，使用后需要手动释放
//         }
//         void Fatal(const std::string & file,const size_t line,const std::string &fmt,...)
//         {
//             //通过传入的参数构造一个日志消息对象，对日志进行格式化
//             //先判断日志等级
//             if(LogLevel::value::FATAL<_limit_level)return;

//             //对fmt格式化字符串和不定参进行组织，得到日志消息字符串
//             // 创建可变参数列表对象，用于访问可变参数
//             va_list vl;
//             // 初始化可变参数列表，vl指向fmt之后的第一个参数
//             va_start(vl, fmt);
//             // 声明指针，用于接收vasprintf动态分配的内存地址
//             char* res = nullptr;
//             // 使用vasprintf格式化字符串：
//             // - 自动分配足够的内存来存储格式化后的字符串
//             // - 返回格式化后的字符串长度（不包括结尾的null字符）
//             // - 返回值小于0表示分配失败
//             int ret = vasprintf(&res, fmt.c_str(), vl);
//             if (ret < 0) {
//                 // vasprintf失败，可能是内存分配失败
//                 std::cout << "vasprintf failed: memory allocation error" << std::endl;               
//                 // 清理可变参数列表
//                 va_end(vl);
//                 return;
//             }
//             // 清理可变参数列表，结束对可变参数的访问
//             va_end(vl);

//             //构建logmsg对象
//             Logmsg msg(LogLevel::value::FATAL,file,line,_logger_name,res);

//             //通过格式化工具对msg进行格式化
//             std::stringstream ss;
//             _formatter->format(ss,msg);

//             //进行日志落地
//             log(ss.str().c_str(),ss.str().size());

//             free(res);//res指向动态分配的内存，使用后需要手动释放
//         }
//         private:
//         virtual void log(const char* data,size_t len)=0;

//         protected:
//         std::mutex _mtx;                      // 互斥锁，保证线程安全
//         std::string _logger_name;             // 日志器名称，用于标识不同的日志器
//         Formatter::ptr _formatter;            // 格式化模块智能指针
//         std::atomic<LogLevel::value> _limit_level;  // 原子操作的日志级别限制
//         std::vector<LogSink::ptr> _logsink;   // 日志落地器智能指针数组，支持多输出目标

//     };
//      // 同步输出日志器：直接在当前线程执行日志输出操作
//     class SyncLogger : public Logger
//     {
//     public:
//          SyncLogger(std::string &logger_name,Formatter::ptr formatter,LogLevel::value level,std::vector<LogSink::ptr>& logsink)
//         :Logger(logger_name,formatter,level,logsink) // 调用基类构造函数
//         {}
//         virtual void log(const char* data, size_t len) override  // 同步输出实现
//         {
//             std::unique_lock<std::mutex>lock(_mtx);
//             if(_logsink.empty())return;
//             for(auto &s:_logsink)
//             {
//                 s->log(data,len);
//             }
//         }
//     };
    
//     // 异步输出日志器：将日志任务放入队列，由后台线程执行输出
//     class AsyncLogger : public Logger  
//     {
//         public:
//          AsyncLogger(std::string &logger_name, Formatter::ptr formatter, 
//            LogLevel::value level, std::vector<LogSink::ptr>& logsink, 
//            AsyncType looper_type)
//           : Logger(logger_name, formatter, level, logsink),
//           _looper(std::make_shared<AsyncLooper>(
//           std::bind(&AsyncLogger::reallog, this, std::placeholders::_1/*表示这个新的可调用对象，只需要传一个参数*/),
//           looper_type))
//           {}
//         virtual void log(const char* data, size_t len) override  // 异步输出实现
//         {
//             _looper->push(data,len);//线程已经安全
//         }
//         void reallog(Buffer &buf)//实际落地函数
//         {
//         //线程内部完成，不需要加锁
//             if(_logsink.empty())return;
//             for(auto &s:_logsink)
//             {
//                 s->log(buf.begin(),buf.abletoreadlen());//写入只存在的数据
//             }
//         }
//         private:
//         AsyncLooper::ptr _looper;//管理异步日志器
//     };

//     //使用建造者模式构造日志器，简化用户使用的复杂度
//     //抽象一个日志器建造者类 1.设置日志器类型 2.将不同类型的日志器放到同一个日志器建造者类里面完成
//     //然后派生出具体的建造者类--局部日志器的建造者、全局日志器的建造者（后面添加全局单例管理）
//     enum class LoggerType 
//     {
//         LOGGER_SYNC,    // 同步日志器
//         LOGGER_ASYNC    // 异步日志器
//     };
//     class LoggerBuilder
//     {
//         public:
//         LoggerBuilder():_logger_type(LoggerType::LOGGER_ASYNC),_level(LogLevel::value::DEBUG),_looper_type(AsyncType::ASYNC_SAFE){}

//         // 设置日志器类型（同步/异步）
//         void buildloggertype(LoggerType type){_logger_type=type;}
//         //设置非安全模式
//         void buildunsafeASYNC(){_looper_type=lcz::AsyncType::ASYNC_UNSAFE;}
//         // 设置日志器名称,必须要有
//         void buildloggername(const std::string &loggername){_logger_name=loggername;}
//         // 设置日志级别
//         void buildloggerlevel(lcz::LogLevel::value level){_level=level;}
//        // 设置日志格式化模式
//         void buildloggerformatter(const std::string &pattern)
//         {
//            _formatter=std::make_shared<Formatter>(pattern);
//         }
//         // 添加日志落地器 - 支持任意类型的落地器
//         template<typename Sinktype,typename ...Args>
//         void buildloggersink(Args &&...args)
//         {
//             _sinks.push_back(std::make_shared<Sinktype>(std::forward<Args>(args)...));
//         }
//         virtual Logger::ptr build()=0;
//         protected:
//         lcz::AsyncType _looper_type;//异步工作类型
//         LoggerType _logger_type;                  // 日志器类型（同步/异步）
//         std::string _logger_name;                 // 日志器名称标识
//         lcz::LogLevel::value _level;              // 日志级别限制
//         lcz::Formatter::ptr _formatter;           // 格式化器智能指针
//         std::vector<lcz::LogSink::ptr> _sinks;    // 落地器智能指针数组
//     };
//     // 局部日志器建造者 - 用于构造局部使用的日志器实例
//     class LocalLoggerBuilder:public LoggerBuilder  
//     {
//         public:
//         virtual Logger::ptr build()override
//         {
//             // 参数校验：日志器名称不能为空
//             assert(!_logger_name.empty());
//              // 设置默认格式化器（如果未设置）
//             if(_formatter.get()==nullptr)
//             {
//                 _formatter=std::make_shared<Formatter>();
//             }
//              // 设置默认落地器（如果未设置） 
//             if(_sinks.empty())
//             {
//                 buildloggersink<StdoutSink>();
//             }
//              // 根据类型创建不同的日志器
//             if(_logger_type==LoggerType::LOGGER_SYNC)
//             {
//                 return std::make_shared<SyncLogger>(_logger_name,_formatter,_level,_sinks);
//             }
//             else
//             {
//                 return std::make_shared<AsyncLogger>(_logger_name, _formatter, _level, _sinks,_looper_type);

//             }
//             return nullptr;
//         }

//     };
//     class LoggerManager
//     {
//         public:
//          void replaceRootLogger(Logger::ptr newRootLogger)
//         {
//             if (!newRootLogger) return;
            
//             std::unique_lock<std::mutex> lock(_mutex);
            
//             // 移除旧 root_logger 对应的名称，避免名称冲突
//             _loggers.erase(_root_logger->getLogger());

//             // 更新 root_logger 指针为新日志器
//             _root_logger = newRootLogger;
            
//             // 以新日志器名称为 key 存入管理器
//             _loggers.emplace(newRootLogger->getLogger(), newRootLogger);
//         }
//         static LoggerManager& getInstance()
//         {
//             static LoggerManager eton;
//             return eton;

//         }
//         void addLogger(Logger::ptr &logger)
//         {
//             if(!logger) return;
//             if(hasLogger(logger->getLogger()))return;
//             std::unique_lock<std::mutex> lock(_mutex);
//             _loggers.insert(std::make_pair(logger->getLogger(),logger));

//         }
//         bool hasLogger(const std::string &name)
//         {
//             std::unique_lock<std::mutex> lock(_mutex);
//             auto it =_loggers.find(name);
//             if(it!=_loggers.end())return true;
//             return false;
//         }
//         Logger::ptr getLogger(const std::string &name)
//         {
//             std::unique_lock<std::mutex> lock(_mutex);
//             auto it=_loggers.find(name);
//             if(it!=_loggers.end())
//             {
//                 return it->second;
//             }
//             return nullptr;
//         }
//         Logger::ptr rootLogger()
//         {
//             // std::unique_lock<std::mutex> lock(_mutex);
//             return _root_logger;
//         }
//         void initRootLogger(Logger::ptr logger)//-
//         {
//             if(!logger) return;
//             std::unique_lock<std::mutex> lock(_mutex);
//             if(_root_logger)
//             {
//                 // 先移除旧默认日志器名称映射
//                 _loggers.erase(_root_logger->getLogger());
//             }
//             _root_logger = logger;
//             _loggers[_root_logger->getLogger()] = _root_logger;
//         }
//         private:
//         LoggerManager()
//         {
//             std::unique_ptr<lcz::LoggerBuilder> builder(new lcz::LocalLoggerBuilder());//不能使用全局建造者，因为会死循环
//             builder->buildloggername("root_logger");
//             _root_logger=builder->build();
//             _loggers.insert(std::make_pair("root_logger",_root_logger));
//         }
//         private:
//         std::mutex _mutex;
//         Logger::ptr _root_logger;//默认日志器      
//         std::unordered_map<std::string,Logger::ptr> _loggers;//管理日志器数组


//     };

//     //全局日志器建造者 - 用于构造全局使用的日志器实例（单例模式）
//     class GlobalLoggerBuilder:public LoggerBuilder
//     {
//         public:
//         virtual Logger::ptr build()override
//         {
//             // 参数校验：日志器名称不能为空
//             assert(!_logger_name.empty());
//              // 设置默认格式化器（如果未设置）
//             if(_formatter.get()==nullptr)
//             {
//                 _formatter=std::make_shared<Formatter>();
//             }
//              // 设置默认落地器（如果未设置） 
//             if(_sinks.empty())
//             {
//                 buildloggersink<StdoutSink>();
//             }
//             Logger::ptr logger;
//              // 根据类型创建不同的日志器
//             if(_logger_type==LoggerType::LOGGER_SYNC)
//             {
//                 logger = std::make_shared<SyncLogger>(_logger_name,_formatter,_level,_sinks);
//             }
//             else
//             {
//                 logger =std::make_shared<AsyncLogger>(_logger_name, _formatter, _level, _sinks,_looper_type);

//             }
//             //将构造好的日志器放入日志器管理器中
//             LoggerManager::getInstance().addLogger(logger);
           
//             return logger;
//         }

//     };
    
    

// }
/*
日志器模块
1. 管理格式化模块对象，落地模块对象数组（一个日志器可能向多个位置进行输出）
2. 默认的日志输出限制
3. 互斥锁（保证线程安全，不会查询交叉日志）
4. 日志器名称（日志的标识，以便后续查询）
***抽象logger基类，派生出同步日志器类，异步日志器类
*/

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