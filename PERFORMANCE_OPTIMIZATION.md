# muduo 性能优化建议

## 优化优先级说明
- ⭐⭐⭐⭐⭐ 高优先级：性能影响大，优化收益高
- ⭐⭐⭐⭐ 中高优先级：性能影响中等，优化收益较高
- ⭐⭐⭐ 中优先级：性能影响较小，但仍有优化价值
- ⭐⭐ 低优先级：性能影响很小，优化收益有限

---

## 1. HTTP路由匹配优化 ⭐⭐⭐⭐⭐

### 问题位置
`net/http/http.hpp:818-831` - `Dispathcher()` 函数

### 问题描述
```cpp
void Dispathcher(HttpRequest &req,HttpResponse *resp,Route_Table &route_table)
{
    for(auto &each:route_table)
    {
        const std::regex &re = each.first;
        const Handler &func=each.second;
        bool ret= std::regex_match(req._path,req._matches,re);  // 每次请求都要做正则匹配
        if(!ret){continue;}
        return func(req,resp);
    }
    resp->_statu=404;
}
```

**性能问题**：
- 每次请求都要遍历路由表，对每个路由做正则匹配
- `std::regex` 在C++11中性能较差（NFA引擎）
- 对于固定路径（如`/hello`），正则匹配是浪费

### 优化方案

**方案1：精确匹配优先（推荐）**
```cpp
// 添加精确匹配表
std::unordered_map<std::string, Handler> _get_exact_route;
std::unordered_map<std::string, Handler> _post_exact_route;
// ...

void Dispathcher(HttpRequest &req,HttpResponse *resp,Route_Table &route_table)
{
    // 先尝试精确匹配（O(1)）
    if (auto it = _get_exact_route.find(req._path); it != _get_exact_route.end()) {
        return it->second(req, resp);
    }
    
    // 再尝试正则匹配
    for(auto &each:route_table) {
        const std::regex &re = each.first;
        const Handler &func=each.second;
        bool ret= std::regex_match(req._path,req._matches,re);
        if(!ret){continue;}
        return func(req,resp);
    }
    resp->_statu=404;
}
```

**方案2：使用更快的字符串匹配库**
- 使用 `hyperscan`（Intel开源，高性能正则引擎）
- 或使用简单的字符串前缀匹配

**预期收益**：QPS提升 10-20%

---

## 2. HTTP请求行解析优化 ⭐⭐⭐⭐⭐

### 问题位置
`net/http/http.hpp:525-535` - `ParseHttpLine()` 函数

### 问题描述
```cpp
bool ParseHttpLine(const std::string &line)
{
    std::smatch matchs;
    static const std::regex e(
        "^(GET|HEAD|POST|PUT|DELETE)\\s+([^?\n\r]*)(?:\\?([^\\n\\r ]*))?\\s+(HTTP/1\\.[01])(?:\\r?\\n)?$",
        std::regex::icase);
    bool ret = std::regex_match(line, matchs, e);  // 正则匹配开销大
    // ...
}
```

**性能问题**：
- 每次请求都要做正则匹配
- 正则表达式编译虽然用了`static`，但匹配本身仍有开销
- HTTP请求行格式固定，可以用简单的字符串解析

### 优化方案

**手动解析（推荐）**：
```cpp
bool ParseHttpLine(const std::string &line)
{
    // 手动解析，避免正则表达式
    size_t pos1 = line.find(' ');
    if (pos1 == std::string::npos) return false;
    
    _request._method = line.substr(0, pos1);
    std::transform(_request._method.begin(), _request._method.end(), 
                   _request._method.begin(), ::toupper);
    
    size_t pos2 = line.find(' ', pos1 + 1);
    if (pos2 == std::string::npos) return false;
    
    std::string path_query = line.substr(pos1 + 1, pos2 - pos1 - 1);
    size_t qpos = path_query.find('?');
    if (qpos != std::string::npos) {
        _request._path = Util::UrlDecode(path_query.substr(0, qpos), false);
        // 解析查询字符串...
    } else {
        _request._path = Util::UrlDecode(path_query, false);
    }
    
    _request._version = line.substr(pos2 + 1);
    // 去除\r\n
    if (!_request._version.empty() && _request._version.back() == '\r') {
        _request._version.pop_back();
    }
    return true;
}
```

**预期收益**：QPS提升 15-25%

---

## 3. 字符串操作优化 ⭐⭐⭐⭐

### 问题位置1
`net/http/main.cc:4-16` - `RequestStr()` 函数

### 问题描述
```cpp
std::string RequestStr(const HttpRequest &req) 
{
    std::stringstream ss;  // stringstream性能差
    ss << req._method << " " << req._path << " " << req._version << "\r\n";
    for (auto &it : req._params) {
        ss << it.first << ": " << it.second << "\r\n";
    }
    for (auto &it : req._headers) {
        ss << it.first << ": " << it.second << "\r\n";
    }
    ss << "\r\n";
    ss << req._body;
    return ss.str();  // 字符串拷贝
}
```

**性能问题**：
- `stringstream` 性能较差，每次操作都有动态分配
- 多次字符串拼接和拷贝
- 返回时还要拷贝整个字符串

### 优化方案

**预分配+直接拼接**：
```cpp
std::string RequestStr(const HttpRequest &req) 
{
    // 估算大小，预分配空间
    size_t estimated_size = req._method.size() + req._path.size() + 
                           req._version.size() + req._body.size() + 100;
    std::string result;
    result.reserve(estimated_size);
    
    result += req._method;
    result += " ";
    result += req._path;
    result += " ";
    result += req._version;
    result += "\r\n";
    
    for (auto &it : req._params) {
        result += it.first;
        result += ": ";
        result += it.second;
        result += "\r\n";
    }
    
    for (auto &it : req._headers) {
        result += it.first;
        result += ": ";
        result += it.second;
        result += "\r\n";
    }
    
    result += "\r\n";
    result += req._body;
    return result;
}
```

**预期收益**：QPS提升 5-10%

---

### 问题位置2
`net/http/http.hpp:106-118` - `Buffer::GetLine()` 和 `GetLineAndPop()`

### 问题描述
```cpp
std::string Buffer::GetLine()
{
    char *pos = FindcrLf();
    if (pos == NULL)
        return "";
    return ReadAsstring(pos - GetReadPtr() + 1);  // 字符串拷贝
}

std::string Buffer::GetLineAndPop()
{
    std::string str = GetLine();  // 拷贝
    MoveReadoffset(str.size());
    return str;  // 再次拷贝
}
```

**性能问题**：
- 每次调用都要创建`std::string`并拷贝数据
- HTTP解析需要频繁调用`GetLine()`

### 优化方案

**返回string_view或避免拷贝**：
```cpp
// 方案1：使用string_view（C++17）
std::string_view Buffer::GetLineView() {
    char *pos = FindcrLf();
    if (pos == NULL) return "";
    return std::string_view(GetReadPtr(), pos - GetReadPtr() + 1);
}

// 方案2：直接解析，不返回字符串
bool Buffer::ReadLine(std::string &out) {
    char *pos = FindcrLf();
    if (pos == NULL) return false;
    out.assign(GetReadPtr(), pos - GetReadPtr() + 1);
    MoveReadoffset(out.size());
    return true;
}
```

**预期收益**：QPS提升 3-5%

---

## 4. Buffer数据移动优化 ⭐⭐⭐⭐

### 问题位置
`src/Buffer.cc:24-40` - `EnsureWritableBytes()` 函数

### 问题描述
```cpp
void Buffer::EnsureWritableBytes(uint64_t len)
{
    if (TailIdleSize() >= len)
        return;
    if (HeadIdleSize() + TailIdleSize() >= len)
    {
        uint64_t rez = ReadableBytes();
        std::copy(GetReadPtr(), GetReadPtr() + rez, Begin());  // 数据移动
        _read_idx = 0;
        _write_idx = rez;
    }
    // ...
}
```

**性能问题**：
- 当缓冲区需要整理时，要用`std::copy`移动数据
- 在36k QPS下，数据移动开销累积明显

### 优化方案

**使用memmove优化**：
```cpp
void Buffer::EnsureWritableBytes(uint64_t len)
{
    if (TailIdleSize() >= len)
        return;
    if (HeadIdleSize() + TailIdleSize() >= len)
    {
        uint64_t rez = ReadableBytes();
        if (rez > 0) {
            // memmove比std::copy快，且支持重叠内存
            memmove(Begin(), GetReadPtr(), rez);
        }
        _read_idx = 0;
        _write_idx = rez;
    }
    // ...
}
```

**预期收益**：QPS提升 2-5%

---

## 5. 线程数配置优化 ⭐⭐⭐

### 问题位置
`net/http/main.cc:38`

### 问题描述
```cpp
const int THREAD_CNT = 8;  // 8线程，但只有4核
```

**性能问题**：
- 配置了8个工作线程，但CPU只有4核
- 线程数 > CPU核心数会导致线程切换开销
- 线程切换会消耗CPU时间，但不会提高吞吐量

### 优化方案

**动态获取CPU核心数**：
```cpp
#include <thread>

int main()
{
    // 获取CPU核心数
    const int THREAD_CNT = std::thread::hardware_concurrency();
    // 或者：CPU核心数 + 1（IO密集型）
    // const int THREAD_CNT = std::thread::hardware_concurrency() + 1;
    
    HttpServer server(8889, 300);
    server.SetThreadCnt(THREAD_CNT);
    // ...
}
```

**预期收益**：减少线程切换开销，CPU利用率更合理

---

## 6. 锁优化 ⭐⭐⭐

### 问题位置
`src/EventLoop.cc:28-37` - `RunAllTask()` 函数

### 问题描述
```cpp
void EventLoop::RunAllTask() {
    std::vector<Tasks> tmp;
    {
        std::unique_lock<std::mutex> _lock(_mutex);  // 锁竞争
        _task.swap(tmp);
    }
    for(auto &task : tmp) {
        task();
    }
}
```

**性能问题**：
- 8个线程可能竞争同一个锁
- 锁竞争会导致线程等待，降低并发性能

### 优化方案

**使用无锁队列**：
```cpp
#include <boost/lockfree/queue.hpp>

class EventLoop {
private:
    boost::lockfree::queue<Tasks> _task_queue{1024};  // 无锁队列
    
public:
    void TasksInLoop(const Tasks& t) {
        while (!_task_queue.push(t)) {
            // 队列满时重试或扩容
        }
    }
    
    void RunAllTask() {
        Tasks task;
        while (_task_queue.pop(task)) {
            task();
        }
    }
};
```

**预期收益**：减少锁竞争，提升并发性能

---

## 7. HTTP响应构建优化 ⭐⭐⭐

### 问题位置
`net/http/http.hpp:758-789` - `WriteResponse()` 函数

### 问题描述
```cpp
void WriteResponse(const PtrConnection &conne,const HttpRequest &req,HttpResponse *resp)
{
    std::string resp_str;
    resp_str += req._version;
    resp_str += " ";
    // ... 多次字符串拼接
}
```

**性能问题**：
- 多次字符串拼接，可能触发多次内存重分配
- 没有预分配空间

### 优化方案

**预分配空间**：
```cpp
void WriteResponse(const PtrConnection &conne,const HttpRequest &req,HttpResponse *resp)
{
    // 估算响应大小
    size_t estimated_size = resp->_body.size() + 512;  // 头部约512字节
    std::string resp_str;
    resp_str.reserve(estimated_size);
    
    resp_str += req._version;
    resp_str += " ";
    // ... 后续拼接
}
```

**预期收益**：QPS提升 2-3%

---

## 8. URL解码优化 ⭐⭐

### 问题位置
`net/http/http.hpp:273-304` - `Util::UrlDecode()` 函数

### 问题描述
每次解析HTTP请求都要做URL解码，可能有性能开销。

### 优化方案

**延迟解码**：
- 只在需要时才解码
- 对于固定路径（如`/hello`），不需要解码

**预期收益**：QPS提升 1-2%

---

## 9. epoll_wait超时优化 ⭐⭐

### 问题位置
`src/Poller.cc:67`

### 问题描述
```cpp
int nfds = epoll_wait(_epfd, _evs, MAX_EPOLLEVENTS, -1); //-1为阻塞监控
```

**性能问题**：
- `timeout = -1` 表示无限阻塞等待
- 当没有事件时，线程会阻塞，CPU利用率自然为0

### 优化方案

**使用小超时**：
```cpp
int nfds = epoll_wait(_epfd, _evs, MAX_EPOLLEVENTS, 1); // 1ms超时
```

**注意**：这会增加CPU占用（空转），需要权衡。

---

## 10. 内存对齐优化 ⭐⭐

### 问题位置
`include/Buffer.hpp` - Buffer类

### 优化方案

**结构体对齐**：
```cpp
class Buffer {
private:
    alignas(64) std::vector<char> _buffer;  // 64字节对齐，避免false sharing
    uint64_t _read_idx;
    uint64_t _write_idx;
};
```

**预期收益**：减少false sharing，提升多核性能

---

## 优化优先级总结

| 优化项 | 优先级 | 预期收益 | 实现难度 |
|--------|--------|----------|----------|
| HTTP路由匹配优化 | ⭐⭐⭐⭐⭐ | 10-20% | 中 |
| HTTP请求行解析优化 | ⭐⭐⭐⭐⭐ | 15-25% | 中 |
| 字符串操作优化 | ⭐⭐⭐⭐ | 5-10% | 低 |
| Buffer数据移动优化 | ⭐⭐⭐⭐ | 2-5% | 低 |
| 线程数配置优化 | ⭐⭐⭐ | - | 低 |
| 锁优化 | ⭐⭐⭐ | - | 中 |
| HTTP响应构建优化 | ⭐⭐⭐ | 2-3% | 低 |
| URL解码优化 | ⭐⭐ | 1-2% | 低 |
| epoll_wait超时优化 | ⭐⭐ | - | 低 |
| 内存对齐优化 | ⭐⭐ | - | 低 |

## 建议优化顺序

1. **第一阶段**（快速收益）：
   - HTTP请求行解析优化（手动解析）
   - 字符串操作优化（RequestStr）
   - Buffer数据移动优化

2. **第二阶段**（中等收益）：
   - HTTP路由匹配优化（精确匹配优先）
   - HTTP响应构建优化

3. **第三阶段**（长期优化）：
   - 锁优化（无锁队列）
   - 线程数配置优化
   - 其他小优化

## 预期总体收益

如果实施前两阶段的优化，预计QPS可以提升 **30-50%**，从36k提升到 **47k-54k**。
