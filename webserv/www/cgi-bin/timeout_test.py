#!/usr/bin/env python3
import time
import sys

print("Content-Type: text/plain\r")
print("\r")
print("CGI timeout test STARTED - looping forever...")
sys.stdout.flush()

# Loop infinito para timeout
while True:
    time.sleep(0.1)
# executar: curl "http://127.0.0.1:8080/cgi-bin/timeout_test.py"