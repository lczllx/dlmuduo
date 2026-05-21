#ifndef MUDUO_POLLER_H
#define MUDUO_POLLER_H

#include <unordered_map>
#include <vector>
#include <sys/epoll.h>
#include "Logger.hpp"

#define MAX_EPOLLEVENTS 8192  // 单次 epoll_wait 最多返回事件数，10万连接需增大

/*对任意描述符进行io事件监控-封装epoll*/
class Channel;
class Poller
{
private:
    int _epfd;
    std::vector<struct epoll_event> _evs;//
    std::unordered_map<int, Channel *> _channels;//管理描述符和对应的channel

    void Update(Channel *channel, int op);//根据具体操作类型更新epoll
    bool HasChannel(Channel *channel);//判断 fd 是否已被 Poller 管理，用于区分 ADD/MOD

public:
    Poller();
    void UpdateEvent(Channel *channel); // 更新事件监控
    void RemoveEvent(Channel *channel); // 移除事件监控
    void Poll(std::vector<Channel *> *active);//开始监控，获取就绪channel
};

#endif