#ifndef SHORTENER_HPP
#define SHORTENER_HPP

#include "redis_pool.hpp"
#include "mysql_pool.hpp"
#include "../http.hpp"
#include "base62.hpp"
#include <json/json.h>

extern std::unique_ptr<Redis_Client> g_redis;
extern std::unique_ptr<Mysql_Pool>   g_mysql;
extern std::string g_base_url;

void ApiShorten(const HttpRequest& req, HttpResponse* rsp);
void Redirect(const HttpRequest& req, HttpResponse* rsp);

#endif
