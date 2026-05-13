#include "../include/Connection.hpp"
#include "../include/EventLoop.hpp"
#include "../include/log_system/lcz_log.h"
#include <cstring>
#include <cerrno>

Connection::Connection(EventLoop *loop, uint64_t cone_id, int sockfd)
    : _conne_id(cone_id), _sockfd(sockfd), _enable_inactive_release(false), _loop(loop), _status(CONNECTING), _socket(sockfd), _conne_channel(loop, sockfd)
{

    _conne_channel.SetCloseCallback(std::bind(&Connection::HandleClose, this));
    _conne_channel.SetReadCallback(std::bind(&Connection::HandleRead, this));
    _conne_channel.SetWriteCallback(std::bind(&Connection::HandleWrite, this));
    _conne_channel.SetErrorCallback(std::bind(&Connection::HandleError, this));
    _conne_channel.SetEventCallback(std::bind(&Connection::HandleEvent, this));
}

Connection::~Connection()
{
    LCZ_DEBUG("release %p", this);
}

void Connection::HandleRead()
{
    char buf[65536];
    ssize_t ret = _socket.NonBlockRecv(buf, 65535);//防止缓冲区没有数据造成阻塞
    if (ret < 0)
    {
        return ShutdownInLoop();
    }
    _in_buffer.WriteAndpush(buf, ret);//数据放入输入缓冲区
    if (_in_buffer.ReadableBytes() > 0 && _message_cb)
    {
        return _message_cb(shared_from_this(), &_in_buffer);//有数据调用有新数据接收成功的回调
    }
}

// 描述符触发可写事件后调用的函数，将输出缓冲区的数据进行发送
void Connection::HandleWrite()
{
    ssize_t ret = _socket.NonBlockSend(_out_buffer.GetReadPtr(), _out_buffer.ReadableBytes());
    if (ret < 0)
    { 
        if (_in_buffer.ReadableBytes() > 0)
        {
            _message_cb(shared_from_this(), &_in_buffer);
        }
        return Release();//关闭连接
    }
    _out_buffer.MoveReadoffset(ret);      // 将读偏移向后移动
    if (_out_buffer.ReadableBytes() == 0) // 解决输出缓冲区没有数据可写，不断触发写事件造成的loopbusy
    {
        _conne_channel.DisableWrite();//关闭写事件监控
        // 如果没有数据且status处于DISCONNECTING
        if (_status == DISCONNECTING)
        {
            return Release();//关闭连接
        }
    }
}

// 描述符触发挂断事件
void Connection::HandleClose()
{
    if (_in_buffer.ReadableBytes() > 0 && _message_cb)
    {
        _message_cb(shared_from_this(), &_in_buffer);
    }
    return Release();
}

// 描述符触发错误事件
void Connection::HandleError() { return HandleClose(); }//复用 HandleClose

// 描述符触发任意事件 刷新活跃度 调用组件使用者的任意事件回调
void Connection::HandleEvent()
{
    if (_enable_inactive_release && _loop)
    {
        _loop->TimerReflesh(_conne_id);//刷新连接活跃度
    }
    if (_event_cb)
        _event_cb(shared_from_this());//调用任意事件回调
}

// 获取链接后，要进行的设置-启动读监控，调用回调
void Connection::EstablishedInLoop()
{
    assert(_status == CONNECTING);//必须是连接办完成状态
    _status = CONNECTED;         // 修改链接状态
    _conne_channel.EnableRead(); // 启动读事件监控
    if (_connected_cb)
        _connected_cb(shared_from_this()); // 调用连接建立回调
}

// 真正的释放接口
void Connection::ReleaseInLoop()
{
    if (_status == DISCONNECTED)
        return;              // 防止重复释放
    _status = DISCONNECTED;  // 修改链接状态
    _conne_channel.Remove(); // 移除监控
    _socket.Close();         // 关闭描述符
    if (_loop->HasTimer(_conne_id))
        CancelInactiveReleaseInLoop(); // 如果有定时任务，取消
    if (_closed_cb)
        _closed_cb(shared_from_this()); // 调用关闭回调
    if (_server_closed_cb)
        _server_closed_cb(shared_from_this()); // 移除服务器内部管理的链接信息
}

void Connection::ShutdownInLoop()
{
    if (_status == DISCONNECTED || _status == DISCONNECTING)
        return;                   // 状态判断
    _status = DISCONNECTING;      // 设置为半关闭状态
    _conne_channel.DisableRead(); // 关闭读事件---
    if (_in_buffer.ReadableBytes() > 0)
    {
        if (_message_cb)
            _message_cb(shared_from_this(), &_in_buffer);
    }
    if (_out_buffer.ReadableBytes() > 0)
    {
        if (!_conne_channel.WriteAble())
            _conne_channel.EnableWrite();
    }
    if (_out_buffer.ReadableBytes() == 0)//没数据或发送完就关闭连接
    {
        Release();
    }
}

// 将数据放到发送缓冲区 -》启动可写事件监控
void Connection::SendInLoop(Buffer buf)
{
    if (_status != CONNECTED)
        return;
    _out_buffer.WriteBufferAndpush(buf);
    if (!_conne_channel.WriteAble())
        _conne_channel.EnableWrite();//启动写事件监控
}

void Connection::EnableInactiveReleaseInLoop(int sec)
{
    _enable_inactive_release = true;
    if (_loop->HasTimer(_conne_id))
        _loop->TimerReflesh(_conne_id); // 存在就刷新活跃度
    else
        _loop->TimerAdd(_conne_id, sec, std::bind(&Connection::Release, this)); // 不存在就添加
}

void Connection::CancelInactiveReleaseInLoop()
{
    _enable_inactive_release = false;
    if (_loop->HasTimer(_conne_id))
        _loop->TimerCancel(_conne_id);
}

void Connection::UpgradeInLoop(const Any &context, const ConnectedCallBack &connected_cb, const ClosedCallBack &closed_cb, const MessageCallBack &message_cb, const AnyEventCallBack &event_cb)
{
    _context = context;
    _connected_cb = connected_cb;
    _closed_cb = closed_cb;
    _message_cb = message_cb;
    _event_cb = event_cb;
}

void Connection::Established()
{
    _loop->RunInLoop(std::bind(&Connection::EstablishedInLoop, this));
}

void Connection::Send(const char *data, size_t len)
{
    //操作时data可能释放了
    Buffer buf;
    buf.WriteAndpush(data, len);
    _loop->RunInLoop(std::bind(&Connection::SendInLoop, this, std::move(buf)));
}

void Connection::EnableInactiveRelease(int sec)
{
    _loop->RunInLoop(std::bind(&Connection::EnableInactiveReleaseInLoop, this, sec));
}

void Connection::CancelInactiveRelease()
{
    _loop->RunInLoop(std::bind(&Connection::CancelInactiveReleaseInLoop, this));
}

void Connection::Shutdown()
{
    _loop->RunInLoop(std::bind(&Connection::ShutdownInLoop, this));
}

void Connection::Release()
{
    // 必须始终入队，不能同步执行：若在 Channel 回调中调用，同步执行会立即销毁
    // Connection，而 HandleEvent 尚未返回，后续还会调用 event_cb，导致 use-after-free
    _loop->TasksInLoop(std::bind(&Connection::ReleaseInLoop, this));
}

//协议切换，必须在eventloop线程内立即执行
void Connection::Upgrade(const Any &context, const ConnectedCallBack &connected_cb, const ClosedCallBack &closed_cb, const MessageCallBack &message_cb, const AnyEventCallBack &event_cb)
{
    _loop->RunInLoop(std::bind(&Connection::UpgradeInLoop, this, context, connected_cb, closed_cb, message_cb, event_cb));
}