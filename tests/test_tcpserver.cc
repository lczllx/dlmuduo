#include <gtest/gtest.h>
#include "TcpServer.hpp"
#include "Buffer.hpp"
#include <thread>
#include <atomic>
#include <chrono>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>

TEST(TcpServer, SetCallbacks) {
    TcpServer server(19996);
    server.SetThreadCnt(0);
    server.SetConnectedCallBack([](const PtrConnection&) {});
    server.SetClosedCallBack([](const PtrConnection&) {});
    server.SetMessageCallBack([](const PtrConnection&, Buffer*) {});
    server.SetAnyEventCallBack([](const PtrConnection&) {});
    server.EnableInactiveRelease(60);
    SUCCEED();
}

TEST(TcpServer, MessageEcho) {
    TcpServer server(19995);
    server.SetThreadCnt(1);

    std::string received;
    server.SetMessageCallBack([&](const PtrConnection& conn, Buffer* buf) {
        if (buf && buf->ReadableBytes() > 0) {
            received = buf->ReadAsstringandpop(buf->ReadableBytes());
            conn->Send(received.c_str(), received.size());
            conn->Shutdown();
        }
    });

    std::thread t([&]() { server.Start(); });
    std::this_thread::sleep_for(std::chrono::milliseconds(300));

    int fd = socket(AF_INET, SOCK_STREAM, 0);
    ASSERT_GE(fd, 0);
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(19995);
    inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);
    ASSERT_EQ(connect(fd, (sockaddr*)&addr, sizeof(addr)), 0);

    const char* msg = "hello_tcp";
    send(fd, msg, strlen(msg), 0);
    shutdown(fd, SHUT_WR);

    char buf[256] = {};
    ssize_t n = recv(fd, buf, sizeof(buf) - 1, 0);
    EXPECT_GT(n, 0);
    if (n > 0) {
        buf[n] = '\0';
        EXPECT_STREQ(buf, "hello_tcp");
    }
    close(fd);

    server.Stop();  // 安全退出 base_loop
    t.join();       // 等待 Start() 返回
}
