#!/usr/bin/env bash

set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
SCRIPT="$ROOT_DIR/package/capos/capos-webpanel/files/90-capos-webpanel-uhttpd"
TMPDIR="$(mktemp -d)"
LOG="$TMPDIR/uci.log"
cleanup() { rm -rf "$TMPDIR"; }
trap cleanup EXIT

mkdir -p "$TMPDIR/bin"
cat > "$TMPDIR/bin/uci" <<'UCI'
#!/usr/bin/env bash
if [[ "${1:-}" == "-q" ]]; then
    shift
fi
printf '%s\n' "$*" >> "$UCI_LOG"
exit 0
UCI
chmod +x "$TMPDIR/bin/uci"

env PATH="$TMPDIR/bin:$PATH" UCI_LOG="$LOG" sh "$SCRIPT"

grep -Fx "add_list uhttpd.main.listen_http=0.0.0.0:2000" "$LOG"
grep -Fx "add_list uhttpd.main.listen_https=0.0.0.0:2020" "$LOG"
grep -Fx "set uhttpd.main.cgi_prefix=/cgi-bin" "$LOG"

echo "webpanel runtime config smoke passed"
