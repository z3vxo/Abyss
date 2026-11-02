#!/usr/bin/env bash
set -euo pipefail

# Usage: ABYSS_USERNAME=... ABYSS_PASSWORD=... ../test.sh
# Optional: ABYSS_TEAMSERVER_URL, ABYSS_AGENT_URL, ABYSS_TEST_OUTPUT, ABYSS_CA_FILE.
# Omitting ABYSS_PASSWORD prompts securely. A CA file enables certificate validation;
# otherwise the HTTPS client accepts the team's self-signed certificate.
exec python3 - "$@" <<'PY'
import getpass
import json
import os
import ssl
import struct
import sys
from urllib.error import HTTPError, URLError
from urllib.parse import urlsplit
from urllib.request import Request, urlopen

AGENT_ID = 940707469
teamserver = os.environ.get("ABYSS_TEAMSERVER_URL", "https://192.168.1.18:8080").rstrip("/")
agent_url = os.environ.get("ABYSS_AGENT_URL", "http://127.0.0.1:8081/test")
username = os.environ.get("ABYSS_USERNAME", "abyss")
password = os.environ.get("ABYSS_PASSWORD")
output = os.environ.get("ABYSS_TEST_OUTPUT", "Test output from whoami")

if urlsplit(teamserver).scheme.lower() != "https":
    sys.exit("Teamserver URL must use https://")
if not username:
    sys.exit("ABYSS_USERNAME must not be empty")
if password is None:
    password = getpass.getpass("Teamserver password: ")
if not password:
    sys.exit("Password must not be empty")

ca_file = os.environ.get("ABYSS_CA_FILE")
context = ssl.create_default_context(cafile=ca_file) if ca_file else ssl._create_unverified_context()


def post(url, data, content_type, token=None, tls_context=None):
    headers = {"Content-Type": content_type}
    if token:
        headers["Authorization"] = "Bearer " + token
    request = Request(url, data=data, headers=headers, method="POST")
    try:
        with urlopen(request, timeout=15, context=tls_context) as response:
            return response.read()
    except HTTPError as exc:
        detail = exc.read().decode("utf-8", errors="replace")
        raise RuntimeError(f"HTTP {exc.code} from {urlsplit(url).path}: {detail}") from exc
    except URLError as exc:
        raise RuntimeError(f"Could not reach {urlsplit(url).hostname}: {exc.reason}") from exc


def json_post(path, payload, token=None):
    response = post(teamserver + path, json.dumps(payload).encode(), "application/json", token, context)
    try:
        result = json.loads(response)
    except (ValueError, UnicodeDecodeError) as exc:
        raise RuntimeError(f"Invalid JSON from {path}") from exc
    if not isinstance(result, dict):
        raise RuntimeError(f"Expected a JSON object from {path}")
    return result


try:
    login = json_post("/rest/auth/login", {"username": username, "password": password})
    token = login.get("access")
    if not isinstance(token, str) or not token:
        raise RuntimeError("Login response did not contain an access token")
    print("[*] Logged in to teamserver", flush=True)

    task = json_post("/rest/tasks/new", {"agent_id": AGENT_ID, "command": "whoami", "args": []}, token)
    task_id = task.get("task_id")
    if isinstance(task_id, bool) or not isinstance(task_id, int) or not -(2**31) <= task_id < 2**31:
        raise RuntimeError("Task response did not contain a valid int32 task_id")
    print(f"[*] Created whoami task {task_id} for agent {AGENT_ID}", flush=True)

    raw_output = output.encode("utf-8")
    if len(raw_output) > 2**31 - 1:
        raise RuntimeError("Test output is too large")
    # int32 little-endian: message type, agent ID, count, task ID, raw-output type, byte length.
    payload = struct.pack("<6i", 3, AGENT_ID, 1, task_id, 4, len(raw_output)) + raw_output
    post(agent_url, payload, "application/octet-stream")
    print(f"[+] Posted test output for task {task_id}")
except (RuntimeError, OSError, ValueError) as exc:
    sys.exit(f"[!] {exc}")
PY
