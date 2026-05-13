# HTTP服务器示例程序
add_executable(http_server main.cc)
target_link_libraries(http_server lczmuduo)
add_custom_command(TARGET http_server POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_CURRENT_SOURCE_DIR}/wwwroot $<TARGET_FILE_DIR:http_server>/wwwroot)

add_subdirectory(shortener)
