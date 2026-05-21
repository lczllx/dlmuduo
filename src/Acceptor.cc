#include "../include/Acceptor.hpp"
#include "../include/EventLoop.hpp"
#include "../include/log_system/lcz_log.h"

Acceptor::Acceptor(EventLoop* loop, int port)
    : _socket(CreateServer(port)), _loop(loop), _acpt_channel(loop, _socket.Fd()) {
    _acpt_channel.SetReadCallback(std::bind(&Acceptor::HandleRead, this));
}

void Acceptor::HandleRead() {
    int newfd = _socket.Accept();
    // LT 模式下 accept 返回 -1 通常意味着连接已被内核处理（对方 RST 或已 accept），忽略即可
    // 非阻塞 socket 不会阻塞在这里
    if(newfd < 0) return;
    if(_acpt_cb) _acpt_cb(newfd);
}

int Acceptor::CreateServer(int port) {
    if (!_socket.CreateServer(port)) {
        LCZ_ERROR("CreateServer failed for port %d", port);
        abort();
    }
    return _socket.Fd();
}

void Acceptor::SetAcceptorCallBack(const AcceptorFunc& acpt_cb) {
    _acpt_cb = acpt_cb;
}

void Acceptor::Listen() {
    _acpt_channel.EnableRead();
}