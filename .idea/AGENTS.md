# Abyss — internal C2 framework (cyberCX red team)

Me and my co-workers started this to escape the cost and instability of commercial and
open-source C2s. It is an internal tool; keep the code clean, testable and easy to extend.

---

## 1. Architecture

Standard three-piece design:

| Piece | What it is | Where |
|-------|------------|-------|
| **Server / teamserver** | source of truth. Owns agents, listeners, tasks, files, operators. Exposes REST to operators and serves the agent transport. | `server/` |
| **Client** | operator-facing CLI (readline REPL). Talks to the teamserver over REST. | `client/` |
| **Agent** | endpoint implant. Talks to a listener's agent transport. | not in this repo yet |

The repo contains **two independent Go modules**:

- `github.com/z3vxo/Abyss/server` (`server/go.mod`)
- `github.com/z3vxo/Abyss/client` (`client/go.mod`)

They are separate modules on purpose. Do **not** import across them; if they ever need to
share protocol types, introduce a third module (e.g. `.../shared`) instead of a replace
directive or a root module.

Go version is **1.25.8** for both. Keep it there so the toolchain doesn't need downloading.

The server is deliberately transport-agnostic in its core: REST handlers are thin, listener
transports are pluggable, and DB access is behind the `db` package. UI/GUI/webUI/MCP are
future frontends over the same service layer.

```
operator CLI  --REST/JSON-->  teamserver  --agent transport-->  agent
                                   |
                                   +--> SQLite (~/.local/share/abyss/database/abyss.db)
```

---

## 2. Current state

Implemented and working:

- server setup (`setup`), YAML config, SQLite schema, JWT auth
- listener CRUD + start/stop/delete (HTTP and HTTPS, self-signed or base64 cert/key)
- listener persistence and reconciliation on server restart
- client login with retries, readline REPL, command dispatch
- client `listeners list|new|start|stop|delete`, `whoami`, `mv`
- task creation persists a command **code**; DB maps command names to codes

Stubs / not done yet:

- `server/internal/teamserver/ts_files.go` — all file endpoints are empty
- `server/internal/agentserver/httptransport` — placeholder handler; it reads an int32
  command type then replies `{"Status":"listener working!"}`. Host checking lives in
  `listeners.BuildServer`: each listener only accepts requests whose `Host` (port ignored)
  matches its configured `hostname`, otherwise 404.
- `agents list` exists (REST + CLI); no agent actions or selection yet (`AgentInUse` exists but nothing sets it)
- WebSocket endpoint exists (`/rest/ws/connect?token=...`); agent registration broadcasts a
  `new-agent` event. Client-side parsing/dispatch lives in `client/internal/ws` + `cli/events.go`.
- no MCP surface, no GUI/webUI
- no logging package; server uses `fmt.Printf`

---

## 3. Repository layout

```
.idea/                 local notes (gitignored): AGENTS.md, plan.md
client/
  cmd/main.go          entrypoint: flags -> cli.New -> cli.Run
  internal/
    cli/               REPL, command dispatch, command implementations
    client/            typed REST client (login, listeners, tasks)
    config/            client YAML config
    render/            pretty-print (go-pretty tables)
    term/              owns the readline instance; Write/Printf/Infof/Errorf/Clear
  abyss_conf.yaml      example operator config
  profiles/            example agent profile (future)
server/
  cmd/main.go          entrypoint: `setup` or `--profile <path>`
  internal/
    setup/             create data dirs/files, DB, base YAML config
    config/            server YAML config types + Load
    db/                SQLite connection, schema/migrations, listener + task queries
    auth/              operator credentials, JWT issue/verify, HTTP middleware
    listeners/         listener manager + lifecycle + HTTP(S) transport factory
    agentserver/       agent-side HTTP transport (placeholder)
    teamserver/        orchestration: wiring, chi routes, handlers
    shared/            cross-package request/response types
scripts/               local test scripts (gitignored), require a running server + jq
```

---

## 4. Server

### Entrypoint / startup

```
abyss setup                      -> setup.Run()
abyss --profile <config.yaml>    -> config.Load -> teamserver.New -> ts.Run
```

`setup.Run()`:

1. creates `~/.local/share/abyss/{database,config,logs}` (0700) and the DB/log files (0600)
2. runs `db.SetupDB` (schema migrations)
3. writes `config/teamserver.yaml` with generated JWT secrets and an initial operator

`teamserver.New` opens the DB, builds the auth service and the listener manager
(injecting the agent handler), then `ts.Run()` mounts routes, restores listeners from the
DB, and serves the operator API.

### Config (`server/internal/config`)

YAML produced by setup at `~/.local/share/abyss/config/teamserver.yaml`:

```yaml
team_server:
  bind_addr: 0.0.0.0
  port: 8443
  tls_enabled: true
  tls_cert: ""
  tls_key: ""
auth:
  users:
    - name: admin
      password: "..."
  jwt_access_secret: "..."
  jwt_refresh_secret: "..."
  jwt_access_ttl: 15m
  jwt_refresh_ttl: 168h
```

- TTLs are strings parsed with `time.ParseDuration` (fallbacks `15m` / `168h`).
- Operator passwords are plaintext on disk by design for now; verify uses
  `subtle.ConstantTimeCompare` on a `map[string]Operator`.
- The file is 0600 and the data dir 0700. Do not weaken that.

### Storage (`server/internal/db`)

SQLite via `modernc.org/sqlite` (pure Go, no cgo). Open with
`file:<path>?_pragma=foreign_keys(1)` so FKs are enabled on every pooled connection.

Tables:

- `agents` — `AgentId` (unique), name, last seen, `PID/TID/PPID/Arch`, user/host/domain/OS/IP
- `listeners` — `ListenerID` (UUIDv7 string), type, name (unique), bind, port,
  status (text), hostname, TLS cert/key; `UNIQUE (ListenerBind, ListenerPort)`
- `http_listener_info` — child rows for a listener: `Kind` = `path` | `header`,
  `Value`, `Position`; FK to `listeners(ListenerID)` with `ON DELETE CASCADE`
- `files` — metadata; FK to `agents(id)`
- `tasks` — `TaskID` (int), `AgentID`, `CommandCode` (int), `Args` (JSON text),
  status, output, timestamps

Rules:

- migrations are `CREATE TABLE IF NOT EXISTS` in `db.go`; they are append-only. Changing a
  table requires deleting the dev DB (`~/.local/share/abyss/database/abyss.db`) or writing
  a real migration. There is no migration versioning yet.
- listener statuses are text: `paused`, `starting`, `running`, `stopping`, `stopped`, `error`.
- `tasks.Status` is still partly inconsistent (column is `INTEGER`, code writes `"queued"`
  and reads it as a string). Standardise this if you touch tasks.
- `db` must not depend on `listeners` or `teamserver`; keep it a leaf package.

### Listener lifecycle (`server/internal/listeners`)

`Manager` (`Listeners`) owns runtime listeners under a mutex:

- `New(req)` — validate, build TLS config + `http.Server`, generate UUIDv7, store `paused`
- `Restore(req, id)` — same but reuses a persisted UUID (startup reconciliation)
- `Start(id)` — `net.Listen` synchronously (so bind errors surface), then serve in a
  goroutine; `https` uses `ServeTLS` with the stored config
- `Stop(id, ctx)` — graceful `Shutdown`; the server object is discarded so a later start
  rebuilds it (Go servers cannot be reused after `Shutdown`)
- `Remove(id)` — only when not running/starting/stopping
- `List()` — snapshots (`ListenerInfo`), sorted by ID; never expose live pointers

The manager takes an `AgentHandler` at `Create`; it does not import `db`. The HTTP(S)
transport factory (`builder.go`) validates paths, dedupes them, uses
`ReadHeaderTimeout`/`IdleTimeout` (not total read/write timeouts, which would break
long-poll and uploads), and supports:
- `http`: no TLS
- `https` with no cert/key: self-signed (SANs for hostname/IP)
- `https` with both `tls_cert`/`tls_key`: base64-decoded PEM keypair

### Persistence and startup reconciliation

`teamserver.CreateListener` is the single create path:

1. `ListenerMgr.New` (runtime, UUID)
2. `DB.InsertListener` (listener row + path/header rows in one tx)
3. on DB failure, `ListenerMgr.Remove` rolls back the runtime object
4. if `start_now`, `Start` then write status `running`/`error`

On boot, `startListeners` reads **all** listener rows, restores each into the manager, and
only auto-starts rows whose persisted status is `running`/`starting`. Paused/stopped rows
still appear in `list` and can be started later.

Deleting a listener removes it from the manager and DB; `http_listener_info` cascades.

### REST API (operator)

All under `/rest`; everything except login/refresh requires
`Authorization: Bearer <access>`.

| Method | Path | Body / notes |
|--------|------|--------------|
| POST | `/rest/auth/login` | `{"username","password"}` → `{"access","refresh"}` |
| POST | `/rest/auth/refresh` | `{"refresh"}` → new tokens |
| GET | `/rest/listener/list` | → `[{"id","name","type","bind_addr","port","status"}]` |
| POST | `/rest/listener/new` | `NewListenerReq` → `{"id"}` |
| POST | `/rest/listener/start/{id}` | → `{"started": id}` |
| PATCH | `/rest/listener/stop/{id}` | → `{"stopped": id}` |
| DELETE | `/rest/listener/delete/{id}` | → `{"deleted": id}` |
| GET | `/rest/agents/list` | → `{total, agent:[...]}` |
| POST | `/rest/tasks/new` | `{"agent_id","command","args":[...]}` → `{"task_id"}` |
| POST | `/rest/tasks/list` | `{"agent_id"}` → `{total, tasks}` |
| POST | `/rest/tasks/delete` | `{"task_id"}` |
| | `/rest/files/...` | stubs |

`NewListenerReq`:

```json
{
  "name": "web",
  "listener_type": "http|https",
  "start_now": true,
  "http_listener_info": {
    "bind_addr": "127.0.0.1",
    "port": 9001,
    "hostname": "127.0.0.1",
    "http_paths": ["/test"],
    "http_response_headers": [],
    "tls_cert": "",
    "tls_key": ""
  }
}
```

### Tasks

The client sends the command **name**; the DB maps it to the wire code in
`db.CommandToCodeMap` (`whoami: 0x1`, `cat: 0x2`, `mv: 0x3`). Unknown names fail with
`invalid command name` (HTTP 400). `args` is a list of `{"data_type","arg"}` (e.g.
`{"data_type":"string","arg":"C:\\a.txt"}`) and is stored as JSON.

### Auth

- HS256 JWTs, separate access/refresh secrets, access and refresh claims include username,
  ID, issued-at, expiry.
- `AuthWare` requires `Bearer <access>`, verifies, and puts claims in the request context.
- Credential check is constant-time over a `map[string]Operator`.
- On failure `AuthWare` must write 401 **and return** (don't fall through with nil claims).

---

## 5. Client

### Startup / wiring

```
main (pflag: -p/--profile, -d/--debug)
  -> config.New(path, debug)         # YAML
  -> cli.New
       -> client.New(cfg)            # builds base URL, logs in + dials WS (3 retries)
       -> term.New(prompt)           # readline instance
       -> register commands
  -> cli.Run                         # REPL loop
```

`client.New` logs in, then dials `/rest/ws/connect?token=<access>` and starts the read loop
in a goroutine. It prints `[*] Connected to teamserver` once both succeed and retries the
login + WS handshake up to 3 times (`[!] Failed connecting to teamserver, retrying...`,
2s apart). 4xx login fails immediately. These prints happen before readline exists, so
plain `fmt` is fine.

### Packages and responsibilities

- `internal/cli` — REPL, dispatch, commands. Owns the last listener snapshot and
  `AgentInUse`. Returns errors; the REPL prints them.
- `internal/client` — typed REST client. Owns base URL, `http.Client`, tokens (mutex),
  `do()` (JSON, Bearer, status → `APIError`), and endpoint methods.
- `internal/term` — the only code allowed to touch the terminal. Owns the single
  readline instance, serializes writes, and redraws the prompt via `Clean -> write ->
  Refresh`. Provides `Printf`, `Infof` (`[*]`), `Warnf` (`[~]`), `Errorf` (`[!]`, only the
  tag is red), `Clear`, `Close`.
- `internal/render` — pretty printing (go-pretty). Takes DTOs, returns a string.
- `internal/config` — YAML connection/logging config.

### Error and output conventions

- Errors are values. Packages return errors; **only the REPL boundary prints them**
  (`c.term.Errorf("%v", err)`).
- Don't bake `[!]`/`[*]` into error strings; `term.Errorf`/`Infof` add the tag.
- Never `fmt.Print`/`log.Print` while readline is active — it corrupts the prompt.
- Fatal startup errors are the only `log.Fatal` (in `main`, before the REPL starts).
- Pretty output: build the whole table in `render`, then write it in one call.

### Command dispatch

`CommandFunc func(args []string) error`; `Parse` shlex-splits input, lowercases the command,
looks it up, and calls it with `values[1:]` (empty slice for zero-arg commands).
Commands validate their own arity, e.g. `mv` requires exactly 2 args.

Current commands: `agents list`, `use <name|id|index>`, `listeners` (with
`list|new|start|stop|delete`), `whoami`, `mv`. `agents list` caches a snapshot; `use`
resolves `#1`/`1`, name, or id and sets `AgentInUse` to the real agent id.

### Listener references (UX)

`listeners list` renders a 1-based `#` column and stores the returned snapshot in the CLI.
`start|stop|delete` accept:
1. `#2` or `2` — index into the last snapshot (requires a prior `list`),
2. exact name,
3. full UUID (name/ID falls back to a fresh fetch if the snapshot is missing them).

The index is presentation-only; the real UUID is always sent to the server. Numeric names
and names starting with `#` are rejected at creation to keep resolution unambiguous.
The snapshot only refreshes on `list` (no background refresh).

### Client config (`client/abyss_conf.yaml`)

```yaml
connection:
  host: "http://127.0.0.1"   # scheme included
  port: 8080
  username: "abyss"
  password: "abyss-operator"
logging:
  level: "info"
  logfile: "./logs/abyss.log"
```

`host` may include a scheme; `client.buildBaseURL` adds `port` only if the host has none.

---

## 6. Build / run

```sh
# server
cd server
go build ./...
go run ./cmd setup
go run ./cmd --profile ~/.local/share/abyss/config/teamserver.yaml

# client
cd client
go build ./...
go run ./cmd -p abyss_conf.yaml
```

There is no CI, Makefile, linter config or test suite yet. If you add tests, use the
standard `go test ./...` in the relevant module. Temporary verification tests during
development should be removed before finishing.

Local scripts (`scripts/`) are gitignored and assume the server is on `127.0.0.1:8080`
and `jq` is installed.

---

## 7. Conventions and rules for agents

- Match existing style; do not add comments unless asked.
- Keep the two modules separate; no cross-imports.
- Keep `listeners` free of `db`/`teamserver` imports; inject interfaces.
- Keep operator REST and the agent listener on separate servers/ports/middleware.
- Never expose live runtime pointers from the manager; return snapshots.
- Generate listener IDs with UUIDv7, server-side; never trust client-supplied IDs.
- Persist status transitions in the DB (create/start/stop/delete), and keep the DB the
  source of truth for what exists. In-memory state is the runtime view and must mirror it.
- Don't reintroduce `ReadTimeout`/`WriteTimeout` on agent listeners (breaks long-poll and
  file transfer); use `ReadHeaderTimeout`.
- Use `net.JoinHostPort` for addresses.
- DB changes: update the migration in `db.go`; tell the user to recreate the dev DB.
- If you change request/response JSON, update both `shared` types and the client DTOs,
  and keep the server's `shared` types and client DTOs in sync.
- Prefer returning errors over printing inside library packages; print at the REPL boundary.
