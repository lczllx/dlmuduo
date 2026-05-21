#include "../include/LoopThread.hpp"

// 构造函数直接启动线程，线程可能在构造函数返回前开始运行
// 此时 _loop 仍为 nullptr，外部 Getloop() 通过条件变量等待 _loop 非空
LoopThread::LoopThread() : _loop(nullptr), _thread(std::thread(&LoopThread::ThreadEntry, this)) {}

// 线程入口：栈上分配 EventLoop，Start() 阻塞直到 Stop() 被调用
// Start() 返回后置 _loop=nullptr，防止外部通过悬空指针访问已析构的栈对象
void LoopThread::ThreadEntry() {
    EventLoop loop;
    {
        std::unique_lock<std::mutex> lock(_mutex);
        _loop = &loop;
        _cond.notify_all();//loop实例化完唤醒可能的阻塞
    }
    loop.Start();//启动eventloop，阻塞直到 Stop() 被调用
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
    Stop();
}

// 停止 EventLoop 并等待线程退出
// 先通过 _loop 指针调用 Stop() 唤醒 epoll_wait，再 join 等待 ThreadEntry 返回
void LoopThread::Stop() {
    {
        std::unique_lock<std::mutex> lock(_mutex);
        if (_loop) {
            _loop->Stop();// 设置 _running=false + wakeup eventfd，使 Start() 退出
        }
    }
    if (_thread.joinable()) {
        _thread.join();// 等待 ThreadEntry 返回，EventLoop 栈对象安全析构
    }
}