#!/bin/bash
echo "=== Testando CGI no webserv ==="
echo "1. Dir test:"
curl -s "http://127.0.0.1:8080/cgi-bin/dir_test.py" | grep "Working directory"
echo -e "\n2. Timeout test (aguarde 6s):"
timeout 6 curl "http://127.0.0.1:8080/cgi-bin/timeout_test.py" || echo "TIMEOUT OK (servidor matou)"
echo -e "\n3. Log servidor:"
tail -10 /tmp/webserv.log
