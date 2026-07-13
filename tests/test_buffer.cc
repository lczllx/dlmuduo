#include <gtest/gtest.h>
#include "Buffer.hpp"

TEST(Buffer, InitialState) {
    Buffer buf;
    EXPECT_EQ(buf.ReadableBytes(), 0);
    EXPECT_EQ(buf.HeadIdleSize(), 0);
    EXPECT_GE(buf.TailIdleSize(), BUFFER_DEFAULT_SIZE);
}

TEST(Buffer, WriteAndRead) {
    Buffer buf;
    const char* msg = "hello";
    buf.WriteAndpush(msg, 5);
    EXPECT_EQ(buf.ReadableBytes(), 5);
    char out[6] = {};
    buf.ReadAndpop(out, 5);
    EXPECT_STREQ(out, "hello");
    EXPECT_EQ(buf.ReadableBytes(), 0);
}

TEST(Buffer, WriteString) {
    Buffer buf;
    buf.WritestringAndpush(std::string("world"));
    EXPECT_EQ(buf.ReadableBytes(), 5);
    EXPECT_EQ(buf.ReadAsstringandpop(5), "world");
    EXPECT_EQ(buf.ReadableBytes(), 0);
}

TEST(Buffer, WriteBuffer) {
    Buffer a, b;
    a.WritestringAndpush("abc");
    b.WriteBufferAndpush(a);
    EXPECT_EQ(b.ReadableBytes(), 3);
    EXPECT_EQ(b.ReadAsstringandpop(3), "abc");
}

TEST(Buffer, EnsureWritable) {
    Buffer buf;
    // 写入超过默认容量
    std::string big(BUFFER_DEFAULT_SIZE + 100, 'x');
    buf.WritestringAndpush(big);
    EXPECT_EQ(buf.ReadableBytes(), big.size());
    EXPECT_EQ(buf.ReadAsstringandpop(big.size()), big);
}

TEST(Buffer, FindCrLf) {
    Buffer buf;
    buf.WritestringAndpush("line1\nline2");
    char* crlf = buf.FindcrLf();
    EXPECT_NE(crlf, nullptr);
    EXPECT_EQ(*crlf, '\n');
    // \n 位于 "line1" (5 字节) 之后
    EXPECT_EQ(crlf - buf.GetReadPtr(), 5);
}

TEST(Buffer, GetLine) {
    Buffer buf;
    buf.WritestringAndpush("first\nsecond\n");
    EXPECT_EQ(buf.GetLineAndPop(), "first\n");
    EXPECT_EQ(buf.GetLineAndPop(), "second\n");
}

TEST(Buffer, Clear) {
    Buffer buf;
    buf.WritestringAndpush("data");
    EXPECT_EQ(buf.ReadableBytes(), 4);
    buf.clear();
    EXPECT_EQ(buf.ReadableBytes(), 0);
    EXPECT_EQ(buf.HeadIdleSize(), 0);
}

TEST(Buffer, ReadBeyondBoundary) {
    Buffer buf;
    buf.WritestringAndpush("hi");
    char out[10];
    buf.ReadAndpop(out, 2);
    // 读空后再读应该什么都不发生（取决于实现，但不应崩溃）
    EXPECT_EQ(buf.ReadableBytes(), 0);
}

TEST(Buffer, EmptyOperations) {
    Buffer buf;
    EXPECT_EQ(buf.ReadAsstring(0), "");
    EXPECT_EQ(buf.FindcrLf(), nullptr);
    EXPECT_EQ(buf.GetLine(), "");
    buf.clear(); // 多次 clear 不应异常
    buf.clear();
}

TEST(Buffer, LargeData) {
    Buffer buf;
    std::string large(100000, 'A');
    buf.WritestringAndpush(large);
    EXPECT_EQ(buf.ReadableBytes(), 100000);
    std::string result = buf.ReadAsstringandpop(50000);
    EXPECT_EQ(result.size(), 50000);
    EXPECT_EQ(buf.ReadableBytes(), 50000);
}

TEST(Buffer, MoveOffsets) {
    Buffer buf;
    buf.WritestringAndpush("1234567890");
    buf.MoveReadoffset(3);
    EXPECT_EQ(buf.ReadableBytes(), 7);
    EXPECT_EQ(buf.ReadAsstringandpop(7), "4567890");
}
