# test_unresponsive.py

import socket
import time

sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
sock.connect(('localhost', 8080))

sock.sendall(b"GET / HTTP/1.1\r\nHost: localhost\r\n")
sock.shutdown(socket.SHUT_RD)
time.sleep(30)

