find_package(PkgConfig REQUIRED)
pkg_check_modules(JSONCPP REQUIRED jsoncpp)

find_library(HIREDIS_LIB hiredis)
find_library(MYSQL_LIB  mysqlclient)

# 校验 find_library 结果
if(NOT HIREDIS_LIB)
    message(FATAL_ERROR "hiredis library not found. Install libhiredis-dev (apt) or hiredis-devel (dnf/yum)")
endif()
if(NOT MYSQL_LIB)
    message(FATAL_ERROR "mysqlclient library not found. Install libmysqlclient-dev (apt) or mysql-devel (dnf/yum)")
endif()

add_executable(shortener_server shortener_server.cc shortener.cc)
target_include_directories(shortener_server PRIVATE ${JSONCPP_INCLUDE_DIRS})
target_link_libraries(shortener_server lczmuduo ${HIREDIS_LIB} ${MYSQL_LIB} ${JSONCPP_LIBRARIES} Threads::Threads)
