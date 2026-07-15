# 短链接服务

基于 dlmuduo Reactor 框架构建的 URL 短链接服务，提供短码生成与 302 重定向跳转。

## 设计

- **短码生成**：Base62 编码自增 ID。MySQL `code` 字段 `utf8mb4_bin` 唯一索引防冲突。写入时先用 `TMP_<pid>_<seq>` 占位获取自增 ID，再 UPDATE 为 Base62(ID)，多进程并发安全
- **缓存策略**：Redis 热点缓存，Cache-Aside 模式（读未命中 → 回源 MySQL → 回填 Redis）。Redis 连接池（4 连接），Acquire/Release 仅锁队列操作，I/O 在临界区外执行
- **数据存储**：MySQL 存长短链映射，连接池管理长连接

## 部署

Docker 多阶段构建（Alpine 3.21），`Dockerfile` 位于仓库根目录。

```bash
bash autobuild/docker.sh setup     # 一键部署
bash autobuild/docker.sh compose   # 启动 MySQL + Redis + App
bash autobuild/docker.sh clean     # 停止并清理
```

## 已知瓶颈

- Redis miss 后同步调 `mysql_query()` 会阻塞 Reactor I/O 线程（单次 0.5–2ms），后续可引入异步 MySQL 客户端解耦
- Redis 连接池 `Acquire()` 中 `_cond.wait()` 无超时，池满时永久阻塞 I/O 线程
- 进程崩溃时 `TMP_` 占位垃圾行可能残留，DB 中无清理机制
