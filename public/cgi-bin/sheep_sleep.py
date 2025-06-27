#!/usr/bin/env python3
import time
import sys
import random

print("Content-Type: text/plain; charset=utf-8")
print()
sys.stdout.flush()

# 0〜10の間でランダムに終了する
sleep_limit = random.randint(0, 10)

for i in range(1, 101):
    print(f"ひつじが{i}ひき...")
    sys.stdout.flush()
    time.sleep(1)
    if i == sleep_limit:
        print("zZZ...")
        sys.stdout.flush()
        break
