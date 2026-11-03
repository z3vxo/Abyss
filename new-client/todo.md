# Network Layer

## Goal

All backend traffic goes through a layered network stack. UI widgets never touch
`QNetworkAccessManager` directly. One `QNetworkAccessManager` per app (it pools
connections, caches, and handles cookies); never one per request.

The sections below describe a proposed future split. The existing
`NetworkService` already provides GET/POST, timeouts, bearer authentication,
login, agent listing, task submission, and listener creation; `WsClient` is
already connected after login.

## Auth flow

```
login
  -> POST /rest/auth/login
  -> server returns access + refresh token (JSON)
  -> GET /rest/ws/connect?token=<access_token>
  -> upgrade to WebSocket
```

- `POST /rest/auth/login` body carries the credentials; response JSON contains
  `access` and `refresh` tokens.
- The access token is then supplied as the `token` query parameter when opening
  the WebSocket via `GET /rest/ws/connect?token=<access_token>`.
- On `401` from any REST call, emit `unauthorized()` and re-enter the login flow
  (or refresh using the refresh token).

## Layout

```
includes/net/
    HttpClient.h        // low-level: QNAM wrapper, one per app
    ApiResult.h         // common result + error type
    AuthApi.h           // typed endpoint groups
    CoreApi.h
    dto/LoginDto.h      // request/response structs + from/toJson
    WsClient.h          // QWebSocket wrapper for /rest/ws/connect
src/net/
    HttpClient.cc
    AuthApi.cc
    CoreApi.cc
    WsClient.cc
```

## Components

### 1. HttpClient (transport only)

Owns the single `QNetworkAccessManager`. Handles base URL, headers, bearer
token, JSON encode/decode, timeout, retries, and maps failures into one
`ApiResult`. No business logic.

```cpp
struct ApiError {
    enum Kind { None, Network, Timeout, Http, Parse, Unauthorized };
    Kind kind = None;
    int status = 0;
    QString message;
};

struct ApiResult {
    QJsonDocument body;
    ApiError error;
    bool ok() const { return error.kind == ApiError::None; }
};

class HttpClient : public QObject {
    Q_OBJECT
public:
    explicit HttpClient(QUrl baseUrl, QObject* parent = nullptr);

    void setBearerToken(const QString& token);
    void get(const QString& path, std::function<void(const ApiResult&)> cb);
    void post(const QString& path, const QJsonObject& body,
              std::function<void(const ApiResult&)> cb);
    // put/patch/delete ...

signals:
    void unauthorized();   // 401 -> let app re-authenticate

private:
    QNetworkAccessManager m_nam;
    QUrl m_base;
    QString m_token;
};
```

Implementation notes:

- `QNetworkRequest::setTransferTimeout(15000)` instead of manual timers.
- Set `Http2AllowedAttribute` and `Content-Type: application/json`.
- In the `finished` lambda: read body, map status
  (`401` -> `Unauthorized`, `>=400` -> `Http`, reply error -> `Network`/
  `Timeout`), then `reply->deleteLater()`.
- Retry only idempotent methods (GET/HEAD) with capped exponential backoff.
- Never block; `QNAM` is async on the UI thread, no threads needed.

### 2. AuthApi (business layer)

Typed wrapper over `HttpClient`, returns parsed DTOs.

```cpp
struct LoginResponse { QString accessToken; QString refreshToken; };

class AuthApi {
public:
    explicit AuthApi(HttpClient& http) : m_http(http) {}
    void login(const QString& user, const QString& pass,
               const QString& domain, quint16 port,
               std::function<void(const ApiResult&, const LoginResponse&)> cb);
private:
    HttpClient& m_http;
};
```

- `login()` POSTs `/rest/auth/login`, parses `access`/`refresh` from the JSON,
  stores them (token store), and calls `HttpClient::setBearerToken(access)`.

### 3. DTOs

Plain structs with `fromJson`/`toJson`, kept out of widgets:

```cpp
struct LoginRequest  { QString username, password, domain; quint16 port; QJsonObject toJson() const; };
struct LoginResponse { QString accessToken, refreshToken; static LoginResponse fromJson(const QJsonObject&); };
```

### 4. WsClient (WebSocket)

Wraps `QWebSocket`. Opens `GET /rest/ws/connect?token=<access_token>` (via
`QWebSocket::open(QUrl)`), reconnects with backoff, and re-emits messages.

```cpp
class WsClient : public QObject {
    Q_OBJECT
public:
    explicit WsClient(QUrl baseUrl, QObject* parent = nullptr);
    void connectWithToken(const QString& accessToken);
    void disconnect();
signals:
    void connected();
    void disconnected();
    void messageReceived(const QJsonObject&);
    void errorOccurred(const QString&);
private:
    QWebSocket m_socket;
    QUrl m_base;
};
```

### 5. Wiring

Create `HttpClient` + services + `WsClient` once in `Application`, pass
references down. Keep the existing pattern: `LoginWindow` only emits
`loginRequest(...)`; `Application::handleLogin` calls `AuthApi::login(...)`,
then on success calls `WsClient::connectWithToken(...)` and switches to core.

## Concurrency / volume

- `QNAM` fans out requests in parallel; if firing hundreds, add a small queue in
  `HttpClient` to cap in-flight requests (e.g. 6-8).
- Avoid dangling callbacks: capture a `QPointer`/context object or use
  `QObject::connect` with a context so replies/callbacks don't outlive windows.

## TODO

- [ ] Split transport and endpoint logic into the proposed `HttpClient` and API
      layers, and introduce structured network, HTTP, timeout, and parse errors.
- [ ] Store and use the refresh token; handle `401` by refreshing or returning
      to login. Coordinate WebSocket reconnection with refreshed credentials.
- [ ] Fix the server authentication middleware to return immediately after an
      invalid token, before invoking a protected handler.
- [ ] Add WebSocket reconnection with backoff and recover missed events.
- [ ] Add retries for idempotent requests and consider an in-flight request cap.
- [ ] Review callback lifetimes and response parsing across the network layer.
- [ ] Test login, token expiry, reconnection, and network failure paths.

## Listeners

- [x] Wire the create-listener form to `POST /rest/listener/new`, including
      HTTP/HTTPS, paths, optional TLS files, and start-now.
- [ ] Test listener creation against the server, including self-signed and
      supplied certificates, validation failures, and start failures.
- [ ] Define callback address semantics and add server storage/API support.
      The client currently requires callbacks but the server request has no
      callback field, so the addresses are not sent or saved.
- [ ] Add a listeners list tab and load it on startup with
      `GET /rest/listener/list`; display and refresh listener status.
- [ ] Add listener deletion with `DELETE /rest/listener/delete/{id}`.
- [ ] Add listener start with `POST /rest/listener/start/{id}` and stop with
      `PATCH /rest/listener/stop/{id}`; reflect success and errors in the list.
- [ ] Handle partial creation when start-now fails: the server can save a
      listener before returning an error. Reconcile the list before retrying.

## Agent generation and listener profiles

Listener profiles define the HTTP-level settings used when generating an agent.
The intended flow is:

1. Create a listener and define its paths, headers, and callback addresses.
2. Save listener profiles locally so they can be selected for agent generation.
3. Press **Generate agent** to open a generation dialog.
4. Select a listener profile from a dropdown listing the available profiles.
5. Choose agent settings such as sleep and jitter, then press **Generate**.
6. Generate the agent using the selected listener profile and agent settings.

- [ ] Define the listener profile format, including paths, HTTP headers, and
      callback addresses, and implement local profile storage and retrieval.
- [ ] Add the missing listener-profile fields to the listener UI and reconcile
      locally stored profiles with listeners managed by the server.
- [ ] Build the Generate agent dialog with a listener-profile dropdown and
      agent settings, including sleep and jitter.
- [ ] Implement and connect the agent generation workflow, including input
      validation, result handling, and generated artifact delivery.

## Agents and tasks

- [x] Include the agent-table fields in `new-agent` and update the table without
      fetching the full agent list for every event.
- [ ] Test registration and task output end to end, including events arriving
      during the initial agent-list request and output for closed tabs.

## Files

- [ ] Implement the server file upload, listing, and download handlers and add
      client support when those workflows are ready.
