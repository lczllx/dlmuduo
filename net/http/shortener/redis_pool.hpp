#include <hiredis/hiredis.h>
#include <iostream>
#include <string>
#include <queue>
#include <mutex>
#include <condition_variable>

class Redis_Client
{
public:
    using ptr = std::unique_ptr<Redis_Client>;
    Redis_Client(int default_ttl = 604800 /*7day*/)
        : _default_ttl_sec(default_ttl) {}
    ~Redis_Client()
    {
        std::lock_guard<std::mutex> lock(_mutex);
        while (!_conns.empty()) {
            redisFree(_conns.front());
            _conns.pop();
        }
    }

    bool Connect(int pool_size = 4)
    {
        for (int i = 0; i < pool_size; ++i) {
            redisContext *c = redisConnect("127.0.0.1", 6379);
            if (!c) return false;
            struct timeval tv = {0, 50000};
            redisSetTimeout(c, tv);
            _conns.push(c);
        }
        return true;
    }

    std::string Get(const std::string &key)
    {
        redisContext *c = Acquire();
        if (!c) return "";
        redisReply *reply = (redisReply *)redisCommand(c, "GET %s", key.c_str());
        std::string value;
        if (reply && reply->type == REDIS_REPLY_STRING)
            value.assign(reply->str);
        freeReplyObject(reply);
        Release(c);
        return value;
    }

    bool Set(const std::string &key, const std::string &val, int ttl_sec = -1)
    {
        if (ttl_sec <= 0) ttl_sec = _default_ttl_sec;
        redisContext *c = Acquire();
        if (!c) return false;
        redisReply *reply = (redisReply *)redisCommand(c, "SETEX %s %d %s",
            key.c_str(), ttl_sec, val.c_str());
        bool success = reply && reply->type == REDIS_REPLY_STATUS;
        freeReplyObject(reply);
        Release(c);
        return success;
    }

    bool expire(const std::string &key, int ttl_sec)
    {
        redisContext *c = Acquire();
        if (!c) return false;
        redisReply *reply = (redisReply *)redisCommand(c, "EXPIRE %s %d",
            key.c_str(), ttl_sec);
        bool success = reply && reply->type == REDIS_REPLY_INTEGER && reply->integer == 1;
        freeReplyObject(reply);
        Release(c);
        return success;
    }

private:
    std::mutex _mutex;
    std::condition_variable _cond;
    std::queue<redisContext *> _conns;
    int _default_ttl_sec;

    redisContext *Acquire()
    {
        std::unique_lock<std::mutex> lock(_mutex);
        _cond.wait(lock, [this] { return !_conns.empty(); });
        redisContext *c = _conns.front();
        _conns.pop();
        return c;
        // 锁在此释放，Redis I/O 在锁外执行
    }

    void Release(redisContext *c)
    {
        std::lock_guard<std::mutex> lock(_mutex);
        _conns.push(c);
        _cond.notify_one();
    }
};