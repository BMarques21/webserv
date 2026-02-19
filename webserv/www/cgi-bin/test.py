#!/usr/bin/env python3
import os, sys
sys.stdout.write("Content-Type: text/html\r\n\r\n")
sys.stdout.write("<html><body>")
sys.stdout.write("<p>METHOD:" + os.environ.get("REQUEST_METHOD","NONE") + "</p>")
sys.stdout.write("</body></html>\n")
sys.stdout.flush()
