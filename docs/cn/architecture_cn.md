# 架构设计

## 概述

dlmuduo 是一个高性能 C++14 网络库，重写了陈硕的 [muduo](https://github.com/chenshuo/muduo)，将 Boost 依赖替换为 C++ 标准库。采用多线程 Reactor 模式实现高并发 I/O。

## Reactor 模型

库采用 **Reactor**（非阻塞同步 I/O）模型：

- **Reactor**：应用程序向内核注册感兴趣的事件（可读、可写），内核在事件就绪时通知应用程序，应用程序自己执行实际的 I/O 操作
- **Proactor**（对比）：异步 I/O 模型，内核完成整个 I/O 操作后再通知应用程序

### 四大组件

| 组件 | 角色 |
|------|------|
| Event（事件） | I/O 事件：可读、可写、错误、关闭 |
| Reactor（`EventLoop`） | 事件分发与调度 |
| Demultiplexer（`Poller`） | epoll 封装，监控文件描述符 |
| EventHandler（`Channel`） | 文件描述符的事件回调绑定 |

### 调用流程

1. EventHandler 向 Reactor 注册 Event
2. Reactor 将 Event 交给 Demultiplexer 监控
3. Demultiplexer 检测到 Event 就绪，通知 Reactor
4. Reactor 将就绪的 Event 分发给对应的 EventHandler 处理

![Reactor 模型流程图](../flowchart/friendship.png)

### 单 Reactor 模型

每个 `EventLoop` 就是一个 Reactor，每个运行在一个线程上，形成 one thread one loop 设计。每个 EventLoop 中有一个 `Poller` 和多个 `Channel`，实现对多个连接的管理。

![单 Reactor 流程图](../flowchart/newtod.png)

### 主从 Reactor 模型

为了支撑高并发，库采用了和 Nginx 相似的主从设计：

- **主 Reactor**（`base_loop`）：通过 `Acceptor` 负责接收新连接，分派给从属 Reactor
- **从 Reactor**（`LoopThreadPool`）：负责一个或多个连接的读写等 I/O 操作

![主从 Reactor 流程图](../flowchart/newtod.png)

## 类参考

### Buffer — 用户态缓冲区

带读写偏移的连续空间，自动扩容：

```
  begin()      read_idx     write_idx    end()
  ├─────────────┼─────────────┼────────────┤
  │  已消费     │  可读数据   │  可写空间   │
  └─────────────┴─────────────┴────────────┘
```

![Buffer 结构图](../flowchart/buffer.png)

可写空间不足时先左移数据回收已消费空间，仍不够则扩容。`clear()` 仅重置偏移，不释放内存。

### Any — 通用类型容器

自定义类型擦除容器（类似 C++17 的 `std::any`），用于 `Connection` 的协议上下文管理。

- `get<T>()` 返回类型指针，类型不匹配返回 `nullptr`
- 支持拷贝、移动、`swap`、`reset`
- `has_value()` 和 `type()` 类型自省

### Channel — 事件通道

管理单个文件描述符的 epoll 事件和回调。

| 方法 | 说明 |
|------|------|
| `EnableRead()` / `EnableWrite()` | 启动读/写事件监控 |
| `DisableRead()` / `DisableWrite()` | 关闭读/写事件监控 |
| `DisableAll()` / `Remove()` | 移除所有监控 |
| `HandleEvent()` | 根据触发事件调用对应回调 |

回调类型：读回调、写回调、错误回调、关闭回调、任意事件回调。

### Poller — 多路事件分发器

epoll 封装，即 Demultiplexer。

- `UpdateEvent(channel)` — `epoll_ctl(ADD/MOD)`
- `RemoveEvent(channel)` — `epoll_ctl(DEL)`
- `Poll(active)` — `epoll_wait`，填充活跃 Channel 列表
- 维护 `fd → Channel*` 映射表，O(1) 查找

### EventLoop — 核心 Reactor

每个线程绑定一个 EventLoop。整合 `Poller`、`Channel` 和 `TimingWheel`。

![EventLoop](../flowchart/head_eventloop.png)

**主循环：**
```
while (!quit) {
    epoll_wait → 就绪 Channel → HandleEvent
    执行任务队列
}
```

**关键机制：**
- `eventfd` 实现跨线程唤醒
- `RunInLoop(task)`：本线程直接执行，否则入队 + 唤醒
- `TasksInLoop(task)`：始终入队 + 唤醒
- `Quit()`：设 quit 标志 + 唤醒（线程安全，可跨线程调用）
- **任务队列**：`_task.swap(tmp)` 持锁交换，回调在锁外执行，临界区极小

**TimingWheel 时间轮：**
- 360 个槽 × 1 秒 = 最长 359 秒延迟
- `TimerAdd(id, delay, cb)` 添加定时器
- `TimerRefresh(id)` 刷新延迟
- `TimerCancel(id)` 标记取消（不删除 `_timers` 条目，槽清空时才真正移除）

### Connection — 连接管理

管理单条 TCP 连接，整合 `Socket`、`Channel`、`Buffer`。

**连接状态：**
| 状态 | 说明 |
|------|------|
| `DISCONNECTED` | 初始 / 已关闭 |
| `CONNECTING` | TCP 握手进行中 |
| `CONNECTED` | 正常通信 |
| `DISCONNECTING` | 待关闭 |

**关键方法：** `Send()` 发送数据到输出缓冲区，`Shutdown()` 关闭连接，`Established()` 启动读监控，`Release()` 释放资源，`EnableInactiveRelease()` 启动非活跃断开。

### TcpServer — 服务器入口

整合所有模块，提供给使用者搭建服务器。

**启动流程：**
1. 创建从属线程池
2. 为 Acceptor 设置新连接回调 → `NewConnection()`
3. Acceptor 开始监听端口
4. 启动主 Reactor 事件循环（阻塞）

**新连接流程：**
1. Acceptor 调用 `accept()` 获取 fd
2. 轮转分配一个从属 EventLoop
3. 创建 `Connection` 对象，设置各种回调
4. `Established()` 启动读事件监控
5. 该连接所有操作都在绑定的从属 Reactor 中进行

**停止流程：**
1. `Stop()` → `_base_loop.Quit()`（线程安全）
2. `Start()` 返回
3. 析构函数按序清理：connections → pool → acceptor → base_loop

### LoopThread / LoopThreadPool — 线程池

- `LoopThread`：将一个 EventLoop 绑定到一个 `std::thread`
- `LoopThreadPool`：轮转分配 EventLoop 给新连接

---

## 整体流程总结

1. 创建 `TcpServer` 时自动创建主 Reactor（`base_loop`）+ `Acceptor` + `LoopThreadPool`
2. `Start()`：创建从属线程池 → 设置 Acceptor 回调 → 监听端口 → 启动主 Reactor 事件循环
3. 新连接到来：`accept()` 获取 fd → 分配从属 EventLoop → 创建 `Connection` → 启动读监控
4. 该连接的所有 I/O 操作都在绑定的从属 Reactor 线程中执行
5. `Stop()`：退出主事件循环 → 从属线程 join → 连接释放 → 资源清理
