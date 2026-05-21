#include "../include/TcpServer.hpp"
#include "../include/Logger.hpp"

TcpServer::TcpServer(int port)
    : _next_id(0), _port(port), _enable_inactive_release(false), _acceptor(&_base_loop, port), _pool(&_base_loop) {}

void TcpServer::SetThreadCnt(int cnt)
{
    _pool.SetThreadCnt(cnt);
}

void TcpServer::SetConnectedCallBack(const ConnectedCallBack &connected_cb)
{
    _connected_cb = connected_cb;
}

void TcpServer::SetClosedCallBack(const ClosedCallBack &closed_cb)
{
    _closed_cb = closed_cb;
}

void TcpServer::SetMessageCallBack(const MessageCallBack &message_cb)
{
    _message_cb = message_cb;
}

void TcpServer::SetAnyEventCallBack(const AnyEventCallBack &event_cb)
{
    _event_cb = event_cb;
}

void TcpServer::SetServerClosedCallBack(const ClosedCallBack &cb)
{
    _server_closed_cb = cb;
}

void TcpServer::EnableInactiveRelease(int timeout)
{
    _timeout = timeout;
    _enable_inactive_release = true;
}
//添加定时任务
void TcpServer::RunAfter(const std::function<void()> &task, int delay)
{
    _base_loop.RunInLoop(std::bind(&TcpServer::RunAfterInLoop, this, task, delay));
}

void TcpServer::Start()
{
    _pool.Create(); // 创建线程池的从属线程
    _acceptor.SetAcceptorCallBack(std::bind(&TcpServer::NewConnection, this, std::placeholders::_1));//收到新连接的回调
    _acceptor.Listen(); // 将监听套接字挂到baseloop上面开始事件监控
    _base_loop.Start();
}

void TcpServer::RemoveConnection(const PtrConnection &conne)
{
    _base_loop.RunInLoop(std::bind(&TcpServer::RemoveConnectionInLoop, this, conne));
}

void TcpServer::RemoveConnectionInLoop(const PtrConnection &conne)
{
    int id = conne->Id();
    _connections.erase(id);
}

void TcpServer::NewConnection(int fd)
{
    _next_id++;
    PtrConnection conne(new Connection(_pool.NextLoop(), _next_id, fd));
    conne->SetMessageCallBack(_message_cb);
    conne->SetClosedCallBack(_closed_cb);
    conne->SetConnectedCallBack(_connected_cb);
    conne->SetAnyEventCallBack(_event_cb);
    conne->SetServerClosedCallBack(std::bind(&TcpServer::RemoveConnection, this, std::placeholders::_1));
    if (_enable_inactive_release)
        conne->EnableInactiveRelease(_timeout);
    conne->Established();
    _connections.insert(std::make_pair(_next_id, conne));
}

void TcpServer::RunAfterInLoop(const std::function<void()> &task, int delay)
{
    _next_id++;
    _base_loop.TimerAdd(_next_id, delay, task);
}

void TcpServer::Stop()
{
    // 1. 关闭所有连接：逐一 ShutdownInLoop，连接释放由 Connection::Release 的 TasksInLoop 排队执行
    for(auto &pair : _connections) {
        pair.second->Shutdown();
    }
    // 2. 停止从属线程池（停止所有 Sub Reactor 的 EventLoop 并 join 线程）
    _pool.Stop();
    // 3. 停止主循环（base loop 的 Start() 退出，_connections 清理由上一步 RunAllTask drain 完成）
    _base_loop.Stop();
}