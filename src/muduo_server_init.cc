#include "../include/TcpServer.hpp"
#include "../include/log_system/lcz_log.h"
#include <csignal>

// 通过全局静态对象的构造函数，在 main 之前完成网络库初始化
class NetWork {
public:
    NetWork() {
        LCZ_DEBUG("SIGPIPE INIT");
        // 核心：忽略 SIGPIPE。向已关闭的 socket 写入会触发 SIGPIPE（默认杀进程），
        // 网络库必须忽略此信号，否则任何一个客户端断开都能让服务端崩溃
        signal(SIGPIPE, SIG_IGN);
    }
};

static NetWork nw;