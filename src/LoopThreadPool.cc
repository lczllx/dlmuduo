#include "../include/LoopThreadPool.hpp"

LoopThreadPool::LoopThreadPool(EventLoop* base_loop)
    : _thread_cnt(0), _next_loop_idx(0), _base_loop(base_loop) {}

//设置从属线程数量
void LoopThreadPool::SetThreadCnt(int cnt) {
    _thread_cnt = cnt;
}

// 串行创建所有从属线程：每个线程构造后立即阻塞等待其 EventLoop 就绪，再创建下一个
// 串行而非并行是为了保证 _loop[i] 在循环体内就被赋值，NextLoop() 可安全轮询
void LoopThreadPool::Create() {
    _threads.resize(_thread_cnt);
    _loop.resize(_thread_cnt);
    for(int i = 0; i < _thread_cnt; i++) {
        _threads[i] = std::unique_ptr<LoopThread>(new LoopThread());//构造即启动线程
        _loop[i] = _threads[i]->Getloop();//阻塞直到该线程 EventLoop 构造完成
    }
}
//获取下一个从属线程的eventloop
EventLoop* LoopThreadPool::NextLoop() {
    if(_thread_cnt == 0) return _base_loop;
    _next_loop_idx = (_next_loop_idx + 1) % _thread_cnt;
    return _loop[_next_loop_idx];
}

void LoopThreadPool::Stop() {
    for(auto &t : _threads) {
        if(t) t->Stop();// 逐个停止每个 LoopThread：Stop() → join() → EventLoop 栈对象安全析构
    }
    _threads.clear();
    _loop.clear();
}