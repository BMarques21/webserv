#!/bin/bash

# Webserver Evaluation Test Script
# Tests all requirements from the evaluation sheet

PORT=8080
HOST="localhost"
BASE_URL="http://$HOST:$PORT"

echo "======================================"
echo "   WEBSERVER EVALUATION TEST SUITE"
echo "======================================"
echo ""

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Test counter
PASSED=0
FAILED=0

test_result() {
    if [ $1 -eq 0 ]; then
        echo -e "${GREEN}✓ PASS${NC}: $2"
        ((PASSED++))
    else
        echo -e "${RED}✗ FAIL${NC}: $2"
        ((FAILED++))
    fi
}

echo "=== BASIC HTTP TESTS ==="
echo ""

# Test 1: GET Request
echo "Test 1: GET Request"
RESPONSE=$(curl -s -o /dev/null -w "%{http_code}" $BASE_URL/)
if [ "$RESPONSE" == "200" ]; then
    test_result 0 "GET / returns 200 OK"
else
    test_result 1 "GET / returns $RESPONSE (expected 200)"
fi
echo ""

# Test 2: 404 Not Found
echo "Test 2: 404 Not Found"
RESPONSE=$(curl -s -o /dev/null -w "%{http_code}" $BASE_URL/nonexistent)
if [ "$RESPONSE" == "404" ]; then
    test_result 0 "GET /nonexistent returns 404"
else
    test_result 1 "GET /nonexistent returns $RESPONSE (expected 404)"
fi
echo ""

# Test 3: POST Request (Upload)
echo "Test 3: POST Request"
echo "test content" > /tmp/test_upload.txt
RESPONSE=$(curl -s -o /dev/null -w "%{http_code}" -X POST -F "file=@/tmp/test_upload.txt" $BASE_URL/upload)
if [ "$RESPONSE" == "200" ] || [ "$RESPONSE" == "201" ]; then
    test_result 0 "POST /upload returns $RESPONSE"
else
    test_result 1 "POST /upload returns $RESPONSE (expected 200/201)"
fi
rm -f /tmp/test_upload.txt
echo ""

# Test 4: DELETE Request
echo "Test 4: DELETE Request"
RESPONSE=$(curl -s -o /dev/null -w "%{http_code}" -X DELETE $BASE_URL/index.html)
if [ "$RESPONSE" == "200" ] || [ "$RESPONSE" == "204" ] || [ "$RESPONSE" == "404" ] || [ "$RESPONSE" == "403" ]; then
    test_result 0 "DELETE /index.html returns $RESPONSE (acceptable)"
else
    test_result 1 "DELETE /index.html returns unexpected $RESPONSE"
fi
echo ""

# Test 5: Unknown Method (PATCH)
echo "Test 5: Unknown/Unsupported Method"
RESPONSE=$(curl -s -o /dev/null -w "%{http_code}" -X PATCH $BASE_URL/)
if [ "$RESPONSE" == "405" ] || [ "$RESPONSE" == "501" ] || [ "$RESPONSE" == "400" ]; then
    test_result 0 "PATCH / returns $RESPONSE (method not allowed)"
else
    test_result 1 "PATCH / returns $RESPONSE (expected 405/501/400)"
fi
echo ""

# Test 6: HEAD Request
echo "Test 6: HEAD Request"
RESPONSE=$(curl -s -o /dev/null -w "%{http_code}" -I $BASE_URL/)
if [ "$RESPONSE" == "200" ]; then
    test_result 0 "HEAD / returns 200"
else
    test_result 1 "HEAD / returns $RESPONSE (expected 200)"
fi
echo ""

# Test 7: Large Body (Body Size Limit)
echo "Test 7: Body Size Limit Test"
python3 -c "print('A' * 10000000)" > /tmp/large_body.txt
RESPONSE=$(curl -s -o /dev/null -w "%{http_code}" -X POST -H "Content-Type: text/plain" --data-binary "@/tmp/large_body.txt" $BASE_URL/upload 2>/dev/null || echo "413")
if [ "$RESPONSE" == "413" ] || [ "$RESPONSE" == "200" ]; then
    test_result 0 "Large body test returns $RESPONSE"
else
    test_result 1 "Large body test returns $RESPONSE (expected 413 or 200)"
fi
rm -f /tmp/large_body.txt
echo ""

# Test 8: Multiple Requests (Connection handling)
echo "Test 8: Multiple Sequential Requests"
COUNT=0
for i in {1..10}; do
    RESPONSE=$(curl -s -o /dev/null -w "%{http_code}" $BASE_URL/)
    if [ "$RESPONSE" == "200" ]; then
        ((COUNT++))
    fi
done
if [ $COUNT -eq 10 ]; then
    test_result 0 "10/10 sequential requests successful"
else
    test_result 1 "$COUNT/10 sequential requests successful"
fi
echo ""

echo "======================================"
echo "   TEST SUMMARY"
echo "======================================"
echo -e "${GREEN}Passed: $PASSED${NC}"
echo -e "${RED}Failed: $FAILED${NC}"
echo "Total: $((PASSED + FAILED))"
echo ""

if [ $FAILED -eq 0 ]; then
    echo -e "${GREEN}All tests passed!${NC}"
    exit 0
else
    echo -e "${YELLOW}Some tests failed. Review the output above.${NC}"
    exit 1
fi
