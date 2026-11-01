# Network Layer

## Goal

All backend traffic goes through a layered network stack. UI widgets never touch
`QNetworkAccessManager` directly. One `QNetworkAccessManager` per app (it pools
connections, caches, and handles cookies); never one per request. Add
`Qt6::Network` to CMake.

## Auth flow

```
login
  -> POST /rest/auth/login
  -> server returns access + refresh token (JSON)
  -> GET /ws/connect?token=<access_token>
  -> upgrade to WebSocket
```

- `POST /rest/auth/login` body carries the credentials; response JSON contains
  `access` and `refresh` tokens.
- The access token is then supplied as the `token` query parameter when opening
  the WebSocket via `GET /ws/connect?token=<access_token>`.
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
    WsClient.h          // QWebSocket wrapper for /ws/connect
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

Wraps `QWebSocket`. Opens `GET /ws/connect?token=<access_token>` (via
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

- [ ] Add `Qt6::Network` to `CMakeLists.txt` (`find_package` + link).
- [ ] Implement `ApiResult.h` / `ApiError`.
- [ ] Implement `HttpClient` (GET/POST, timeout, error mapping, bearer token).
- [ ] Implement `AuthApi::login()` -> `POST /rest/auth/login`, parse access/refresh.
- [ ] Add token storage (access + refresh).
- [ ] Implement `WsClient` -> `GET /ws/connect?token=<access>` and connect.
- [ ] Wire `Application::handleLogin` -> `AuthApi::login` -> `WsClient`.
- [ ] Handle `401` -> re-login/refresh.
