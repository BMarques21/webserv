#!/usr/bin/env python3
import os
import sys

print("Content-Type: text/plain\r")
print("\r")
print(f"Working directory: {os.getcwd()}")
print(f"Script path: {sys.argv[0]}")
print(f"SCRIPT_FILENAME: {os.environ.get('SCRIPT_FILENAME', 'N/A')}")
print(f"PATH_INFO: {os.environ.get('PATH_INFO', 'N/A')}")
print("CGI dir test OK")
