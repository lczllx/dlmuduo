# 火焰图

压测期间 `perf record` 采样生成的 CPU 火焰图。

## test.svg

- **采样场景**：跨机器内网 wrk -t4 -c10000 /hello，QPS ~15 万
- **采样时长**：30 秒
- **采样命令**：`perf record -g -p $(pgrep http_server) -- sleep 30`
- **生成工具**：[FlameGraph](https://github.com/brendangregg/FlameGraph)

## 交互查看

SVG 文件可直接在浏览器中打开，点击栈帧放大，搜索函数名定位热点。

```bash
# 克隆 FlameGraph 工具
git clone https://github.com/brendangregg/FlameGraph.git

# 以后生成新的火焰图
sudo perf record -g -p $(pgrep http_server) -- sleep 30
perf script | FlameGraph/stackcollapse-perf.pl | FlameGraph/flamegraph.pl > assets/new_flamegraph.svg
```
