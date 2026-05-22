#ifndef MUDUO_LOOPTHREADPOOL_H
#define MUDUO_LOOPTHREADPOOL_H

#include "EventLoop.hpp"
#include "LoopThread.hpp"
#include <vector>
#include <memory>

//结合loopthread的线程池
class LoopThreadPool {
private:
    int _thread_cnt;//线程数量
    int _next_loop_idx;//下一个线程索引
    EventLoop* _base_loop;//主reactor，
    std::vector<EventLoop*> _loop;//用于分配eventloop
    std::vector<std::unique_ptr<LoopThread>> _threads;//保存LoopThread

public:
    LoopThreadPool(EventLoop* base_loop);
    void SetThreadCnt(int cnt);//设置线程数量
    void Create();//创建所有从属线程
    EventLoop* NextLoop();//采用rr轮转分配线程
};

#endif