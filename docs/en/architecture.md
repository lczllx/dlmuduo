# Architecture

## Overview

dlmuduo is a high-performance C++14 network library — a rewrite of Chen Shuo's [muduo](https://github.com/chenshuo/muduo), replacing Boost dependencies with the C++ standard library. It implements a multi-threaded Reactor pattern for high-concurrency I/O.

## Reactor Model

The library uses the **Reactor** (non-blocking synchronous I/O) model:

- **Reactor**: The application registers interest in events (readable, writable) with the kernel. The kernel notifies the application when events are ready; the application performs the actual I/O.
- **Proactor** (comparison): In the asynchronous I/O model, the kernel completes the entire I/O operation before notifying the application.

### Components

| Component | Role |
|-----------|------|
| Event | I/O event (readable, writable, error, close) |
| Reactor (`EventLoop`) | Event demultiplexing and dispatch |
| Demultiplexer (`Poller`) | epoll wrapper, monitors file descriptors |
| EventHandler (`Channel`) | Callback binding for fd events |

### Call Flow

1. EventHandler registers Events with Reactor
2. Reactor delegates monitoring to Demultiplexer
3. Demultiplexer detects ready events, notifies Reactor
4. Reactor dispatches events to corresponding EventHandlers

![Reactor model](../flowchart/friendship.png)

### Single Reactor

Each `EventLoop` is a Reactor running on one thread (one thread one loop). Each EventLoop contains a `Poller` and multiple `Channel` instances.

![Single Reactor](../flowchart/newtod.png)

### Master-Slave Reactor

For high concurrency, the library uses a master-slave pattern similar to Nginx:

- **Master Reactor** (`base_loop`): Handles new connections via `Acceptor`, dispatches to slave reactors
- **Slave Reactors** (`LoopThreadPool`): Handle I/O for assigned connections

![Master-Slave Reactor](../flowchart/newtod.png)

## Class Reference

### Buffer

User-space buffer with read/write offsets and automatic expansion:

```
  begin()      read_idx     write_idx    end()
  ├─────────────┼─────────────┼────────────┤
  │  consumed   │  readable   │  writable  │
  └─────────────┴─────────────┴────────────┘
```

![Buffer structure](../flowchart/buffer.png)

When writable space is insufficient, data is shifted left (reclaiming consumed space) or the buffer is expanded. Memory is not released on `clear()` — only offsets are reset.

### Any

Custom type-erased container (similar to `std::any` from C++17). Used for protocol context management in `Connection`.

### Channel

Manages a single file descriptor's epoll events and callbacks: `EnableRead/Write`, `DisableRead/Write`, `DisableAll`, `Remove`, `HandleEvent`.

### Poller

epoll wrapper — the Demultiplexer. `UpdateEvent()`, `RemoveEvent()`, `Poll()`.

### EventLoop

Core Reactor — one per thread. Integrates `Poller`, `Channel`, and `TimingWheel`.

![EventLoop](../flowchart/head_eventloop.png)

```
while (!quit) {
    epoll_wait → active channels → HandleEvent
    drain task queue (swap-to-stack pattern)
}
```

- `eventfd` for cross-thread wakeup
- `RunInLoop(task)`: execute directly if in loop thread, else enqueue + wakeup
- `Quit()`: set quit flag + wakeup (thread-safe)
- **Task queue**: `_task.swap(tmp)` under mutex, then execute callbacks outside lock — minimizes critical section
- **TimingWheel**: O(1) insert/delete, 360 slots × 1 second tick via `timerfd`

### Connection

Manages a single TCP connection — integrates `Socket`, `Channel`, `Buffer`. States: DISCONNECTED → CONNECTING → CONNECTED → DISCONNECTING.

### TcpServer

Top-level server — integrates `Acceptor`, `LoopThreadPool`, `EventLoop`.

**Start:** Create pool → set accept callback → listen → enter base loop (blocking)

**New connection:** `accept()` → allocate slave loop (round-robin) → create Connection → callbacks

**Stop:** `Stop()` → `base_loop.Quit()` (thread-safe) → `Start()` returns → destructor cleanup

### LoopThread / LoopThreadPool

- `LoopThread`: binds one EventLoop to one thread
- `LoopThreadPool`: round-robin distribution of connections across N loops

---

## Flow Summary

1. `TcpServer` creates master Reactor + `Acceptor` + `LoopThreadPool`
2. `Start()`: create pool threads → set accept callback → listen → enter base loop
3. New connection: `accept()` fd → allocate slave loop → create `Connection` → start read monitoring
4. All connection I/O runs on its assigned slave loop thread
5. `Stop()`: quit base loop → pool threads join → connections released → cleanup
