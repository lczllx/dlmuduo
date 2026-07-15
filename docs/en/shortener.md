# URL Shortener

A URL shortener service built on the dlmuduo Reactor framework. Provides short-code generation and 302 redirect.

## Design

- **Short code**: Base62-encoded auto-increment ID. MySQL `code` column with `utf8mb4_bin` unique index prevents collisions. Write uses `TMP_<pid>_<seq>` placeholder to obtain auto-increment ID, then UPDATE to Base62(ID) — safe under multi-process concurrency.
- **Cache**: Redis hot cache via Cache-Aside pattern (read miss → MySQL → backfill Redis). Redis connection pool (4 connections), Acquire/Release with lock only on queue operations, I/O outside critical section.
- **Storage**: MySQL stores short/long URL mappings, connection pool manages persistent connections.

## Deployment

Docker multi-stage build (Alpine 3.21). `Dockerfile` at repository root.

```bash
bash autobuild/docker.sh setup     # one-click deploy
bash autobuild/docker.sh compose   # start MySQL + Redis + App
bash autobuild/docker.sh clean     # stop and cleanup
```

## Known Bottlenecks

- Redis miss triggers synchronous `mysql_query()` blocking the Reactor I/O thread (0.5–2ms per call)
- Redis pool `Acquire()` has no timeout on `_cond.wait()`, blocking I/O thread indefinitely when exhausted
- `TMP_` garbage rows may persist after process crash (no cleanup mechanism)
