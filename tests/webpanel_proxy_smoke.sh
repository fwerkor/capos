#!/usr/bin/env bash

set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
APP_BIN="${APP_BIN:-/tmp/capos-app-smoke}"
TMPDIR="$(mktemp -d)"
SESSION_ID="0123456789abcdef0123456789abcdef0123456789abcdef"
SESSION_DIR="/tmp/capos-webpanel/sessions"
SESSION_PATH="$SESSION_DIR/$SESSION_ID.session"
HTTPS_PID=""
WS_PID=""

cleanup() {
    [[ -n "$HTTPS_PID" ]] && kill "$HTTPS_PID" 2>/dev/null || true
    [[ -n "$WS_PID" ]] && kill "$WS_PID" 2>/dev/null || true
    rm -f "$SESSION_PATH"
    rm -rf "$TMPDIR"
}
trap cleanup EXIT

if [[ ! -x "$APP_BIN" ]]; then
    g++ -std=gnu++17 -I"$ROOT_DIR/package/capos/capos-webpanel/src" \
        -o "$APP_BIN" "$ROOT_DIR/package/capos/capos-webpanel/src/app.cpp" \
        -lcrypt -lssl -lcrypto
fi

mkdir -p "$SESSION_DIR" "$TMPDIR/bin"
cat >"$SESSION_PATH" <<SESSION
id=$SESSION_ID
username=testuser
uid=1000
is_sudo=1
created_at=1700000000
expires_at=4102444800
SESSION

cat >"$TMPDIR/bin/capbox" <<'CAPBOX'
#!/usr/bin/env sh
if [ "$1 $2" = "desktop get" ]; then
  printf '{"ok":true,"app":"demo"}\n'
  exit 0
fi
if [ "$1 $2 $3" = "app info demo" ]; then
  cat "$CAPBOX_INFO_JSON"
  exit 0
fi
echo "unexpected capbox call: $*" >&2
exit 1
CAPBOX
chmod +x "$TMPDIR/bin/capbox"

wait_for_port_file() {
    local path="$1"
    for _ in $(seq 1 50); do
        [[ -s "$path" ]] && return 0
        sleep 0.1
    done
    echo "timed out waiting for $path" >&2
    exit 1
}

openssl req -x509 -newkey rsa:2048 -nodes \
    -keyout "$TMPDIR/key.pem" \
    -out "$TMPDIR/cert.pem" \
    -subj "/CN=127.0.0.1" \
    -days 1 >/dev/null 2>&1

python3 - "$TMPDIR/https.port" "$TMPDIR/cert.pem" "$TMPDIR/key.pem" <<'PY' &
import socket
import ssl
import sys

port_file, cert_file, key_file = sys.argv[1:4]
server = socket.socket()
server.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
server.bind(("127.0.0.1", 0))
server.listen(1)
with open(port_file, "w", encoding="utf-8") as handle:
    handle.write(str(server.getsockname()[1]))
context = ssl.SSLContext(ssl.PROTOCOL_TLS_SERVER)
context.load_cert_chain(cert_file, key_file)
conn, _ = server.accept()
with context.wrap_socket(conn, server_side=True) as tls:
    data = b""
    while b"\r\n\r\n" not in data:
        chunk = tls.recv(4096)
        if not chunk:
            break
        data += chunk
    body = b'<html><head></head><body><a href="/next">next</a></body></html>'
    tls.sendall(
        b"HTTP/1.1 200 OK\r\n"
        b"Content-Type: text/html; charset=utf-8\r\n"
        b"Set-Cookie: upstream=1; Path=/\r\n"
        b"Location: /next\r\n"
        + b"Content-Length: " + str(len(body)).encode() + b"\r\n"
        b"\r\n" + body
    )
server.close()
PY
HTTPS_PID=$!
wait_for_port_file "$TMPDIR/https.port"
HTTPS_PORT="$(cat "$TMPDIR/https.port")"

cat >"$TMPDIR/info-https.json" <<JSON
{
  "ok": true,
  "app": {"name": "demo", "version": "1.0", "nickname": "Demo"},
  "runtime": {"running": true, "container_name": "capbox_u_1000_demo", "ip": "127.0.0.1"},
  "desktop": {"scheme": "https", "port": $HTTPS_PORT, "target_host": "127.0.0.1", "https_skip_check": true},
  "permissions": {"host_network": false, "host_exec": false},
  "portmaps": []
}
JSON

https_output="$(
  env -i \
    PATH="$TMPDIR/bin:$PATH" \
    CAPBOX_INFO_JSON="$TMPDIR/info-https.json" \
    REQUEST_METHOD=GET \
    PATH_INFO=/ \
    QUERY_STRING= \
    HTTP_COOKIE="capos_session=$SESSION_ID" \
    "$APP_BIN" < /dev/null
)"

grep -q "Status: 200 OK" <<<"$https_output"
grep -q "Set-Cookie: upstream=1; Path=/cgi-bin/cap/app" <<<"$https_output"
grep -q "Location: /cgi-bin/cap/app/next" <<<"$https_output"
grep -q 'href="/cgi-bin/cap/app/next"' <<<"$https_output"

python3 - "$TMPDIR/ws.port" <<'PY' &
import socket
import sys
import time

port_file = sys.argv[1]
server = socket.socket()
server.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
server.bind(("127.0.0.1", 0))
server.listen(1)
with open(port_file, "w", encoding="utf-8") as handle:
    handle.write(str(server.getsockname()[1]))
conn, _ = server.accept()
with conn:
    data = b""
    while b"\r\n\r\n" not in data:
        chunk = conn.recv(4096)
        if not chunk:
            break
        data += chunk
    conn.sendall(
        b"HTTP/1.1 101 Switching Protocols\r\n"
        b"Upgrade: websocket\r\n"
        b"Connection: Upgrade\r\n"
        b"Sec-WebSocket-Accept: smoke\r\n"
        b"\r\n"
        b"\x81\x02ok"
    )
    time.sleep(0.1)
server.close()
PY
WS_PID=$!
wait_for_port_file "$TMPDIR/ws.port"
WS_PORT="$(cat "$TMPDIR/ws.port")"

cat >"$TMPDIR/info-ws.json" <<JSON
{
  "ok": true,
  "app": {"name": "demo", "version": "1.0", "nickname": "Demo"},
  "runtime": {"running": true, "container_name": "capbox_u_1000_demo", "ip": "127.0.0.1"},
  "desktop": {"scheme": "http", "port": $WS_PORT, "target_host": "127.0.0.1", "https_skip_check": false},
  "permissions": {"host_network": false, "host_exec": false},
  "portmaps": []
}
JSON

ws_output="$(
  env -i \
    PATH="$TMPDIR/bin:$PATH" \
    CAPBOX_INFO_JSON="$TMPDIR/info-ws.json" \
    REQUEST_METHOD=GET \
    PATH_INFO=/socket \
    QUERY_STRING= \
    HTTP_COOKIE="capos_session=$SESSION_ID" \
    HTTP_UPGRADE=websocket \
    HTTP_CONNECTION=Upgrade \
    HTTP_SEC_WEBSOCKET_KEY=smoke \
    HTTP_SEC_WEBSOCKET_VERSION=13 \
    "$APP_BIN" < /dev/null
)"

grep -q "Status: 101 Switching Protocols" <<<"$ws_output"
grep -q "Sec-WebSocket-Accept: smoke" <<<"$ws_output"
grep -q "ok" <<<"$ws_output"

echo "webpanel proxy smoke passed"
