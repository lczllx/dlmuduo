#include "shortener.hpp"
#include <atomic>
#include <chrono>
#include <cstdint>

void ApiShorten(const HttpRequest& req, HttpResponse* rsp)
{
    std::string long_url = req.GetParam("url");
    LCZ_INFO("ApiShorten called, url=%s", long_url.c_str());
    if (long_url.empty()) {
        rsp->SetStatu(400);
        rsp->SetContent("url is required", "text/plain");
        return;
    }

    MYSQL* conn = g_mysql->Acquire();
    if (!conn) {
        LCZ_ERROR("Failed to acquire MySQL connection");
        rsp->SetStatu(500);
        rsp->SetContent("Database Error", "text/plain");
        return;
    }

    char escaped[8192];
    char sql[16384];
    mysql_real_escape_string(conn, escaped, long_url.c_str(), long_url.size());

    // 两步写入：先 INSERT 占位拿到自增 ID，用 ID 生成 short code，再 UPDATE 回写
    // TMP_ 前缀：占位值，避免 code 列唯一约束冲突；用 pid+seq 保证不同进程不碰撞
    static std::atomic<int64_t> seq{0};
    snprintf(sql, sizeof(sql),
        "INSERT INTO short_url (code, long_url) VALUES ('TMP_%d_%ld', '%s')",
        getpid(), ++seq, escaped);

    if (mysql_query(conn, sql) != 0) {
        LCZ_ERROR("MySQL INSERT error: %s", mysql_error(conn));
        g_mysql->Release(conn);
        rsp->SetStatu(500);
        rsp->SetContent("Database Error", "text/plain");
        return;
    }

    uint64_t id = mysql_insert_id(conn);
    std::string code = Base62::Encode(id);  // 用自增 ID 做 Base62 编码得到短码
    LCZ_INFO("INSERT success, id=%lu code=%s", id, code.c_str());

    mysql_real_escape_string(conn, escaped, code.c_str(), code.size());
    snprintf(sql, sizeof(sql),
        "UPDATE short_url SET code='%s' WHERE id=%lu", escaped, id);

    if (mysql_query(conn, sql) != 0) {
        LCZ_WARN("UPDATE code failed for id=%lu, error=%s", id, mysql_error(conn));
    }

    g_mysql->Release(conn);

    if (!g_redis->Set(code, long_url)) {
        LCZ_WARN("Redis SET failed, code=%s", code.c_str());
    }

    Json::Value resp;
    resp["code"]      = code;
    resp["short_url"] = g_base_url + code;
    resp["long_url"]  = long_url;

    LCZ_INFO("ApiShorten done, code=%s", code.c_str());
    rsp->SetContent(resp.toStyledString(), "application/json");
}

void Redirect(const HttpRequest& req, HttpResponse* rsp)
{
    std::string code     = req._matches[1].str();
    std::string long_url = g_redis->Get(code);

    LCZ_DEBUG("Redirect code=%s, redis_hit=%d", code.c_str(), !long_url.empty());

    if (!long_url.empty()) {
        rsp->SetRedirect(long_url, 302);
        return;
    }

    // Redis未命中，走异步MySQL回源
    LCZ_DEBUG("Redis miss, async MySQL for code=%s", code.c_str());

    // short code 只含 [0-9A-Za-z]，不需要mysql_real_escape_string
    char sql[512];
    snprintf(sql, sizeof(sql),
        "SELECT long_url FROM short_url WHERE code='%s' LIMIT 1", code.c_str());

    // 跨线程异步流程（3 个线程参与）：
    // 1. I/O 线程：设 _deferred=true，持有 _conne，投递 SQL 任务到 MySQL worker
    // 2. MySQL worker 线程：执行查询，拿到结果后通过 RunInLoop 回调 I/O 线程
    // 3. I/O 线程：构造 HTTP 响应，Send 数据，ReSet 上下文
    rsp->_deferred = true;
    PtrConnection conne = rsp->_conne;  // shared_ptr 保证异步回调时连接仍存活
    bool is_close = req.IsClose();

    g_mysql->QueryAsync(sql, [conne, is_close, code = std::move(code)](MYSQL_RES* result) {
        std::string long_url;
        if (result) {
            MYSQL_ROW row = mysql_fetch_row(result);
            if (row && row[0]) {
                long_url = row[0];
                g_redis->Set(code, long_url);
                LCZ_DEBUG("MySQL found, code=%s url=%s", code.c_str(), long_url.c_str());
            } else {
                LCZ_DEBUG("MySQL not found, code=%s", code.c_str());
            }
        }
        // 回到I/O线程发送响应（RunInLoop 跨线程投递）
        conne->GetLoop()->RunInLoop([conne, long_url, is_close]() {
            HttpContext* ctx = conne->GetContext()->get<HttpContext>();
            HttpRequest& req = ctx->Request();
            // 构造HTTP响应
            std::string resp_str;
            resp_str.reserve(256);
            if (long_url.empty()) {
                std::string body = "<h1>404 Not Found</h1>";
                resp_str.append(req._version).append(" 404 Not Found\r\n");
                resp_str.append("Connection: ");
                resp_str.append(is_close ? "close" : "keep-alive");
                resp_str.append("\r\nContent-Length: ").append(std::to_string(body.size()));
                resp_str.append("\r\nContent-Type: text/html\r\n\r\n");
                resp_str.append(body);
            } else {
                resp_str.append(req._version).append(" 302 Found\r\n");
                resp_str.append("Location: ").append(long_url).append("\r\n");
                resp_str.append("Connection: ");
                resp_str.append(is_close ? "close" : "keep-alive");
                resp_str.append("\r\nContent-Length: 0\r\n\r\n");
            }
            conne->Send(resp_str.c_str(), resp_str.size());
            ctx->ReSet();
            if (is_close) conne->Shutdown();
        });
    });
}
