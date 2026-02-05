#include "../include/TcpServer.hpp"
#include "../include/Logger.hpp"
#include <iostream>
#include <sys/resource.h>
class EchoServer
{
    private:
    TcpServer _server;
    void OnConnected(const PtrConnection&conne)
    {
        L_DEBUG("new connected %p", conne.get());
    }
    void OnClosed(const PtrConnection&conne)
    {
        L_DEBUG("close connected %p", conne.get());
    }
    void OnMessage(const PtrConnection&conne,Buffer *buf)
    {
    if (!buf || buf->ReadableBytes() == 0) return;  // 增加空指针和空数据检查
        std::string response = "hello ";
        conne->Send(response.c_str(), response.size());
        conne->Shutdown();
    }
    public:

    EchoServer(int port):_server(port){
    // 机器配置: 4c8g 用 8 线程; 2c2g 建议改为 2
    _server.SetThreadCnt(8);
    _server.EnableInactiveRelease(300);  // 压测时保持 5 分钟，避免中途断开
    _server.SetMessageCallBack(std::bind(&EchoServer::OnMessage,this,std::placeholders::_1,std::placeholders::_2));
    _server.SetConnectedCallBack(std::bind(&EchoServer::OnConnected,this,std::placeholders::_1));
    _server.SetClosedCallBack(std::bind(&EchoServer::OnClosed,this,std::placeholders::_1));
    
    }
    void Start(){_server.Start();}

};


int main()
{
    struct rlimit rl;
    if (getrlimit(RLIMIT_NOFILE, &rl) == 0) {
        std::cout << "[Server] 当前进程 fd 上限: " << rl.rlim_cur
                  << " (10万连接需 >= 110000，否则最多约6.5万)" << std::endl;
    }
    EchoServer echo(8889);
    echo.Start();
    return 0;
}