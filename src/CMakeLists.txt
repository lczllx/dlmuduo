#构建muduo静态库
# 源文件目录的CMakeLists.txt
set(SOURCES
    Acceptor.cc
    Buffer.cc
    Channel.cc
    Connection.cc
    EventLoop.cc
    LoopThread.cc
    LoopThreadPool.cc
    Poller.cc
    Socket.cc
    TcpServer.cc
    Timer.cc
    muduo_server_init.cc
)

# 创建静态库
add_library(lczmuduo STATIC ${SOURCES})

# 库的公开头文件目录（链接此库的目标自动获得 include 路径）
target_include_directories(lczmuduo
    PUBLIC
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include/lczmuduo>
)

# 线程库（替代裸 pthread）
target_link_libraries(lczmuduo PUBLIC Threads::Threads)

# 安装静态库
install(TARGETS lczmuduo
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
)
