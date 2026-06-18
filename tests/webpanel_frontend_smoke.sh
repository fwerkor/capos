#!/usr/bin/env bash

set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
HTDOCS="$ROOT_DIR/package/capos/capos-webpanel/htdocs"
INDEX="$HTDOCS/index.html"
APP_JS="$HTDOCS/assets/app.js"
STYLES="$HTDOCS/assets/styles.css"

test -f "$INDEX"
test -f "$APP_JS"
test -f "$STYLES"

node --check "$APP_JS" >/dev/null

if grep -Eq '<style|<script>[[:space:]]*[^<]' "$INDEX"; then
    echo "index.html should not contain inline style/script blocks" >&2
    exit 1
fi

if grep -RInE 'https?://|cdn|telemetry|window\.prompt|window\.confirm' "$HTDOCS"; then
    echo "frontend should stay local-only and avoid prompt/confirm" >&2
    exit 1
fi

node - "$INDEX" "$APP_JS" <<'NODE'
const fs = require("fs");
const [indexPath, appPath] = process.argv.slice(2);
const index = fs.readFileSync(indexPath, "utf8");
const app = fs.readFileSync(appPath, "utf8");
const missing = [];
for (const match of app.matchAll(/\$\("([^"]+)"\)/g)) {
  const id = match[1];
  if (!index.includes(`id="${id}"`)) {
    missing.push(id);
  }
}
if (missing.length) {
  console.error(`missing DOM ids referenced by app.js: ${[...new Set(missing)].join(", ")}`);
  process.exit(1);
}
NODE

echo "webpanel frontend smoke passed"
