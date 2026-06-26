# Java wrapper

[Português (Brasil)](java.md) | [English (US)](java.en-US.md)

The Java wrapper lives under [`src/wrappers/java/`](../../src/wrappers/java/) and uses **JNA** (Java Native Access) on top of the SDK's C ABI. It supports JDK 7+ on Linux, Windows, and macOS.

> The wrapper exposes the SDK's three client surfaces:
>
> | Class | Native counterpart |
> | ----- | ------------------ |
> | `com.pumatronix.itscam.ItscamClient` | `src/core/itscam_client.h` |
> | `com.pumatronix.itscam.ItscamRestClient` | `src/core/itscam_rest_client.h` |
> | `com.pumatronix.itscam.ItscamCgiClient` | `src/core/itscam_cgi_client.h` |

## Installation

### From the pre-built SDK package (recommended)

The distribution archive (`itscam-sdk-<version>.tar.gz`) ships a Maven JAR with native binaries embedded under `META-INF/native/<os>-<arch>/`:

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

Then declare the dependency in your `pom.xml`:

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

The native library is automatically extracted from the JAR to a temporary JVM directory on the first call.

The packaged JAR is a fat / multi-arch artefact: `META-INF/native/` includes every platform produced by the builder (`linux-x86_64/`, `linux-arm/`, `linux-aarch64/`, `win-x86_64/`, `win-x86/`). The runtime loader picks the entry matching the current JVM's `os.name` + `os.arch`.

### Building from source (advanced)

When working inside the SDK source tree:

```bash
make lib                        # build libitscam_sdk.so first
make java                       # JAR with native binaries bundled
make java-examples              # JAR + runnable examples
make docker-java                # same, inside the Docker builder
make docker-java-jdk7-check     # compile wrapper + examples with a real JDK 7 javac
```

`make java` does three things:

1. Builds `libitscam_sdk.so` if it isn't already present.
2. Runs `tools/packaging/stage-java-natives.sh` which copies the pre-built binaries from `src/core/build/<rid>/` into `src/wrappers/java/itscam-sdk/src/main/resources/META-INF/native/<os>-<arch>/`.
3. Runs `mvn -pl itscam-sdk -am package -DskipTests`, producing `src/wrappers/java/itscam-sdk/target/itscam-sdk-<version>.jar`.

## Native library resolution

The `com.pumatronix.itscam.internal.NativeLibrary` class searches for `libitscam_sdk` in the following order:

1. `-Ditscam.sdk.library=/absolute/path/to/libitscam_sdk.so` (system property override).
2. `java.library.path` / `LD_LIBRARY_PATH` / system loader, via JNA.
3. `META-INF/native/<os>-<arch>/<libname>` inside the JAR (extracted to a JVM temp directory).

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

## Idiomatic surface

| Concern | What you get |
| ------- | ------------ |
| Async | Every blocking call has an `*Async` companion returning `java.util.concurrent.Future<T>`. |
| Lifetime | Every client implements `AutoCloseable`; use try-with-resources. |
| Streaming | `startMjpegStream(ItscamConsumer<CgiStreamFrame>)` invokes the callback on the SDK worker thread. |
| Errors | `RuntimeException` hierarchy rooted in `ItscamException` (`ItscamTimeoutException`, `ItscamAuthException`, ...). |
| Strings | UTF-8 by default (delegated to JNA marshalling). |

## CGI usage (auth optional)

```java
import com.pumatronix.itscam.*;

try (ItscamCgiClient cgi = new ItscamCgiClient()) {
    cgi.setBaseUrl("192.168.254.254", 80, "http");
    // cgi.setBaseUrl("camera.example.com", 443, "https");
    // cgi.setVerifyServerCertificate(false);
    // cgi.login("admin", "1234", 10000);   // only when blockAPI=true

    CgiImage last = cgi.getLastFrame(10000);
    last.save("lastframe.jpg");

    java.util.List<CgiImage> images = cgi.getSnapshot(
        new SnapshotCgiRequest().setQuality(80), 15000);

    cgi.startMjpegStream(new ItscamConsumer<CgiStreamFrame>() {
        @Override public void accept(CgiStreamFrame frame) {
            // Runs on the SDK worker thread; do not block.
        }
    }, 10000);
    Thread.sleep(5000);
    cgi.stopMjpegStream();
}
```

## REST usage (auth required)

The REST client exposes two surfaces that coexist:

* **Typed convenience helpers** (preferred): `getProfiles`, `setOcrConfig`, `setItscamproConfig`, `getAutoFocus`, etc. return objects from `com.pumatronix.itscam.resttypes`. They use partial serialization -- only the fields you set are included in the PUT body.
* **Generic verbs** (escape hatch): `httpGet`, `httpPut`, `httpPost`, `httpDelete`, `patchJson`. They return the raw JSON body as a `String` for endpoints without a typed helper.

* **Generic partial PUT** -- `patchJson(path, partialJson, timeoutMs)` sends only changed fields. Available for untyped payloads or endpoints without a typed helper. See [`docs/api/rest-client.md`](../api/rest-client.md).

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

## Binary client usage

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

Under [`src/wrappers/java/examples/src/main/java/com/pumatronix/itscam/examples/`](../../src/wrappers/java/examples/src/main/java/com/pumatronix/itscam/examples/):

| Class | Demonstrates |
| ----- | ------------ |
| `CaptureExample` | Binary client connect + authenticate + snapshot. |
| `RestExample` | REST login + reading configuration over HTTP/HTTPS. |
| `CgiSnapshotExample` | `lastframe.cgi`, `snapshot.cgi`, MJPEG streaming. |

Run them with:

```bash
make java-examples

cd src/wrappers/java
mvn -pl examples exec:java \
    -Dexec.mainClass=com.pumatronix.itscam.examples.CaptureExample \
    -Dexec.args="192.168.254.254 1234"

# Or via the shaded JAR:
java -cp examples/target/itscam-sdk-examples-*-all.jar \
    com.pumatronix.itscam.examples.CgiSnapshotExample 192.168.254.254
```

## Step-by-step tutorial

For a from-scratch walkthrough (create a Maven project, save the first image to disk), see [First image with Java](../tutorials/first-image-java.md).
