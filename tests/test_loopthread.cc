#include <gtest/gtest.h>
#include "LoopThread.hpp"
#include "EventLoop.hpp"

TEST(LoopThread, GetLoopReturnsValidPointer) {
    LoopThread lt;
    EventLoop* loop = lt.Getloop();
    EXPECT_NE(loop, nullptr);
}

TEST(LoopThread, GetLoopIsIdempotent) {
    LoopThread lt;
    EventLoop* a = lt.Getloop();
    EventLoop* b = lt.Getloop();
    EXPECT_EQ(a, b);
}

TEST(LoopThread, LoopIsRunning) {
    LoopThread lt;
    EventLoop* loop = lt.Getloop();
    EXPECT_FALSE(loop->IsInLoop());
}
