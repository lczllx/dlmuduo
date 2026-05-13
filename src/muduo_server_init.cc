#include "../include/TcpServer.hpp"
#include "../include/log_system/lcz_log.h"
#include <csignal>

class NetWork {
public:
    NetWork() {
        LCZ_DEBUG("SIGPIPE INIT");
        signal(SIGPIPE, SIG_IGN);//忽略连接断开时向已经关闭的socket写数据造成的信号
    }
};

static NetWork nw;