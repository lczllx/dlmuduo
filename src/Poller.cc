#include "../include/Poller.hpp"
#include "../include/Channel.hpp"
#include "../include/log_system/lcz_log.h"
#include <cassert>
#include <cstring>
#include <cerrno>
#include <unistd.h>

Poller::Poller() : _evs(MAX_EPOLLEVENTS)
{
    _epfd = epoll_create(MAX_EPOLLEVENTS);
    if (_epfd < 0)
    {
        LCZ_ERROR("epoll create failed");
        abort();
    }
}

void Poller::Update(Channel *channel, int op)
{
    int fd = channel->Fd();
    struct epoll_event ev;
    ev.data.fd = fd;
    ev.events = channel->Events();
    int ret = epoll_ctl(_epfd, op, fd, &ev);
    if (ret < 0)
    {
        LCZ_ERROR("EpollCTL failed op=%d fd=%d errno=%d (%s)", op, fd, errno, strerror(errno));
    }
}

bool Poller::HasChannel(Channel *channel) //
{
    auto it = _channels.find(channel->Fd());
    if (it == _channels.end())
    {
        return false;
    }
    return true;
}
// 添加或修改监控：首次 ADD，后续 MOD
// HasChannel 用 _channels map 判断是否已注册，避免每次用 EPOLL_CTL_ADD 覆盖
void Poller::UpdateEvent(Channel *channel)
{
    bool ret = HasChannel(channel);
    if (ret == false)
    {
        _channels.insert(std::make_pair(channel->Fd(), channel));
        return Update(channel, EPOLL_CTL_ADD);
    }
    return Update(channel, EPOLL_CTL_MOD);
}
// 移除监控
void Poller::RemoveEvent(Channel *channel)
{
    auto it = _channels.find(channel->Fd());
    if (it != _channels.end())
    {
        _channels.erase(it);
    }
    Update(channel, EPOLL_CTL_DEL);
}
// 开始监控，返回活跃链接
// void Poll(std::vector<Channel*>&active)///感觉问题在这里
void Poller::Poll(std::vector<Channel *> *active)
{
    // timeout=-1：无限阻塞直到有事件。只有 eventfd wakeup 或定时器 timerfd 到期才会返回
    int nfds = epoll_wait(_epfd, _evs.data(), MAX_EPOLLEVENTS, -1);
    if (nfds < 0)
    {
        // EINTR 阻塞被信号打断了
        if (errno  == EINTR)
        {
            LCZ_DEBUG("epoll_wait interrupted by signal");
            return;
        }
        LCZ_ERROR("Epollwait ERROR: %s", strerror(errno));
        abort();
    }
    for (int i = 0; i < nfds; i++)
    {
        auto it = _channels.find(_evs[i].data.fd);
        // assert(it != _channels.end());
        // it->second->SetREvent(_evs[i].events);
        // active->push_back(it->second);
        if (it != _channels.end())
        {
            Channel* channel = it->second;
            channel->SetREvent(_evs[i].events); // 设置返回事件
            active->push_back(channel);
        }
        else
        {
            LCZ_WARN("Epoll event for unknown fd=%d",_evs[i].data.fd); // 处理未知fd事件
        }
    }
}