# dlmuduo — High-Performance C++ Network Library

[![License: MIT](https://img.shields.io/badge/License-MIT-green.svg)](LICENSE)
[![CI](https://github.com/lczllx/dlmuduo/actions/workflows/ci.yml/badge.svg)](https://github.com/lczllx/dlmuduo/actions/workflows/ci.yml)
[![Standard](https://img.shields.io/badge/C%2B%2B-14-blue.svg)](https://en.cppreference.com/w/cpp/14)
[![Tests](https://img.shields.io/badge/tests-62%20passing-brightgreen.svg)](tests/)

A high-performance C++14 network library — a rewrite of Chen Shuo's [muduo](https://github.com/chenshuo/muduo), replacing Boost with the C++ standard library. Features an event-driven Reactor pattern with multi-threaded I/O, built-in HTTP server, and URL shortener application.

## Features

- **One loop per thread** — multi-reactor master/slave architecture
- **Non-blocking I/O** — epoll-based event loop with `eventfd` cross-thread wakeup
- **Timing wheel** — O(1) timer add/cancel/refresh, 1-second granularity
- **HTTP server** — regex routing, static file serving, keep-alive
- **URL shortener** — Base62 encoding, MySQL + Redis, Cache-Aside pattern
- **Docker deployment** — multi-stage Alpine build, zero-friction `autobuild/docker.sh setup`

## Quick Start

```bash
git clone https://github.com/lczllx/dlmuduo.git
cd dlmuduo

# Install dependencies + build all targets
bash autobuild/deps.sh
mkdir build && cd build && cmake .. -DCMAKE_BUILD_TYPE=Release && make -j$(nproc)

# Run
../bin/test              # Echo server (port 8889)
../bin/http_server       # HTTP server (/hello, /file)
../bin/shortener_server  # URL shortener (needs MySQL + Redis)
```

Or with Docker:

```bash
bash autobuild/docker.sh setup     # One-click: configure + build + start
bash autobuild/docker.sh compose   # Start MySQL + Redis + App
bash autobuild/docker.sh clean     # Stop and cleanup
```

## Library Installation

```bash
bash autobuild/deps.sh              # Auto-install build tools
sudo bash autobuild/install.sh      # Install to /usr/local
sudo bash autobuild/install.sh ~/local  # Or custom prefix
```

```cpp
#include <lczmuduo/TcpServer.hpp>
#include <lczmuduo/Buffer.hpp>

int main() {
    TcpServer server(8889);
    server.SetThreadCnt(4);
    server.SetMessageCallback([](const PtrConnection&, Buffer* buf) {
        std::string msg = buf->ReadAsstringandpop(buf->ReadableBytes());
        // handle message
    });
    server.Start();
}
```

## Performance

| Metric | Value |
|--------|-------|
| LAN QPS ceiling | **150,853** (/hello, 140B response) |
| 200k idle + 10k active QPS | **149,955** (0.6% drop) |
| Per-connection memory | ~1.5KB RSS |

Full benchmarks: [docs/benchmark.md](docs/benchmark.md)

## Project Structure

```
dlmuduo/
├── include/        Public headers (TcpServer, EventLoop, Buffer, ...)
├── src/            Core library (Reactor, Poller, Timer, ...)
├── net/http/       HTTP server + URL shortener
├── example/        Echo server + concurrent connection test
├── tests/          62 unit tests (GTest)
├── autobuild/      Build, dependency, and Docker management scripts
├── docs/           Architecture, benchmarks, shortener design
├── flowchart/      Reactor model diagrams
└── scripts/bench/  Benchmark scripts (wrk, QPS, 100k connections)
```

## Testing

```bash
cmake .. -DBUILD_TESTS=ON && make -j$(nproc) && ctest
```

62 tests covering 10 modules: Buffer, Any, Socket, Channel, EventLoop, TimingWheel, Connection, LoopThread, LoopThreadPool, TcpServer.

## Known Limitations

- **Test coverage incomplete** — Acceptor, Poller, and HTTP parser lack unit tests
- **No backpressure** — unbounded output buffer, slow clients can exhaust server memory
- **No TLS / rate limiting / connection limit**
- **HTTP parser gaps** — no chunked encoding, Trailer headers, or `Expect: 100-continue`
- **Timing wheel granularity** — 1-second tick, unsuitable for sub-second timeouts
- **Single acceptor bottleneck** — short-lived connections >50k accept/s may saturate

详见：[docs/shortener.md](docs/shortener.md)

## Documentation

| Document | Content |
|----------|---------|
| [docs/architecture.md](docs/architecture.md) | Reactor model, class reference, call flow |
| [docs/architecture_cn.md](docs/architecture_cn.md) | 中文：Reactor 模型、类参考、调用流程 |
| [docs/benchmark.md](docs/benchmark.md) | QPS data, optimization history, memory analysis |
| [docs/benchmark_cn.md](docs/benchmark_cn.md) | 中文：QPS 数据、优化历程、内存分析 |
| [docs/shortener.md](docs/shortener.md) | Shortener design, cache strategy, deployment |
| [docs/shortener_cn.md](docs/shortener_cn.md) | 中文：短链接设计、缓存策略、部署 |

## License

MIT © 2024 lczllx — see [LICENSE](LICENSE).

## Acknowledgments

Inspired by Chen Shuo's [muduo](https://github.com/chenshuo/muduo) network library.
