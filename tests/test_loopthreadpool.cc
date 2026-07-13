#include <gtest/gtest.h>
#include "LoopThreadPool.hpp"
#include "EventLoop.hpp"
#include <set>

TEST(LoopThreadPool, ZeroThreadsReturnsBaseLoop) {
    EventLoop base;
    LoopThreadPool pool(&base);
    pool.SetThreadCnt(0);
    pool.Create();
    EXPECT_EQ(pool.NextLoop(), &base);
    EXPECT_EQ(pool.NextLoop(), &base);
}

TEST(LoopThreadPool, MultipleThreadsRoundRobin) {
    EventLoop base;
    LoopThreadPool pool(&base);
    pool.SetThreadCnt(4);
    pool.Create();

    std::set<EventLoop*> loops;
    for (int i = 0; i < 4; i++) {
        EventLoop* lp = pool.NextLoop();
        EXPECT_NE(lp, nullptr);
        EXPECT_NE(lp, &base);
        loops.insert(lp);
    }
    EXPECT_EQ(loops.size(), 4);
}

TEST(LoopThreadPool, RoundRobinWraps) {
    EventLoop base;
    LoopThreadPool pool(&base);
    pool.SetThreadCnt(2);
    pool.Create();

    EventLoop* first = pool.NextLoop();
    EventLoop* second = pool.NextLoop();
    EventLoop* third = pool.NextLoop();
    EXPECT_EQ(third, first);
    EXPECT_NE(first, second);
}
