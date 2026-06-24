# Primeira imagem com Java

Walkthrough do zero: criar um projeto Maven, declarar a dependência `com.pumatronix:itscam-sdk` e salvar a primeira imagem JPEG da câmera em disco. Caminho principal usa o **`ItscamCgiClient`** (HTTP, anônimo por default) e há uma seção opcional no final usando o **`ItscamClient`** (Cougar TCP :60000).

## 1. Pré-requisitos

| Item | Versão mínima | Verificar com |
| ---- | ------------- | ------------- |
| JDK | 7+ | `java -version` |
| Maven | 3.9+ | `mvn -v` |
| Pacote SDK | `itscam-sdk-<version>.tar.gz` | extrair e localizar `linux-x64/java/` |
| Câmera ITSCAM | ITSCAM450 / ITSCAM600 alcançável na rede | `ping <ip-da-camera>` |

## 2. Extrair o SDK e instalar o JAR no Maven local

Baixe `itscam-sdk-<version>.tar.gz` na [página de releases](https://github.com/pumatronix/itscam-sdk/releases):

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

> **Compilando o SDK do zero?** Se você precisa buildar a partir do source em vez de usar o pacote pré-compilado, veja a [seção avançada de build](../getting-started.md#build-do-sdk-a-partir-do-source). Após `make java`, o JAR fica em `src/wrappers/java/itscam-sdk/target/itscam-sdk-<version>.jar`.

## 3. Criar o projeto

```bash
mvn archetype:generate -DgroupId=com.example -DartifactId=meu-app \
    -DarchetypeArtifactId=maven-archetype-quickstart \
    -DinteractiveMode=false
cd meu-app
```

Edite `pom.xml` (ajuste `itscam.sdk.version` para coincidir com `-Dversion` do passo 2):

```xml
<project>
    <modelVersion>4.0.0</modelVersion>
    <groupId>com.example</groupId>
    <artifactId>meu-app</artifactId>
    <version>1.0-SNAPSHOT</version>
    <properties>
        <maven.compiler.source>1.7</maven.compiler.source>
        <maven.compiler.target>1.7</maven.compiler.target>
        <itscam.sdk.version>0.3.1</itscam.sdk.version>
    </properties>
    <dependencies>
        <dependency>
            <groupId>com.pumatronix</groupId>
            <artifactId>itscam-sdk</artifactId>
            <version>${itscam.sdk.version}</version>
        </dependency>
        <dependency>
            <groupId>net.java.dev.jna</groupId>
            <artifactId>jna</artifactId>
            <version>5.14.0</version>
        </dependency>
    </dependencies>
</project>
```

## 4. Escrever o código mínimo

```java
// src/main/java/com/example/MeuApp.java
package com.example;

import com.pumatronix.itscam.CgiImage;
import com.pumatronix.itscam.ItscamCgiClient;

import java.nio.file.Paths;

public final class MeuApp {

    public static void main(String[] args) throws Exception {
        if (args.length < 1) {
            System.err.println("uso: java MeuApp <ip-da-camera>");
            System.exit(1);
        }
        String host = args[0];

        try (ItscamCgiClient cgi = new ItscamCgiClient()) {
            cgi.setBaseUrl(host, 80, "http");
            // Para HTTPS:
            //   cgi.setBaseUrl(host, 443, "https");
            // Para auth opcional (somente se configCgi.blockAPI=true):
            //   cgi.login("admin", "1234", 10000);

            CgiImage frame = cgi.getLastFrame(10000);
            frame.save(Paths.get("primeira-imagem.jpg"));

            System.out.printf("OK: %d bytes salvos em primeira-imagem.jpg (%s)%n",
                    frame.data().length, frame.mimeType());
        }
    }
}
```

## 5. Executar

```bash
mvn -q -DskipTests package
java -cp target/meu-app-1.0-SNAPSHOT.jar:$(mvn -q dependency:build-classpath -Dmdep.outputFile=/dev/stdout) \
     com.example.MeuApp 192.168.254.254
```

Saída esperada:

```text
OK: 87421 bytes salvos em primeira-imagem.jpg (image/jpeg)
```

## 6. Troubleshooting

| Sintoma | Causa provável | Solução |
| ------- | -------------- | ------- |
| `UnsatisfiedLinkError: Unable to load library 'itscam_sdk'` | JAR sem native binary embutido (build manual) | Use `make java` que faz staging dos binaries, ou passe `-Ditscam.sdk.library=/path/to/libitscam_sdk.so`. |
| `ItscamConnectionException` | IP errado ou porta 80 bloqueada | `curl -v http://<ip>/api/lastframe.cgi -o /dev/null` |
| `ItscamAuthException` em CGI | A câmera tem `configCgi.blockAPI=true` | Chame `cgi.login("user", "pass", 10000)` antes do `getLastFrame()`. |
| TLS errors em HTTPS | CA bundle não configurado | `cgi.setCaCertFile("/etc/ssl/certs/ca-bundle.pem")` ou, só em dev, `cgi.setVerifyServerCertificate(false)`. |

## 7. Opcional: capture via `ItscamClient` (TCP :60000)

```java
import com.pumatronix.itscam.*;
import java.util.List;

try (ItscamClient camera = new ItscamClient()) {
    camera.connect(host, 60000, 10000);
    camera.authenticate("1234", 10000);
    camera.subscribeCaptures(10000);

    List<CaptureResult> frames = camera.captureSnapshot(15000);
    if (frames.isEmpty()) {
        System.err.println("nenhum frame retornado");
        System.exit(2);
    }
    frames.get(0).save("primeira-imagem-binary.jpg");
}
```

## Próximos passos

- [Guia do wrapper Java](../wrappers/java.md) — API completa.
- [Examples Java](../../src/wrappers/java/examples/src/main/java/com/pumatronix/itscam/examples/) — CGI, REST e binary.
- [HTTPS / TLS](../https-tls.md) — configurar mbedTLS para produção.
- [Adicionar novo wrapper](../adding-a-new-wrapper.md) — checklist para futuros bindings.
