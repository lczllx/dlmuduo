#include <mysql/mysql.h>
#include <string>
#include <log_system/lcz_log.h>
#include <queue>
#include <unordered_map>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <functional>
#include <atomic>

class Mysql_Pool
{
public:
    using ptr = std::unique_ptr<Mysql_Pool>;
    using AsyncCallback = std::function<void(MYSQL_RES*)>;

    Mysql_Pool(const std::string &host, const std::string &user,
               const std::string &password, const std::string &db, int port, int pool_size,
               int async_workers = 2)
        : _m_host(host), _m_user(user), _m_password(password), _m_db(db), _m_port(port),
          _m_pool_size(pool_size), _async_workers(async_workers), _async_stop(false) {}
    ~Mysql_Pool()
    {
        // 停止异步worker
        {
            std::lock_guard<std::mutex> lock(_async_mutex);
            _async_stop = true;
            _async_cond.notify_all();
        }
        for (auto &t : _async_threads) {
            if (t.joinable()) t.join();
        }
        // 关闭同步连接池
        std::lock_guard<std::mutex> lock(_mutex);
        while (_mysqls.size())
        {
            auto mysql = _mysqls.front();
            _mysqls.pop();
            mysql_close(mysql);
        }
    }
    // 预创建 N 个连接，任一失败返回 false
    bool Init()
    {
        for (int i = 1; i <= _m_pool_size; ++i)
        {
            MYSQL *conn = mysql_init(nullptr);
            if (conn == nullptr)
                return false;
            // 设置超时（必须在 connect 之前，所以放 Init 里）
            unsigned int timeout = 2;
            mysql_options(conn, MYSQL_OPT_CONNECT_TIMEOUT, &timeout);
            mysql_options(conn, MYSQL_OPT_READ_TIMEOUT, &timeout);

            // 建立 TCP 连接
            if (mysql_real_connect(conn, _m_host.c_str(), _m_user.c_str(),
                                   _m_password.c_str(), _m_db.c_str(), _m_port, nullptr, 0) == nullptr)
            {
                LCZ_ERROR("MySQL connect failed: %s (host=%s, port=%d)",
                       mysql_error(conn), _m_host.c_str(), _m_port);
                mysql_close(conn);
                return false; // 连接失败
            }
            mysql_set_character_set(conn, "utf8mb4");
            _mysqls.push(conn);
        }

        // 启动异步worker线程，每个持有自己的连接
        for (int i = 0; i < _async_workers; ++i) {
            MYSQL *conn = mysql_init(nullptr);
            if (!conn) return false;
            unsigned int timeout = 2;
            mysql_options(conn, MYSQL_OPT_CONNECT_TIMEOUT, &timeout);
            mysql_options(conn, MYSQL_OPT_READ_TIMEOUT, &timeout);
            if (mysql_real_connect(conn, _m_host.c_str(), _m_user.c_str(),
                                   _m_password.c_str(), _m_db.c_str(),
                                   _m_port, nullptr, 0) == nullptr) {
                LCZ_ERROR("Async MySQL connect failed: %s", mysql_error(conn));
                mysql_close(conn);
                return false;
            }
            mysql_set_character_set(conn, "utf8mb4");
            _async_threads.emplace_back(&Mysql_Pool::AsyncWorker, this, conn);
        }
        return true;
    }
    // 获取数据库连接（同步）
    MYSQL *Acquire()
    {
        MYSQL *mysql;
        {
            std::unique_lock<std::mutex> lock(_mutex);
            _m_cond_v.wait(lock, [this]
                           { return !_mysqls.empty(); });
            mysql = _mysqls.front();
            _mysqls.pop();
        }
        if (mysql_ping(mysql) != 0) {
            mysql_close(mysql);
            mysql = mysql_init(nullptr);
            if (mysql) {
                unsigned int timeout = 2;
                mysql_options(mysql, MYSQL_OPT_CONNECT_TIMEOUT, &timeout);
                mysql_options(mysql, MYSQL_OPT_READ_TIMEOUT, &timeout);
                if (mysql_real_connect(mysql, _m_host.c_str(), _m_user.c_str(),
                                       _m_password.c_str(), _m_db.c_str(),
                                       _m_port, nullptr, 0)) {
                    mysql_set_character_set(mysql, "utf8mb4");
                } else {
                    mysql_close(mysql);
                    mysql = nullptr;
                }
            }
        }
        return mysql;
    }
    // 释放连接池连接
    void Release(MYSQL *conn)
    {
        std::lock_guard<std::mutex> lock(_mutex);
        _mysqls.push(conn);
        _m_cond_v.notify_one(); // 唤醒一个等待
    }

    // 异步查询：sql在worker线程执行，callback在worker线程调用
    // 注意：cb 不在 I/O 线程执行，需要 RunInLoop 才能安全操作 Connection
    void QueryAsync(const std::string &sql, AsyncCallback cb)
    {
        {
            std::lock_guard<std::mutex> lock(_async_mutex);
            _async_queue.emplace(sql, std::move(cb));
        }
        _async_cond.notify_one();
    }

private:
    std::mutex _mutex;
    std::queue<MYSQL *> _mysqls; // 数据库连接池
    std::condition_variable _m_cond_v;

    // 连接参数
    std::string _m_host;     // 数据库主机地址
    std::string _m_user;     // 用户名
    std::string _m_password; // 密码
    std::string _m_db;       // 数据库名称
    int _m_port;             // mysql端口
    int _m_pool_size;        // 连接池大小

    // 异步查询
    int _async_workers;
    std::atomic<bool> _async_stop;
    std::mutex _async_mutex;
    std::condition_variable _async_cond;
    std::queue<std::pair<std::string, AsyncCallback>> _async_queue;
    std::vector<std::thread> _async_threads;

    void AsyncWorker(MYSQL *conn)
    {
        while (!_async_stop) {
            std::pair<std::string, AsyncCallback> task;
            {
                std::unique_lock<std::mutex> lock(_async_mutex);
                _async_cond.wait(lock, [this] {
                    return _async_stop || !_async_queue.empty();
                });
                if (_async_stop && _async_queue.empty()) break;
                task = std::move(_async_queue.front());
                _async_queue.pop();
            }
            const std::string &sql = task.first;
            MYSQL_RES *result = nullptr;
            // 心跳检测，连接断开则重连
            if (conn && mysql_ping(conn) != 0) {
                mysql_close(conn);
                conn = mysql_init(nullptr);
                if (conn) {
                    unsigned int timeout = 2;
                    mysql_options(conn, MYSQL_OPT_CONNECT_TIMEOUT, &timeout);
                    mysql_options(conn, MYSQL_OPT_READ_TIMEOUT, &timeout);
                    if (!mysql_real_connect(conn, _m_host.c_str(), _m_user.c_str(),
                                           _m_password.c_str(), _m_db.c_str(),
                                           _m_port, nullptr, 0)) {
                        mysql_close(conn);
                        conn = nullptr;
                    } else {
                        mysql_set_character_set(conn, "utf8mb4");
                    }
                }
            }
            if (conn && mysql_query(conn, sql.c_str()) == 0) {
                result = mysql_store_result(conn);
            } else if (!conn) {
                LCZ_ERROR("AsyncWorker: no valid MySQL connection");
            }
            task.second(result);//callback在worker线程执行
            if (result) mysql_free_result(result);
        }
        mysql_close(conn);
    }
};