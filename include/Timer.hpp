#ifndef MUDUO_TIMER_H
#define MUDUO_TIMER_H

#include <memory>
#include <unordered_map>
#include <vector>
#include <functional>
#include <sys/timerfd.h>
#include <unistd.h>
#include "log_system/lcz_log.h"
#include "CallbackTypes.hpp"
#include "Channel.hpp"

/*定时任务模块*/
class EventLoop;
class Channel;
class TimerTask
{
private:
    uint64_t _timer_id;      // 定时器id
    uint32_t _timeout;       // 延迟时间
    bool _cancel;            // 为false时任务正常执行
    TaskFunc _timer_cb;      // 析构时要调用的定时任务
    ReleaseFunc _release_cb; // 用于删除TimingWheel中保存的定时器对象信息
public:
    TimerTask(uint64_t id, uint32_t delay, const TaskFunc &cb)
        : _timer_id(id), _timeout(delay), _cancel(false), _timer_cb(cb) {}
    ~TimerTask()
    {
        // if(_timer_cb && _cancel==false)_timer_cb();//忘记将_cancel==false加上，导致段错误
        try
        {
            if (_cancel == false && _timer_cb)
            {
                _timer_cb();
            }
        }
        catch (...)
        {
            LCZ_ERROR("TimerTask callback threw exception");
        }
        if (_release_cb)
            _release_cb();
    }
    void SetRelease(const ReleaseFunc &cb) { _release_cb = cb; }
    uint32_t DelayTime() { return _timeout; }
    void Cancel() { _cancel = true; } // 添加取消接口
};
class TimingWheel
{
private:
    using PtrTask = std::shared_ptr<TimerTask>;
    using WeakTask = std::weak_ptr<TimerTask>;
    int _tick;     // 秒针
    int _capacity; // 时间轮的大小
    std::vector<std::vector<PtrTask>> _wheel;
    std::unordered_map<uint64_t, WeakTask> _timers;

    EventLoop *_loop;
    int _timefd; // 定时器描述符 可读事件回调就是读取计数器，执行定时任务
    std::unique_ptr<Channel> _timer_channel;
private:
    void RemoveTimer(uint64_t id)
    {
        auto it = _timers.find(id);
        if (it == _timers.end())
            return;
        _timers.erase(it);
    }

    static int CreateTimerfd()
    {
        int timefd = timerfd_create(CLOCK_MONOTONIC, 0);//创建timerfd
        if (timefd < 0)
        {
            LCZ_ERROR("timerfd create failed");
            abort();
        }
        struct itimerspec itime;
        itime.it_value.tv_sec = 1;
        itime.it_value.tv_nsec = 0; // 第一次超时时间为1s后
        itime.it_interval.tv_sec = 1;
        itime.it_interval.tv_nsec = 0; // 第一次超时后每次的超时时限
        timerfd_settime(timefd, 0, &itime, NULL);
        return timefd;
    }

    int ReadTimerfd()//触发可读调用
    {
        uint64_t times;
        // 有可能因为其他描述符的事件处理花费时间比较长，任何在处理定时器描述符时，已经超时很多次了
        int ret = read(_timefd, &times, 8);
        if (ret < 0)
        {
            LCZ_ERROR("read timerfd failed");
            abort();
        }
        return times; // 返回从上一次read之后超时的次数
    }

    void RunTimerTask() //*
    {
        //版本1
        // _tick=(_tick+1)%_capacity;
        // //当前槽任务逐个手动取消触发析构
        // for (auto& task : _wheel[_tick]) {
        //         task->Cancel();  // 标记已执行
        // }

        // _wheel[_tick].clear();// 清除当前槽中所有任务
        //-------------------
        //版本2
        auto &slot = _wheel[_tick];      // 获取当前槽位
        slot.clear();                    // 清空槽位，触发所有Task析构（自动执行回调）
        _tick = (_tick + 1) % _capacity; // 移动秒针
    }

    void OnTime()//时间到了
    {
        // 根据实际超时的次数，执⾏对应的超时任务
        int times = ReadTimerfd();
        for (int i = 0; i < times; i++)
        {
            RunTimerTask();
        }
    }

    void TimerAddInLoop(uint64_t id, uint32_t delay, const TaskFunc &cb)
    {
        // PtrTask pt=std::make_shared<TimerTask>(id, timeout, cb);
        if (delay == 0 || delay >= static_cast<uint32_t>(_capacity))
        {
            LCZ_ERROR("Invalid timer delay: %u (max: %d)", delay, (_capacity - 1));
            return;
        }
        PtrTask pt(new TimerTask(id, delay, cb));
        pt->SetRelease(std::bind(&TimingWheel::RemoveTimer, this, id));
        int pos = (delay + _tick) % _capacity;
        _wheel[pos].push_back(pt);
        _timers[id] = WeakTask(pt);
        LCZ_DEBUG("Added timer %lu to slot %d (delay %u)", id, pos, delay);
    }
    
    void TimerRefleshInLoop(uint64_t id)
    {
        auto it = _timers.find(id);
        if (it == _timers.end())
            return;
        PtrTask pt = it->second.lock(); // 获取weak_ptr管理的对应的shared_ptr
        if (!pt)
        { // 定时任务已过期
            _timers.erase(it);
            return;
        }
        int delay = pt->DelayTime();
        int pos = (delay + _tick) % _capacity;
        _wheel[pos].push_back(pt);
    }

    void TimerCancelInLoop(uint64_t id)
    {
        auto it = _timers.find(id);
        if (it == _timers.end())
            return;
        PtrTask pt = it->second.lock();
        if (pt)
            pt->Cancel();
    }

public:
    TimingWheel(EventLoop *loop) : _tick(0), _capacity(360), _wheel(_capacity), _loop(loop),
                                   _timefd(CreateTimerfd()), _timer_channel(new Channel(_loop, _timefd))
    {
        _timer_channel->SetReadCallback(std::bind(&TimingWheel::OnTime, this));
        _timer_channel->EnableRead(); // 开启读事件监控
    }
    // 定时器有个_timers成员，定时器信息的操作可能会在多线程中进行，因此需要考虑线程安全问题
    // 如果不加锁，就要把对定时器的操作放到同一个线程中执行
    void TimerAdd(uint64_t id, uint32_t delay, const TaskFunc &cb);
    // 刷新或延迟定时任务
    void TimerReflesh(uint64_t id);
    void TimerCancel(uint64_t id);
    // 存在线程安全问题，只能在对应的EventLoop的线程内执行
    bool HasTimer(uint64_t id)
    {
        auto it = _timers.find(id);
        if (it == _timers.end())
        {
            return false;
        }
        return true;
    }
};

#endif