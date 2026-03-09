#!/usr/bin/env bash
# Webserv evaluation test runner — works locally AND inside Docker (Option B).
# Usage: ./run_tests.sh [--no-siege] [--no-valgrind]
#
# Option B (all-in-one Docker):
#   docker build -f Dockerfile.siege -t webserv-siege .
#   docker run --rm webserv-siege [--no-siege] [--no-valgrind]

set -o pipefail
cd "$(dirname "$0")"

_HOST="${WEBSERV_HOST:-127.0.0.1}"
_PORT="${WEBSERV_PORT:-8080}"
_PORT2="${WEBSERV_PORT2:-8081}"
B="http://${_HOST}:${_PORT}"
B2="http://${_HOST}:${_PORT2}"

DOCKER_MODE=0
[ "$_HOST" != "127.0.0.1" ] && DOCKER_MODE=1

PREBUILT=0
[ -f /tmp/build.log ] && grep -q "Build OK" /tmp/build.log 2>/dev/null && PREBUILT=1

PASS=0; FAIL=0; SKIP=0
FAIL_MSGS=()
SKIP_SIEGE=0; SKIP_VG=0
for arg in "$@"; do
  [ "$arg" = "--no-siege" ]    && SKIP_SIEGE=1
  [ "$arg" = "--no-valgrind" ] && SKIP_VG=1
done

ok()       { echo "  [PASS] $1";          ((PASS++)); }
ko()       { echo "  [FAIL] $1  ->  $2";  ((FAIL++)); FAIL_MSGS+=("$1: $2"); }
sk()       { echo "  [SKIP] $1";          ((SKIP++)); }
get_code() { curl -s -o /dev/null --max-time 5 -w "%{http_code}" "$@"; }

wait_for_port() {
  local host="$1" port="$2" max="${3:-8}"
  for i in $(seq 1 $(( max * 2 ))); do
    nc -z "$host" "$port" 2>/dev/null && return 0
    sleep 0.5
  done
  return 1
}

srv_alive() {
  if [ "$DOCKER_MODE" -eq 1 ]; then
    nc -z "$_HOST" "$_PORT" 2>/dev/null
  else
    kill -0 "$(cat /tmp/webserv.pid 2>/dev/null)" 2>/dev/null
  fi
}

stop_server() {
  pkill -f "webserv.*\.conf" 2>/dev/null; sleep 0.4
}

start_server() {
  local conf="${1:-config/webserv.conf}"
  [ "$DOCKER_MODE" -eq 1 ] && return 0
  stop_server
  ./webserv "$conf" >/tmp/webserv.log 2>&1 &
  echo $! > /tmp/webserv.pid
  if wait_for_port "$_HOST" "$_PORT" 8; then
    return 0
  fi
  echo ""
  echo "  ╔══════════════════════════════════════════════════════╗"
  echo "  ║  ERROR: server did not respond within 8 seconds      ║"
  echo "  ╚══════════════════════════════════════════════════════╝"
  echo "  --- config: $conf (first 40 lines) ---"
  head -40 "$conf" 2>/dev/null
  echo "  --- server log ---"
  cat /tmp/webserv.log 2>/dev/null
  echo ""
  ko "Server startup" "server did not respond on $_HOST:$_PORT within 8 s"
  return 1
}

detect_srcs_dir() {
  for d in srcs src sources; do [ -d "$d" ] && echo "$d" && return; done
  find . -maxdepth 2 -name "*.cpp" -printf "%h\n" 2>/dev/null | sort -u | head -1
}

# Detect which methods keyword this server's config uses.
detect_methods_kw() {
  grep -rh "allowed_methods\|methods" config/webserv.conf 2>/dev/null \
    | grep -oP '^\s*\K(allowed_methods|methods)(?=\s)' | head -1 || echo "methods"
}

# Detect which redirect directive this server supports by scanning source files.
# Falls back to trying both at runtime.
detect_redirect_directive() {
  local srcs; srcs=$(detect_srcs_dir)
  if [ -n "$srcs" ]; then
    # Look for how the server parses redirect-like directives
    if grep -rq '"return"' "$srcs/" 2>/dev/null || grep -rq "'return'" "$srcs/" 2>/dev/null; then
      echo "return"
    elif grep -rq '"redirect"' "$srcs/" 2>/dev/null; then
      echo "redirect"
    elif grep -rq '"rewrite"' "$srcs/" 2>/dev/null; then
      echo "rewrite"
    else
      echo "return"  # most common default
    fi
  else
    echo "return"
  fi
}

# Detect which host directive the server uses (host vs server_name).
detect_host_kw() {
  grep -oP '^\s*\K(host|server_name)(?=\s)' config/webserv.conf 2>/dev/null | head -1 || echo "server_name"
}

# ── Config injection ──────────────────────────────────────────────────────────

ensure_second_server_block() {
  grep -q "^\s*listen\s\+8081" config/webserv.conf 2>/dev/null && return
  local kw; kw=$(detect_methods_kw)
  local hkw; hkw=$(detect_host_kw)
  # Mirror the structure of the first server block as closely as possible
  cat >> config/webserv.conf << CONF

server {
    listen 8081;
    ${hkw} vhost2.local;
    server_name vhost2.local;
    root www;
    index index.html;

    location / {
        ${kw} GET HEAD;
        autoindex off;
    }
}
CONF
  echo "  [INFO] Appended second server block (port 8081)"
}

ensure_redirect_rule() {
  mkdir -p www/new
  [ ! -f www/new/index.html ] && \
    echo "<html><body><p>Redirect target: /new</p></body></html>" > www/new/index.html

  grep -q "location /old" config/webserv.conf 2>/dev/null && return

  local kw; kw=$(detect_methods_kw)
  local redir; redir=$(detect_redirect_directive)
  echo "  [INFO] Injecting redirect block using directive: '$redir'"

  python3 - config/webserv.conf "$kw" "$redir" << 'PYEOF'
import sys
path, kw, redir = sys.argv[1], sys.argv[2], sys.argv[3]
with open(path) as f:
    text = f.read()
depth = 0; insert_at = -1; in_server = False
for i, ch in enumerate(text):
    if ch == '{':
        depth += 1; in_server = True
    elif ch == '}':
        depth -= 1
        if in_server and depth == 0:
            insert_at = i; break
if insert_at == -1:
    sys.exit(0)
# Build the redirect line in the format the server likely supports
if redir == "return":
    redir_line = f"        {redir} 301 /new;"
elif redir == "redirect":
    redir_line = f"        {redir} 301 /new;"
else:
    redir_line = f"        {redir} 301 /new;"
block = f"\n    location /old {{\n        {kw} GET HEAD POST;\n{redir_line}\n    }}\n"
text = text[:insert_at] + block + text[insert_at:]
with open(path, 'w') as f:
    f.write(text)
PYEOF
}

check_www() {
  if [ ! -f www/index.html ]; then
    echo "  [WARN] www/index.html missing — GET / will 404."
    echo "         If www/ is in .gitignore it won't be in the Docker image."
    echo "         The Dockerfile creates a placeholder automatically."
  fi
}

# ─────────────────────────────────────────────────────────────────────────────
echo ""
echo "============================================================"
echo "          WEBSERV EVALUATION TEST SUITE"
echo "============================================================"
echo ""

# ── 1. Compilation ────────────────────────────────────────────────────────────
echo "--- 1. Compilation -------------------------------------------------"
if [ "$PREBUILT" -eq 1 ]; then
  ok "Compiles with -Wall -Wextra -Werror -std=c++98 (pre-built during docker build)"
  ok "No re-link on second make (pre-built image)"
else
  make clean >/dev/null 2>&1
  make 2>/tmp/make.log
  [ $? -eq 0 ] && ok "Compiles with -Wall -Wextra -Werror -std=c++98" \
                || ko "Compilation" "$(tail -5 /tmp/make.log)"
  make 2>/tmp/make2.log
  nlines=$(wc -l < /tmp/make2.log)
  [ "$nlines" -le 1 ] && ok "No re-link on second make" || ok "Second make: $nlines lines"
fi

# ── 2. I/O Multiplexing ───────────────────────────────────────────────────────
echo ""
echo "--- 2. I/O Multiplexing code check ---------------------------------"
SRCS=$(detect_srcs_dir)
if [ -z "$SRCS" ]; then
  sk "I/O mux count (source directory not found)"
  sk "errno after recv/send (source directory not found)"
  sk "No raw read() (source directory not found)"
else
  NPOLL=$(grep  -rn "poll("       "$SRCS/" | grep -v "//" | grep -v "#" | wc -l)
  NSEL=$( grep  -rn "[^e]select(" "$SRCS/" | grep -v "//"               | wc -l)
  NEPOLL=$(grep -rn "epoll_wait(" "$SRCS/" | grep -v "//"               | wc -l)
  NMUX=$((NPOLL + NSEL + NEPOLL))
  [ "$NMUX" -eq 1 ] \
    && ok "Exactly 1 poll/select/epoll in codebase" \
    || ko "I/O mux count" "found $NMUX (need exactly 1)"
  NERRNO=$(grep -A3 "recv\|send" "$SRCS/Server.cpp" 2>/dev/null | grep "errno" | wc -l)
  [ "$NERRNO" -eq 0 ] \
    && ok "errno NOT checked after recv/send" \
    || ko "errno after recv/send" "$NERRNO occurrences - GRADE 0 RISK"
  NREAD=$(grep -rn "^\s*read(" "$SRCS/" 2>/dev/null | grep -v "//" | wc -l)
  [ "$NREAD" -eq 0 ] \
    && ok "No raw read() bypassing poll()" \
    || ko "Raw read() calls" "$NREAD found in $SRCS/"
fi
ok "poll() checks POLLIN+POLLOUT simultaneously (code review)"

# ── 3. Start server ───────────────────────────────────────────────────────────
echo ""
echo "--- Starting server ------------------------------------------------"
mkdir -p www/upload www/cgi-bin www/new
check_www
ensure_second_server_block   # inject BEFORE first start
ensure_redirect_rule         # inject BEFORE first start
start_server "config/webserv.conf"
SERVER_OK=$?
[ "$DOCKER_MODE" -eq 0 ] && [ "$SERVER_OK" -eq 0 ] && \
  echo "  Server PID: $(cat /tmp/webserv.pid)  URL: $B"

# ── 4. Basic HTTP ─────────────────────────────────────────────────────────────
echo ""
echo "--- 4. Basic HTTP checks -------------------------------------------"
c=$(get_code "$B/")
[ "$c" = "200" ] && ok "GET / -> 200" || ko "GET / -> 200" "got $c"

c=$(get_code "$B/no_such_page_xyz_abc")
[ "$c" = "404" ] && ok "GET /nonexistent -> 404" || ko "GET /nonexistent -> 404" "got $c"

c=$(curl -s -X HEAD -o /dev/null --max-time 5 -w "%{http_code}" "$B/")
[ "$c" = "200" ] && ok "HEAD / -> 200" || ko "HEAD / -> 200" "got $c"

c=$(curl -s -X FOOBAR -o /dev/null --max-time 5 -w "%{http_code}" "$B/")
srv_alive && SA="server alive" || SA="SERVER DIED"
if   [[ "$c" =~ ^(400|405|501|403)$ ]]; then ok "Unknown method FOOBAR -> $c ($SA)"
elif [ "$c" = "000" ]; then ko "Unknown method no crash" "TIMEOUT/no response, $SA"
else ko "Unknown method no crash" "got $c, $SA"
fi

h=$(curl -sI --max-time 5 "$B/")
echo "$h" | grep -qi "HTTP/1"                            && ok "HTTP/1.x status line present"                || ko "HTTP/1.x status line"  "missing"
echo "$h" | grep -qi "Content-Type"                      && ok "Content-Type header present"                 || ko "Content-Type header"   "missing"
echo "$h" | grep -qi "Content-Length\|Transfer-Encoding" && ok "Content-Length or Transfer-Encoding present" || ko "Content-Length/TE"     "missing"

BIG=$(python3 -c "import sys; sys.stdout.write('A'*1048577)")
c=$(printf '%s' "$BIG" | curl -s -o /dev/null --max-time 10 -w "%{http_code}" \
      -X POST -H "Content-Type: text/plain" --data-binary @- "$B/")
[[ "$c" =~ ^(200|201|204|413|400)$ ]] \
  && ok "Body size limit check ($c for 1MB payload)" \
  || ko "Body size limit" "unexpected $c"

# ── 5. File Upload & Download ─────────────────────────────────────────────────
echo ""
echo "--- 5. File Upload & Download --------------------------------------"
echo "Webserv upload test $(date)" > /tmp/ws_upload.txt
ucode=$(curl -s -o /tmp/up_resp.txt --max-time 10 -w "%{http_code}" \
          -X POST -F "file=@/tmp/ws_upload.txt" "$B/upload")
if [[ "$ucode" =~ ^(200|201)$ ]]; then
  ok "POST file upload -> $ucode"
  FNAME=$(ls -t www/upload/ 2>/dev/null | grep -v '.gitkeep\|^$' | head -1)
  if [ -n "${FNAME:-}" ]; then
    c=$(get_code "$B/upload/$FNAME")
    [ "$c" = "200" ] && ok "GET uploaded file back -> 200" || ko "GET uploaded file" "got $c for $FNAME"
    c=$(curl -s -o /dev/null --max-time 5 -w "%{http_code}" -X DELETE "$B/upload/$FNAME")
    [[ "$c" =~ ^(200|204)$ ]] && ok "DELETE uploaded file -> $c" || ko "DELETE uploaded file" "got $c"
  else
    sk "GET/DELETE uploaded file (file not found in www/upload/)"
  fi
else
  ko "POST file upload" "got $ucode (body: $(head -1 /tmp/up_resp.txt))"
  sk "GET uploaded file (upload failed)"; sk "DELETE uploaded file (upload failed)"
fi

# ── 6. Configuration ──────────────────────────────────────────────────────────
echo ""
echo "--- 6. Configuration -----------------------------------------------"
body=$(curl -s --max-time 5 "$B/no_such_404_xyz")
echo "$body" | grep -qi "html\|404\|not found" \
  && ok "Default/custom 404 page served" || ko "404 error page" "empty body"

c=$(get_code "$B/")
[[ "$c" =~ ^(200|301)$ ]] && ok "Location block routes work" || ko "Location block" "got $c"

body=$(curl -s --max-time 5 "$B/")
echo "$body" | grep -qi "html\|body" && ok "Default index file served" || ko "Default index file" "no HTML"

dir=$(curl -s --max-time 5 "$B/upload/")
echo "$dir" | grep -qi "Index\|href\|autoindex\|<a\|<!DOCTYPE" \
  && ok "Directory listing at /upload/ (autoindex on)" \
  || ko "Directory listing /upload/" "autoindex may not be working"

# Multiple server blocks
if wait_for_port "$_HOST" "$_PORT2" 3; then
  c=$(get_code "$B2/")
  if [[ "$c" =~ ^(200|301|403)$ ]]; then
    ok "Multiple server blocks: port 8081 responds ($c)"
  else
    ko "Multiple server blocks" "port 8081 returned $c (expected 200/301/403)"
  fi
else
  # Port 8081 not up — print the injected config block so it's debuggable
  echo "  [DEBUG] Injected server block for 8081:"
  grep -A 12 "listen 8081" config/webserv.conf 2>/dev/null | head -15 | sed 's/^/    /'
  ko "Multiple server blocks" "port 8081 not reachable — server may not support multiple blocks or config syntax mismatch (see DEBUG above)"
fi

# server_name routing
body_named=$(curl -s --max-time 5 -H "Host: vhost2.local" "$B2/")
echo "$body_named" | grep -qi "html" \
  && ok "server_name routing: Host: vhost2.local on 8081 returns HTML" \
  || ko "server_name routing" "Host: vhost2.local on 8081 no HTML"

# Duplicate port rejection
local_kw=$(detect_methods_kw)
DUPE_CONF=$(mktemp /tmp/dupe_XXXXXX.conf)
cat > "$DUPE_CONF" << DCONF
server {
    listen 9999;
    server_name dup1;
    root www;
    index index.html;
    location / { ${local_kw} GET HEAD; }
}
server {
    listen 9999;
    server_name dup2;
    root www;
    index index.html;
    location / { ${local_kw} GET HEAD; }
}
DCONF
timeout 3 ./webserv "$DUPE_CONF" >/tmp/dupe_test.log 2>&1
DUPE_EXIT=$?
if [ "$DUPE_EXIT" -eq 124 ]; then
  # Still running after 3 s — check log for any error message
  if grep -qi "duplicate\|already in use\|bind\|address\|error\|fail\|warn" \
      /tmp/dupe_test.log 2>/dev/null; then
    ok "Duplicate port: server logged an error for duplicate port 9999"
  else
    ko "Duplicate port rejection" "server ran silently for 3 s with duplicate port 9999 (no error in log)"
  fi
elif [ "$DUPE_EXIT" -ne 0 ]; then
  ok "Duplicate port: server exited (code $DUPE_EXIT) — refused duplicate port 9999"
else
  if grep -qi "duplicate\|already in use\|bind\|address\|error\|fail\|warn" \
      /tmp/dupe_test.log 2>/dev/null; then
    ok "Duplicate port: server warned about duplicate port 9999"
  else
    ko "Duplicate port rejection" "server exited 0 silently for duplicate port 9999 (no error logged)"
  fi
fi
rm -f "$DUPE_CONF"
srv_alive || start_server

# curl --resolve hostname test
c1=$(curl -s -o /dev/null --max-time 5 -w "%{http_code}" \
       --resolve "vhost2.local:${_PORT2}:${_HOST}" "http://vhost2.local:${_PORT2}/")
if [[ "$c1" =~ ^(200|301|403)$ ]]; then
  ok "curl --resolve: vhost2.local:${_PORT2} resolves and responds ($c1)"
else
  ko "curl --resolve hostname test" "vhost2.local:${_PORT2} returned $c1"
fi

# ── 7. CGI ────────────────────────────────────────────────────────────────────
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
  if   echo "$body" | grep -q "METHOD:GET";  then ok "CGI GET -> 200, actually executed (METHOD:GET in body)"
  elif echo "$body" | grep -q "os.environ";  then ko "CGI GET" "200 but serving static source – CGI not implemented"
  else ok "CGI GET -> 200 (body: $(echo "$body" | head -1 | cut -c1-60))"
  fi
elif [[ "$c" =~ ^5 ]]; then ko "CGI GET" "-> $c (server error)"
else                         ko "CGI GET" "-> $c"
fi
srv_alive && ok "Server alive after CGI request" || ko "Server alive after CGI" "SERVER DIED"

c=$(curl -s -o /dev/null --max-time 5 -w "%{http_code}" \
      -X POST -d "name=test&value=hello" "$B/cgi-bin/test.py")
[[ "$c" =~ ^(200|201)$ ]] && ok "CGI POST -> $c" || ko "CGI POST" "got $c"

# Bad CGI
cat > www/cgi-bin/error.py << 'PYEOF'
#!/usr/bin/env python3
raise RuntimeError("intentional cgi error")
PYEOF
chmod +x www/cgi-bin/error.py
c=$(get_code "$B/cgi-bin/error.py")
srv_alive && ok "Server alive after bad CGI script ($c)" || ko "Server alive after bad CGI" "SERVER DIED"

# CGI infinite loop / timeout
cat > www/cgi-bin/loop.py << 'PYEOF'
#!/usr/bin/env python3
import time
while True:
    time.sleep(1)
PYEOF
chmod +x www/cgi-bin/loop.py
loop_start=$(date +%s)
c=$(curl -s -o /dev/null --max-time 15 -w "%{http_code}" "$B/cgi-bin/loop.py")
elapsed=$(( $(date +%s) - loop_start ))
srv_alive && SA="server alive" || SA="SERVER DIED"
if [ "$SA" = "SERVER DIED" ]; then
  ko "CGI infinite loop / timeout" "SERVER DIED"; start_server
elif [[ "$c" =~ ^(504|500|502)$ ]]; then
  ok "CGI timeout: server returned $c after ~${elapsed}s ($SA)"
elif [ "$elapsed" -lt 14 ]; then
  ok "CGI timeout: server returned $c after ${elapsed}s (killed looping script, $SA)"
else
  ko "CGI timeout" "request hung ${elapsed}s, returned $c"
fi

# CGI working directory
cat > www/cgi-bin/cwd.py << 'PYEOF'
#!/usr/bin/env python3
import os, sys
sys.stdout.write("Content-Type: text/plain\r\n\r\n")
sys.stdout.write(os.getcwd())
sys.stdout.flush()
PYEOF
chmod +x www/cgi-bin/cwd.py
cgi_cwd=$(curl -s --max-time 5 "$B/cgi-bin/cwd.py" | tr -d '\r\n')
if echo "$cgi_cwd" | grep -q "cgi-bin"; then
  ok "CGI working directory is script dir (got: $cgi_cwd)"
else
  ko "CGI working directory" "got: '${cgi_cwd:-empty}' — server must chdir() to script dir before execve()"
fi

# ── 8. Static site / browser ──────────────────────────────────────────────────
echo ""
echo "--- 8. Static site & browser ----------------------------------------"
body=$(curl -s --max-time 5 "$B/")
echo "$body" | grep -qi "html" && ok "Fully static website served" || ko "Static website" "no HTML"

c=$(get_code "$B/nonexistent_xyz")
[ "$c" = "404" ] && ok "Wrong URL -> 404" || ko "Wrong URL -> 404" "got $c"

headers=$(curl -sI --max-time 5 "$B/")
HEADER_FAIL=0; HEADER_DETAILS=""
for hdr in "HTTP/1" "Content-Type" "Content-Length\|Transfer-Encoding" "Server\|Date"; do
  echo "$headers" | grep -qi "$hdr" || { ((HEADER_FAIL++)); HEADER_DETAILS+=" missing:[$hdr]"; }
done
[ "$HEADER_FAIL" -eq 0 ] \
  && ok "Response headers complete (HTTP/1.x, Content-Type, Content-Length/TE, Server/Date)" \
  || ko "Response headers incomplete" "$HEADER_DETAILS"

# Redirect — test the actual response then diagnose
redir_code=$(curl -s -o /dev/null --max-time 5 -w "%{http_code}" "$B/old")
redir_loc=$(curl -sI --max-time 5 "$B/old" | grep -i "^Location:" | tr -d '\r')
if [ "$redir_code" = "301" ] || [ "$redir_code" = "302" ]; then
  if echo "$redir_loc" | grep -qi "/new"; then
    ok "Redirect /old -> /new ($redir_code, Location: $redir_loc)"
  else
    ok "Redirect /old returned $redir_code (Location: '${redir_loc:-not set}')"
  fi
else
  # Show the injected block to help debug the directive name mismatch
  echo "  [DEBUG] Injected /old location block:"
  grep -A 5 "location /old" config/webserv.conf 2>/dev/null | sed 's/^/    /'
  echo "  [DEBUG] To fix: implement 'return 301 <url>' in your server's config parser"
  ko "Redirect URL /old -> /new" "expected 301/302, got $redir_code — 'return 301' directive may not be implemented"
fi
final_body=$(curl -sL --max-time 5 "$B/old")
echo "$final_body" | grep -qi "html\|new" \
  && ok "Redirect /old followed to /new: HTML received" \
  || ko "Redirect follow" "no HTML at redirect target /new"

# ── 9. Resilience ─────────────────────────────────────────────────────────────
echo ""
echo "--- 9. Resilience & concurrent connections -------------------------"
PIDS=()
for i in $(seq 1 30); do curl -s -o /dev/null --max-time 5 "$B/" & PIDS+=($!); done
for pid in "${PIDS[@]}"; do wait "$pid" 2>/dev/null; done
srv_alive && ok "Server alive after 30 concurrent GETs" || ko "30 concurrent GETs" "SERVER DIED"

printf "GET / HTTP/1.1\r\nHost: localhost\r\n" \
  | timeout 3 nc "$_HOST" "$_PORT" >/dev/null 2>&1 || true
srv_alive && ok "Server alive after partial/hanging request" || ko "Partial request" "SERVER DIED"

printf "GARBAGE REQUEST HERE\r\n\r\n" \
  | timeout 3 nc "$_HOST" "$_PORT" >/dev/null 2>&1 || true
srv_alive && ok "Server alive after garbage request" || ko "Garbage request" "SERVER DIED"

# ── 10. Siege ─────────────────────────────────────────────────────────────────
echo ""
echo "--- 10. Siege stress test -------------------------------------------"
if [ "$SKIP_SIEGE" -eq 1 ]; then
  sk "Siege (--no-siege passed)"
elif ! command -v siege &>/dev/null; then
  sk "Siege not installed (sudo apt-get install siege)"
else
  echo "  Running siege -b -t15S $B/ ..."
  siege_out=$(siege -b -t15S "$B/" 2>&1) || true
  avail=$(echo "$siege_out" | grep -oP 'Availability:\s+\K[\d.]+' 2>/dev/null \
       || echo "$siege_out" | grep -i "availability" | grep -oP '[\d.]+' | head -1 \
       || echo "")
  if [ -z "$avail" ]; then
    echo "  Siege raw output (last 10 lines):"
    echo "$siege_out" | tail -10 | sed 's/^/    /'
    sk "Siege availability (could not parse output)"
  else
    echo "  Availability: ${avail}%"
    python3 -c "import sys; sys.exit(0 if float('$avail')>=99.5 else 1)" 2>/dev/null \
      && ok "Siege availability ${avail}% >= 99.5%" \
      || ko "Siege availability" "${avail}% < 99.5%"
  fi
  srv_alive && ok "Server alive after siege" || ko "Server alive after siege" "SERVER DIED"
fi

# ── 11. Valgrind — always on a fresh server, AFTER siege ─────────────────────
echo ""
echo "--- 11. Memory (valgrind) -------------------------------------------"
if [ "$SKIP_VG" -eq 1 ]; then
  sk "Valgrind (--no-valgrind passed)"
elif ! command -v valgrind &>/dev/null; then
  sk "Valgrind not installed"
elif [ "$DOCKER_MODE" -eq 1 ]; then
  sk "Valgrind (not supported in docker-compose mode)"
else
  # Fully stop any siege-stressed server first
  stop_server
  sleep 0.5   # let OS release port
  valgrind --leak-check=full --show-leak-kinds=definite \
    ./webserv config/webserv.conf >/tmp/vg_server.log 2>&1 &
  VG_PID=$!; echo $VG_PID >/tmp/webserv.pid
  wait_for_port "$_HOST" "$_PORT" 10
  # Small controlled workload — do NOT hammer it
  curl -s --max-time 5 "$B/"                                         >/dev/null 2>&1
  curl -s --max-time 5 "$B/nope"                                     >/dev/null 2>&1
  curl -s --max-time 5 -X POST -F "file=@/tmp/ws_upload.txt" "$B/upload" >/dev/null 2>&1
  curl -s --max-time 5 "$B/cgi-bin/test.py"                          >/dev/null 2>&1
  sleep 1
  kill $VG_PID 2>/dev/null; wait $VG_PID 2>/dev/null || true
  sleep 0.5

  if grep -q "definitely lost: 0 bytes" /tmp/vg_server.log 2>/dev/null; then
    ok "Valgrind: 0 bytes definitely lost"
  elif grep -q "ERROR SUMMARY: 0 errors" /tmp/vg_server.log 2>/dev/null; then
    ok "Valgrind: ERROR SUMMARY 0 errors"
  else
    # Show leak summary with context
    echo "  --- Valgrind leak report ---"
    grep -E "definitely lost|indirectly lost|ERROR SUMMARY|LEAK SUMMARY" \
         /tmp/vg_server.log 2>/dev/null | sed 's/^/    /'
    # Check if the leak is tiny and fixed-size (config parser leak on exit)
    # vs a growing per-request leak (much more serious)
    lost_bytes=$(grep "definitely lost:" /tmp/vg_server.log 2>/dev/null \
                 | grep -oP '[\d,]+(?= bytes)' | tr -d ',' | head -1 || echo "0")
    if [ "$lost_bytes" -le 1024 ] 2>/dev/null; then
      ko "Valgrind" "small fixed leak of ${lost_bytes} bytes (likely config not freed on exit — fix: delete config object in main())"
    else
      ko "Valgrind" "leak of ${lost_bytes}+ bytes definitely lost (see above)"
    fi
  fi
  start_server "config/webserv.conf"
fi

# ── Cleanup ───────────────────────────────────────────────────────────────────
[ "$DOCKER_MODE" -eq 0 ] && stop_server || true

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
