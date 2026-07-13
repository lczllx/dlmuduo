# HTTP服务器示例程序
add_executable(http_server main.cc)
target_link_libraries(http_server lczmuduo)

# 复制 wwwroot 到输出目录（文件级依赖跟踪，避免每次 POST_BUILD 都复制）
add_custom_command(
    OUTPUT ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/wwwroot
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/wwwroot ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/wwwroot
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/wwwroot
    COMMENT "Copying wwwroot to output directory"
)
add_custom_target(copy_wwwroot DEPENDS ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/wwwroot)
add_dependencies(http_server copy_wwwroot)

add_subdirectory(shortener)
