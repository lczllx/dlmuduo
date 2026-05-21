#ifndef MUDUO_ACCEPTOR_H
#define MUDUO_ACCEPTOR_H

#include "Socket.hpp"
#include "Channel.hpp"
#include <functional>

/*对监听套接字进行管理的模块
当新连接到来为其封装connection对象和设置对应回调（由服务器模块实现）*/
class EventLoop;
class Acceptor
{
private:
    Socket _socket;
    EventLoop *_loop;
    Channel _acpt_channel; // 用于对监听套接字进行事件管理

    using AcceptorFunc = std::function<void(int)>;
    AcceptorFunc _acpt_cb; // 新连接回调

    void HandleRead();
    int CreateServer(int port); // 创建监听reactor

public:
    Acceptor(EventLoop *loop, int port);
    void SetAcceptorCallBack(const AcceptorFunc &acpt_cb);
    void Listen(); // 开始监听接口
};

#endif