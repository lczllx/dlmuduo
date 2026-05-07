# HTTP服务器示例程序
add_executable(http_server main.cc)
target_link_libraries(http_server lczmuduo)

add_subdirectory(shortener)
