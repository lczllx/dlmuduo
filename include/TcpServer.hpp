#ifndef MUDUO_TCPSERVER_H
#define MUDUO_TCPSERVER_H

#include "Acceptor.hpp"
#include "LoopThreadPool.hpp"
#include "Connection.hpp"
#include "EventLoop.hpp"
#include <unordered_map>

/*整合所有模块，提供给使用者搭建服务器*/
class TcpServer {
private:
    uint64_t _next_id;//连接id -目前是自增，可以做其他实现
    int _port;//服务器监听的端口
    int _timeout;//非活跃连接超时秒数
    bool _enable_inactive_release;//是否启用非活跃连接自动断开
    EventLoop _base_loop;//主reactor
    Acceptor _acceptor;//监听套接字的管理对象
    LoopThreadPool _pool;//从属线程池
    std::unordered_map<uint64_t, PtrConnection> _connections;//保存管理所有的连接对应的指针

    //使用者设置给connection模块的回调
    ConnectedCallBack _connected_cb;
    ClosedCallBack _closed_cb;
    MessageCallBack _message_cb;
    AnyEventCallBack _event_cb;
    ClosedCallBack _server_closed_cb;

    void RemoveConnection(const PtrConnection& conne);//移除连接信息
    void NewConnection(int fd);//为新连接构造一个connection进行管理

    //考虑线程安全 将任务压入任务池
    void RemoveConnectionInLoop(const PtrConnection& conne);
    void RunAfterInLoop(const std::function<void()>& task, int delay);

public:
    TcpServer(int port);
    void SetThreadCnt(int cnt);//设置 Sub Reactor 线程数量，必须在 Start() 前调用

    //设置回调
    void SetConnectedCallBack(const ConnectedCallBack& connected_cb);
    void SetClosedCallBack(const ClosedCallBack& closed_cb);
    void SetMessageCallBack(const MessageCallBack& message_cb);
    void SetAnyEventCallBack(const AnyEventCallBack& event_cb);
    void SetServerClosedCallBack(const ClosedCallBack& cb);

    void EnableInactiveRelease(int timeout);//启动非活跃连接自动断开，timeout 单位秒
    void RunAfter(const std::function<void()>& task, int delay);//一次性定时任务，delay 秒后执行
    void Start();//创建线程池 → 注册新连接回调 → 启动监听 → 进入主事件循环（阻塞）
    void Stop() { _base_loop.Quit(); } // 安全退出 Start()，可跨线程调用
};

#endif