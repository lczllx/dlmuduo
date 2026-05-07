#!/usr/bin/env python3
"""Step 1: 直写 MySQL 10 万条短链（不写 Redis），模拟线上存量数据
用法:
  python3 step1_seed_data.py 100000 | docker exec -i mysql-short mysql -uroot -pshortener shortener
输出:
  all_codes.txt  (100K 短码，供 step2/step3 使用)
"""

import sys
import random
import subprocess

ALPHABET = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz"

def base62_encode(n):
    if n == 0:
        return "0"
    chars = []
    while n > 0:
        chars.append(ALPHABET[n % 62])
        n //= 62
    return ''.join(reversed(chars))

def gen_long_url(i):
    domains = ["example.com", "github.com", "stackoverflow.com", "reddit.com",
               "medium.com", "docs.python.org", "en.wikipedia.org", "youtube.com"]
    paths  = ["page", "article", "post", "question", "video", "doc", "wiki", "user"]
    d = domains[i % len(domains)]
    p = paths[(i * 7) % len(paths)]
    return f"https://{d}/{p}/{i}?t={random.randint(10000, 99999)}"

def main():
    count = int(sys.argv[1]) if len(sys.argv) > 1 else 100000

    try:
        out = subprocess.check_output(
            ['docker', 'exec', 'mysql-short', 'mysql', '-uroot', '-pshortener',
             'shortener', '-sN', '-e', "SELECT COALESCE(MAX(id),0) + 1 FROM short_url"],
            stderr=subprocess.DEVNULL).decode().strip()
        start_id = max(int(out), 100000)
    except Exception:
        start_id = 100000

    sys.stderr.write(f"Start ID: {start_id}, count: {count}\n")
    sys.stderr.flush()
    sys.stdout.write("SET autocommit=0;\n")

    all_codes = []
    batch_size = 2000
    batch = []

    for i in range(count):
        uid = start_id + i
        code = base62_encode(uid)
        url = gen_long_url(i)
        batch.append((uid, code, url))
        all_codes.append(code)

        if len(batch) >= batch_size or i == count - 1:
            lines = [f"({uid},'{code}','{url}')" for uid, code, url in batch]
            sys.stdout.write("INSERT INTO short_url (id,code,long_url) VALUES ")
            sys.stdout.write(",".join(lines))
            sys.stdout.write(";\n")
            batch.clear()

        if (i + 1) % 20000 == 0:
            print(f"  {i+1}/{count}", file=sys.stderr)

    sys.stdout.write("COMMIT;\n")

    with open("all_codes.txt", "w") as f:
        for c in all_codes:
            f.write(c + "\n")

    print(f"Done: {len(all_codes)} codes → all_codes.txt", file=sys.stderr)

if __name__ == "__main__":
    main()
