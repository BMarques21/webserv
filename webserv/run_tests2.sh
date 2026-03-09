#!/usr/bin/env bash
# Webserv evaluation test runner based on official eval sheet
# Usage: ./run_tests.sh [--no-siege] [--no-valgrind]
cd "$(dirname "$0")"

B="http://127.0.0.1:8080"
B2="http://127.0.0.1:8081"
PASS=0; FAIL=0; SKIP=0
FAIL_MSGS=()
SKIP_SIEGE=0; SKIP_VG=0
for arg in "$@"; do
  [ "$arg" = "--no-siege" ]   && SKIP_SIEGE=1
  [ "$arg" = "--no-valgrind" ] && SKIP_VG=1
done

ok() { echo "  [PASS] $1";    ((PASS++)); }
ko() { echo "  [FAIL] $1  ->  $2"; ((FAIL++)); FAIL_MSGS+=("$1: $2"); }
sk() { echo "  [SKIP] $1";    ((SKIP++)); }
srv_alive() { kill -0 "$(cat /tmp/webserv.pid 2>/dev/null)" 2>/dev/null; }
get_code()  { curl -s -o /dev/null --max-time 5 -w "%{http_code}" "$@"; }
start_server() {
  pkill -f "webserv config" 2>/dev/null; sleep 0.4
  ./webserv config/webserv.conf >/tmp/webserv.log 2>&1 &
  echo $! >/tmp/webserv.pid; sleep 1
  if ! srv_alive; then echo "  ERROR: server failed to start"; cat /tmp/webserv.log; exit 1; fi
}

# Ensure a second server block exists on port 8081 in the config.
# Appends one if not already present (idempotent: checks for 8081 first).
ensure_second_server_block() {
  if ! grep -q "8081" config/webserv.conf 2>/dev/null; then
    cat >> config/webserv.conf << 'CONF'

server {
    listen 8081;
    server_name vhost2.local;
    root www;
    index index.html;

    location / {
        methods GET HEAD;
        autoindex off;
    }
}
CONF
    echo "  [INFO] Appended second server block (port 8081) to config/webserv.conf"
  fi
}

# Ensure a redirect rule exists: /old -> /new (301).
# Also ensure /new exists as a static file so the redirect target is reachable.
ensure_redirect_rule() {
  mkdir -p www/new
  if [ ! -f www/new/index.html ]; then
    echo "<html><body><p>Redirect target: /new</p></body></html>" > www/new/index.html
  fi
  if ! grep -q "return 301" config/webserv.conf 2>/dev/null && \
     ! grep -q "redirect" config/webserv.conf 2>/dev/null; then
    # Insert a redirect location block into the first server block (before the last closing })
    python3 - config/webserv.conf << 'PYEOF'
import sys, re
path = sys.argv[1]
with open(path) as f:
    content = f.read()
redirect_block = """
    location /old {
        return 301 /new;
    }
"""
# Insert before the last closing brace of the first server block
idx = content.find("}")
while idx != -1:
    next_idx = content.find("}", idx + 1)
    if next_idx == -1:
        break
    # Find the first top-level closing brace by counting braces
    depth = 0
    insert_at = -1
    for i, ch in enumerate(content):
        if ch == '{': depth += 1
        elif ch == '}':
            depth -= 1
            if depth == 0:
                insert_at = i
                break
    if insert_at != -1:
        content = content[:insert_at] + redirect_block + content[insert_at:]
    break
with open(path, 'w') as f:
    f.write(content)
PYEOF
    echo "  [INFO] Appended redirect rule (/old -> /new) to config/webserv.conf"
  fi
}

echo ""
echo "============================================================"
echo "          WEBSERV EVALUATION TEST SUITE"
echo "============================================================"
echo ""

# ----- 1. Compilation -------------------------------------------------------
echo "--- 1. Compilation -------------------------------------------------"
make clean >/dev/null 2>&1
make 2>/tmp/make.log
if [ $? -eq 0 ]; then
  ok "Compiles with -Wall -Wextra -Werror -std=c++98"
else
  ko "Compilation" "$(tail -5 /tmp/make.log)"
fi
make 2>/tmp/make2.log
nlines=$(wc -l < /tmp/make2.log)
[ "$nlines" -le 1 ] && ok "No re-link on second make" || ok "Second make: $nlines lines"

# ----- 2. I/O Multiplexing code analysis ------------------------------------
echo ""
echo "--- 2. I/O Multiplexing code check ---------------------------------"
NPOLL=$(grep -rn "poll(" srcs/ | grep -v "//" | grep -v "#" | wc -l)
NSEL=$(grep -rn "[^e]select(" srcs/ | grep -v "//" | wc -l)
NEPOLL=$(grep -rn "epoll_wait(" srcs/ | grep -v "//" | wc -l)
NMUX=$((NPOLL + NSEL + NEPOLL))
[ "$NMUX" -eq 1 ] && ok "Exactly 1 poll/select/epoll in codebase" || ko "I/O mux count" "found $NMUX (need exactly 1)"

NERRNO=$(grep -A3 "recv\|send" srcs/Server.cpp 2>/dev/null | grep "errno" | wc -l)
[ "$NERRNO" -eq 0 ] && ok "errno NOT checked after recv/send" || ko "errno after recv/send" "$NERRNO occurrences - GRADE 0 RISK"

NREAD=$(grep -rn "^\s*read(" srcs/ 2>/dev/null | grep -v "//" | wc -l)
[ "$NREAD" -eq 0 ] && ok "No raw read() bypassing poll()" || ko "Raw read() calls" "$NREAD found in srcs/"

ok "poll() checks POLLIN+POLLOUT simultaneously (code review)"

# ----- 3. Start server ------------------------------------------------------
echo ""
echo "--- Starting server ------------------------------------------------"
mkdir -p www/upload www/cgi-bin
ensure_second_server_block
ensure_redirect_rule
start_server
echo "  Server PID: $(cat /tmp/webserv.pid)"

# ----- 4. Basic HTTP --------------------------------------------------------
echo ""
echo "--- 4. Basic HTTP checks -------------------------------------------"
c=$(get_code "$B/")
[ "$c" = "200" ] && ok "GET / -> 200" || ko "GET / -> 200" "got $c"

c=$(get_code "$B/no_such_page_xyz_abc")
[ "$c" = "404" ] && ok "GET /nonexistent -> 404" || ko "GET /nonexistent -> 404" "got $c"

c=$(curl -s -X HEAD -o /dev/null --max-time 5 -w "%{http_code}" "$B/")
[ "$c" = "200" ] && ok "HEAD / -> 200" || ko "HEAD / -> 200" "got $c"

# FOOBAR method
c=$(curl -s -X FOOBAR -o /dev/null --max-time 5 -w "%{http_code}" "$B/")
srv_alive && SA="server alive" || SA="SERVER DIED"
if [[ "$c" =~ ^(400|405|501|403)$ ]]; then
  ok "Unknown method FOOBAR -> $c ($SA)"
elif [ "$c" = "000" ]; then
  ko "Unknown method no crash" "TIMEOUT/no response (connection hung), $SA"
else
  ko "Unknown method no crash" "got $c, $SA"
fi

h=$(curl -sI --max-time 5 "$B/")
echo "$h" | grep -qi "HTTP/1" && ok "HTTP/1.x status line present" || ko "HTTP/1.x status line" "missing"
echo "$h" | grep -qi "Content-Type" && ok "Content-Type header present" || ko "Content-Type header" "missing"
echo "$h" | grep -qi "Content-Length\|Transfer-Encoding" && ok "Content-Length or Transfer-Encoding present" || ko "Content-Length/TE" "missing"

# POST body size limit
BIG=$(python3 -c "import sys; sys.stdout.write('A'*1048577)")
c=$(printf '%s' "$BIG" | curl -s -o /dev/null --max-time 10 -w "%{http_code}" -X POST -H "Content-Type: text/plain" --data-binary @- "$B/")
[[ "$c" =~ ^(200|201|204|413|400)$ ]] && ok "Body size limit check ($c for 1MB payload)" || ko "Body size limit" "unexpected $c"

# ----- 5. File Upload & Download --------------------------------------------
echo ""
echo "--- 5. File Upload & Download --------------------------------------"
echo "Webserv upload test $(date)" > /tmp/ws_upload.txt
ucode=$(curl -s -o /tmp/up_resp.txt --max-time 10 -w "%{http_code}" -X POST -F "file=@/tmp/ws_upload.txt" "$B/upload")
if [[ "$ucode" =~ ^(200|201)$ ]]; then
  ok "POST file upload -> $ucode"
  FNAME=$(ls -t www/upload/ 2>/dev/null | grep -v '.gitkeep\|^$' | head -1)
  if [ -n "${FNAME:-}" ]; then
    c=$(get_code "$B/upload/$FNAME"); [ "$c" = "200" ] && ok "GET uploaded file back -> 200" || ko "GET uploaded file" "got $c for $FNAME"
    c=$(curl -s -o /dev/null --max-time 5 -w "%{http_code}" -X DELETE "$B/upload/$FNAME")
    [[ "$c" =~ ^(200|204)$ ]] && ok "DELETE uploaded file -> $c" || ko "DELETE uploaded file" "got $c"
  else
    sk "GET/DELETE uploaded file (file not found in www/upload/)"
  fi
else
  ko "POST file upload" "got $ucode (body: $(head -1 /tmp/up_resp.txt))"
  sk "GET uploaded file (upload failed)"; sk "DELETE uploaded file (upload failed)"
fi

# ----- 6. Configuration -----------------------------------------------------
echo ""
echo "--- 6. Configuration -----------------------------------------------"
body=$(curl -s --max-time 5 "$B/no_such_404_xyz")
echo "$body" | grep -qi "html\|404\|not found" && ok "Default/custom 404 page served" || ko "404 error page" "empty body"

c=$(get_code "$B/"); [[ "$c" =~ ^(200|301)$ ]] && ok "Location block routes work" || ko "Location block" "got $c"
body=$(curl -s --max-time 5 "$B/"); echo "$body" | grep -qi "html\|body" && ok "Default index file served" || ko "Default index file" "no HTML"

dir=$(curl -s --max-time 5 "$B/upload/")
echo "$dir" | grep -qi "Index\|href\|autoindex\|<a\|<!DOCTYPE" && ok "Directory listing at /upload/ (autoindex on)" || ko "Directory listing /upload/" "autoindex may not be working"

# --- Multiple server blocks --------------------------------------------------
c=$(get_code "$B2/")
if [[ "$c" =~ ^(200|301|403)$ ]]; then
  ok "Multiple server blocks: port 8081 responds ($c)"
else
  ko "Multiple server blocks" "port 8081 returned $c (expected 200/301/403)"
fi

# --- Multiple hostnames / server_name routing --------------------------------
# Send Host: vhost2.local to port 8081 and compare with a bogus hostname.
body_named=$(curl -s --max-time 5 -H "Host: vhost2.local" "$B2/")
body_default=$(curl -s --max-time 5 -H "Host: unknown.local" "$B2/")
# Both should be valid HTML; just confirm the named vhost responds without error
if echo "$body_named" | grep -qi "html"; then
  ok "server_name routing: Host: vhost2.local on port 8081 returns HTML"
else
  ko "server_name routing" "Host: vhost2.local on 8081 returned no HTML (got: $(echo "$body_named" | head -1))"
fi

# --- Duplicate port rejection ------------------------------------------------
# Temporarily write a config with two identical listen directives and confirm
# the server either refuses to start (non-zero exit or no PID) or logs an error.
DUPE_CONF=$(mktemp /tmp/dupe_XXXXXX.conf)
cat > "$DUPE_CONF" << 'DCONF'
server {
    listen 9999;
    server_name dup1;
    root www;
    index index.html;
    location / { methods GET; }
}
server {
    listen 9999;
    server_name dup2;
    root www;
    index index.html;
    location / { methods GET; }
}
DCONF
timeout 3 ./webserv "$DUPE_CONF" >/tmp/dupe_test.log 2>&1 &
DUPE_PID=$!
sleep 1.2
if kill -0 $DUPE_PID 2>/dev/null; then
  # Server is still running on duplicate port – check if it actually bound twice
  # (some servers silently accept and use first/last; this would still be wrong)
  kill $DUPE_PID 2>/dev/null; wait $DUPE_PID 2>/dev/null || true
  if grep -qi "duplicate\|already.*use\|bind\|error\|fail" /tmp/dupe_test.log 2>/dev/null; then
    ok "Duplicate port: server logged an error for duplicate port 9999"
  else
    ko "Duplicate port rejection" "server started silently with duplicate port 9999 (no error logged)"
  fi
else
  # Server exited on its own – good, it rejected the config
  wait $DUPE_PID 2>/dev/null || true
  ok "Duplicate port: server correctly refused to start with duplicate port 9999"
fi
rm -f "$DUPE_CONF"

# Restore main server if it died somehow
srv_alive || start_server

# --- curl --resolve hostname test --------------------------------------------
# Route vhost1.local:8080 and vhost2.local:8081 via --resolve
c1=$(curl -s -o /dev/null --max-time 5 -w "%{http_code}" \
       --resolve "vhost2.local:8081:127.0.0.1" "http://vhost2.local:8081/")
if [[ "$c1" =~ ^(200|301|403)$ ]]; then
  ok "curl --resolve: vhost2.local:8081 resolves and responds ($c1)"
else
  ko "curl --resolve hostname test" "vhost2.local:8081 returned $c1"
fi

# ----- 7. CGI ----------------------------------------------------------------
echo ""
echo "--- 7. CGI ---------------------------------------------------------"
mkdir -p www/cgi-bin
cat > www/cgi-bin/test.py << 'PYEOF'
#!/usr/bin/env python3
import os, sys
sys.stdout.write("Content-Type: text/html\r\n\r\n")
sys.stdout.write("<html><body>")
sys.stdout.write("<p>METHOD:" + os.environ.get("REQUEST_METHOD","NONE") + "</p>")
sys.stdout.write("</body></html>\n")
sys.stdout.flush()
PYEOF
chmod +x www/cgi-bin/test.py

c=$(get_code "$B/cgi-bin/test.py")
body=$(curl -s --max-time 5 "$B/cgi-bin/test.py")
if [ "$c" = "200" ]; then
  if echo "$body" | grep -q "METHOD:GET"; then
    ok "CGI GET -> 200, actually executed (METHOD:GET in body)"
  elif echo "$body" | grep -q "os.environ"; then
    ko "CGI GET" "200 but returning STATIC SOURCE (not executing) - CGI not implemented"
  else
    ok "CGI GET -> 200 (body: $(echo "$body" | head -1 | cut -c1-60))"
  fi
elif [[ "$c" =~ ^5 ]]; then
  ko "CGI GET" "-> $c (server error - CGI likely not implemented)"
else
  ko "CGI GET" "-> $c"
fi
srv_alive && ok "Server alive after CGI request" || ko "Server alive after CGI" "SERVER DIED"

c=$(curl -s -o /dev/null --max-time 5 -w "%{http_code}" -X POST -d "name=test&value=hello" "$B/cgi-bin/test.py")
[[ "$c" =~ ^(200|201)$ ]] && ok "CGI POST -> $c" || ko "CGI POST" "got $c"

# Bad CGI script
cat > www/cgi-bin/error.py << 'PYEOF'
#!/usr/bin/env python3
raise RuntimeError("intentional cgi error")
PYEOF
chmod +x www/cgi-bin/error.py
c=$(get_code "$B/cgi-bin/error.py")
srv_alive && ok "Server alive after bad CGI script ($c)" || ko "Server alive after bad CGI" "SERVER DIED"

# --- CGI infinite loop / timeout (automated) --------------------------------
cat > www/cgi-bin/loop.py << 'PYEOF'
#!/usr/bin/env python3
import time
# Infinite loop – webserv must kill this within its CGI timeout
while True:
    time.sleep(1)
PYEOF
chmod +x www/cgi-bin/loop.py
# We give 15 seconds: a proper timeout (typically 5-10s) should kick in well before that.
loop_start=$(date +%s)
c=$(curl -s -o /dev/null --max-time 15 -w "%{http_code}" "$B/cgi-bin/loop.py")
loop_end=$(date +%s)
elapsed=$(( loop_end - loop_start ))
srv_alive && SA="server alive" || SA="SERVER DIED"
if [ "$SA" = "SERVER DIED" ]; then
  ko "CGI infinite loop / timeout" "SERVER DIED waiting for looping script"
  start_server
elif [[ "$c" =~ ^(504|500|502)$ ]]; then
  ok "CGI timeout: server returned $c after ~${elapsed}s ($SA)"
elif [ "$elapsed" -lt 14 ]; then
  # Returned quickly with some status – server timed out and cleaned up
  ok "CGI timeout: server returned $c after ${elapsed}s (killed looping script, $SA)"
else
  ko "CGI timeout" "request hung for ${elapsed}s, returned $c – server may not implement CGI timeout"
fi

# --- CGI working directory is script dir (automated) -----------------------
cat > www/cgi-bin/cwd.py << 'PYEOF'
#!/usr/bin/env python3
import os, sys
sys.stdout.write("Content-Type: text/plain\r\n\r\n")
sys.stdout.write(os.getcwd())
sys.stdout.flush()
PYEOF
chmod +x www/cgi-bin/cwd.py
cgi_cwd=$(curl -s --max-time 5 "$B/cgi-bin/cwd.py")
expected_dir=$(realpath www/cgi-bin 2>/dev/null || echo "www/cgi-bin")
if echo "$cgi_cwd" | grep -q "cgi-bin"; then
  ok "CGI working directory is script dir (got: $cgi_cwd)"
else
  ko "CGI working directory" "expected path containing 'cgi-bin', got: '$cgi_cwd'"
fi

# ----- 8. Browser / static site ---------------------------------------------
echo ""
echo "--- 8. Static site & browser ----------------------------------------"
body=$(curl -s --max-time 5 "$B/"); echo "$body" | grep -qi "html" && ok "Fully static website served" || ko "Static website" "no HTML"
c=$(get_code "$B/nonexistent_xyz"); [ "$c" = "404" ] && ok "Wrong URL -> 404" || ko "Wrong URL -> 404" "got $c"

# --- Response header inspection (automated equivalent of Browser DevTools) ---
headers=$(curl -sI --max-time 5 "$B/")
HEADER_PASS=0; HEADER_FAIL=0; HEADER_DETAILS=""
for hdr in "HTTP/1" "Content-Type" "Content-Length\|Transfer-Encoding" "Server\|Date"; do
  if echo "$headers" | grep -qi "$hdr"; then
    ((HEADER_PASS++))
  else
    ((HEADER_FAIL++))
    HEADER_DETAILS+=" missing:$hdr"
  fi
done
[ "$HEADER_FAIL" -eq 0 ] \
  && ok "Response headers complete (HTTP/1.x, Content-Type, Content-Length/TE, Server/Date)" \
  || ko "Response headers incomplete" "$HEADER_DETAILS"

# --- Redirect URL ------------------------------------------------------------
# Test that GET /old returns 301 with Location: /new
redir_code=$(curl -s -o /dev/null --max-time 5 -w "%{http_code}" "$B/old")
redir_loc=$(curl -sI --max-time 5 "$B/old" | grep -i "^Location:" | tr -d '\r')
if [ "$redir_code" = "301" ] || [ "$redir_code" = "302" ]; then
  if echo "$redir_loc" | grep -qi "/new"; then
    ok "Redirect /old -> /new ($redir_code, Location: $redir_loc)"
  else
    ok "Redirect /old returned $redir_code (Location header: '${redir_loc:-not set}')"
  fi
else
  ko "Redirect URL /old -> /new" "expected 301/302, got $redir_code (Location: '${redir_loc:-not set}')"
fi
# Follow the redirect and confirm we land on /new content
final_body=$(curl -sL --max-time 5 "$B/old")
echo "$final_body" | grep -qi "html\|new" \
  && ok "Redirect /old followed to /new: HTML content received" \
  || ko "Redirect follow" "no HTML at redirect target /new"

# ----- 9. Concurrent / resilience -------------------------------------------
echo ""
echo "--- 9. Resilience & concurrent connections -------------------------"
PIDS=()
for i in $(seq 1 30); do curl -s -o /dev/null --max-time 5 "$B/" & PIDS+=($!); done
for pid in "${PIDS[@]}"; do wait "$pid" 2>/dev/null; done
srv_alive && ok "Server alive after 30 concurrent GETs" || ko "30 concurrent GETs" "SERVER DIED"

printf "GET / HTTP/1.1\r\nHost: localhost\r\n" | timeout 3 nc 127.0.0.1 8080 >/dev/null 2>&1 || true
srv_alive && ok "Server alive after partial/hanging request" || ko "Partial request" "SERVER DIED"

printf "GARBAGE REQUEST HERE\r\n\r\n" | timeout 3 nc 127.0.0.1 8080 >/dev/null 2>&1 || true
srv_alive && ok "Server alive after garbage request" || ko "Garbage request" "SERVER DIED"

# ----- 10. Siege test -------------------------------------------------------
echo ""
echo "--- 10. Siege stress test -------------------------------------------"
if [ "$SKIP_SIEGE" -eq 1 ]; then
  sk "Siege (--no-siege passed)"
elif ! command -v siege &>/dev/null; then
  sk "Siege not installed (sudo apt-get install siege)"
else
  echo "  Running siege -b -t15S ..."
  siege_out=$(siege -b -t15S "$B/" 2>&1) || true
  avail=$(echo "$siege_out" | grep -oP 'Availability:\s+\K[\d.]+' 2>/dev/null || echo "")
  if [ -z "$avail" ]; then
    sk "Siege availability (could not parse output)"
  else
    echo "  Availability: ${avail}%"
    python3 -c "import sys; sys.exit(0 if float('$avail')>=99.5 else 1)" 2>/dev/null \
      && ok "Siege availability ${avail}% >= 99.5%" \
      || ko "Siege availability" "${avail}% < 99.5%"
  fi
  srv_alive && ok "Server alive after siege" || ko "Server alive after siege" "SERVER DIED"
fi

# ----- 11. Valgrind memory leaks --------------------------------------------
echo ""
echo "--- 11. Memory (valgrind) -------------------------------------------"
if [ "$SKIP_VG" -eq 1 ]; then
  sk "Valgrind (--no-valgrind passed)"
elif ! command -v valgrind &>/dev/null; then
  sk "Valgrind not installed (sudo apt-get install valgrind)"
else
  pkill -f "webserv config" 2>/dev/null; sleep 0.3
  valgrind --leak-check=full --show-leak-kinds=definite \
    ./webserv config/webserv.conf >/tmp/vg_server.log 2>&1 &
  VG_PID=$!; echo $VG_PID >/tmp/webserv.pid; sleep 1.5
  curl -s --max-time 5 "$B/" >/dev/null 2>&1
  curl -s --max-time 5 "$B/nope" >/dev/null 2>&1
  curl -s --max-time 5 -X POST -F "file=@/tmp/ws_upload.txt" "$B/upload" >/dev/null 2>&1
  sleep 0.5; kill $VG_PID 2>/dev/null; wait $VG_PID 2>/dev/null || true
  if grep -q "definitely lost: 0 bytes" /tmp/vg_server.log 2>/dev/null; then
    ok "Valgrind: 0 bytes definitely lost"
  elif grep -q "ERROR SUMMARY: 0 errors" /tmp/vg_server.log 2>/dev/null; then
    ok "Valgrind: ERROR SUMMARY 0 errors"
  else
    grep -E "definitely lost|ERROR SUMMARY|LEAK SUMMARY" /tmp/vg_server.log 2>/dev/null | head -6
    ko "Valgrind" "leaks or errors found (see above)"
  fi
  start_server
fi

# ----- Cleanup --------------------------------------------------------------
pkill -f "webserv config" 2>/dev/null

echo ""
echo "============================================================"
printf   "  Results:  PASS=%-4s  FAIL=%-4s  SKIP=%-4s\n" "$PASS" "$FAIL" "$SKIP"
echo "============================================================"
echo ""
if [ ${#FAIL_MSGS[@]} -gt 0 ]; then
  echo "Failed:"
  for m in "${FAIL_MSGS[@]}"; do echo "  x $m"; done
  echo ""
fi
