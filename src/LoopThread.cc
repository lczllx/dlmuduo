#include "../include/LoopThread.hpp"
#include "../include/log_system/lcz_log.h"

LoopThread::LoopThread() : _loop(nullptr), _thread(std::thread(&LoopThread::ThreadEntry, this)) {}

// 线程入口：栈上分配 EventLoop，Start() 阻塞直到 Quit() 被调用
void LoopThread::ThreadEntry() {
    LCZ_DEBUG("ThreadEntry() begin");
    EventLoop loop;
    LCZ_DEBUG("ThreadEntry() EventLoop constructed, entering Start()");
    {
        std::unique_lock<std::mutex> lock(_mutex);
        _loop = &loop;
        _cond.notify_all();//loop实例化完唤醒可能的阻塞
    }
    loop.Start();//启动eventloop，阻塞直到 Quit()
    {
        std::unique_lock<std::mutex> lock(_mutex);
        _loop = nullptr;
        _cond.notify_all();
    }
}

// 阻塞等待直到 ThreadEntry 中 EventLoop 构造完成并设置 _loop
EventLoop* LoopThread::Getloop() {
    EventLoop* loop = nullptr;
    {
        std::unique_lock<std::mutex> lock(_mutex);
        _cond.wait(lock, [&](){ return _loop != nullptr; });//阻塞至loop实例化完成
        loop = _loop;
    }
    return loop;
}

LoopThread::~LoopThread() {
    {
        std::unique_lock<std::mutex> lock(_mutex);
        if (_loop) {
            _loop->Quit();  // 通知 loop 退出
        }
    }
    if (_thread.joinable()) {
        _thread.join();     // 等待线程退出
    }
}
