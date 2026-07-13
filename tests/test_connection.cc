#include <gtest/gtest.h>
#include "Connection.hpp"
#include "EventLoop.hpp"
#include "Socket.hpp"
#include <sys/socket.h>

static std::pair<int, int> MakePair() {
    int sv[2];
    socketpair(AF_UNIX, SOCK_STREAM, 0, sv);
    return {sv[0], sv[1]};
}

TEST(Connection, ConstructAndDestroy) {
    auto [srv, cli] = MakePair();
    EventLoop loop;
    PtrConnection conn(new Connection(&loop, 1, cli));
    EXPECT_EQ(conn->Fd(), cli);
    EXPECT_EQ(conn->Id(), 1);
    EXPECT_FALSE(conn->Connected());
    close(srv);
}

TEST(Connection, SetCallbacks) {
    auto [srv, cli] = MakePair();
    EventLoop loop;
    PtrConnection conn(new Connection(&loop, 1, cli));
    conn->SetConnectedCallBack([](const PtrConnection&) {});
    conn->SetClosedCallBack([](const PtrConnection&) {});
    conn->SetMessageCallBack([](const PtrConnection&, Buffer*) {});
    conn->SetAnyEventCallBack([](const PtrConnection&) {});
    close(srv);
    SUCCEED();
}

TEST(Connection, Context) {
    auto [srv, cli] = MakePair();
    EventLoop loop;
    PtrConnection conn(new Connection(&loop, 1, cli));
    conn->SetContext(Any(42));
    EXPECT_TRUE(conn->GetContext()->has_value());
    EXPECT_EQ(*conn->GetContext()->get<int>(), 42);
    close(srv);
}

TEST(Connection, EnableInactiveRelease) {
    auto [srv, cli] = MakePair();
    EventLoop loop;
    PtrConnection conn(new Connection(&loop, 1, cli));
    conn->EnableInactiveRelease(60);
    conn->CancelInactiveRelease();
    close(srv);
}
