/*
日志等级类
1.定义日志等级枚举
2.提供日志等级转字符串接口
*/
#pragma once

#include<iostream>

namespace lcz
{
    class LogLevel
   {
    public:
        enum value
        {
            UNKNOW=0,
            DEBUG ,//调试等级
            INFO,//提示等级
            WARN,//警告等级
            ERROR,//错误等级
            FATAL,//严重错误等级
            OFF//关闭等级
        };
        static const char *toString(LogLevel::value level) 
        {
            switch(level) 
            {
                
                case LogLevel::value::DEBUG: return "DEBUG";
                case LogLevel::value::INFO: return "INFO";
                case LogLevel::value::WARN: return "WARN";
                case LogLevel::value::ERROR: return "ERROR";
                case LogLevel::value::FATAL: return "FATAL";
                case LogLevel::value::OFF: return "OFF";
                
            }
            return "UNKNOW";
        }

    };
}