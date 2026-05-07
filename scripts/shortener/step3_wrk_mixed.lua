-- Step 3: wrk 混合压测 — 冷热两个独立池
-- 热池: warm_codes.txt (在 Redis) — 92% 请求
-- 冷池: cold_codes.txt (仅 MySQL) — 8% 请求
-- init() 预计算全部路径，request() 只做 O(1) 循环取值，消除 math.random 瓶颈

paths = {}
p_cnt = 0
idx = 0

function init(args)
    local hot = {}
    local cold = {}
    local f = io.open("warm_codes.txt", "r")
    if not f then
        print("ERROR: warm_codes.txt not found. Run step2 first.")
        return
    end
    for line in f:lines() do hot[#hot + 1] = "/" .. line end
    f:close()

    f = io.open("cold_codes.txt", "r")
    if not f then
        print("ERROR: cold_codes.txt not found. Run step2 first.")
        return
    end
    for line in f:lines() do cold[#cold + 1] = "/" .. line end
    f:close()

    -- 92:8 交错填入，保证命中率稳定
    local hi, ci = 1, 1
    while hi <= #hot or ci <= #cold do
        for i = 1, 92 do
            if hi <= #hot then
                paths[#paths + 1] = hot[hi]
                hi = hi + 1
            end
        end
        for i = 1, 8 do
            if ci <= #cold then
                paths[#paths + 1] = cold[ci]
                ci = ci + 1
            end
        end
    end

    p_cnt = #paths
    print(string.format("Loaded: %d paths (%d hot + %d cold) expect-hit=%.0f%%",
        p_cnt, #hot, #cold, #hot * 100 / (#hot + #cold)))
end

request = function()
    idx = idx % p_cnt + 1
    return wrk.format("GET", paths[idx])
end
