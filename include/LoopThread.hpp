#ifndef MUDUO_LOOPTHREAD_H
#define MUDUO_LOOPTHREAD_H

#include "EventLoop.hpp"
#include <thread>
#include <mutex>
#include <condition_variable>

//eventloop和线程整合的模块
class LoopThread {
private:
    std::mutex _mutex;
    std::condition_variable _cond;//条件变量 结合互斥锁实现loop获取同步关系

    EventLoop* _loop;//在线程内实例化eventloop指针，ThreadEntry 返回前置 nullptr
    std::thread _thread;//eventloop对应线程

    void ThreadEntry();//实例化eventloop对象并启动eventloop

public:
    LoopThread();
    ~LoopThread();
    EventLoop* Getloop();
};

#endif