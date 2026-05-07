#include "shortener.hpp"
#include <log_system/Logger.hpp>

std::unique_ptr<Redis_Client> g_redis;
std::unique_ptr<Mysql_Pool>   g_mysql;
std::string g_base_url = "http://localhost:8889/";

int main()
{
    lcz::LoggerManager::getInstance().rootLogger()->setLevel(lcz::LogLevel::value::INFO);

    g_mysql.reset(new Mysql_Pool("127.0.0.1", "root", "shortener", "shortener", 3307, 4));
    if (!g_mysql->Init()) {
        L_ERROR("MySQL init failed");
        return 1;
    }
    L_INFO("MySQL init OK");

    g_redis.reset(new Redis_Client());
    if (!g_redis->Connect()) {
        L_ERROR("Redis connect failed");
        return 1;
    }
    L_INFO("Redis connect OK");

    HttpServer server(8889, 10);
    server.SetThreadCnt(4);
    server.Post("/api/shorten", ApiShorten);
    server.Get("/([A-Za-z0-9]+)", Redirect);
    L_INFO("Short URL server starting on :8889");
    server.Listen();

    return 0;
}
