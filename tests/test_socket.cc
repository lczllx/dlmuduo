#include <gtest/gtest.h>
#include "Socket.hpp"

TEST(Socket, DefaultConstructed) {
    Socket s;
    EXPECT_EQ(s.Fd(), -1);
}

TEST(Socket, CreateSocket) {
    Socket s;
    EXPECT_TRUE(s.Create());
    EXPECT_GE(s.Fd(), 0);
}

TEST(Socket, NoBlock) {
    Socket s;
    s.Create();
    s.NoBlock();
    // 不应崩溃
    SUCCEED();
}

TEST(Socket, ReuseAddress) {
    Socket s;
    s.Create();
    s.ReuseAdderess();
    SUCCEED();
}

TEST(Socket, Close) {
    Socket s;
    s.Create();
    EXPECT_GE(s.Fd(), 0);
    s.Close();
    EXPECT_EQ(s.Fd(), -1);
}

TEST(Socket, DoubleClose) {
    Socket s;
    s.Create();
    s.Close();
    s.Close(); // 不应崩溃
    SUCCEED();
}

TEST(Socket, CreateServer) {
    Socket s;
    EXPECT_TRUE(s.CreateServer(19999));
    EXPECT_GE(s.Fd(), 0);
}
