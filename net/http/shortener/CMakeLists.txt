find_package(PkgConfig REQUIRED)
pkg_check_modules(JSONCPP REQUIRED jsoncpp)

find_library(HIREDIS_LIB hiredis PATHS /usr/lib /usr/lib/x86_64-linux-gnu)
find_library(MYSQL_LIB  mysqlclient PATHS /usr/lib /usr/lib/x86_64-linux-gnu)

add_executable(shortener_server shortener_server.cc shortener.cc)
target_include_directories(shortener_server PRIVATE ${JSONCPP_INCLUDE_DIRS})
target_link_libraries(shortener_server lczmuduo ${HIREDIS_LIB} ${MYSQL_LIB} ${JSONCPP_LIBRARIES} pthread)
