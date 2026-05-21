#include "../include/EventLoop.hpp"
#include "../include/Channel.hpp"
#include "../include/log_system/lcz_log.h"
#include <cstring>
#include <cerrno>


EventLoop::EventLoop() : 
    _thread_id(std::this_thread::get_id()),
    _eventfd(CreateEventfd()),
    _event_channel(new Channel(this, _eventfd)),
    _timerwheel(this) {
    //添加可读事件回调 启动eventfd的事件监控
    _event_channel->SetReadCallback(std::bind(&EventLoop::ReadEventfd, this));
    _event_channel->EnableRead();
}

// 主循环：epoll_wait → 处理 IO 事件 → 执行任务队列
// Stop() 将 _running 设为 false 并 wakeup eventfd 后，循环退出
void EventLoop::Start() {
    _running = true;
    while(_running) {
        std::vector<Channel*> actives;
        _poller.Poll(&actives);
        for(auto &e : actives) {
            e->HandleEvent();//处理io
        }
        RunAllTask();//执行任务池任务
    }
    // 退出前排空任务队列：Stop() 或 Release() 可能通过 TasksInLoop 投递了清理任务
    RunAllTask();
}

void EventLoop::Stop() {
    _running = false;
    WeakupEventfd();// 唤醒可能因无事件而阻塞在 epoll_wait 的 Start()
}

// swap 到栈上再执行：减小临界区（只持有锁做 swap），避免在执行回调时持有锁导致死锁
void EventLoop::RunAllTask() {
    std::vector<Tasks> tmp;
    {
        std::unique_lock<std::mutex> _lock(_mutex);
        _task.swap(tmp);
    }
    for(auto &task : tmp) {
        task();
    }
}

int EventLoop::CreateEventfd() {
    //EFD_CLOEXEC禁止进程复制 EFD_NONBLOCK启动非阻塞
    int efd = eventfd(0, EFD_CLOEXEC | EFD_NONBLOCK);
    if(efd < 0) {
        LCZ_ERROR("create eventfd failed");
        abort();
    }
    return efd;
}

void EventLoop::ReadEventfd() {
    uint64_t res = 0;
    int ret = read(_eventfd, &res, sizeof(res));
    if(ret < 0) {
        if(errno == EINTR || errno == EAGAIN) return;
        LCZ_ERROR("read eventfd failed");
        abort();
    }
}

void EventLoop::WeakupEventfd() {
    uint64_t val = 1;
    int ret = write(_eventfd, &val, sizeof(val));//写入触发可读事件
    if(ret < 0) {
        if(errno == EINTR) return;
        LCZ_ERROR("write eventfd failed");
        abort();
    }
}

void EventLoop::RunInLoop(const Tasks& t) {
    if(IsInLoop()) return t();
    return TasksInLoop(t);//压入任务池
}

void EventLoop::TasksInLoop(const Tasks& t) {
    {
        std::unique_lock<std::mutex> _lock(_mutex);
        _task.push_back(t);
    }
    WeakupEventfd();//唤醒有可能因为没有事件就绪而造成的epoll阻塞
}

bool EventLoop::IsInLoop() {
    return (_thread_id == std::this_thread::get_id());
}

void EventLoop::UpdateEvent(Channel* channel) {
    _poller.UpdateEvent(channel);
}

void EventLoop::RemoveEvent(Channel* channel) {
    _poller.RemoveEvent(channel);
}

void EventLoop::TimerAdd(uint64_t id, uint32_t delay, const TaskFunc& cb) {
    _timerwheel.TimerAdd(id, delay, cb);
}

void EventLoop::TimerReflesh(uint64_t id) {
    _timerwheel.TimerReflesh(id);
}

void EventLoop::TimerCancel(uint64_t id) {
    _timerwheel.TimerCancel(id);
}

bool EventLoop::HasTimer(uint64_t id) {
    return _timerwheel.HasTimer(id);
}

void EventLoop::AssertInLoop() {
    assert(_thread_id == std::this_thread::get_id());
}