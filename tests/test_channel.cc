#include <gtest/gtest.h>
#include "Channel.hpp"
#include "EventLoop.hpp"
#include <sys/eventfd.h>
#include <thread>
#include <atomic>

TEST(Channel, EnableDisableRead) {
    // 在一个独立线程中运行 EventLoop
    std::atomic<bool> ready{false};
    std::thread t([&]() {
        EventLoop loop;
        int efd = eventfd(0, EFD_NONBLOCK);
        Channel ch(&loop, efd);

        EXPECT_EQ(ch.Fd(), efd);
        EXPECT_FALSE(ch.ReadAble());
        EXPECT_FALSE(ch.WriteAble());

        ch.EnableRead();
        EXPECT_TRUE(ch.ReadAble());

        ch.DisableRead();
        EXPECT_FALSE(ch.ReadAble());

        ch.Remove();
        close(efd);
        ready = true;
    });
    t.join();
    EXPECT_TRUE(ready);
}

TEST(Channel, SetCallbacks) {
    EventLoop loop;
    int efd = eventfd(0, EFD_NONBLOCK);
    Channel ch(&loop, efd);

    int read_called = 0;
    ch.SetReadCallback([&]() { read_called++; });
    ch.SetWriteCallback([&]() {});
    ch.SetErrorCallback([&]() {});
    ch.SetCloseCallback([&]() {});
    ch.SetEventCallback([&]() {});

    ch.EnableRead();
    ch.DisableAll();
    close(efd);

    // 回调在事件触发时调用，这里仅验证设置不崩溃
    SUCCEED();
}

TEST(Channel, UpdateAndRemove) {
    EventLoop loop;
    int efd = eventfd(0, EFD_NONBLOCK);
    Channel ch(&loop, efd);

    ch.EnableRead();
    ch.EnableWrite();
    ch.DisableAll();
    ch.Remove();
    close(efd);
    SUCCEED();
}
