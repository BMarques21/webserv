# Webserv — Evaluation Progress

> Based on: https://neon-naiad-9db266.netlify.app/cursus/webserv/

Last tested: 2025-02-19  
**`bash run_tests.sh --no-siege --no-valgrind` → 30 PASS / 0 FAIL / 10 SKIP**

---

## ✅ Passing

### Compilation
- Compiles with `-Wall -Wextra -Werror -std=c++98`
- No re-link on second `make`

### I/O Multiplexing
- Exactly 1 `poll()` call (`srcs/Server.cpp:53`)
- `errno` NOT checked after `recv`/`send`
- No raw `read()` bypassing poll
- `poll()` checks both `POLLIN` and `POLLOUT`

### Basic HTTP
- `GET /` → 200
- `GET /nonexistent` → 404
- `HEAD /` → 200
- `FOOBAR` → 405 (server stays alive)
- Proper status line, `Content-Type`, `Content-Length`
- Body size limit enforced (400 for oversized payload)

### File Upload & Download
- `POST /upload` → 200, saved to `www/upload/`
- `GET /upload/<file>` → 200
- `DELETE /upload/<file>` → 200

### Configuration
- Custom 404 error page served
- Location block routing works
- Default index file served
- `autoindex on` at `/upload/` → 200 HTML directory listing

### CGI
- `GET /cgi-bin/test.py` → 200, **executed** (body contains `METHOD:GET`)
- `POST /cgi-bin/test.py` → 200, body contains `METHOD:POST`
- Server survives bad CGI script → 500, keeps running
- Interpreter: `/usr/bin/python3`, configured via `cgi .py /usr/bin/python3;`

### Resilience
- 30 concurrent GETs → server alive
- Partial/hanging request → server alive
- Garbage request → server alive

---

## ⏭️ Skipped / Manual Tests

| Test | How to test |
|---|---|
| Multiple server blocks | Uncomment second block in `config/webserv.conf` |
| Multiple hostnames | Requires multiple server blocks |
| Duplicate port rejection | Requires two server blocks on same port |
| CGI infinite loop timeout | Run `GET /cgi-bin/loop.py` (5s timeout via select) |
| CGI working directory | Manually verify `os.getcwd()` in script = script dir |
| Browser DevTools | Open Firefox, check Network tab for headers |
| Redirect | Add `redirect` directive to a location block |
| Siege | `siege -b -t15S http://127.0.0.1:8080/` |
| Valgrind | `valgrind --leak-check=full ./webserv config/webserv.conf` |

---

## Bugs Fixed

1. **Unknown method hangs** → `HttpRequest::parse()` returned `false` on ERROR state but `_handleClientData` never sent a response. Fixed: detect `state == ERROR` and send 405/505/400 immediately.

2. **Config location block bug** → `block.find("{", i)` used line index `i` as char position, so all location blocks parsed the first block's content. Fixed: track `block_search_pos` cursor advancing past each parsed `}`.

3. **Upload routing** → Files saved to `./uploads/` but served from `www/upload/`. Fixed: config `upload_path www/upload` + created `www/upload/` directory.

4. **Autoindex ignored** → `StaticFileHandler(root)` called without `autoindex` arg. Fixed: pass `location->autoindex` and `location->index`.

5. **CGI not implemented** → `_handleCgiRequest` was a stub. Implemented `_executeCgi()`: `fork()` + `execve()`, stdin/stdout pipes, CGI env vars, 5s `select()` timeout, CGI response header parsing.

6. **Config CRLF** → Windows line endings in config file. Fixed: `sed -i 's/\r//'`.
