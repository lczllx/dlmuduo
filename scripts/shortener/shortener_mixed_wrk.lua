-- 模拟真实访问分布：热点集中 + 长尾稀疏 + 缓存穿透
-- 80% 请求命中 Redis（热点）
-- 15% 请求回源 MySQL（长尾/过期）
--  5% 请求返回 404（无效短码/穿透）

hot = {}    -- 200 个热点短码，全部 Redis 命中
warm = {}   -- 800 个温短码，30% 被 Redis 删除
miss = {}   -- 100 个无效短码，始终 404

h_cnt, w_cnt, m_cnt = 0, 0, 0

function init(args)
    local f = io.open("short_codes.txt", "r")
    if not f then
        print("ERROR: short_codes.txt not found")
        return
    end
    local all = {}
    for line in f:lines() do all[#all + 1] = line end
    f:close()

    local n = #all
    -- 前 200 条作为热点
    for i = 1, 200 do hot[i] = all[i] end
    h_cnt = 200

    -- 第 201~1000 条作为温数据
    for i = 201, math.min(1000, n) do warm[#warm + 1] = all[i] end
    w_cnt = #warm

    -- 无效短码：不可能存在的 code（带特殊字符或太长）
    miss = {"NOTEXIST00", "INVALID0001", "XXXXXXXXX0", "RANDOMCODE", "4040000000",
            "ABCDE00000", "ZZZZZZZZZZ", "DEADBEEF00", "NOPENOPENO", "FAKE000000",
            "GHOST00000", "MISSING000", "UNKNOWN000", "BOGUS00000", "WRONG00000"}
    m_cnt = #miss

    -- 从 Redis 删除 30% warm codes 模拟缓存过期
    local to_del = math.floor(w_cnt * 0.3)
    for i = 1, to_del do
        os.execute("redis-cli -h 127.0.0.1 DEL " .. warm[i] .. " > /dev/null 2>&1")
    end

    local total = h_cnt + w_cnt + m_cnt
    print(string.format("Loaded: hot=%d warm=%d(30%%del) miss=%d total=%d", h_cnt, w_cnt, m_cnt, total))
end

request = function()
    local r = math.random(1, 100)
    local code
    if r <= 80 then
        code = hot[math.random(h_cnt)]           -- 80% 热点，Redis 命中
    elseif r <= 95 then
        code = warm[math.random(w_cnt)]           -- 15% 温数据，部分回源 MySQL
    else
        code = miss[math.random(m_cnt)]           --  5% 无效，404
    end
    return wrk.format("GET", "/" .. code)
end
