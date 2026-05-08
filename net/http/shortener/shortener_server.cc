#include "shortener.hpp"
#include "index_html.hpp"
#include <log_system/Logger.hpp>
#include <cstdlib>

std::unique_ptr<Redis_Client> g_redis;
std::unique_ptr<Mysql_Pool>   g_mysql;
std::string g_base_url = "http://localhost:8889/";

static const char* env(const char* key, const char* def) {
    const char* v = std::getenv(key);
    return v && v[0] ? v : def;
}

static int safe_stoi(const char* s, int def) {
    if (!s || !s[0]) return def;
    try { return std::stoi(s); }
    catch (...) { return def; }
}

static void HomePage(const HttpRequest&, HttpResponse* rsp) {
    rsp->SetContent(kIndexHtml, "text/html; charset=utf-8");
}

static void HealthCheck(const HttpRequest&, HttpResponse* rsp) {
    rsp->SetContent("OK", "text/plain");
}

int main()
{
    lcz::LoggerManager::getInstance().rootLogger()->setLevel(lcz::LogLevel::value::INFO);

    const char* mysql_host = env("MYSQL_HOST", "127.0.0.1");
    int mysql_port = safe_stoi(env("MYSQL_PORT", "3306"), 3306);
    const char* mysql_user = env("MYSQL_USER", "root");
    const char* mysql_pass = env("MYSQL_PASS", "shortener");
    const char* mysql_db   = env("MYSQL_DB", "shortener");
    int mysql_pool = safe_stoi(env("MYSQL_POOL", "4"), 4);

    g_mysql.reset(new Mysql_Pool(mysql_host, mysql_user, mysql_pass, mysql_db, mysql_port, mysql_pool));
    if (!g_mysql->Init()) {
        L_ERROR("MySQL init failed");
        return 1;
    }
    L_INFO("MySQL init OK");

    const char* redis_host = env("REDIS_HOST", "127.0.0.1");
    int redis_port = safe_stoi(env("REDIS_PORT", "6379"), 6379);

    g_redis.reset(new Redis_Client());
    if (!g_redis->Connect(redis_host, redis_port)) {
        L_ERROR("Redis connect failed");
        return 1;
    }
    L_INFO("Redis connect OK");

    const char* base_url = std::getenv("BASE_URL");
    if (base_url && base_url[0]) g_base_url = base_url;

    HttpServer server(8889, 10);
    server.SetThreadCnt(4);
    server.Get("/", HomePage);
    server.Get("/health", HealthCheck);
    server.Post("/api/shorten", ApiShorten);
    server.Get("/([A-Za-z0-9]+)", Redirect);
    L_INFO("Short URL server starting on :8889");
    server.Listen();

    return 0;
}
