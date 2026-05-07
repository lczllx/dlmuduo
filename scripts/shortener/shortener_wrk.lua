-- wrk Lua script: random short URL redirect benchmark
-- Usage: wrk -t4 -c200 -d30s --latency -s shortener_wrk.lua http://localhost:8889/
-- Data file short_codes.txt must exist in the same directory (generate with shortener_gen_urls.py)

codes = {}
count = 0

function init(args)
    local f = io.open("short_codes.txt", "r")
    if not f then
        -- try absolute path alongside this script
        f = io.open(arg[0]:match("(.*/)") .. "short_codes.txt", "r")
    end
    if not f then
        print("ERROR: short_codes.txt not found")
        return
    end
    for line in f:lines() do
        codes[#codes + 1] = line
    end
    f:close()
    count = #codes
    print("Loaded " .. count .. " short codes")
end

request = function()
    local code = codes[math.random(count)]
    return wrk.format("GET", "/" .. code)
end
