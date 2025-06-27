#!/usr/bin/env python3
import sys

body = "Hello, world! 🌍\n"
body_bytes = body.encode("utf-8")
length = len(body_bytes)

sys.stdout.write("Content-Type: text/plain; charset=utf-8\r\n")
sys.stdout.write(f"Content-Length: {length}\r\n")
sys.stdout.write("\r\n")
sys.stdout.write(body)
sys.stdout.flush()
