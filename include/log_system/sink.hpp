/*
日志落地模块的实现
1.抽象日志落地基类
2.派生子类
  1.控制台落地
  2.文件落地：固定文件，滚动文件
3.使用工厂模式进行创建与表示分离
*/
#pragma once
#include<iostream>
#include<sstream>
#include<memory>
#include<fstream>
#include<atomic>
#include "utility.hpp"
#include <unistd.h> // for fileno, fsync
#include <fcntl.h>

namespace lcz
{
    //日志落地基类
    class LogSink
    {
        public:
        using ptr=std::shared_ptr<LogSink>;
        virtual ~LogSink(){}//方便用户对其进行更改
        virtual void log(const char*data,size_t len)=0;//日志落地接口
        //virtual void flush()=0;
    };
    //1----------
    //控制台落地
//     class StdoutSink:public LogSink
//     {
//         public:
//         virtual void log(const char* data,size_t len) override {
//             std::cout.write(data, (std::streamsize)len);
//             std::cout.flush();
//         }
//     };
//     //固定文件落地
//     class FileSink : public LogSink {
//     public:
//         explicit FileSink(const std::string &filepath) {
//             _ofs.open(filepath, std::ios::out | std::ios::app | std::ios::binary);
//             if(!_ofs.is_open()){
//                 std::cerr << "FileSink open failed: " << filepath << std::endl;
//             }
//         }
//         virtual ~FileSink(){
//             if(_ofs.is_open()) _ofs.close();
//         }
//         virtual void log(const char* data,size_t len) override {
//             if(!_ofs.is_open()) return;
//             _ofs.write(data, (std::streamsize)len);
//             _ofs.flush(); // 把 C++ 流缓冲区写到内核
//             // 可选：强制写到磁盘（代价较大）
//             // int fd = fileno((FILE*)_ofs); // 不能直接这样转换
//             // 使用底层 fd：
//             int fd = _fileno();
//             if(fd >= 0) {
//                 fsync(fd);
//             }
//         }
//     private:
//         std::ofstream _ofs;
//         int _fileno() {
//             // 从 std::ofstream 获取底层 fd（实现依赖 libc++/libstdc++）
//             // 下面是可行的通用方式：通过 rdbuf()->fd() 不是标准，但常见实现支持
//             auto *fb = _ofs.rdbuf();
// #if defined(__gnu_linux__) || defined(__linux__)
//             int fd = -1;
//             // try dynamic_cast to filebuf and use fileno from FILE* if available
//             // portable approach: open file with open() + write() instead of ofstream
//             return ::fileno(fdopen(dup(::open("/proc/self/fd/0", O_RDONLY)), "r")); // fallback - return -1
// #else
//             (void)fb;
//             return -1;
// #endif
//         }
//     };
    //2----------------
     //控制台落地
//     class StdoutSink:public LogSink
//     {
//         public:
//         virtual void log(const char*data,size_t len)override
//         {
//             std::cout.write(data,len);
//         }
//     };
//     class FileSink : public LogSink {
//    public:
//     explicit FileSink(const std::string &filepath) {
//         _fd = ::open(filepath.c_str(), O_CREAT | O_WRONLY | O_APPEND, 0644);
//         if(_fd < 0) std::cerr<<"open file sink failed: "<<strerror(errno)<<"\n";
//     }
//     ~FileSink(){
//         if(_fd>=0) ::close(_fd);
//     }
//     virtual void log(const char* data,size_t len) override {
//         if(_fd<0) return;
//         ssize_t n = ::write(_fd, data, len);
//         (void)n;
//         // 如果想确保落盘，取消下面注释（会极大影响性能）
//         // ::fsync(_fd);
//     }
// private:
//     int _fd = -1;
// };
//3----------------------
    // //控制台落地
    class StdoutSink:public LogSink
    {
        public:
        using ptr = std::shared_ptr<StdoutSink>;
        StdoutSink() = default;
        virtual void log(const char*data,size_t len)override
        {
            std::cout.write(data,len);
        }
    };
    //固定文件落地
    class FileSink:public LogSink
    {
        public:
        //构造时传入文件名，并打开文件，将操作句柄管理起来
        FileSink(const std::string &pathname)
        :_pathname(pathname)
        {
            lcz::utility::File::createDirectory(utility::File::path(_pathname));
            _ofs.open(_pathname,std::ios::binary | std::ios::app);
            assert(_ofs.is_open());

        //     // 获取文件描述符用于后续的fsync
        // _fd = fileno(_ofs.rdbuf()->__file());
        }
        //const std::string &file() {return _filename; }
        virtual void log(const char*data,size_t len)override
        {           
            _ofs.write(data,len);//写入这个流中
            assert(_ofs.good());
            //  //立即刷新缓冲区到操作系统
            //  _ofs.flush();
        }
        // // 添加析构函数确保资源释放
        // virtual ~FileSink() {
        //     if (_ofs.is_open()) {
        //         _ofs.close();
        //     }
        // }
        private:
            std::string _pathname;//文件路径名
            std::ofstream _ofs;
            // int _fd; // 文件描述符
    };
    //滚动文件落地（按大小滚动）
    class RollfileSink:public LogSink
    {
        public:
        //构造时传入文件名，并打开文件，将操作句柄管理起来
        RollfileSink(const std::string &basename,size_t max_size)
        :_basename(basename),_max_size(max_size),_cur_size(0)
        {
            std::string pathname = craetefilename();
            lcz::utility::File::createDirectory(utility::File::path(pathname));
            _ofs.open(pathname,std::ios::binary | std::ios::app);
            assert(_ofs.is_open());

        }
        virtual void log(const char*data,size_t len)override
        {
            if(_cur_size+len>_max_size)
            {
                _ofs.close();//需要先关闭当前文件
                //重新创建一个新文件
                _cur_size=0;
                std::string pathname = craetefilename();
                lcz::utility::File::createDirectory(utility::File::path(pathname));
                _ofs.open(pathname,std::ios::binary | std::ios::app);
                assert(_ofs.is_open());
                _cur_size=len;
            }
            else
            {
                _cur_size+=len;
            }
            _ofs.write(data,len);
            assert(_ofs.good());

        }
        private:
        std::string craetefilename()
        {
            static int seq = 0;
            std::stringstream ss;
            ss<<_basename;
            time_t t=time(nullptr);
            struct tm tt;
            localtime_r(&t,&tt);
            ss<<tt.tm_year+1900;
            ss<<tt.tm_mon+1;
            ss<<tt.tm_mday;
            ss<<tt.tm_hour;
            ss<<tt.tm_min;
            ss<<tt.tm_sec;
            ss << "_" << seq++;
            ss<<".log";
            return ss.str();

        }

        private:
        //通过基础文件名+时间戳+后缀.log来命名
            std::string _basename;//文件基础名
            std::ofstream _ofs;
            size_t _max_size;//最大文件大小
            size_t _cur_size;//当前文件大小
    };
//滚动文件落地（按时间滚动）
class RollbytimefileSink:public LogSink
{
    public:
    //构造时传入文件名，并打开文件，将操作句柄管理起来
    RollbytimefileSink(const std::string &basename,time_t roll_interval)
    :_basename(basename),_roll_interval(roll_interval),_last_roll_time(0)
    {
        std::string pathname = craetefilename();
        lcz::utility::File::createDirectory(utility::File::path(pathname));
        _ofs.open(pathname,std::ios::binary | std::ios::app);
        assert(_ofs.is_open());
        _last_roll_time = time(nullptr);

    }
    virtual void log(const char*data,size_t len)override
    {
        time_t now = time(nullptr);
        if(now - _last_roll_time >= _roll_interval)
        {
            _ofs.close();//需要先关闭当前文件
            //重新创建一个新文件
            std::string pathname = craetefilename();
            lcz::utility::File::createDirectory(utility::File::path(pathname));
            _ofs.open(pathname,std::ios::binary | std::ios::app);
            assert(_ofs.is_open());
            _last_roll_time = now;
        }
        _ofs.write(data,len);
        assert(_ofs.good());

    }
    private:
    std::string craetefilename()
    {
        static int seq = 0;
        std::stringstream ss;
        ss<<_basename;
        time_t t=time(nullptr);
        struct tm tt;
        localtime_r(&t,&tt);
        ss<<tt.tm_year+1900;
        ss<<tt.tm_mon+1;
        ss<<tt.tm_mday;
        ss<<tt.tm_hour;
        ss<<tt.tm_min;
        ss<<tt.tm_sec;
        ss << "_" << seq++;
        ss<<".log";
        return ss.str();

    }

    private:
    //通过基础文件名+时间戳+后缀.log来命名
        std::string _basename;//文件基础名
        std::ofstream _ofs;
        time_t _roll_interval;//滚动时间间隔（秒）
        time_t _last_roll_time;//上次滚动时间
};

    //工厂模式，通过指定创建不同的落地方式
    //由于不同的落地模式有不同的参数数量，所以需要使用可变型参数
    class SinkFactory
    {
        public:
        template<typename Sinktype,typename ...Args>
        static LogSink::ptr create(Args &&... args)
        {
            return std::make_shared<Sinktype>(std::forward<Args>(args)...);
        }

    };
}