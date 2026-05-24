# Error handling & async results

[Português (Brasil)](error-handling.md) | [English (US)](error-handling.en-US.md)

Every SDK method returns one of two value-or-error types: `Result<T>` for synchronous calls and `Future<T>` for asynchronous ones. Both are defined in [`src/core/itscam_types.h`](../src/core/itscam_types.h) and shared across `ItscamClient`, `ItscamRestClient` and `ItscamCgiClient`.

## `Result<T>`

```cpp
auto r = camera.authenticate("1234");
if (!r) {
    std::cerr << r.error().message << std::endl;
    return 1;
}
```

| Member            | Meaning                                                            |
| ----------------- | ------------------------------------------------------------------ |
| `operator bool()` | `true` on success, `false` on error.                               |
| `.value()`        | The payload when successful (lvalue / rvalue overloads).           |
| `.error()`        | An `Error` struct with `code` and `message`.                       |
| `Result<void>`    | A specialization for setter-style methods that have no payload.    |

`Error::Code` values:

| Code                | Typical cause                                                |
| ------------------- | ------------------------------------------------------------ |
| `Timeout`           | The operation didn't complete within the deadline.           |
| `ConnectionFailed`  | TCP / TLS connection could not be established.               |
| `Disconnected`      | The link went away during the call.                          |
| `NotAuthenticated`  | The server rejected the request (HTTP 401 or auth failure).  |
| `InvalidParameter`  | Bad input or HTTP 400 / 404 / 422.                           |
| `ServerError`       | HTTP 5xx (other than 503).                                   |
| `Unknown`           | Anything else.                                                |

## `Future<T>`

Long-running calls have `*Async()` variants that return a `Future<T>`:

```cpp
auto f = camera.captureSnapshotAsync(req);
// ... do other work ...
auto r = f.get();          // block until ready
auto r = f.get(5000);      // block up to 5 seconds
bool ready = f.isReady();  // non-blocking probe
```

Use `waitAll()` to wait on several futures at once:

```cpp
auto f1 = camera.captureSnapshotAsync(req1);
auto f2 = camera.captureSnapshotAsync(req2);
if (!waitAll(f1, f2)) {
    // at least one failed -- inspect each via f1.get(), f2.get()
}
```

## HTTP status mapping

The REST and CGI clients translate HTTP status codes into the same `Error::Code` taxonomy so wrappers and applications can switch on a single enum:

| HTTP status | `Error::Code`        |
| ----------- | -------------------- |
| `401`       | `NotAuthenticated`   |
| `400`, `422`| `InvalidParameter`   |
| `404`       | `InvalidParameter`   |
| `503`       | `ConnectionFailed`   |
| `5xx`       | `ServerError`        |
| Timeout     | `Timeout`            |
| Conn. fail  | `ConnectionFailed`   |

## Logging

Every client supports an optional log handler. Set it once early in your program; the SDK never owns stdout/stderr.

```cpp
rest.setLogHandler([](LogLevel lvl, const std::string& msg) {
    if (lvl == LogLevel::Error)
        std::cerr << "[REST ERROR] " << msg << '\n';
    else
        std::cout << "[REST] " << msg << '\n';
});
```

`LogLevel` values: `Debug`, `Info`, `Warn`, `Error`.

## Wrappers

Each language wrapper maps the table above into idiomatic constructs:

- **C# / .NET** raises an `ItscamException` hierarchy (`ItscamTimeoutException`, `ItscamAuthException`, ...) and `Task<T>` propagates them.
- **Python** raises `ItscamError` subclasses (`ItscamTimeoutError`, `ItscamAuthError`, `ItscamConnectionError`).
- **Go** returns a typed `error` whose underlying type carries the `Error::Code` value so callers can `errors.Is(err, itscam.ErrTimeout)`.

See the per-wrapper chapters for details.
