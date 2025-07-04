#!/usr/bin/env python3
import time
import sys
import random

print("Content-Type: text/plain; charset=utf-8")
print()
sys.stdout.flush()

# 0〜10の間でランダムに終了する
count_limit = random.randint(1, 10)
for i in range(1, count_limit + 1):
    print(f"ひつじが{i}ひき...")
    sys.stdout.flush()
    time.sleep(1)
print("zZZ...")
