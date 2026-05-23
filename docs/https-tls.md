# HTTPS / TLS

[Português (Brasil)](https-tls.md) | [English (US)](https-tls.en-US.md)

`ItscamRestClient` e `ItscamCgiClient` falam HTTPS através de um build estaticamente linkado de [mbedTLS](https://github.com/Mbed-TLS/mbedtls) embutido em `libitscam_sdk`. **Não há dependência de OpenSSL ou mbedTLS do sistema** -- o binário do SDK já é tudo o que você precisa.

## O que está vendored

A árvore completa do mbedTLS (atualmente **v3.6.2 LTS**) fica em [`src/core/3rdparty/mbedtls/`](../src/core/3rdparty/mbedtls). Veja [`src/core/3rdparty/mbedtls/README.md`](../src/core/3rdparty/mbedtls/README.md) para a version policy e o procedimento de upgrade.

A configuration custom (`itscam_mbedtls_config.h`, no mesmo diretório) enxuga o mbedTLS para um perfil **client** TLS 1.2 / 1.3: ECDHE-ECDSA, ECDHE-RSA, RSA-PSK; AEAD ciphers (AES-GCM, ChaCha20-Poly1305); parsing PEM / X.509; PSA crypto. A contribuição estática resultante é de aproximadamente 600 KB em release builds ARM64.

## API de configuration (C++)

Knobs idênticos nos dois clients:

```cpp
itscam::ItscamCgiClient cgi;

// Liga HTTPS passando o scheme para setBaseUrl.
cgi.setBaseUrl("camera.example.com", 443, "https");

// Confia em um CA bundle custom -- file ou PEM blob em memória.
cgi.setCaCertFile("/etc/itscam/ca-bundle.pem");
// cgi.setCaCertData(R"(-----BEGIN CERTIFICATE-----\n...)");

// Liga/desliga verificação do server cert. Default: true. Desabilite
// só em ambiente de desenvolvimento descartável.
cgi.setVerifyServerCertificate(false);

// Mutual TLS (raramente necessário -- câmeras ITSCAM não exigem).
cgi.setClientCertificate(clientPem, clientKeyPem);
```

Os mesmos setters existem em `ItscamRestClient` e estão espelhados em todos os language wrappers (`SetCaCertFile` / `set_ca_cert_file` / etc.).

## Notas de auth

- **REST** exige autenticação em todo endpoint. Chame `rest.login(user, pass)` (ou `rest.setAuthToken(jwt)`).
- **CGI** auth é gateado pelo flag `configCgi.blockAPI` da câmera, que por default é `false`. Chamar `cgi.login(...)` ou `cgi.setBasicAuth(...)` é **opcional**; pule esse passo enquanto a câmera estiver na configuration default.

## Failure modes

| Sintoma                                          | Causa provável                                                       |
| ------------------------------------------------ | -------------------------------------------------------------------- |
| `Error::ConnectionFailed: TLS handshake failed`  | Scheme/porta errados, CA não confiável, cert expirado ou server TLS 1.0/1.1. |
| `Error::ConnectionFailed: connection failed`     | TCP recusou -- cheque IP, porta, firewall.                           |
| `Error::NotAuthenticated`                        | REST devolveu 401, ou CGI auth está ligado mas nenhuma credential foi passada. |
| Símbolos `mbedtls_*` vazios em `libitscam_sdk`   | Sources do mbedTLS foram removidos -- o build cai silenciosamente para HTTP-only. Restaure `src/core/3rdparty/mbedtls/` do git. |

## Quando usar HTTPS

| Cenário                                  | Recomendação                                |
| ---------------------------------------- | ------------------------------------------- |
| Lab / bench testing                       | HTTP serve; `--insecure` para testar TLS.   |
| Field deployment com PKI pública          | HTTPS, CA real, verificação habilitada.     |
| Rede fechada com certs self-signed        | HTTPS + `setCaCertData(...)` para pinning.  |
| Câmeras atrás de proxy com TLS termination| HTTPS até o proxy, HTTP por dentro.         |
