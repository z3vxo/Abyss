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


echo "[*] Inserting New Task.."

TASK_ID=$(curl -sS -H "Authorization: bearer $token" "$BASE_URL/rest/tasks/new" -X POST -d '{"agent_id":11111, "command_id":1, "args":[{"data_type":"string", "arg":"test"}]}' | jq -r '.task_id')
BODY=$(jq -n --arg k "task_id" --argjson v "$TASK_ID" '{($k): $v}')
echo "[*] Listing Tasks"


curl -H "Authorization: bearer $token" -sS "$BASE_URL/rest/tasks/list" -d '{"agent_id":11111}' | jq




curl -sS -H "Authorization: bearer $token" "$BASE_URL/rest/tasks/delete" -X POST -d "$BODY"
