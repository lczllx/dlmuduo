#include <gtest/gtest.h>
#include "EventLoop.hpp"

TEST(EventLoop, IsInLoop) {
    EventLoop loop;
    EXPECT_TRUE(loop.IsInLoop());
}

TEST(EventLoop, AssertInLoopDoesNotCrash) {
    EventLoop loop;
    loop.AssertInLoop();
}

TEST(EventLoop, RunInLoopFromSameThread) {
    EventLoop loop;
    bool called = false;
    loop.RunInLoop([&]() { called = true; });
    EXPECT_TRUE(called);
}

TEST(EventLoop, TimerAddAndCancel) {
    EventLoop loop;
    loop.TimerAdd(1, 10, []() {});
    EXPECT_TRUE(loop.HasTimer(1));
    loop.TimerCancel(1);
    EXPECT_TRUE(loop.HasTimer(1));  // cancel 不删 _timers 条目
}

TEST(EventLoop, TimerRefresh) {
    EventLoop loop;
    loop.TimerAdd(42, 30, []() {});
    EXPECT_TRUE(loop.HasTimer(42));
    loop.TimerReflesh(42);
    EXPECT_TRUE(loop.HasTimer(42));
    loop.TimerCancel(42);
}

TEST(EventLoop, HasTimerFalseForNonexistent) {
    EventLoop loop;
    EXPECT_FALSE(loop.HasTimer(999));
}

TEST(EventLoop, MultipleTimers) {
    EventLoop loop;
    loop.TimerAdd(1, 100, []() {});
    loop.TimerAdd(2, 200, []() {});
    loop.TimerAdd(3, 300, []() {});
    EXPECT_TRUE(loop.HasTimer(1));
    EXPECT_TRUE(loop.HasTimer(2));
    EXPECT_TRUE(loop.HasTimer(3));
    loop.TimerCancel(2);
    loop.TimerCancel(1);
    loop.TimerCancel(3);
}
