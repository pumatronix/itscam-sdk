# HTTPS / TLS

[Português (Brasil)](https-tls.md) | [English (US)](https-tls.en-US.md)

`ItscamRestClient` and `ItscamCgiClient` both speak HTTPS through a
statically-linked build of [mbedTLS](https://github.com/Mbed-TLS/mbedtls)
that ships inside `libitscam_sdk`.  There is **no system OpenSSL or
mbedTLS dependency** -- a single SDK binary is everything you need.

## What's vendored

The full mbedTLS source tree (currently **v3.6.2 LTS**) lives under
[`src/core/3rdparty/mbedtls/`](../src/core/3rdparty/mbedtls).  See
[`src/core/3rdparty/mbedtls/README.md`](../src/core/3rdparty/mbedtls/README.md)
for the version-policy and upgrade procedure.

The custom build configuration (`itscam_mbedtls_config.h`, also in that
directory) trims mbedTLS down to a TLS 1.2 / 1.3 **client** profile:
ECDHE-ECDSA, ECDHE-RSA, RSA-PSK; AEAD ciphers (AES-GCM, ChaCha20-Poly1305);
PEM / X.509 parsing; PSA crypto.  The resulting static contribution is
roughly 600 KB on ARM64 release builds.

## Configuration API (C++)

Identical knobs on both clients:

```cpp
itscam::ItscamCgiClient cgi;

// Switch to HTTPS by passing the scheme to setBaseUrl.
cgi.setBaseUrl("camera.example.com", 443, "https");

// Trust a custom CA bundle -- either a file or an in-memory PEM blob.
cgi.setCaCertFile("/etc/itscam/ca-bundle.pem");
// cgi.setCaCertData(R"(-----BEGIN CERTIFICATE-----\n...)");

// Toggle server cert verification.  Default: true.  Disable only for
// throw-away development.
cgi.setVerifyServerCertificate(false);

// Mutual TLS (rarely needed -- ITSCAM cameras don't require it).
cgi.setClientCertificate(clientPem, clientKeyPem);
```

The same setters exist on `ItscamRestClient` and are mirrored in every
language wrapper (`SetCaCertFile` / `set_ca_cert_file` / etc.).

## Auth notes

- **REST** requires authentication for every endpoint.  Call
  `rest.login(user, pass)` (or `rest.setAuthToken(jwt)`).
- **CGI** auth is gated by the camera's `configCgi.blockAPI` flag, which
  defaults to `false`.  Calling `cgi.login(...)` or
  `cgi.setBasicAuth(...)` is **optional**; skip it when the camera is in
  the default configuration.

## Failure modes

| Symptom                                          | Likely cause                                                        |
| ------------------------------------------------ | ------------------------------------------------------------------- |
| `Error::ConnectionFailed: TLS handshake failed`  | Wrong scheme/port, untrusted CA, expired cert, or TLS 1.0/1.1 server. |
| `Error::ConnectionFailed: connection failed`     | TCP refused -- check IP, port, firewall.                            |
| `Error::NotAuthenticated`                        | REST returned 401, or CGI auth turned on but no credentials passed. |
| Empty `mbedtls_*` symbols in `libitscam_sdk`     | mbedTLS sources were pruned -- the build silently falls back to     |
|                                                  | HTTP-only.  Restore `src/core/3rdparty/mbedtls/` from git.          |

## When to use HTTPS

| Scenario                                | Recommendation                              |
| --------------------------------------- | ------------------------------------------- |
| Lab / bench testing                      | HTTP is fine; `--insecure` for TLS testing. |
| Field deployment with a public PKI       | HTTPS, real CA, verification enabled.       |
| Closed network with self-signed certs    | HTTPS + `setCaCertData(...)` pinning.       |
| Cameras behind a TLS-terminating proxy   | HTTPS to the proxy, HTTP inside.            |
