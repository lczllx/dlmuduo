/**
 * 十万并发连接压测客户端
 * 
 * 用于测试 muduo 网络库能否支持高并发连接
 * 使用 epoll + 非阻塞 connect 实现单进程管理大量连接
 * 
 * 用法: ./concurrent_test <host> <port> <connections> [hold_seconds] [bind_ip]
 * 示例: ./concurrent_test 127.0.0.1 8889 100000 60
 * 示例: ./concurrent_test 127.0.0.1 8889 25000 60 127.0.0.2  # 指定源IP，多进程压测用
 *
 * 单机 10 万连接: 多进程 + 多源 IP，如 run_100k.sh
 */

#include <iostream>
#include <string>
#include <cstring>
#include <vector>
#include <set>
#include <algorithm>
#include <unordered_map>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <time.h>

static volatile bool g_running = true;

void signal_handler(int) { g_running = false; }

int set_nonblocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

int main(int argc, char* argv[]) {
    if (argc < 4) {
        std::cerr << "用法: " << argv[0] << " <host> <port> <connections> [hold_seconds] [bind_ip]\n"
                  << "  bind_ip: 可选，绑定源IP，多进程压测时用不同IP突破端口限制\n";
        return 1;
    }

    const char* host = argv[1];
    int port = atoi(argv[2]);
    int target_conns = atoi(argv[3]);
    int hold_seconds = (argc >= 5) ? atoi(argv[4]) : 60;
    const char* bind_ip = (argc >= 6) ? argv[5] : nullptr;

    if (target_conns <= 0 || target_conns > 500000) {
        std::cerr << "连接数建议范围: 1 ~ 500000\n";
        return 1;
    }

    signal(SIGINT, signal_handler);

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    if (inet_pton(AF_INET, host, &addr.sin_addr) <= 0) {
        std::cerr << "无效地址: " << host << "\n";
        return 1;
    }

    int epfd = epoll_create1(EPOLL_CLOEXEC);
    if (epfd < 0) {
        perror("epoll_create1");
        return 1;
    }

    std::vector<int> fds;
    fds.reserve(target_conns);
    int batch = 5000;  // 每批建立连接数，避免瞬间打满
    int connected = 0;
    int failed = 0;
    int created = 0;
    std::set<int> counted_fds;  // 已计入 connected 的 fd，避免 EPOLL 重复计数
    std::vector<int> immediate_connects;  // 立即连接成功的 fd

    std::cout << "目标连接数: " << target_conns 
              << ", 保持时长: " << hold_seconds << " 秒\n"
              << "开始建立连接...\n";

    time_t start_time = time(nullptr);

    while (g_running && created < target_conns) {
        int to_create = std::min(batch, target_conns - created);
        
        for (int i = 0; i < to_create && g_running; ++i) {
            int fd = socket(AF_INET, SOCK_STREAM, 0);
            if (fd < 0) {
                if (errno == EMFILE || errno == ENFILE) {
                    std::cerr << "\n文件描述符耗尽，当前已创建 " << created << " 个\n";
                    goto poll_phase;
                }
                failed++;
                continue;
            }

            set_nonblocking(fd);
            if (bind_ip) {
                struct sockaddr_in local;
                memset(&local, 0, sizeof(local));
                local.sin_family = AF_INET;
                local.sin_port = 0;
                if (inet_pton(AF_INET, bind_ip, &local.sin_addr) <= 0) {
                    close(fd);
                    failed++;
                    continue;
                }
                if (bind(fd, (struct sockaddr*)&local, sizeof(local)) != 0) {
                    close(fd);
                    failed++;
                    continue;
                }
            }
            int ret = connect(fd, (struct sockaddr*)&addr, sizeof(addr));
            
            if (ret == 0) {
                connected++;
                created++;
                immediate_connects.push_back(fd);  // 先不加入 epoll，避免重复计数
            } else if (errno == EINPROGRESS) {
                struct epoll_event ev;
                ev.events = EPOLLIN | EPOLLOUT | EPOLLET;
                ev.data.fd = fd;
                epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &ev);
                fds.push_back(fd);
                created++;
            } else {
                failed++;
                close(fd);
            }
        }

        // 处理 epoll 中已就绪的连接（EINPROGRESS 完成），需多次 drain 避免 2048/4096 等上限
        // 注意：evs[8192] ≈ 98KB 栈空间，musl (Alpine) 默认栈 128KB 需确保 ulimit -s >= 256
        struct epoll_event evs[8192];
        int drain_rounds = 0;
        while (drain_rounds++ < 20) {  // 每批最多多轮 drain，确保高并发时不积压
            int n = epoll_wait(epfd, evs, 8192, 5);
            if (n <= 0) break;
            for (int i = 0; i < n; ++i) {
                int fd = evs[i].data.fd;
                if (counted_fds.count(fd)) continue;  // 已计数过
                int err = 0;
                socklen_t len = sizeof(err);
                if (getsockopt(fd, SOL_SOCKET, SO_ERROR, &err, &len) == 0 && err == 0) {
                    connected++;
                    counted_fds.insert(fd);
                } else {
                    epoll_ctl(epfd, EPOLL_CTL_DEL, fd, nullptr);
                    close(fd);
                    auto it = std::find(fds.begin(), fds.end(), fd);
                    if (it != fds.end()) fds.erase(it);
                    failed++;
                }
            }
        }

        if (created % 10000 == 0 && created > 0) {
            std::cout << "  已发起: " << created << ", 已连接: " << connected 
                      << ", 失败: " << failed << "\r" << std::flush;
        }
    }

poll_phase:
    std::cout << "\n连接建立阶段完成。已发起: " << created << ", 已连接: " << connected 
              << ", 失败: " << failed << "\n";

    // 继续 poll 等待剩余 EINPROGRESS 连接完成（最多等 60 秒）
    time_t poll_deadline = time(nullptr) + 60;
    while (g_running && time(nullptr) < poll_deadline) {
        struct epoll_event evs[4096];
        int n = epoll_wait(epfd, evs, 4096, 500);
        for (int i = 0; i < n; ++i) {
            int fd = evs[i].data.fd;
            if (counted_fds.count(fd)) continue;
            int err = 0;
            socklen_t len = sizeof(err);
            if (getsockopt(fd, SOL_SOCKET, SO_ERROR, &err, &len) == 0 && err == 0) {
                connected++;
                counted_fds.insert(fd);
            } else {
                epoll_ctl(epfd, EPOLL_CTL_DEL, fd, nullptr);
                close(fd);
                auto it = std::find(fds.begin(), fds.end(), fd);
                if (it != fds.end()) fds.erase(it);
                failed++;
            }
        }
    }

    // 将立即连接成功的 fd 加入 epoll，供后续保持阶段使用
    for (int fd : immediate_connects) {
        struct epoll_event ev;
        ev.events = EPOLLIN | EPOLLOUT | EPOLLET;
        ev.data.fd = fd;
        epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &ev);
        fds.push_back(fd);
    }
    immediate_connects.clear();

    time_t connect_done = time(nullptr);
    std::cout << "最终统计 - 成功连接: " << connected << ", 失败: " << failed 
              << ", 耗时: " << (connect_done - start_time) << " 秒\n";

    if (connected == 0) {
        std::cerr << "没有成功连接，请检查服务器是否启动、端口、防火墙及 ulimit\n";
        for (int fd : fds) close(fd);
        for (int fd : immediate_connects) close(fd);
        close(epfd);
        return 1;
    }

    // 保持连接
    if (hold_seconds > 0) {
        std::cout << "保持 " << connected << " 个连接 " << hold_seconds << " 秒...\n";
        time_t hold_until = time(nullptr) + hold_seconds;
        while (g_running && time(nullptr) < hold_until) {
            struct epoll_event evs[4096];
            int n = epoll_wait(epfd, evs, 4096, 1000);
            for (int i = 0; i < n; ++i) {
                int fd = evs[i].data.fd;
                if (evs[i].events & (EPOLLERR | EPOLLHUP)) {
                    epoll_ctl(epfd, EPOLL_CTL_DEL, fd, nullptr);
                    close(fd);
                    auto it = std::find(fds.begin(), fds.end(), fd);
                    if (it != fds.end()) fds.erase(it);
                    connected--;
                }
            }
            static time_t last_print = 0;
            if (time(nullptr) - last_print >= 5 && time(nullptr) < hold_until) {
                last_print = time(nullptr);
                std::cout << "  保持中，当前连接数: " << connected << "\r" << std::flush;
            }
        }
        std::cout << "\n保持阶段结束，剩余连接: " << connected << "\n";
    }

    for (int fd : fds) { close(fd); }
    close(epfd);

    std::cout << "压测完成。\n";
    return 0;
}
