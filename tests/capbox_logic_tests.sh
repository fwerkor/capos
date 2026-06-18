#!/usr/bin/env bash

set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
CAPBOX_LIB_ONLY=1
source "$ROOT_DIR/package/capos/capbox/files/capbox"

TMPDIR="$(mktemp -d)"
trap 'rm -rf "$TMPDIR"' EXIT

export CAPBOX_STATE_DIR="$TMPDIR/state"
export CAPBOX_RUN_DIR="$TMPDIR/run"
export PATH="$TMPDIR/bin:$PATH"

mkdir -p "$TMPDIR/bin"
cat >"$TMPDIR/bin/apk" <<'APK'
#!/usr/bin/env sh
if [ "$1" = "info" ] && [ "$2" = "-e" ] && [ "$3" = "busybox" ]; then
  exit 0
fi
exit 1
APK
chmod +x "$TMPDIR/bin/apk"

fail() {
    echo "FAIL: $*" >&2
    exit 1
}

assert_ok() {
    ( "$@" ) || fail "expected success: $*"
}

assert_fail() {
    if ( "$@" ) >/dev/null 2>&1; then
        fail "expected failure: $*"
    fi
}

assert_contains() {
    local haystack="$1"
    local needle="$2"
    [[ "$haystack" == *"$needle"* ]] || fail "expected [$haystack] to contain [$needle]"
}

current_user="$(id -un)"
ensure_app_dirs "$current_user" sharedapp
cat >"$(app_manifest_json_path "$current_user" sharedapp)" <<'JSON'
{"version":"1.1","app":{"name":"sharedapp","version":"1.0"},"network":{},"container":{}}
JSON
cat >"$(app_meta_path "$current_user" sharedapp)" <<JSON
{"container_name":"$(container_name_for_app "$current_user" sharedapp)","network_name":"$(network_name_for_user "$current_user")","image_ref":"localhost/shared:latest"}
JSON

valid_manifest="$TMPDIR/valid.json"
cat >"$valid_manifest" <<'JSON'
{
  "version": "1.1",
  "app": {
    "name": "demo_app",
    "version": "2.3.4",
    "nickname": "Demo",
    "description": "Demo application"
  },
  "dependencies": {
    "apk": ["busybox"],
    "opkg": ["legacy-should-not-be-checked"],
    "capp": ["sharedapp"]
  },
  "network": {
    "publish": [
      "18080:80",
      {"listen": 18443, "target": 443, "proto": "tcp"}
    ],
    "service": {"http": 80}
  },
  "container": {
    "environment": {
      "PUID": "1000",
      "PGID": "1000"
    },
    "volumes": {
      "from": ["sharedapp:/data"]
    }
  },
  "host": {
    "exec": {
      "enabled": true,
      "allow": ["/bin/echo"]
    }
  }
}
JSON

invalid_env_manifest="$TMPDIR/invalid-env.json"
cat >"$invalid_env_manifest" <<'JSON'
{
  "version": "1.1",
  "app": {"name": "badenv", "version": "1.0"},
  "container": {"environment": ["BAD-NAME=value"]}
}
JSON

invalid_hostexec_manifest="$TMPDIR/invalid-hostexec.json"
cat >"$invalid_hostexec_manifest" <<'JSON'
{
  "version": "1.1",
  "app": {"name": "badexec", "version": "1.0"},
  "host": {"exec": {"enabled": true, "allow": ["echo unsafe"]}}
}
JSON

assert_ok validate_app_name "demo_app"
assert_fail validate_app_name "Demo-App"

publish_rows="$(manifest_publish_rules_tsv "$valid_manifest")"
assert_contains "$publish_rows" $'18080:80\ttcp\t18080\t80'
assert_contains "$publish_rows" $'{"listen":18443,"target":443,"proto":"tcp"}\ttcp\t18443\t443'

env_rows="$(manifest_environment_items "$valid_manifest")"
assert_contains "$env_rows" "PUID=1000"
assert_contains "$env_rows" "PGID=1000"

risks="$(manifest_risks_json "$valid_manifest")"
assert_contains "$risks" "apk_dependencies"
assert_contains "$risks" "app_dependencies"
assert_contains "$risks" "environment"
assert_contains "$risks" "shared_volumes"
assert_contains "$risks" "host_exec"

assert_ok validate_manifest_for_user "$valid_manifest" "$current_user"
assert_fail validate_manifest_for_user "$invalid_env_manifest" "$current_user"
assert_fail validate_manifest_for_user "$invalid_hostexec_manifest" "$current_user"

assert_ok validate_publish_mapping 18080 80 tcp "$current_user"
assert_ok validate_publish_mapping 5353 53 udp "$current_user"
assert_fail validate_publish_mapping 0 80 tcp "$current_user"
assert_fail validate_publish_mapping 18080 70000 tcp "$current_user"
assert_fail validate_publish_mapping 18080 80 sctp "$current_user"

ensure_app_dirs "$current_user" hostapp
cat >"$(app_manifest_json_path "$current_user" hostapp)" <<'JSON'
{"version":"1.1","app":{"name":"hostapp","version":"1.0"},"host":{"exec":{"enabled":true,"allow":["/bin/echo","relative"]}}}
JSON
cat >"$(app_meta_path "$current_user" hostapp)" <<JSON
{"container_name":"$(container_name_for_app "$current_user" hostapp)","network_name":"$(network_name_for_user "$current_user")","image_ref":"localhost/host:latest"}
JSON
write_app_hostexec "$current_user" hostapp "$(app_manifest_json_path "$current_user" hostapp)"
allowlist="$(cat "$(app_hostexec_path "$current_user" hostapp)")"
assert_contains "$allowlist" "/bin/echo"
[[ "$allowlist" != *"relative"* ]] || fail "relative hostexec command leaked into allowlist"
assert_ok hostexec_validate_request "$current_user" hostapp /bin/echo
assert_fail hostexec_validate_request "$current_user" hostapp /bin/sh

CAPBOX_HOSTEXEC_TIMEOUT=15
[[ "$(hostexec_timeout_seconds)" = "15" ]] || fail "expected explicit hostexec timeout"
CAPBOX_HOSTEXEC_TIMEOUT=999
[[ "$(hostexec_timeout_seconds)" = "10" ]] || fail "expected capped hostexec timeout fallback"

echo "capbox logic tests passed"
