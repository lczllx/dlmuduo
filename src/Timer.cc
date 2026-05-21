#include "../include/Timer.hpp"
#include "../include/EventLoop.hpp"

// 所有公开接口通过 _loop->RunInLoop 将实际操作路由到 EventLoop 线程
// 这样可以从任意线程调用，内部通过 eventfd 唤醒 epoll_wait
// RunInLoop 判断：如果当前是 EventLoop 所在线程则直接执行（减少锁+唤醒开销），
// 否则入队并通过 eventfd 跨线程唤醒
void TimingWheel::TimerAdd(uint64_t id, uint32_t delay, const TaskFunc& cb) {
    _loop->RunInLoop(std::bind(&TimingWheel::TimerAddInLoop, this, id, delay, cb));
}

void TimingWheel::TimerReflesh(uint64_t id) {
    _loop->RunInLoop(std::bind(&TimingWheel::TimerRefleshInLoop, this, id));
}

void TimingWheel::TimerCancel(uint64_t id) {
    _loop->RunInLoop(std::bind(&TimingWheel::TimerCancelInLoop, this, id));
}
