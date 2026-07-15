# Benchmarks

## Cross-Machine LAN (real network, 2026-05-20)

Server and client are two 4C8G cloud instances on the same LAN (192.168.175.86 / 192.168.175.87). Server has exclusive CPU, no local wrk contention.

| Scenario | Tool | QPS | P50 | P99 | Bandwidth | Notes |
|----------|------|-----|-----|-----|-----------|-------|
| Public c10k | wrk -t4 | 2,838 | 33ms | 1.01s | 385 KB/s | 5M public bandwidth bottleneck |
| **LAN c10k keep-alive** | wrk -t4 | **150,853** | 41ms | 1.49s | 20 MB/s | **Baseline QPS** |
| LAN c20k keep-alive | wrk -t4 | 134,645 | 41ms | 1.60s | 17.9 MB/s | 94k timeouts, near limit |
| LAN c10k short-lived | wrk -t4 | 4,806 | 718ms | 1.83s | 718 KB/s | TCP handshake dominates |
| **LAN 60k idle + 10k active** | wrk -t4 | **149,955** | 19ms | 504ms | 19.9 MB/s | Only 0.6% QPS drop |

**Key findings:**
- LAN QPS ceiling: **~150k** (/hello, ~140B response, WARN log level)
- Public network: 5M bandwidth is the bottleneck, not the server
- Short-lived connection QPS is ~3.2% of keep-alive (TCP overhead dominates)
- 60k idle connections + 10k active: QPS nearly unchanged, epoll overhead negligible
- Per-connection RSS: ~1.5KB (Connection + 2×1024 Buffer + kernel socket buffer)

## Optimization History

```
2c2g WebBench c1000 3,759 qps   (sync + async logging)
  → 2c2g WebBench c1000 12,532  (regex optimization)
  → 2c2g wrk c10k 16,379        (switch to wrk keep-alive)
  → 4c8g wrk c10k 30,000        (disable logging, WARN log level)
  → 4c8g wrk c10k 40,000        (string prealloc, HTTP status table → static)
  → 4c8g wrk c10k 115,493       (WARN logging re-added, RSS +300MB)
  → 4c8g LAN wrk c10k 150,853   (dedicated client machine)
```

## Localhost History

| Env | Endpoint | Tool | QPS | Memory/CPU | Notes |
|-----|----------|------|-----|------------|-------|
| 4c8g | /hello | wrk 4t 10k | **115,493** | RSS 96MB | Idle, WARN level |
| 4c8g | /hello | wrk 4t 10k | **110,154** | RSS 547MB | 200k idle connections, WARN (4% drop) |
| 4c8g | /hello | wrk 4t 10k | 40,352 | RSS 212MB | Idle, DEBUG level |
| 4c8g | /hello | WebBench 10k | 29,799 | — | Short-lived connections |
| 2c2g | /hello | WebBench 2k | 12,500+ | — | Historical |

**Memory analysis:**
- No long connections: RSS **96MB** avg, **156MB** peak
- 200k long connections: QPS **110,154** (4% drop), RSS **547MB** avg, **608MB** peak
- 200k connection memory delta: **308MB** (156→464MB)
- Per-connection cost: ~1.54KB
- Log level impact: WARN vs DEBUG → QPS 2.9× (115k vs 40k)

## Throughput

- 1,000 concurrent: 13+ MB/s
- 500 concurrent: 13+ MB/s
- Single connection file transfer: 47 MB/s (10MB file)
- 10 concurrent file transfer: 74 MB/s (aggregate)
