# Compatibilidade e verificação

O que o SDK garante às aplicações construídas sobre ele, como cada garantia é verificada e o que uma mudança precisa fazer antes de quebrar alguma delas. English: [`compatibility.en-US.md`](compatibility.en-US.md).

## Plataformas suportadas

| Plataforma | Artefato do release | Exige no alvo | Aplicações compilam com |
| ---------- | ------------------- | ------------- | ----------------------- |
| Linux x86_64 | `libitscam_sdk.so`, `.a` | glibc 2.23 ou mais nova (Ubuntu 16.04+) | qualquer GCC ou Clang a partir do GCC 5, C++14 ou C++17, ou C99 |
| Linux ARMv7 hard-float (ITSCAM450) | idem | glibc 2.23 ou mais nova | idem, p.ex. `arm-linux-gnueabihf` do Ubuntu ou Arm GNU-A |
| Linux ARM64 (ITSCAM600, ITSCAM450b) | idem | glibc 2.23 ou mais nova | idem, `aarch64-linux-gnu` |
| Windows x64 e x86 | `itscam_sdk.dll` (API C), `libitscam_sdk_static.a` | nada (sem DLLs do MinGW) | MinGW-w64: C com a DLL, C++ com a biblioteca estática |

A biblioteca compartilhada Linux leva o próprio runtime C++: libstdc++ e libgcc vão linkados dentro dela, com os símbolos locais, então ela precisa só da glibc. Ela linka com qualquer toolchain, seja qual for a libstdc++ dele, inclusive as que não têm versionamento de símbolos (Arm GNU-A 8.3). Ela exporta só a API C e o namespace `itscam` ([`src/core/itscam_sdk.map`](../src/core/itscam_sdk.map)). As cópias de libstdc++, nlohmann::json, cpp-httplib e mbedTLS que ela contém nunca substituem nem se ligam às cópias do mesmo código que a aplicação tenha.

Consequências para aplicações C++:

- Use a ABI "nova" da libstdc++ (`_GLIBCXX_USE_CXX11_ABI=1`, padrão desde o GCC 5). `std::string` e outros tipos padrão atravessam a API C++.
- Objetos que a biblioteca devolve são liberados pela aplicação, e vice-versa. Os dois lados alocam via `malloc`. Uma aplicação que substitui o `operator new` global por um alocador que não usa `malloc` deve linkar a biblioteca estática, ou compilar a compartilhada com `STATIC_RUNTIME=0`.
- Inclua `itscam_sdk.h` antes de qualquer coisa que configure o optional-lite para o `std::optional` padrão. Senão a compilação para com um erro dizendo isso: os tipos REST precisam ter o layout com que a biblioteca foi compilada.

Wrappers de linguagem, versão mais antiga suportada e testada:

| Wrapper | Mais antiga suportada | Verificado pelo `make verify` com |
| ------- | --------------------- | --------------------------------- |
| C | C99 | GCC 5.4, GCC 7.5, Arm GNU-A 8.3, MinGW-w64 |
| C++ | C++14 | os mesmos, como C++14 e C++17 |
| Python | 3.7 | Python 3.7 em x64, ARMv7 e ARM64 |
| Node.js | 16; no ARMv7, glibc 2.34 ou mais nova (binário pré-compilado do koffi) | Node.js 16 em x64 e ARM64, Node.js 20 no ARMv7 (Debian 12) e em x64 |
| Go | a diretiva `go` de `src/wrappers/go/go.mod` | o Go do builder, em x64, ARMv7 e ARM64 |
| C# / .NET | netstandard2.0 | .NET 8 em x64 |
| Java | Java 7 | compilado com JDK 7; executado no Java 11 (x64), também com charset padrão que não é UTF-8 |
| CMake | 3.10 | CMake 3.10 ([`cmake.md`](cmake.md)) |

## `make verify`

Um comando compila tudo o que o SDK entrega e verifica do jeito que uma aplicação usa, em todas as plataformas acima. Ele roda nas imagens Docker do repositório, as mesmas da CI, então passar na sua máquina significa passar na CI. Rode antes de todo pull request. A CI também roda em todo pull request ([`.github/workflows/verify.yml`](../.github/workflows/verify.yml)).

```bash
make verify                                  # tudo, ~30-60 min na primeira vez
VERIFY_STEPS="cores consumers run" make verify   # algumas etapas
VERIFY_ARCHES=arm64 make qemu-smoke          # só a parte ARM
make compat-check                            # só API/ABI contra o último release
```

| Etapa | O que comprova |
| ----- | -------------- |
| `cores` | As bibliotecas Linux compilam para x64, ARMv7 e ARM64 no piso da glibc 2.23, não usam símbolo mais novo que esse piso e não dependem de libstdc++ nem libgcc (`tools/check-glibc.sh --no-cxx-runtime`). |
| `windows` | A DLL e as bibliotecas estáticas Windows compilam, e a DLL não precisa de DLL de runtime do MinGW. |
| `consumers` | Os smoke tests C e C++ ([`tests/smoke`](../tests/smoke)) compilam contra as bibliotecas do release com cada compilador suportado: GCC 5.4 (o piso), GCC 7.5, Arm GNU-A 8.3, como C++14, C++17 e C99, e com MinGW. Incluir o optional padrão antes do SDK não compila, como deve ser. |
| `wrappers` | Os smoke tests de Go, Node.js, Java e C# compilam. |
| `run` | Todos os smoke tests rodam contra [`tests/standin`](../tests/standin), um substituto da API web da câmera. C e C++ rodam no Ubuntu 16.04 (o piso) e no runtime do próprio compilador, e Go no runtime do próprio compilador, em x64, ARMv7 e ARM64 (ARM sob qemu). Python 3.7 e Node.js rodam nos três. C# e Java rodam em x64, Java também com charset padrão ASCII. |
| `cmake` | [`tools/verify/cmake-matrix.sh`](../tools/verify/cmake-matrix.sh): build standalone com os testes, subprojeto com vários conjuntos de opções (até só o cliente binário em C++14), pacote instalado, toolchains ARM e Windows. |
| `compat` | [`tools/compat/check-compat.sh`](../tools/compat/check-compat.sh): API e ABI contra o último release (abaixo). |
| `codegen` | Os tipos REST gerados batem com [`tools/codegen/spec`](../tools/codegen). |
| `java7` | O wrapper Java compila com um JDK 7 de verdade. |

Os smoke tests verificam as mesmas coisas em todas as linguagens: a versão, o login REST, uma leitura REST tipada (uma string não ASCII e um inteiro, o que pega tanto erro de codificação quanto de layout de struct), um 404 mapeado para o erro "parâmetro inválido", um snapshot CGI com os metadados de placa do JPEG, e uma conexão do cliente binário que falha rápido. Uma funcionalidade nova no SDK acrescenta sua verificação ao stand-in e ao smoke test de cada linguagem, no mesmo pull request.

## O contrato de compatibilidade

Comparado em todo pull request com o release em [`compat/baseline`](../compat/baseline):

- **ABI** da `libitscam_sdk.so` em x64, ARMv7 e ARM64, pelo `abidiff` (libabigail) sobre os headers públicos: as funções C e cada função, variável e layout de tipo C++ que os headers expõem. Acrescentar é permitido. Remover um símbolo, ou mudar uma assinatura ou o layout de um tipo que a API passa, quebra aplicações compiladas contra o release que não forem recompiladas.
- **API** de cada wrapper: uma listagem das declarações públicas dos wrappers Python, Go, Node.js (typings), C# e Java. Acrescentar é permitido. Uma declaração removida ou alterada quebra o código-fonte de quem chama.

O que conta como quebra, além do que as ferramentas veem:

- Comportamento de que quem chama depende: o código de erro de uma falha, um valor padrão, a thread ou a ordem de um callback, uma unidade.
- Uma exigência nova no alvo: glibc mais nova, uma biblioteca que o alvo precise ter, um runtime de linguagem mais novo que as tabelas acima.

## Mudanças que quebram

Enquanto a versão major for 0, um release minor pode quebrar quem chama. Um release patch nunca quebra. Toda quebra precisa:

1. **Ser aceita em [`compat/`](../compat)**, para quem revisa ver:
   - ABI: uma supressão do libabigail em [`compat/abi.suppr`](../compat/abi.suppr), com um comentário dizendo o que mudou e o que quem chama precisa fazer.
   - API: a linha removida, como o `check-compat.sh` imprimiu, em [`compat/accepted-api-breaks.txt`](../compat/accepted-api-breaks.txt), prefixada pela superfície (`python`, `go`, `nodejs`, `csharp`, `java`).
2. **Sair no próximo release minor** (0.x.0), nunca num patch.
3. **Ter uma nota de release** dizendo o que quebra e o que quem chama deve fazer, p.ex. "aplicações C++ que chamam `getRestApiClientConfig` precisam ser recompiladas".

Depois de um release, o commit dele move `compat/baseline` para a tag nova e esvazia `abi.suppr` e `accepted-api-breaks.txt` (mantendo os comentários do cabeçalho).

O soname da biblioteca compartilhada é `libitscam_sdk.so.0` em todo release 0.x, então o carregador dinâmico não distingue um release 0.x que quebra quem chama de um que não quebra. As aplicações precisam ser recompiladas quando uma nota de release mandar.

## Proteções para mudanças

- API C ([`src/core/c_api`](../src/core/c_api)): nunca remova nem mude uma função, valor de enum ou campo de struct. Acrescente funções novas.
- API C++: uma struct que a API passa por valor ou devolve (os tipos REST, `Result<T>`) muda de layout quando ganha um campo. Isso é uma quebra: veja acima.
- Uma funcionalidade nova chega a todos os wrappers no mesmo pull request ([`adding-a-new-wrapper.md`](adding-a-new-wrapper.md)), junto com as verificações nos smoke tests.
- Uma exigência nova de build (compilador, glibc, runtime) atualiza as tabelas acima, as imagens Docker e o `make verify` no mesmo pull request.
- Sistemas de build: o Makefile gera os artefatos do release e o CMake gera para quem consome. Os dois usam as mesmas fontes e flags, e o `make verify` compila os dois.
