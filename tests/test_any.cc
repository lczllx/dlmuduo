#include <gtest/gtest.h>
#include "Any.hpp"
#include <string>

TEST(Any, DefaultConstructed) {
    Any a;
    EXPECT_FALSE(a.has_value());
    EXPECT_EQ(a.get<int>(), nullptr);
}

TEST(Any, StoreAndRetrieve) {
    Any a(42);
    EXPECT_TRUE(a.has_value());
    EXPECT_EQ(*a.get<int>(), 42);
}

TEST(Any, StoreString) {
    Any a(std::string("hello"));
    EXPECT_TRUE(a.has_value());
    EXPECT_EQ(*a.get<std::string>(), "hello");
}

TEST(Any, WrongTypeReturnsNull) {
    Any a(42);
    EXPECT_EQ(a.get<std::string>(), nullptr);
    EXPECT_EQ(a.get<double>(), nullptr);
}

TEST(Any, CopyConstructor) {
    Any a(100);
    Any b(a);
    EXPECT_EQ(*b.get<int>(), 100);
    // 修改不影响原对象
    *b.get<int>() = 200;
    EXPECT_EQ(*a.get<int>(), 100);
    EXPECT_EQ(*b.get<int>(), 200);
}

TEST(Any, MoveConstructor) {
    Any a(42);
    Any b(std::move(a));
    EXPECT_FALSE(a.has_value());
    EXPECT_TRUE(b.has_value());
    EXPECT_EQ(*b.get<int>(), 42);
}

TEST(Any, CopyAssignment) {
    Any a(10);
    Any b(20);
    b = a;
    EXPECT_EQ(*b.get<int>(), 10);
}

TEST(Any, MoveAssignment) {
    Any a(10);
    Any b(20);
    // Any 没有移动赋值运算符，通过 swap 实现移动语义
    a.swap(b);
    EXPECT_TRUE(a.has_value());
    EXPECT_TRUE(b.has_value());
    EXPECT_EQ(*a.get<int>(), 20);
    EXPECT_EQ(*b.get<int>(), 10);
}

TEST(Any, Reset) {
    Any a(42);
    EXPECT_TRUE(a.has_value());
    a.reset();
    EXPECT_FALSE(a.has_value());
    a.reset(); // double reset should be safe
    EXPECT_FALSE(a.has_value());
}

TEST(Any, Swap) {
    Any a(10);
    Any b(std::string("world"));
    a.swap(b);
    EXPECT_EQ(*a.get<std::string>(), "world");
    EXPECT_EQ(*b.get<int>(), 10);
}

TEST(Any, TypeInfo) {
    Any a(42);
    EXPECT_EQ(a.type(), typeid(int));
    Any empty;
    EXPECT_EQ(empty.type(), typeid(void));
}

TEST(Any, ConstGet) {
    const Any a(3.14);
    const double* p = a.get<double>();
    EXPECT_NE(p, nullptr);
    EXPECT_DOUBLE_EQ(*p, 3.14);
    EXPECT_EQ(a.get<int>(), nullptr);
}

TEST(Any, SelfAssignment) {
    Any a(42);
    a = a;
    EXPECT_TRUE(a.has_value());
    EXPECT_EQ(*a.get<int>(), 42);
}
