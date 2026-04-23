/*
日志信息类
1.存储一条信息所需要的各项信息（时间、日志等级、文件名、行号、线程id，函数名、日志内容）
*/
#pragma once

#include<iostream>
#include<ctime>
#include"level.hpp"
#include<thread>
#include"utility.hpp"

namespace lcz
{
    struct Logmsg
    {
        time_t _time; // 时间
        lcz::LogLevel::value _level; // 日志等级
        std::string _file; // 文件名
        size_t _line; // 行号
        std::thread::id _tid; // 线程id
        std::string _logger; //日志器名称
        std::string _payload; // 日志内容   
        Logmsg(){}
        Logmsg(lcz::LogLevel::value level,
            const std::string &file,
            size_t line,
            const std::string &logger,
            const std::string &payload)
            :_time(utility::Date::getTime()),
            _level(level),
            _file(file),
            _line(line),
            _tid(std::this_thread::get_id()),
            _logger(logger),
            _payload(payload)
            {}
    };
}