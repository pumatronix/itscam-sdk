# ITSCAM SDK - Java Wrapper

Idiomatic Java bindings for the Pumatronix ITSCAM Client SDK, backed by
the native `libitscam_sdk.{so,dll,dylib}` shared library through
[JNA](https://github.com/java-native-access/jna).

The wrapper exposes the same three client classes as the C++, C#, Python
and Go wrappers:

| Class | Use it for |
| ----- | ---------- |
| `com.pumatronix.itscam.ItscamClient` | Real-time triggers, GPIO/serial, exposure groups (binary TCP 60000). |
| `com.pumatronix.itscam.ItscamRestClient` | Equipment / daemon configuration over HTTP/HTTPS. **Always** requires `login()` first. |
| `com.pumatronix.itscam.ItscamCgiClient` | `snapshot.cgi`, `lastframe.cgi`, `mjpegvideo.cgi`. CGI auth is opt-in. |

## Layout

```
src/wrappers/java/
  pom.xml                     # aggregator POM (Maven multi-module)
  itscam-sdk/                 # the library JAR
    pom.xml
    src/main/java/com/pumatronix/itscam/...
    src/main/resources/META-INF/native/<os>-<arch>/   # native libs (populated by `make java-pack`)
  examples/                   # runnable demos
    pom.xml
    src/main/java/com/pumatronix/itscam/examples/{CaptureExample,RestExample,CgiSnapshotExample}.java
```

## Build

```bash
# Library JAR + examples (incremental):
make java                 # only the library JAR
make java-examples        # library + runnable examples

# Reproducible Docker build:
make docker-java
make docker-java-examples
make docker-java-jdk7-check # compile wrapper + examples with a real JDK 7 javac
```

`make java-pack` stages the native binaries from `src/core/build/<rid>/`
into `itscam-sdk/src/main/resources/META-INF/native/<os>-<arch>/` and
runs `mvn -pl itscam-sdk package`, producing a JAR that consumers can
drop straight into their own Maven / Gradle build without installing
the native library system-wide.

## Native library resolution

`com.pumatronix.itscam.internal.NativeLibrary` looks for `libitscam_sdk`
in this order:

1. `-Ditscam.sdk.library=/absolute/path/to/libitscam_sdk.so` (system property override).
2. `java.library.path` / `LD_LIBRARY_PATH` / system loader, via JNA.
3. `META-INF/native/<os>-<arch>/<libname>` inside the JAR (extracted to
   a JVM-temp directory on first use).

The bundled JAR layout maps to:

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

## Quick start

```java
import com.pumatronix.itscam.*;

try (ItscamClient cam = new ItscamClient()) {
    cam.connect("192.168.254.254", 60000, 10000);
    cam.authenticate("1234", 10000);
    cam.subscribeCaptures(10000);
    java.util.List<CaptureResult> frames = cam.captureSnapshot(15000);
    frames.get(0).save("snapshot.jpg");
}
```

```java
try (ItscamCgiClient cgi = new ItscamCgiClient()) {
    cgi.setBaseUrl("192.168.254.254", 80, "http");
    CgiImage last = cgi.getLastFrame(10000);
    java.nio.file.Files.write(java.nio.file.Paths.get("lastframe.jpg"),
                              last.data());
}
```

```java
try (ItscamRestClient rest = new ItscamRestClient()) {
    rest.setBaseUrl("192.168.254.254", 80, "http");
    rest.login("admin", "1234", 10000);

    java.util.List<com.pumatronix.itscam.resttypes.ProfileConfig> profiles =
        rest.getProfiles(10000);

    com.pumatronix.itscam.resttypes.LensConfig lens =
        new com.pumatronix.itscam.resttypes.LensConfig()
            .setZoom(1200)
            .setFocus(300);
    rest.updateProfileById(0,
        new com.pumatronix.itscam.resttypes.ProfileConfig().setLens(lens),
        10000);
}
```

## Async surface

Every blocking call has a `*Async` counterpart returning
`java.util.concurrent.Future<T>`. The futures complete on a daemon-backed
SDK executor so the API stays compatible with JDK 7.

## Errors

The wrapper raises a hierarchy of `RuntimeException`s rooted at
`com.pumatronix.itscam.ItscamException`:

| Native code | Java exception |
| ----------- | -------------- |
| `Timeout` | `ItscamTimeoutException` |
| `NotAuthenticated` | `ItscamAuthException` |
| `ConnectionFailed` / `Disconnected` | `ItscamConnectionException` |
| `InvalidParameter` | `ItscamInvalidParameterException` |
| `ServerError` | `ItscamServerException` |
| (everything else) | `ItscamException` |

Every exception carries the underlying `ErrorCode`, accessible via
`getCode()` for callers that want to discriminate without parsing
the message.

## Examples

| Class | Source | Demonstrates |
| ----- | ------ | ------------ |
| `CaptureExample` | [examples/.../CaptureExample.java](examples/src/main/java/com/pumatronix/itscam/examples/CaptureExample.java) | Binary client connect + authenticate + snapshot. |
| `RestExample` | [examples/.../RestExample.java](examples/src/main/java/com/pumatronix/itscam/examples/RestExample.java) | REST login + read configuration over HTTP/HTTPS. |
| `CgiSnapshotExample` | [examples/.../CgiSnapshotExample.java](examples/src/main/java/com/pumatronix/itscam/examples/CgiSnapshotExample.java) | `lastframe.cgi`, `snapshot.cgi`, MJPEG streaming. |

Run them with:

```bash
make java-examples

cd src/wrappers/java
mvn -pl examples exec:java \
    -Dexec.mainClass=com.pumatronix.itscam.examples.CaptureExample \
    -Dexec.args="192.168.254.254 1234"
```

Or as a single-file shaded JAR:

```bash
java -cp examples/target/itscam-sdk-examples-0.0.1-SNAPSHOT-all.jar \
    com.pumatronix.itscam.examples.CgiSnapshotExample 192.168.254.254
```

## License

Copyright (c) 2026 Pumatronix Equipamentos Eletronicos. Proprietary.
