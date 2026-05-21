#ifndef MUDUO_SOCKET_H
#define MUDUO_SOCKET_H

#include <string>
#include <cstdint>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include "Logger.hpp"

#define MAX_LISTEN 65535  // 最大监听队列，支持高并发
/*封装套接字的操作，简化对套接字的操作*/
class Socket {
private:
    int _sockfd;
public:
    Socket();
    Socket(int fd);
    ~Socket();
    
    // 简单方法在头文件实现
    int Fd() { return _sockfd; }
    void Close() { if(_sockfd != -1) { close(_sockfd); _sockfd = -1; } }
    
    bool Create();//创建套接字
    bool Bind(const std::string &ip, uint16_t port);//绑定地址信息
    bool Listen(int backlog = MAX_LISTEN);//开始监听
    bool Connect(const std::string &ip, uint16_t port);//向服务器发起连接
    int Accept();//获取新连接 返回描述符
    ssize_t Recv(void *buf, size_t len, int flag = 0);//接收数据
    ssize_t NonBlockRecv(void *buf, size_t len);//非阻塞接收数据
    ssize_t Send(const void *buf, size_t len, int flag = 0);//发送数据
    ssize_t NonBlockSend(const void *buf, size_t len);//非阻塞发送
    //创建监听连接
    bool CreateServer(uint16_t port, const std::string &ip = "0.0.0.0", bool block_flag = false);
    //创建客户端连接
    bool CreateClient(uint16_t port, const std::string &ip);
    void ReuseAdderess();//设置地址重用，允许快速重启时绑定仍处于 TIME_WAIT 的端口
    void NoBlock();//设置套接字为非阻塞
};

#endif