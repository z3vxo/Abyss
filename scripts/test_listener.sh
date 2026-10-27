#!/usr/bin/env bash
set -euo pipefail

BASE_URL="${BASE_URL:-http://127.0.0.1:8080}"
OPERATOR_USER="${OPERATOR_USER:-abyss}"
OPERATOR_PASS="${OPERATOR_PASS:-abyss-operator}"

LISTENER_NAME="${LISTENER_NAME:-test-http}"
LISTENER_TYPE="${LISTENER_TYPE:-https}"
LISTENER_BIND="${LISTENER_BIND:-127.0.0.1}"
LISTENER_PORT="${LISTENER_PORT:-9001}"
LISTENER_HOST="${LISTENER_HOST:-127.0.0.1}"
LISTENER_PATH="${LISTENER_PATH:-/api/v1/poll}"
PROBE_HOST="${PROBE_HOST:-127.0.0.1}"

command -v curl >/dev/null || { echo "[!] curl is required" >&2; exit 1; }
command -v jq >/dev/null || { echo "[!] jq is required" >&2; exit 1; }

echo "[*] Logging in as ${OPERATOR_USER} at ${BASE_URL}"
login=$(curl -sS "${BASE_URL}/rest/auth/login" \
	-H 'Content-Type: application/json' \
	-d "$(jq -n --arg u "$OPERATOR_USER" --arg p "$OPERATOR_PASS" '{username:$u,password:$p}')") \
	|| { echo "[!] could not reach ${BASE_URL}" >&2; exit 1; }

token=$(jq -r '.access // empty' <<<"$login")
if [[ -z "$token" ]]; then
	echo "[!] login failed: ${login}" >&2
	exit 1
fi

echo "[*] Creating ${LISTENER_TYPE} listener '${LISTENER_NAME}' on ${LISTENER_BIND}:${LISTENER_PORT} (start_now=true)"

body=$(jq -n \
	--arg name "$LISTENER_NAME" \
	--arg type "$LISTENER_TYPE" \
	--arg bind "$LISTENER_BIND" \
	--arg host "$LISTENER_HOST" \
	--arg path "$LISTENER_PATH" \
	--argjson port "$LISTENER_PORT" \
	'{
		name: $name,
		listener_type: $type,
		start_now: true,
		http_listener_info: {
			bind_addr: $bind,
			port: $port,
			hostname: $host,
			http_paths: [$path],
			http_response_headers: [],
			tls_key: "",
			tls_cert: ""
		}
	}')

resp=$(curl -sS -w $'\n%{http_code}' "${BASE_URL}/rest/listener/new" \
	-H "Authorization: Bearer ${token}" \
	-H 'Content-Type: application/json' \
	-d "$body") || { echo "[!] listener request failed" >&2; exit 1; }

status=$(tail -n1 <<<"$resp")
payload=$(sed '$d' <<<"$resp")

if [[ "$status" != "201" ]]; then
	echo "[!] listener creation failed (HTTP ${status}): ${payload}" >&2
	exit 1
fi

id=$(jq -r '.id // empty' <<<"$payload")
echo "[+] Listener created: ${id}"

scheme=http
[[ "$LISTENER_TYPE" == "https" ]] && scheme=https

echo "[*] Probing ${scheme}://${PROBE_HOST}:${LISTENER_PORT}${LISTENER_PATH}"
curl -sSk -o /dev/null -w '    HTTP %{http_code}\n' \
	-X POST "${scheme}://${PROBE_HOST}:${LISTENER_PORT}${LISTENER_PATH}"
