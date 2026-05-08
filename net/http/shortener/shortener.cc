#include "shortener.hpp"
#include <atomic>
#include <chrono>

void ApiShorten(const HttpRequest& req, HttpResponse* rsp)
{
    std::string long_url = req.GetParam("url");
    L_INFO("ApiShorten called, url=%s", long_url.c_str());
    if (long_url.empty()) {
        rsp->SetStatu(400);
        rsp->SetContent("url is required", "text/plain");
        return;
    }

    MYSQL* conn = g_mysql->Acquire();
    if (!conn) {
        L_ERROR("Failed to acquire MySQL connection");
        rsp->SetStatu(500);
        rsp->SetContent("Database Error", "text/plain");
        return;
    }

    char escaped[8192];
    char sql[16384];
    mysql_real_escape_string(conn, escaped, long_url.c_str(), long_url.size());

    static std::atomic<long> seq{0};
    snprintf(sql, sizeof(sql),
        "INSERT INTO short_url (code, long_url) VALUES ('TMP_%d_%ld', '%s')",
        getpid(), ++seq, escaped);

    if (mysql_query(conn, sql) != 0) {
        L_ERROR("MySQL INSERT error: %s", mysql_error(conn));
        g_mysql->Release(conn);
        rsp->SetStatu(500);
        rsp->SetContent("Database Error", "text/plain");
        return;
    }

    uint64_t id = mysql_insert_id(conn);
    std::string code = Base62::Encode(id);
    L_INFO("INSERT success, id=%lu code=%s", id, code.c_str());

    mysql_real_escape_string(conn, escaped, code.c_str(), code.size());
    snprintf(sql, sizeof(sql),
        "UPDATE short_url SET code='%s' WHERE id=%lu", escaped, id);

    if (mysql_query(conn, sql) != 0) {
        L_WARN("UPDATE code failed for id=%lu, error=%s", id, mysql_error(conn));
    }

    g_mysql->Release(conn);

    if (!g_redis->Set(code, long_url)) {
        L_WARN("Redis SET failed, code=%s", code.c_str());
    }

    Json::Value resp;
    resp["code"]      = code;
    resp["short_url"] = g_base_url + code;
    resp["long_url"]  = long_url;

    L_INFO("ApiShorten done, code=%s", code.c_str());
    rsp->SetContent(resp.toStyledString(), "application/json");
}

void Redirect(const HttpRequest& req, HttpResponse* rsp)
{
    std::string code     = req._matches[1].str();
    std::string long_url = g_redis->Get(code);

    L_DEBUG("Redirect code=%s, redis_hit=%d", code.c_str(), !long_url.empty());

    if (long_url.empty()) {
        L_DEBUG("Redis miss, querying MySQL for code=%s", code.c_str());
        MYSQL* conn = g_mysql->Acquire();
        if (!conn) {
            L_ERROR("Redirect: Failed to acquire MySQL connection");
            rsp->SetStatu(500);
            rsp->SetContent("Database Error", "text/plain");
            return;
        }

        char escaped[32];
        mysql_real_escape_string(conn, escaped, code.c_str(), code.size());

        char sql[512];
        snprintf(sql, sizeof(sql),
            "SELECT long_url FROM short_url WHERE code='%s' LIMIT 1", escaped);

        if (mysql_query(conn, sql) != 0) {
            L_ERROR("MySQL SELECT error: %s", mysql_error(conn));
        } else {
            MYSQL_RES* result = mysql_store_result(conn);
            if (result) {
                MYSQL_ROW row = mysql_fetch_row(result);
                if (row && row[0]) {
                    long_url = row[0];
                    g_redis->Set(code, long_url);
                    L_DEBUG("MySQL found, code=%s url=%s", code.c_str(), long_url.c_str());
                } else {
                    L_DEBUG("MySQL not found, code=%s", code.c_str());
                }
                mysql_free_result(result);
            }
        }
        g_mysql->Release(conn);
    }

    if (long_url.empty()) {
        rsp->SetStatu(404);
        rsp->SetContent("<h1>404 Not Found</h1>", "text/html");
    } else {
        rsp->SetRedirect(long_url, 302);
    }
}
