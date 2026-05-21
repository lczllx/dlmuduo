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

#include "Logger.hpp"
#include "Poller.hpp"
#include "Timer.hpp"
#include <atomic>

/*One Loop Per Thread 模型核心：每个线程绑定一个 EventLoop
  主循环：epoll_wait(永久阻塞，-1) → 处理就绪事件 → 执行任务队列 → 回到 epoll_wait
  跨线程唤醒：RunInLoop 判断是否在本线程，不在则 TasksInLoop+eventfd 唤醒
  线程安全：RunInLoop/TasksInLoop 通过 mutex+eventfd 实现跨线程安全投递
  优雅退出：Stop() 设置 _running=false + 唤醒 eventfd → Start() 退出循环 → 最后排空任务队列
  调试：AssertInLoop() 用于断言当前在 EventLoop 线程内*/
class Channel;
class LoopThread;
class EventLoop
{
private:
    std::thread::id _thread_id; // 创建时记录本线程ID，用于 IsInLoop() 校验
    int _eventfd;               // 跨线程唤醒：写入8字节触发可读，打断 epoll_wait 永久阻塞
    std::atomic<bool> _running{false}; // 运行标志，Stop() 设为 false 后 Start() 退出主循环
    Poller _poller;
    std::unique_ptr<Channel> _event_channel; // eventfd 对应的 Channel
    using Tasks = std::function<void()>;
    std::vector<Tasks> _task; // 任务队列，加锁后 swap 到栈上执行，减少锁持有时间
    std::mutex _mutex;
    TimingWheel _timerwheel;

    void RunAllTask();          // 运行任务池所有任务
    static int CreateEventfd(); // 创建eventfd
    void ReadEventfd();         // 读取eventfd
    void WeakupEventfd();       // 唤醒epoll_wait可能因为没有事件就绪而造成的阻塞

public:
    EventLoop();
    void Start(); // 启动事件循环，阻塞直到 Stop() 被调用
    void Stop();  // 设置 _running=false 并通过 eventfd 唤醒，使 Start() 退出（线程安全）
    // 若当前在 EventLoop 线程则直接执行，否则入队并通过 eventfd 唤醒（减少锁+唤醒开销）
    void RunInLoop(const Tasks &t);
    // 始终入队，不判断当前线程——用于必须延迟执行或在析构路径中使用的场景
    void TasksInLoop(const Tasks &t);
    bool IsInLoop();                                                // 判断当前线程是不是在Eventloop所在线程里面
    void UpdateEvent(Channel *channel);                             // 更新描述符的事件监控
    void RemoveEvent(Channel *channel);                             // 移除描述符的事件监控
    void TimerAdd(uint64_t id, uint32_t delay, const TaskFunc &cb); // 添加定时器
    void TimerReflesh(uint64_t id);                                 // 刷新定时器
    void TimerCancel(uint64_t id);                                  // 取消定时
    bool HasTimer(uint64_t id);                                     // 是否有这个定时器
    void AssertInLoop();                                            // 断言在eventloop线程里面
};

#endif