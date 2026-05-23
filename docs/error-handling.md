# Error handling e async results

[Português (Brasil)](error-handling.md) | [English (US)](error-handling.en-US.md)

Todo método do SDK devolve um de dois tipos value-or-error: `Result<T>`
para chamadas síncronas e `Future<T>` para assíncronas. Os dois estão
definidos em
[`src/core/itscam_types.h`](../src/core/itscam_types.h) e são
compartilhados por `ItscamClient`, `ItscamRestClient` e
`ItscamCgiClient`.

## `Result<T>`

```cpp
auto r = camera.authenticate("1234");
if (!r) {
    std::cerr << r.error().message << std::endl;
    return 1;
}
```

| Membro            | Significado                                                        |
| ----------------- | ------------------------------------------------------------------ |
| `operator bool()` | `true` em sucesso, `false` em erro.                                |
| `.value()`        | O payload em sucesso (overloads lvalue / rvalue).                  |
| `.error()`        | Struct `Error` com `code` e `message`.                             |
| `Result<void>`    | Especialização para setters que não devolvem payload.              |

Valores de `Error::Code`:

| Code                | Causa típica                                                |
| ------------------- | ------------------------------------------------------------ |
| `Timeout`           | A operação não completou dentro do deadline.                |
| `ConnectionFailed`  | Conexão TCP / TLS não pôde ser estabelecida.                |
| `Disconnected`      | O link caiu durante a chamada.                               |
| `NotAuthenticated`  | O server rejeitou a request (HTTP 401 ou auth failure).     |
| `InvalidParameter`  | Input ruim ou HTTP 400 / 404 / 422.                         |
| `ServerError`       | HTTP 5xx (exceto 503).                                       |
| `Unknown`           | Qualquer outro caso.                                          |

## `Future<T>`

Chamadas long-running têm variantes `*Async()` que devolvem um
`Future<T>`:

```cpp
auto f = camera.captureSnapshotAsync(req);
// ... faça outras coisas ...
auto r = f.get();          // bloqueia até estar pronto
auto r = f.get(5000);      // bloqueia até 5 segundos
bool ready = f.isReady();  // probe não-bloqueante
```

Use `waitAll()` para esperar várias futures de uma vez:

```cpp
auto f1 = camera.captureSnapshotAsync(req1);
auto f2 = camera.captureSnapshotAsync(req2);
if (!waitAll(f1, f2)) {
    // pelo menos uma falhou -- inspecione cada via f1.get(), f2.get()
}
```

## Mapeamento de HTTP status

Os clients REST e CGI traduzem status codes HTTP para a mesma taxonomia
`Error::Code`, então wrappers e applications podem fazer switch em um
único enum:

| Status HTTP | `Error::Code`        |
| ----------- | -------------------- |
| `401`       | `NotAuthenticated`   |
| `400`, `422`| `InvalidParameter`   |
| `404`       | `InvalidParameter`   |
| `503`       | `ConnectionFailed`   |
| `5xx`       | `ServerError`        |
| Timeout     | `Timeout`            |
| Conn. fail  | `ConnectionFailed`   |

## Logging

Cada client suporta um log handler opcional. Defina uma vez no início
do programa; o SDK nunca toma posse do stdout/stderr.

```cpp
rest.setLogHandler([](LogLevel lvl, const std::string& msg) {
    if (lvl == LogLevel::Error)
        std::cerr << "[REST ERROR] " << msg << '\n';
    else
        std::cout << "[REST] " << msg << '\n';
});
```

Valores de `LogLevel`: `Debug`, `Info`, `Warn`, `Error`.

## Wrappers

Cada language wrapper mapeia a tabela acima para constructs idiomáticos:

- **C# / .NET** levanta uma hierarquia `ItscamException`
  (`ItscamTimeoutException`, `ItscamAuthException`, ...) e `Task<T>`
  propaga as exceções.
- **Python** levanta subclasses de `ItscamError` (`ItscamTimeoutError`,
  `ItscamAuthError`, `ItscamConnectionError`).
- **Go** devolve um `error` tipado que carrega o valor de `Error::Code`,
  então o caller pode usar `errors.Is(err, itscam.ErrTimeout)`.

Veja os capítulos de cada wrapper para os detalhes.
