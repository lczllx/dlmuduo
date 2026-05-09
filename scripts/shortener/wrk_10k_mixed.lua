math.randomseed(os.time())
math.random() math.random() math.random()

local codes = {}
local count = 0

function init(args)
    local f = io.open("scripts/shortener/codes_10k_mixed.txt", "r")
    if not f then
        print("ERROR: cannot open file")
        return
    end
    for line in f:lines() do
        if line ~= "" then
            count = count + 1
            codes[count] = line
        end
    end
    f:close()
    print("Loaded " .. count .. " codes (~20% cold, async MySQL backfill)")
end

function request()
    return wrk.format("GET", "/" .. codes[math.random(1, count)])
end

function done(summary, latency, requests)
    io.write("------------------------------\n")
    io.write(string.format("Requests:  %d\n", summary.requests))
    io.write(string.format("Duration:  %.2fs\n", summary.duration / 1000000))
    io.write(string.format("QPS:       %.1f\n", summary.requests / (summary.duration / 1000000)))
    local non_ok = summary.errors.status + summary.errors.timeout + summary.errors.connect + summary.errors.read + summary.errors.write
    io.write(string.format("Non-2xx/3xx: %d\n", non_ok))
    io.write(string.format("P50:       %.2fms\n", latency:percentile(50) / 1000))
    io.write(string.format("P99:       %.2fms\n", latency:percentile(99) / 1000))
    io.write(string.format("P999:      %.2fms\n", latency:percentile(99.9) / 1000))
    io.write("------------------------------\n")
end
