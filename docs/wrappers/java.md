# Wrapper Java

[Português (Brasil)](java.md) | [English (US)](java.en-US.md)

O wrapper Java fica em [`src/wrappers/java/`](../../src/wrappers/java/) e usa **JNA** (Java Native Access) sobre a C API do SDK. Suporta JDK 7+ no Linux, Windows e macOS.

> O wrapper expõe as três superfícies de cliente do SDK:
>
> | Classe | Contraparte nativa |
> | ------ | ------------------ |
> | `com.pumatronix.itscam.ItscamClient` | `src/core/itscam_client.h` |
> | `com.pumatronix.itscam.ItscamRestClient` | `src/core/itscam_rest_client.h` |
> | `com.pumatronix.itscam.ItscamCgiClient` | `src/core/itscam_cgi_client.h` |

## Instalação

### Via pacote SDK pré-compilado (recomendado)

O pacote de distribuição (`itscam-sdk-<version>.tar.gz`) inclui um JAR Maven com os native binaries embutidos sob `META-INF/native/<os>-<arch>/`. Baixe na [página de releases](https://github.com/pumatronix/itscam-sdk/releases):

```bash
tar xzf itscam-sdk-<version>.tar.gz
export SDK=$PWD/itscam-sdk-<version>

mvn install:install-file \
    -Dfile=$SDK/linux-x64/java/itscam-sdk-<version>.jar \
    -DgroupId=com.pumatronix \
    -DartifactId=itscam-sdk \
    -Dversion=<version> \
    -Dpackaging=jar \
    -DgeneratePom=true
```

Depois disso, declare no seu `pom.xml`:

```xml
<dependency>
    <groupId>com.pumatronix</groupId>
    <artifactId>itscam-sdk</artifactId>
    <version>...</version>
</dependency>
<dependency>
    <groupId>net.java.dev.jna</groupId>
    <artifactId>jna</artifactId>
    <version>5.14.0</version>
</dependency>
```

A native lib é extraída automaticamente do JAR para um diretório temporário do JVM na primeira chamada.

O JAR empacotado é um artefato fat / multi-arch: `META-INF/native/` inclui todas as plataformas produzidas pelo builder (`linux-x86_64/`, `linux-arm/`, `linux-aarch64/`, `win-x86_64/`, `win-x86/`). O loader em runtime escolhe a entrada que combina com `os.name` + `os.arch` da JVM atual.

### Build a partir do source (avançado)

Se você está desenvolvendo dentro do source tree do SDK:

```bash
make lib                        # build libitscam_sdk.so primeiro
make java                       # JAR com native bundled
make java-examples              # JAR + runnable examples
make docker-java                # mesma coisa dentro do Docker
make docker-java-jdk7-check     # compila wrapper + examples com javac real do JDK 7
```

`make java` faz três coisas:

1. Compila `libitscam_sdk.so` se ainda não existe.
2. Roda `tools/packaging/stage-java-natives.sh` para copiar os binaries pré-compilados em `src/core/build/<rid>/` para `src/wrappers/java/itscam-sdk/src/main/resources/META-INF/native/<os>-<arch>/`.
3. Roda `mvn -pl itscam-sdk -am package -DskipTests`, que produz `src/wrappers/java/itscam-sdk/target/itscam-sdk-<version>.jar`.

## Resolução da native lib

A classe `com.pumatronix.itscam.internal.NativeLibrary` procura `libitscam_sdk` na seguinte ordem:

1. `-Ditscam.sdk.library=/absolute/path/to/libitscam_sdk.so` (override por system property).
2. `java.library.path` / `LD_LIBRARY_PATH` / system loader, via JNA.
3. `META-INF/native/<os>-<arch>/<libname>` dentro do JAR (extraído para um diretório temp do JVM).

| OS | `<os>` |
| --- | --- |
| Linux | `linux` |
| Windows | `windows` |
| macOS | `darwin` |

| `os.arch` | `<arch>` |
| --------- | -------- |
| `amd64`, `x86_64` | `x86_64` |
| `aarch64`, `arm64` | `aarch64` |
| `arm*` | `arm` |
| `x86`, `i386`, `i686` | `x86` |

## Superfície idiomática

| Aspecto | O que você ganha |
| ------- | ---------------- |
| Async | Toda blocking call tem `*Async` retornando `java.util.concurrent.Future<T>`. |
| Lifetime | Todos os clients implementam `AutoCloseable`; use try-with-resources. |
| Streaming | `startMjpegStream(ItscamConsumer<CgiStreamFrame>)` invoca o callback na worker thread do SDK. |
| Errors | Hierarquia `RuntimeException` rooted em `ItscamException` (`ItscamTimeoutException`, `ItscamAuthException`, ...). |
| Strings | UTF-8 por padrão (delegado para o marshalling do JNA). |

## Uso do CGI (auth opcional)

```java
import com.pumatronix.itscam.*;

try (ItscamCgiClient cgi = new ItscamCgiClient()) {
    cgi.setBaseUrl("192.168.254.254", 80, "http");
    // cgi.setBaseUrl("camera.example.com", 443, "https");
    // cgi.setVerifyServerCertificate(false);
    // cgi.login("admin", "1234", 10000);   // somente quando blockAPI=true

    CgiImage last = cgi.getLastFrame(10000);
    last.save("lastframe.jpg");

    java.util.List<CgiImage> images = cgi.getSnapshot(
        new SnapshotCgiRequest().setQuality(80), 15000);

    cgi.startMjpegStream(new ItscamConsumer<CgiStreamFrame>() {
        @Override public void accept(CgiStreamFrame frame) {
            // Roda na worker thread do SDK; não bloqueie.
        }
    }, 10000);
    Thread.sleep(5000);
    cgi.stopMjpegStream();
}
```

## Uso do REST (auth obrigatória)

O REST client expõe duas superfícies que coexistem:

* **Typed convenience helpers** (preferencial): `getProfiles`, `setOcrConfig`, `setItscamproConfig`, `getAutoFocus` etc. retornam objetos de `com.pumatronix.itscam.resttypes`. Usam serialização parcial -- apenas os fields que você seta são incluídos no body PUT.
* **Generic verbs** (escape hatch): `httpGet`, `httpPut`, `httpPost`, `httpDelete`, `patchJson` retornam o body JSON cru como `String` para endpoints sem helper tipado.

* **Generic partial PUT** — `patchJson(path, partialJson, timeoutMs)` envia somente os campos que mudaram. Disponível para payloads não tipados ou endpoints sem typed helper. Veja [`docs/api/rest-client.md`](../api/rest-client.md).

```java
try (ItscamRestClient rest = new ItscamRestClient()) {
    rest.setBaseUrl("192.168.254.254", 80, "http");
    rest.login("admin", "1234", 10000);

    java.util.List<ProfileConfig> profiles = rest.getProfiles(10000);

    LensConfig lens = new LensConfig().setZoom(1200).setFocus(300);
    rest.updateProfileById(0, new ProfileConfig().setLens(lens), 10000);

    AutoFocus autofocus = rest.getAutoFocus(10000);
    rest.setAutoFocus(new AutoFocus().setRun(Boolean.TRUE), 10000);
}
```

## Uso do binary client

```java
try (ItscamClient camera = new ItscamClient()) {
    camera.connect("192.168.254.254", 60000, 10000,
                   new AutoReconnectConfig().setEnabled(true));
    camera.authenticate("1234", 10000);
    camera.subscribeCaptures(10000);

    java.util.List<CaptureResult> frames = camera.captureSnapshot(15000);
    for (CaptureResult f : frames) {
        f.save("snap-" + f.info().requestId() + ".jpg");
    }
}
```

## Examples

Em [`src/wrappers/java/examples/src/main/java/com/pumatronix/itscam/examples/`](../../src/wrappers/java/examples/src/main/java/com/pumatronix/itscam/examples/):

| Classe | Demonstra |
| ------ | --------- |
| `CaptureExample` | Binary client connect + authenticate + snapshot. |
| `RestExample` | REST login + leitura de configuration sobre HTTP/HTTPS. |
| `CgiSnapshotExample` | `lastframe.cgi`, `snapshot.cgi`, MJPEG streaming. |

Run com:

```bash
make java-examples

cd src/wrappers/java
mvn -pl examples exec:java \
    -Dexec.mainClass=com.pumatronix.itscam.examples.CaptureExample \
    -Dexec.args="192.168.254.254 1234"

# Ou via JAR shaded:
java -cp examples/target/itscam-sdk-examples-*-all.jar \
    com.pumatronix.itscam.examples.CgiSnapshotExample 192.168.254.254
```

## Tutorial passo a passo

Para um walkthrough do zero (criar projeto Maven e salvar a primeira imagem em disco), veja [Primeira imagem com Java](../tutorials/first-image-java.md).
