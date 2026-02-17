#!/bin/bash

# Colors
GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo "======================================"
echo "    Webserv Testing Script"
echo "======================================"
echo ""

# Check if server is running
if ! pgrep -x "webserv" > /dev/null; then
    echo -e "${YELLOW}Starting webserv...${NC}"
    ./webserv > /tmp/webserv.log 2>&1 &
    SERVER_PID=$!
    sleep 2

    if ! pgrep -x "webserv" > /dev/null; then
        echo -e "${RED}Failed to start server${NC}"
        cat /tmp/webserv.log
        exit 1
    fi
    echo -e "${GREEN}Server started (PID: $SERVER_PID)${NC}"
    CLEANUP=1
else
    echo -e "${YELLOW}Server already running${NC}"
    CLEANUP=0
fi

echo ""

# Test 1: GET request for index page
echo "Test 1: GET / (expect 200 OK)"
RESPONSE=$(curl -s -o /dev/null -w "%{http_code}" http://localhost:8080/)
if [ "$RESPONSE" -eq 200 ]; then
    echo -e "${GREEN}✓ PASS${NC} - HTTP $RESPONSE"
else
    echo -e "${RED}✗ FAIL${NC} - HTTP $RESPONSE (expected 200)"
fi

# Test 2: GET non-existent page
echo "Test 2: GET /nonexistent.html (expect 404)"
RESPONSE=$(curl -s -o /dev/null -w "%{http_code}" http://localhost:8080/nonexistent.html)
if [ "$RESPONSE" -eq 404 ]; then
    echo -e "${GREEN}✓ PASS${NC} - HTTP $RESPONSE"
else
    echo -e "${RED}✗ FAIL${NC} - HTTP $RESPONSE (expected 404)"
fi

# Test 3: Multiple concurrent requests
echo "Test 3: Multiple concurrent requests"
SUCCESS=0
for i in {1..5}; do
    RESPONSE=$(curl -s -o /dev/null -w "%{http_code}" http://localhost:8080/)
    if [ "$RESPONSE" -eq 200 ]; then
        ((SUCCESS++))
    fi
done
if [ "$SUCCESS" -eq 5 ]; then
    echo -e "${GREEN}✓ PASS${NC} - All 5 requests succeeded"
else
    echo -e "${RED}✗ FAIL${NC} - Only $SUCCESS/5 requests succeeded"
fi

# Test 4: Check if server is still running
echo "Test 4: Server stability"
if pgrep -x "webserv" > /dev/null; then
    echo -e "${GREEN}✓ PASS${NC} - Server still running"
else
    echo -e "${RED}✗ FAIL${NC} - Server crashed"
fi

# Test 5: Check Content-Type header
echo "Test 5: Content-Type header"
CONTENT_TYPE=$(curl -s -I http://localhost:8080/ | grep -i "content-type:" | cut -d' ' -f2 | tr -d '\r\n')
if [[ "$CONTENT_TYPE" == *"text/html"* ]]; then
    echo -e "${GREEN}✓ PASS${NC} - Content-Type: $CONTENT_TYPE"
else
    echo -e "${RED}✗ FAIL${NC} - Content-Type: $CONTENT_TYPE (expected text/html)"
fi

echo ""
echo "======================================"
echo "    Testing Complete"
echo "======================================"

# Cleanup if we started the server
if [ "$CLEANUP" -eq 1 ]; then
    echo ""
    echo "Stopping server..."
    kill $SERVER_PID 2>/dev/null
    sleep 1
    pkill -9 webserv 2>/dev/null
fi

