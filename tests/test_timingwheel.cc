#include <gtest/gtest.h>
#include "EventLoop.hpp"

// TimerCancel 仅标记 _cancel=true 防止回调触发，不删除 _timers 条目。
// HasTimer 在 cancel 后仍返回 true。

TEST(TimingWheel, AddAndHasTimer) {
    EventLoop loop;
    EXPECT_FALSE(loop.HasTimer(1));
    loop.TimerAdd(1, 10, []() {});
    EXPECT_TRUE(loop.HasTimer(1));
}

TEST(TimingWheel, CancelMarksTimer) {
    EventLoop loop;
    loop.TimerAdd(1, 10, []() { FAIL() << "cancelled timer fired"; });
    EXPECT_TRUE(loop.HasTimer(1));
    loop.TimerCancel(1);
    EXPECT_TRUE(loop.HasTimer(1));
}

TEST(TimingWheel, RefreshExtendsTimer) {
    EventLoop loop;
    loop.TimerAdd(1, 20, []() {});
    loop.TimerReflesh(1);
    EXPECT_TRUE(loop.HasTimer(1));
}

TEST(TimingWheel, CancelNonexistent) {
    EventLoop loop;
    loop.TimerCancel(999);
    loop.TimerReflesh(999);
    SUCCEED();
}

TEST(TimingWheel, MultipleTimers) {
    EventLoop loop;
    for (int i = 1; i <= 5; i++)
        loop.TimerAdd(i, i * 10, []() {});
    for (int i = 1; i <= 5; i++)
        EXPECT_TRUE(loop.HasTimer(i));
}
