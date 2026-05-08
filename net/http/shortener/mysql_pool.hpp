#include <mysql/mysql.h>
#include <iostream>
#include <string>
#include <queue>
#include <unordered_map>
#include <mutex>
#include <condition_variable>
// - `Init()`：预创建 N 个连接，任一失败返回 false
// - `Acquire()`：阻塞等待直到有可用连接（用 `std::mutex` + `std::condition_variable`）
// - `Release(MYSQL* conn)`：归还连接
// - 析构函数：遍历关闭所有连接，`mysql_close`
class Mysql_Pool
{
public:
    using ptr = std::unique_ptr<Mysql_Pool>;

    Mysql_Pool(const std::string &host, const std::string &user,
               const std::string &password, const std::string &db, int port, int pool_size)
        : _m_host(host), _m_user(user), _m_password(password), _m_db(db), _m_port(port),
          _m_pool_size(pool_size) {}
    ~Mysql_Pool()
    {
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
                mysql_close(conn);
                return false; // 连接失败
            }
            mysql_set_character_set(conn, "utf8mb4");
            _mysqls.push(conn);
        }
        return true;
    }
    // 获取数据库连接
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
};