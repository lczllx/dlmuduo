# 单元测试 — Google Test
# 优先级：系统包 (GTest::Main) > 系统源码编译 (/usr/src/googletest) > FetchContent 下载
enable_testing()

find_package(GTest QUIET)
if(GTest_FOUND)
    message(STATUS "Using system GTest (libgtest-dev)")
    set(GTEST_TARGET GTest::gtest_main)
elseif(EXISTS /usr/src/googletest/CMakeLists.txt)
    message(STATUS "Building GTest from /usr/src/googletest")
    add_subdirectory(/usr/src/googletest ${CMAKE_BINARY_DIR}/googletest-build
        EXCLUDE_FROM_ALL)
    set(GTEST_TARGET gtest_main)
else()
    include(FetchContent)
    FetchContent_Declare(googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG        v1.15.2
    )
    FetchContent_MakeAvailable(googletest)
    set(GTEST_TARGET gtest_main)
endif()

set(TEST_SOURCES
    test_buffer.cc test_any.cc test_logger.cc
    test_loopthread.cc test_loopthreadpool.cc test_timingwheel.cc
    test_eventloop.cc test_socket.cc test_channel.cc
    test_connection.cc test_tcpserver.cc
)

foreach(test_file ${TEST_SOURCES})
    get_filename_component(test_name ${test_file} NAME_WE)
    add_executable(${test_name} ${test_file})
    target_link_libraries(${test_name} PRIVATE lczmuduo ${GTEST_TARGET} Threads::Threads pthread)
    target_include_directories(${test_name} PRIVATE ${PROJECT_SOURCE_DIR}/include)
    add_test(NAME ${test_name} COMMAND ${test_name})
endforeach()
