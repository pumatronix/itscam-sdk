# Wrapper Python

[Português (Brasil)](python.md) | [English (US)](python.en-US.md)

O wrapper Python fica em [`src/wrappers/python/`](../../src/wrappers/python/) e usa **ctypes** sobre a C API do SDK. Suporta Python 3.7+ no Linux e no Windows.

> **Referência completa de classes e métodos** (gerada do código): [pdoc do módulo `itscam`](/api-ref/python/itscam.html). Esta página cobre instalação, padrões de uso e exemplos.

## Instalação

### Via pacote SDK pré-compilado (recomendado)

O pacote de distribuição (`itscam-sdk-<version>.tar.gz`) inclui uma wheel Python com a native lib embutida:

```bash
tar xzf itscam-sdk-<version>.tar.gz
pip install itscam-sdk-<version>/linux-x64/python/itscam-*.whl
python -c "import itscam; print(itscam.get_version())"
```

A wheel já embute `libitscam_sdk.so` -- nenhuma configuração de `LD_LIBRARY_PATH` é necessária.

### A partir do source (avançado)

Se você está desenvolvendo no source tree do SDK:

```bash
make lib                         # build libitscam_sdk.so primeiro
cd src/wrappers/python
pip install -e .                 # editable install
```

Nesse caso, o ctypes localiza a shared library na seguinte ordem:

1. O diretório que contém `bindings.py`, depois seu pai.
2. `<pai>/build/` (útil quando empacotado junto com o output de build).
3. Paths do sistema (`/usr/local/lib`, `/usr/lib`).
4. Todas as entradas de `LD_LIBRARY_PATH`.

Para uso ad-hoc sem `pip install`, aponte `LD_LIBRARY_PATH` para o diretório de build e adicione o wrapper ao `PYTHONPATH`:

```bash
export LD_LIBRARY_PATH=$PWD/src/core/build/linux:$LD_LIBRARY_PATH
export PYTHONPATH=$PWD/src/wrappers/python:$PYTHONPATH
python -c "import itscam; print(itscam.get_version())"
```

## Superfície

```python
from itscam import (
    ItscamClient, ItscamRestClient, ItscamCgiClient,
    SnapshotCgiRequest, CgiImage, CgiStreamFrame,
    SnapshotRequest, CaptureResult, FrameInfo,
    ItscamError, ItscamTimeoutError, ItscamConnectionError, ItscamAuthError,
    LogLevel, ConnectionState,
)
```

Cada client é uma classe Python compatível com `with` (context manager) que espelha 1:1 a sua contraparte em C++. Erros são lançados como exceptions em vez de retornados como `Result<T>`.

## Uso do CGI (auth opcional)

```python
from itscam import ItscamCgiClient, SnapshotCgiRequest

with ItscamCgiClient() as cgi:
    cgi.set_base_url("192.168.254.254", 80)
    # cgi.set_base_url("camera.example.com", 443, "https")
    # cgi.set_verify_server_certificate(False)
    # cgi.login("admin", "1234")   # somente quando blockAPI=true

    with open("lastframe.jpg", "wb") as f:
        f.write(cgi.get_last_frame().data)

    images = cgi.get_snapshot(SnapshotCgiRequest(quality=80))
    for i, img in enumerate(images):
        with open(f"snap-{i}.jpg", "wb") as f:
            f.write(img.data)

    def on_frame(frame: CgiStreamFrame) -> None:
        ...  # roda na worker thread do SDK; não bloqueie
    cgi.start_mjpeg_stream(on_frame)
    time.sleep(5)
    cgi.stop_mjpeg_stream()
```

## Uso do REST (auth obrigatória)

O REST client expõe duas superfícies que coexistem:

* **Typed helpers** (preferencial) -- `rest.get_ocr_config()`, `rest.set_ocr_config(cfg)`, `rest.get_profiles()` etc. retornam dataclasses geradas a partir do documento OpenAPI da câmera. Veja [`docs/codegen.md`](../codegen.md) para o workflow de regeneração (maintainer e downstream).
* **Generic verbs** (escape hatch) -- `rest.get(path)`, `rest.put(path, body)`, `rest.post`, `rest.delete` retornam JSON já parseado (`dict` / `list`).

* **Partial PUT** -- `rest.patch_json(path, patch)` envia somente os campos que mudaram. Obrigatório para image profiles e recomendado para a maioria das configuration updates. Veja [`docs/api/rest-client.md`](../api/rest-client.md).

```python
from itscam import ItscamRestClient, ProfileConfig

with ItscamRestClient() as rest:
    rest.set_base_url("192.168.254.254", 80)
    rest.login("admin", "1234")

    # Read-only: typed POCO é conveniente.
    profiles = rest.get_profiles()

    # Write: partial PUT (não faça round-trip do profile completo).
    rest.patch_json("/api/image/profiles/0",
                    {"trigger": {"enabled": False}})

    # Generic verbs:
    print(rest.get("/api/equipment/misc/readonly/constants"))
```

O módulo completo de POCOs também está disponível em `itscam.rest_types` para imports explícitos.

## Uso do binary client

```python
from itscam import ItscamClient

camera = ItscamClient()
camera.connect("192.168.254.254")
camera.authenticate("1234")
camera.subscribe_captures()
result = camera.capture_snapshot()
# result é uma lista de CaptureResult (um por exposure step)
```

## Examples

Scripts prontos para rodar em [`src/wrappers/python/examples/`](../../src/wrappers/python/examples/):

| Script                                   | Função                                |
| ---------------------------------------- | ------------------------------------- |
| `capture_example.py <host>`              | Capture loop pelo binary client.      |
| `rest_example.py <host> <user> <pass>`   | REST login + leitura de configuration.|
| `cgi_snapshot_example.py <host> [...]`   | CGI lastframe + snapshot + MJPEG.     |

O example de CGI aceita flags opcionais `--user` / `--password`; sem elas, ele fala com a câmera de forma anônima.

## Tutorial passo a passo

Para um walkthrough do zero (criar projeto, instalar dependência e salvar a primeira imagem em disco), veja [Primeira imagem com Python](../tutorials/first-image-python.md).
