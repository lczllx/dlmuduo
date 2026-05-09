#ifndef MUDUO_CONNECTION_H
#define MUDUO_CONNECTION_H

#include "Any.hpp"
#include "Buffer.hpp"
#include "Socket.hpp"
#include "Channel.hpp"
#include "CallbackTypes.hpp"
#include <memory>

/*对通信连接进行管理的模块，对socket，buffer，channel的整合
由组件使用者设置各种回调*/
class EventLoop;
class Connection : public std::enable_shared_from_this<Connection> //
{
private:
    uint64_t _conne_id;            // 连接唯一id
    uint64_t _timer_id;            // 定时器唯一id，这里使用_conne_id，可以自行生成
    int _sockfd;                   // 连接套接字描述符
    bool _enable_inactive_release; // 是否启动非活跃连接销毁
    EventLoop *_loop;              // 连接关联的eventloop
    ConneStatus _status;           // 连接状态
    Socket _socket;
    Channel _conne_channel;
    Buffer _in_buffer;  // 输入缓冲区
    Buffer _out_buffer; // 输出缓冲区
    Any _context;       // 协议上下文管理

    ConnectedCallBack _connected_cb;  // 连接建立成功回调
    ClosedCallBack _closed_cb;        // 连接关闭回调
    MessageCallBack _message_cb;      // 有新数据接收成功的回调
    AnyEventCallBack _event_cb;       // 任意事件回调
    ClosedCallBack _server_closed_cb; //组件内的连接关闭回调

    void HandleRead();//描述符触发可读
    void HandleWrite();//描述符触发可写
    void HandleClose();//描述符触发关闭
    void HandleError();//描述符触发错误
    void HandleEvent();//描述符触发任意事件

    // 将操作放入eventloop线程中
    void EstablishedInLoop();
    void ReleaseInLoop();
    void ShutdownInLoop();
    void SendInLoop(Buffer buf);
    void EnableInactiveReleaseInLoop(int sec);
    void CancelInactiveReleaseInLoop();
    void UpgradeInLoop(const Any &context, const ConnectedCallBack &connected_cb, const ClosedCallBack &closed_cb, const MessageCallBack &message_cb, const AnyEventCallBack &event_cb);

public:
    Connection(EventLoop *loop, uint64_t cone_id, int sockfd);
    ~Connection();

    int Fd() const { return _sockfd; }                      // 获取管理的文件描述符
    int Id() const { return _conne_id; }                    // 获取链接id
    bool Connected() const { return _status == CONNECTED; } // 判断连接是否处于CONNECTED

    void SetContext(const Any &context) { _context = context; } // 设置上下文
    Any *GetContext() { return &_context; }                     // 获取上下文信息
    EventLoop *GetLoop() const { return _loop; }                // 获取所属EventLoop

    // 设置回调接口
    void SetConnectedCallBack(const ConnectedCallBack &connected_cb) { _connected_cb = connected_cb; }
    void SetClosedCallBack(const ClosedCallBack &closed_cb) { _closed_cb = closed_cb; }
    void SetMessageCallBack(const MessageCallBack &message_cb) { _message_cb = message_cb; }
    void SetAnyEventCallBack(const AnyEventCallBack &event_cb) { _event_cb = event_cb; }
    void SetServerClosedCallBack(const ClosedCallBack &cb) { _server_closed_cb = cb; }

    void Established();                                                                                                                                                            // 连接获取后，所处的状态进行各种设置-设置事件回调，启动读监控，调用连接建立完成的回调
    void Send(const char *data, size_t len);                                                                                                                                       // 发送数据-数据放到发送缓冲区
    void EnableInactiveRelease(int sec);                                                                                                                                           // 启动非活跃销毁
    void CancelInactiveRelease();                                                                                                                                                  // 取消非活跃销毁
    void Shutdown();                                                                                                                                                               // 提供给使用者的关闭接口
    void Release();                                                                                                                                                                // 释放接口
    void Upgrade(const Any &context, const ConnectedCallBack &connected_cb, const ClosedCallBack &closed_cb, const MessageCallBack &message_cb, const AnyEventCallBack &event_cb); // 切换协议
};
#endif