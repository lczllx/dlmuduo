#ifndef MUDUO_EVENTLOOP_H
#define MUDUO_EVENTLOOP_H

#include <thread>
#include <mutex>
#include <vector>
#include <functional>
#include <memory>
#include <condition_variable>
#include <sys/eventfd.h>
#include <unistd.h>
#include <cassert>
#include <atomic>

#include "Logger.hpp"
#include "Poller.hpp"
#include "Timer.hpp"
/*One Loop Per Thread 模型核心：每个线程绑定一个 EventLoop
  主循环：epoll_wait(永久阻塞，-1) → 处理就绪事件 → 执行任务队列 → 回到 epoll_wait
  跨线程唤醒：RunInLoop 判断是否在本线程，不在则 TasksInLoop+eventfd 唤醒
  退出机制：Quit() 设置 _quit=true + eventfd 唤醒 → Start() 退出循环 → 执行残留任务 → 返回
  线程安全：RunInLoop/TasksInLoop 通过 mutex+eventfd 实现跨线程安全投递
  调试：AssertInLoop() 用于断言当前在 EventLoop 线程内*/
class Channel;
class LoopThread;
class EventLoop
{
private:
    std::thread::id _thread_id;
    std::atomic<bool> _quit;     // Quit() 置 true，Start() 检查后退出循环
    int _eventfd;
    Poller _poller;
    std::unique_ptr<Channel> _event_channel;
    using Tasks = std::function<void()>;
    std::vector<Tasks> _task;
    std::mutex _mutex;
    TimingWheel _timerwheel;

    void RunAllTask();
    static int CreateEventfd();
    void ReadEventfd();
    void WeakupEventfd();

public:
    EventLoop();
    ~EventLoop();
    void Start();                 // 启动事件循环，Quit() 可安全退出
    void Quit();                  // 安全退出事件循环（可跨线程调用）
    void RunInLoop(const Tasks &t);
    void TasksInLoop(const Tasks &t);
    bool IsInLoop();
    void UpdateEvent(Channel *channel);
    void RemoveEvent(Channel *channel);
    void TimerAdd(uint64_t id, uint32_t delay, const TaskFunc &cb);
    void TimerReflesh(uint64_t id);
    void TimerCancel(uint64_t id);
    bool HasTimer(uint64_t id);
    void AssertInLoop();
};

#endif