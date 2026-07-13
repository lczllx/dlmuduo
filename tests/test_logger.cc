#include <gtest/gtest.h>
#include "Logger.hpp"

TEST(Logger, LevelsExist) {
    EXPECT_NE(muduo::DEBUG, muduo::FATAL);
    EXPECT_NE(muduo::INFO, muduo::ERROR);
}

TEST(Logger, DoesNotCrash) {
    muduo::Logger(muduo::DEBUG, __FILE__, __LINE__)("test debug %d", 1);
    muduo::Logger(muduo::INFO, __FILE__, __LINE__)("test info %s", "hello");
    muduo::Logger(muduo::WARN, __FILE__, __LINE__)("test warn");
    muduo::Logger(muduo::ERROR, __FILE__, __LINE__)("test error");
    SUCCEED();
}

TEST(Logger, StringArg) {
    muduo::Logger(muduo::INFO, __FILE__, __LINE__)(std::string("string message"));
    muduo::Logger(muduo::WARN, __FILE__, __LINE__)("c-string message");
    SUCCEED();
}
