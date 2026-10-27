#!/usr/bin/env bash
set -euo pipefail

BASE_URL="${BASE_URL:-http://127.0.0.1:8080}"
OPERATOR_USER="${OPERATOR_USER:-abyss}"
OPERATOR_PASS="${OPERATOR_PASS:-abyss-operator}"

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

resp=$(curl -sS -w $'\n%{http_code}' "${BASE_URL}/rest/listener/list" \
	-H "Authorization: Bearer ${token}") || { echo "[!] listener request failed" >&2; exit 1; }

status=$(tail -n1 <<<"$resp")
payload=$(sed '$d' <<<"$resp")

if [[ "$status" != "200" ]]; then
	echo "[!] listing listeners failed (HTTP ${status}): ${payload}" >&2
	exit 1
fi

if ! jq -e 'type == "array"' >/dev/null 2>&1 <<<"$payload"; then
	echo "[!] unexpected response: ${payload}" >&2
	exit 1
fi

count=$(jq 'length' <<<"$payload")
echo "[+] ${count} listener(s)"
jq '.' <<<"$payload"
