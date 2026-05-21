#ifndef MUDUO_CALLBACK_TYPES_H
#define MUDUO_CALLBACK_TYPES_H

#include <functional>
#include <memory>

class Connection;
class Buffer;

using TaskFunc = std::function<void()>;
using ReleaseFunc = std::function<void()>; 
using PtrConnection = std::shared_ptr<Connection>;

// 连接状态机：CONNECTING → CONNECTED → DISCONNECTING → DISCONNECTED
// 新连接创建时处于 CONNECTING，Establish() 后切换到 CONNECTED
// Shutdown() 进入 DISCONNECTING（半关闭，等待输出缓冲清空），Release() 进入 DISCONNECTED
enum ConneStatus { DISCONNECTED/*已关闭*/, CONNECTING/*建立中-已完成TCP连接等待应用层确认*/, CONNECTED/*正常通信*/, DISCONNECTING/*半关闭-本地主动断开，等待输出缓冲清空*/ };
using ConnectedCallBack = std::function<void(const PtrConnection&)>;
using ClosedCallBack = std::function<void(const PtrConnection&)>;
using MessageCallBack = std::function<void(const PtrConnection&, Buffer*)>;
using AnyEventCallBack = std::function<void(const PtrConnection&)>;

#endif