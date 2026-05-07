#!/usr/bin/env python3
"""Batch create short URLs for wrk benchmark.
Usage: python3 shortener_gen_urls.py [--count N] [--host HOST:PORT] [--output FILE]
"""
import argparse
import json
import sys
import urllib.request

def main():
    parser = argparse.ArgumentParser(description="Generate short URLs for benchmark")
    parser.add_argument("--count", type=int, default=10000, help="Number of URLs to create")
    parser.add_argument("--host", default="http://localhost:8889", help="Server host:port")
    parser.add_argument("--output", default="short_codes.txt", help="Output file for short codes")
    args = parser.parse_args()

    codes = []
    for i in range(1, args.count + 1):
        long_url = f"https://example.com/page/{i}?ts={i}"
        enc = urllib.request.quote(long_url, safe='')
        url = f"{args.host}/api/shorten?url={enc}"
        req = urllib.request.Request(url, method="POST")
        try:
            resp = urllib.request.urlopen(req, timeout=2)
            body = json.loads(resp.read())
            codes.append(body['code'])
        except Exception as e:
            print(f"ERR {i}: {e}", file=sys.stderr)
        if i % 2000 == 0:
            print(f"Progress: {i}/{args.count}")

    with open(args.output, "w") as f:
        for c in codes:
            f.write(c + "\n")
    print(f"Done: {len(codes)} codes written to {args.output}")

if __name__ == "__main__":
    main()
