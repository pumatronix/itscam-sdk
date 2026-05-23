# Primeira imagem com Python

Walkthrough do zero: criar um projeto Python, instalar o wrapper `itscam` e salvar a primeira imagem JPEG da câmera em disco. Caminho principal usa o **`ItscamCgiClient`** (HTTP, anônimo por default) e há uma seção opcional no final usando o **`ItscamClient`** (Cougar TCP :60000).

## 1. Pré-requisitos

| Item | Versão mínima | Verificar com |
| ---- | ------------- | ------------- |
| Python | 3.7+ | `python3 --version` |
| Compilador C++17 | só para `make lib` | `g++ --version` |
| GNU make | qualquer | `make --version` |
| Git | qualquer | `git --version` |
| Câmera ITSCAM | ITSCAM450 / ITSCAM600 alcançável na rede | `ping <ip-da-camera>` |

## 2. Buildar a shared library nativa

O wrapper Python é puro ctypes -- ele carrega `libitscam_sdk.so` em runtime, então você precisa buildar essa biblioteca uma vez:

```bash
git clone https://github.com/pumatronix/itscam-sdk.git
cd itscam-sdk
make lib
```

O resultado fica em `src/core/build/linux/libitscam_sdk.so.1.0.0` (com symlinks).

## 3. Criar o projeto

```bash
mkdir -p ~/projetos/meu-app && cd ~/projetos/meu-app
python3 -m venv .venv
source .venv/bin/activate
```

## 4. Instalar o wrapper `itscam`

Editable install a partir do checkout do SDK:

```bash
pip install -e /caminho/para/itscam-sdk/src/wrappers/python
```

Confirme que o pacote acha a shared library:

```bash
LD_LIBRARY_PATH=/caminho/para/itscam-sdk/src/core/build/linux \
    python -c "import itscam; print(itscam.get_version())"
```

> Por que `LD_LIBRARY_PATH`? Se você instalou o SDK em `/usr/local/lib` (`sudo make install`) não é necessário. Caso contrário, exporte a variável para o terminal da sessão ou rode sempre prefixando.

## 5. Escrever o código mínimo

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

## 6. Executar

```bash
LD_LIBRARY_PATH=/caminho/para/itscam-sdk/src/core/build/linux \
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

## 7. Troubleshooting

| Sintoma | Causa provável | Solução |
| ------- | -------------- | ------- |
| `OSError: libitscam_sdk.so: cannot open shared object file` | shared library não encontrada | Exporte `LD_LIBRARY_PATH=.../src/core/build/linux` ou rode `sudo make install` na raiz do SDK. |
| `ItscamConnectionError` | IP errado ou porta 80 bloqueada | `curl -v http://<ip>/api/lastframe.cgi -o /dev/null` |
| `ItscamAuthError` em CGI | A câmera tem `configCgi.blockAPI=true` | Chame `cgi.login("user", "pass")` antes do `get_last_frame()`. |
| `ItscamError: SSL` em HTTPS | CA bundle não configurado | `cgi.set_ca_cert_file("/etc/ssl/certs/ca-bundle.pem")` ou, só em dev, `cgi.set_verify_server_certificate(False)`. |

## 8. Opcional: capture via `ItscamClient` (TCP :60000)

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
