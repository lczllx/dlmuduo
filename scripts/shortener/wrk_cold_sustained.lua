-- Sustained cold path: generate random codes that don't exist
-- All requests → Redis miss → async MySQL → 404 (no cache backfill for negatives)
math.randomseed(os.time())
math.random() math.random() math.random()

local chars = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz"
local nchars = #chars

function init(args)
    print("Sustained cold path: all random non-existent codes")
end

function request()
    -- Generate a random 6-char code (won't match real base62 pattern but any
    -- non-existent code triggers the full async MySQL path)
    local code = ""
    for i = 1, 6 do
        code = code .. chars:sub(math.random(1, nchars), math.random(1, nchars))
    end
    return wrk.format("GET", "/Z" .. code)
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
