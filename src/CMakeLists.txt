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

# 设置库的链接库
target_link_libraries(lczmuduo pthread)

# 安装静态库
install(TARGETS lczmuduo
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
)
