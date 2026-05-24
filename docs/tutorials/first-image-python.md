# Primeira imagem com Python

Walkthrough do zero: criar um projeto Python, instalar o wrapper `itscam` e salvar a primeira imagem JPEG da câmera em disco. Caminho principal usa o **`ItscamCgiClient`** (HTTP, anônimo por default) e há uma seção opcional no final usando o **`ItscamClient`** (Cougar TCP :60000).

## 1. Pré-requisitos

| Item | Versão mínima | Verificar com |
| ---- | ------------- | ------------- |
| Python | 3.7+ | `python3 --version` |
| pip | qualquer | `pip --version` |
| Pacote SDK | `itscam-sdk-<version>.tar.gz` | extrair e localizar `linux-x64/python/` |
| Câmera ITSCAM | ITSCAM450 / ITSCAM600 alcançável na rede | `ping <ip-da-camera>` |

## 2. Extrair o SDK e instalar a wheel

Baixe `itscam-sdk-<version>.tar.gz` na [página de releases](https://github.com/pumatronix/itscam-sdk/releases):

```bash
tar xzf itscam-sdk-<version>.tar.gz
export SDK=$PWD/itscam-sdk-<version>
```

A wheel Python já embute a native lib (`libitscam_sdk.so`), então a instalação é um único comando:

```bash
pip install $SDK/linux-x64/python/itscam-*.whl
```

Confirme:

```bash
python -c "import itscam; print(itscam.get_version())"
```

> **Compilando o SDK do zero?** Se você precisa buildar a partir do source em vez de usar o pacote pré-compilado, veja a [seção avançada de build](../getting-started.md#build-do-sdk-a-partir-do-source). Após `make lib`, instale o wrapper via `pip install -e src/wrappers/python` e exporte `LD_LIBRARY_PATH` apontando para `src/core/build/linux/`.

## 3. Criar o projeto

```bash
mkdir -p ~/projetos/meu-app && cd ~/projetos/meu-app
python3 -m venv .venv
source .venv/bin/activate
pip install $SDK/linux-x64/python/itscam-*.whl
```

## 4. Escrever o código mínimo

```python
# meu_app.py
"""Salva o último frame da câmera ITSCAM em primeira-imagem.jpg."""
from __future__ import annotations

import sys

from itscam import ItscamCgiClient


def main() -> int:
    if len(sys.argv) < 2:
        print(f"uso: {sys.argv[0]} <ip-da-camera>", file=sys.stderr)
        return 1

    host = sys.argv[1]

    with ItscamCgiClient() as cgi:
        cgi.set_base_url(host, 80)
        # Para HTTPS:
        #   cgi.set_base_url(host, 443, "https")
        # Para auth opcional (somente se configCgi.blockAPI=true):
        #   cgi.login("admin", "1234")

        frame = cgi.get_last_frame()
        with open("primeira-imagem.jpg", "wb") as f:
            f.write(frame.data)

        print(f"OK: {len(frame.data)} bytes salvos em primeira-imagem.jpg "
              f"({frame.mime_type})")
    return 0


if __name__ == "__main__":
    sys.exit(main())
```

## 5. Executar

```bash
python meu_app.py 192.168.254.254
```

Saída esperada:

```text
OK: 87421 bytes salvos em primeira-imagem.jpg (image/jpeg)
```

Verifique o arquivo:

```bash
file primeira-imagem.jpg
# primeira-imagem.jpg: JPEG image data, JFIF standard 1.01, ...
```

## 6. Troubleshooting

| Sintoma | Causa provável | Solução |
| ------- | -------------- | ------- |
| `OSError: libitscam_sdk.so: cannot open shared object file` | Wheel instalada sem a native lib embutida (build manual) | Re-instale a wheel do pacote SDK, ou exporte `LD_LIBRARY_PATH` apontando para o diretório com `libitscam_sdk.so`. |
| `ItscamConnectionError` | IP errado ou porta 80 bloqueada | `curl -v http://<ip>/api/lastframe.cgi -o /dev/null` |
| `ItscamAuthError` em CGI | A câmera tem `configCgi.blockAPI=true` | Chame `cgi.login("user", "pass")` antes do `get_last_frame()`. |
| `ItscamError: SSL` em HTTPS | CA bundle não configurado | `cgi.set_ca_cert_file("/etc/ssl/certs/ca-bundle.pem")` ou, só em dev, `cgi.set_verify_server_certificate(False)`. |

## 7. Opcional: capture via `ItscamClient` (TCP :60000)

O CGI é o caminho mais simples. Se você precisa de **trigger em real time** ou multi-exposure, use o binary client (porta 60000):

```python
# meu_app_binary.py
import sys
from itscam import ItscamClient

host = sys.argv[1]
password = sys.argv[2] if len(sys.argv) > 2 else "1234"

with ItscamClient(host) as camera:
    camera.authenticate(password)
    camera.subscribe_captures()

    results = camera.capture_snapshot()
    if not results:
        print("nenhum frame retornado", file=sys.stderr)
        sys.exit(2)

    with open("primeira-imagem-binary.jpg", "wb") as f:
        f.write(results[0].jpeg)
    print(f"OK: {len(results[0].jpeg)} bytes (binary)")
```

Detalhe completo (auto-reconnect, exposure groups, eventos de trigger contínuos via `on_trigger_image()`) em [docs/api/binary-client.md](../api/binary-client.md) e no example [`capture_example.py`](../../src/wrappers/python/examples/capture_example.py).

## Próximos passos

- [Guia do wrapper Python](../wrappers/python.md) -- API completa.
- [Examples Python](../../src/wrappers/python/examples/) -- CGI, REST e binary.
- [HTTPS / TLS](../https-tls.md) -- configurar mbedTLS para produção.
- [Codegen REST](../codegen.md) -- regenerar `rest_types.py` para um firmware específico.
