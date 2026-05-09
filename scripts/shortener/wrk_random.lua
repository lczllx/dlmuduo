-- wrk Lua script: random short code redirect
-- Usage: wrk -t4 -c200 -d30s -s wrk_random.lua http://localhost:8889/

math.randomseed(os.time())
math.random() math.random() math.random()

local codes = {}
local count = 0

function init(args)
    local f = io.open("scripts/shortener/codes.txt", "r")
    if not f then
        print("ERROR: cannot open codes.txt")
        return
    end
    for line in f:lines() do
        if line ~= "" then
            count = count + 1
            codes[count] = line
        end
    end
    f:close()
    print("Loaded " .. count .. " short codes")
end

function request()
    local idx = math.random(1, count)
    return wrk.format("GET", "/" .. codes[idx])
end

function done(summary, latency, requests)
    io.write("------------------------------\n")
    io.write(string.format("Requests:  %d\n", summary.requests))
    io.write(string.format("Duration:  %.2fs\n", summary.duration / 1000000))
    io.write(string.format("QPS:       %.1f\n", summary.requests / (summary.duration / 1000000)))
    io.write(string.format("Errors:    %d\n", summary.errors.status + summary.errors.timeout + summary.errors.connect + summary.errors.read + summary.errors.write))
    io.write(string.format("P50:       %.2fms\n", latency:percentile(50) / 1000))
    io.write(string.format("P99:       %.2fms\n", latency:percentile(99) / 1000))
    io.write(string.format("P999:      %.2fms\n", latency:percentile(99.9) / 1000))
    io.write("------------------------------\n")
end
