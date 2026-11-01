# AGENTS.md

## What this is

`Abyss-client` is a Qt 6 (C++17) desktop client. It has a login dialog, then a
main window ("core") with a user list and an embedded command shell. Backend
communication goes over HTTP and (planned) a WebSocket. The app also carries
desktop-recon tooling for a bug-bounty workflow, described in the project root
instructions; keep that scope in mind when touching target/backend code.

## Build & run

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build
./build/abyss-client          # run from the project root
```

- **Run from the repo root.** All assets are loaded with paths relative to the
  current working directory (`./resc/ui/*.ui`, `./resc/qss/core.qss`).
- Executable: `build/abyss-client`.
- Requirements: CMake ≥ 3.16, Qt 6 (`Widgets`, `Network`, `UiTools`), C++17.
- `CMAKE_EXPORT_COMPILE_COMMANDS=ON` writes `build/compile_commands.json`, used
  by clangd/Zed. `.clangd` points clangd at it.
- There is a stale `./abyss-client` artifact in the repo root — do not run it;
  rebuild/run `build/abyss-client`.

## Layout

```
main.cc                     entry point: constructs Application, runs event loop
includes/
  app/app.h                 Application (QApplication subclass) + members
  net/net.h                 NetworkService, ApiResult, LoginResponse
  windows/login/login.h     LoginWindow
src/
  app/app.cc                Application: stylesheet, login<->core flow, shell
  net/net.cc                HTTP (QNetworkAccessManager), login endpoint
  windows/login.cc          loads login.ui, wires widgets, emits loginRequest
resc/
  ui/login.ui               login dialog (loaded at runtime via QUiLoader)
  ui/core.ui                main window (loaded at runtime via QUiLoader)
  qss/core.qss              global stylesheet applied at startup
todo.md                     planned network/layering tasks (incl. WebSocket)
```

## Architecture

`Application` is the controller and the only long-lived object. It owns:

- a value member `NetworkService m_net`
- `LoginWindow* m_login`, loaded from `login.ui`
- `QWidget* m_core`, loaded from `core.ui`
- shell widget pointers (`m_commandInput`, `m_shellOutput`)

Flow:

```
Application ctor
  -> initialise()          apply core.qss to the whole app
  -> showLogin()           load login.ui, connect loginRequest -> handleLogin
User submits credentials
  -> LoginWindow::onDoneClicked()  validate, emit loginRequest(...)
  -> Application::handleLogin()    setEndpoint(domain,port); m_net.login(...)
       success -> showCore()       load core.ui, connect shell, close login
       failure -> message box      "Failed to connect"/"Invalid credentials"
```

UI never talks to the network layer directly. Views emit signals; `Application`
calls `NetworkService`. Keep it that way.

## Network layer (`NetworkService`)

- Owns one `QNetworkAccessManager` (connection pooling/caching). Do not create
  per-request managers.
- `setEndpoint(domain, port)` builds `https://domain:port` (the base URL).
  `domain:port` identifies the backend; it is NOT sent in the login JSON.
- `login(user, pass, cb)` POSTs JSON `{username,password}` to
  `/rest/auth/login`, parses `access`/`refresh` from the response, keeps the
  access token, and invokes `cb(ApiResult, LoginResponse)`.
- `getAgents(cb)` GETs `/rest/agents/list`, parses `{total, agent:[...]}`
  into `ListClientsResponse { int total; QVector<AgentInfo> agents; }`, and
  `Application::showCore()` renders it into `usersTable` (Name, Username,
  Hostname, Domain, Process, PID, Last Seen).
- `ApiResult { bool ok; int status; QString error; QJsonObject body; }`.
  `ok` is a **field**, not a method (`r.ok`, never `r.ok()`).
- Error mapping used by the app: `status == 0` → connection failure,
  `status == 401` → invalid credentials.
- `QNetworkAccessManager::sslErrors` is connected once and **always** calls
  `ignoreSslErrors()` (self-signed certs are accepted; dev convenience).
- Callbacks are `std::function`, invoked on the main thread when the reply
  finishes. Always `deleteLater()` replies (already done in `post()`).
- Planned: WebSocket via `GET /ws/connect?token=<access_token>`; see `todo.md`.
  Adding it requires `Qt6::WebSockets` in CMake and a `QWebSocket` member.

## Qt conventions in this repo

- UI is loaded dynamically with `QUiLoader` from `.ui` files, not compiled with
  `uic`. Widgets are found by `objectName` via `findChild<T*>("...")`.
- Signals/slots use the pointer-to-member syntax. `LoginWindow` emits
  `loginRequest`; `Application` handles it. Views do not own the network layer.
- `Q_OBJECT` classes (headers) must be listed in `CMakeLists.txt` target sources
  so AUTOMOC generates their moc code — otherwise you get "undefined reference
  to vtable/staticMetaObject".
- Connection lifetime is tied to the receiver/context `QObject`; widgets loaded
  from `.ui` are children of the loaded window, so connections die with it.
- Keep `.ui` `<connections>` minimal: `QUiLoader` honors them, which can close
  dialogs unexpectedly (the `doneButton -> accept()` connection was removed for
  this reason).

## Current UI details

- `login.ui` object names: `usernameEdit`, `passwordEdit`, `hostEdit`,
  `portEdit`, `doneButton`, `exitButton`. `LoginWindow` binds these exactly.
- `core.ui` object names: `shellTabs` (browser-style `QTabWidget`),
  `usersTable` (user list).
- `usersTable` is a `QTableWidget` with 7 columns set in
  `Application::showCore()`: Name, Username, Hostname, Domain, Process, PID,
  Last Seen. The agent id is stored on the Name item under `Qt::UserRole`.
- Double-clicking a row calls `openAgentTab()`: one shell tab per agent (own
  `QPlainTextEdit` + `QLineEdit`, built in `addShellTab()`); clicking a tab
  switches to that agent. Tabs are closable; `closeShellTab()` deletes them.
  A lazily-created `console` tab (agent id `-1`) receives app messages.
- Shell Enter appends `"$ <input>"` to that tab's output; not yet sent to the
  backend (TODO in the tab's input handler).

## Conventions & gotchas

- Qt headers: use `<QApplication>` / `<QNetworkAccessManager>` etc. and rely on
  CMake's Qt targets, not hard-coded paths like `<qt6/...>`.
- Qt is case-sensitive: `QFile::open` / `readAll`, `QMessageBox::warning`.
- No build system-less editing: after adding source/header files, add them to
  `CMakeLists.txt` and re-run `cmake -S . -B build`.
- Build artifacts go under `build/`; do not commit binaries in the root.
- The `.qss` parser rejects malformed comments; the header must open with `/*`.

## Checks before finishing

```bash
cmake --build build     # must compile and link clean
```

There is no test suite or linter configured. Verify UI changes by running
`./build/abyss-client` from the repo root against the backend.
