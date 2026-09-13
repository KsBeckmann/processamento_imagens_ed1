# Porte do framework do Windows/CodeBlocks para Linux

O projeto original (`PDI-Aula-2026.zip`) foi feito para MinGW + CodeBlocks no
Windows. As alterações abaixo foram necessárias para compilar e executar no
Linux com g++ e freeglut.

## 1. Separador de diretório nos includes

`\` não é separador de caminho fora do Windows.

- `image_class.h`, `image_class.cpp`, `main.cpp`: `<GL\gl.h>` → `<GL/gl.h>`
- `pdi.h`: `"..\ImageClass.h"` → `"image_class.h"`

## 2. Cabeçalhos específicos do Windows

- `main.cpp` (originalmente `ImageTest.cpp`): removido `#include <windows.h>` (não era usado).
- `main.cpp`: `#include "glut.h"` → `#include <GL/glut.h>`.
  O `glut.h` que acompanha o projeto é de 1997 e inclui `<windows.h>`;
  foi removido em favor do freeglut do sistema.
- Removidos `util.cpp` / `util.h`: usam a API Win32 (`FindFirstFile`) e não
  faziam parte do projeto (`PDI-Aula.cbp`). A listagem de arquivos já é feita
  em `PDI::carregaNomesImagens()` com `dirent.h`, que é portável.

## 3. Nomes de arquivo diferenciam maiúsculas

- `main.cpp`: `#include "pdi.h"` → `#include "PDI.h"` (o arquivo chamava-se
  `PDI.h` na época; hoje é `pdi.h`, ver seção 8).
  No Windows funcionava; em sistemas de arquivos sensíveis a maiúsculas, não.

## 4. Cabeçalho C ausente

- `image_class.cpp`: adicionado `#include <cstring>`, usado por `memset` e
  `memcpy`.

## 5. Tamanho de `long` (bug crítico)

`bmp_lib2.cpp` lê os cabeçalhos do arquivo BMP diretamente para dentro de
structs, com `fread`. Os campos eram declarados como `unsigned long` / `long`.

- Windows (LLP64): `long` = 4 bytes → `sizeof(imginfoheader)` = 40 (correto).
- Linux x86-64 (LP64): `long` = 8 bytes → `sizeof(imginfoheader)` = 80.

Com 80 bytes, todos os campos ficam desalinhados em relação ao arquivo, a
validação `headersize != 40` falha e **toda** imagem é rejeitada com
`Error: Invalid bitmap image`.

Correção: incluído `<cstdint>` e os campos dos structs `bmpinfoheader` e
`imginfoheader` passaram a usar larguras fixas — `uint32_t` e `int32_t` —
que têm o mesmo tamanho em qualquer plataforma. As variáveis locais `long`
do restante do arquivo não influenciam o layout binário e foram mantidas.

## 6. Ponteiro não inicializado (bug crítico)

`ImageClass::ImageClass(int, int)` chama `Realloc()`, que chama `Delete()`,
que executa `if (data) free(data)` — mas `data` ainda não havia sido
inicializado nesse construtor. O valor era lixo de pilha.

No Windows o valor calhava de ser zero e o `if` protegia a chamada. No Linux
o programa aborta com `free(): invalid pointer` (confirmado com
AddressSanitizer). Correção: `data = NULL;` como primeira instrução do
construtor.

## 7. Caminho das imagens

`pdi.cpp`: `pasta` alterado de `"../ImagensGL/"` para `"ImagensGL/"`,
de modo que as imagens fiquem dentro do repositório e o programa seja
executado a partir da raiz do projeto.

Observação: `carregaNomesImagens()` remove as duas primeiras entradas
retornadas por `readdir()` supondo que sejam `.` e `..`, e não filtra por
extensão. Arquivos que não sejam BMP na pasta fazem a carga falhar — por isso
`casa1.jpg` não foi incluído.

## 8. Reorganização do projeto

O projeto original separava os arquivos em `include/` e `src/`, com o `main`
em `ImageTest.cpp` na raiz. A estrutura foi unificada: todo o código-fonte
(`.cpp` e `.h`) está em `src/`, e o arquivo do `main` foi renomeado para
`src/main.cpp`. A compilação usa `-Isrc` e o `Makefile` localiza os fontes
com `$(wildcard src/*.cpp)`, de modo que novos arquivos entram na build sem
edição adicional.

Os nomes de arquivo passaram para snake_case:

| Original          | Atual              |
|-------------------|--------------------|
| `ImageTest.cpp`   | `main.cpp`         |
| `PDI.cpp` / `.h`  | `pdi.cpp` / `.h`   |
| `ImageClass.*`    | `image_class.*`    |
| `BmpLib2.*`       | `bmp_lib2.*`       |

Os nomes de classes e métodos (`ImageClass`, `PDI`, `BMP`) foram mantidos
como no código original, para que a correspondência com o material da
disciplina continue direta.

## 9. Voltando a compilar também no Windows

O porte inicial deixou o código funcionando apenas no Linux. Os pontos abaixo
restabeleceram a compatibilidade com os dois sistemas ao mesmo tempo.

### Ordem dos cabeçalhos

No Windows, `<GL/gl.h>` usa tipos (`APIENTRY`, `WINGDIAPI`) declarados em
`<windows.h>` e não compila sem ele. No Linux esse cabeçalho não existe. A
inclusão passou a ser condicional, em `main.cpp` e em `image_class.h`:

```cpp
#ifdef _WIN32
#include <windows.h>
#endif
```

### Criação de diretório

`mkdir()` recebe permissões no POSIX e não no Windows, onde a função se chama
`_mkdir()` e vem de `<direct.h>`. Em `pdi.cpp`:

```cpp
#ifdef _WIN32
  #include <direct.h>
  #define CRIA_PASTA(p) _mkdir(p)
#else
  #include <sys/stat.h>
  #define CRIA_PASTA(p) mkdir(p, 0755)
#endif
```

### Bibliotecas de ligação

O `Makefile` distingue os dois casos pela variável `OS`, que só existe no
Windows: `-lglut -lGLU -lGL` no Linux, `-lfreeglut -lopengl32 -lglu32` no
Windows. Foi acrescentado também `pdi.cbp`, projeto do Code::Blocks já
configurado.

### Verificação dos cabeçalhos BMP

Os dois structs lidos byte a byte do arquivo BMP passaram a ter o tamanho
verificado em tempo de compilação:

```cpp
static_assert(sizeof(bmpinfoheader) == 16, "cabecalho BMP com tamanho inesperado");
static_assert(sizeof(imginfoheader) == 40, "cabecalho DIB com tamanho inesperado");
```

Se algum compilador inserir preenchimento entre os campos, o erro aparece na
compilação em vez de virar uma imagem corrompida em tempo de execução.

### Observação sobre `dirent.h`

A listagem da pasta usa `opendir`/`readdir`, que o MinGW fornece. Com o
compilador da Microsoft (MSVC) seria necessário substituir por `FindFirstFile`.
O ambiente da disciplina é Code::Blocks com MinGW, onde funciona.

## 10. Ordem de inicialização de objetos globais

Este defeito vinha do código original e não se manifestava por sorte na ordem
de ligação. O objeto `pdi` é global, e seu construtor chamava
`carregaNomesImagens()`, que lê a variável global `pasta` — um `std::string`
declarado em outro arquivo `.cpp`.

A linguagem não define a ordem em que objetos globais de arquivos diferentes
são construídos. Se `pdi` fosse construído primeiro, `pasta` ainda seria uma
string não inicializada, e `pasta.c_str()` devolveria um ponteiro inválido.
O programa abortava antes de chegar ao `main()`.

Duas mudanças eliminam a dependência:

1. `pasta` virou `const char PASTA[] = "ImagensGL/"`. Um vetor de `char` com
   iniciador constante não precisa ser construído em tempo de execução: já
   está pronto antes de qualquer construtor rodar.
2. `carregaNomesImagens()` saiu do construtor e passou para `Init()`, que é
   chamada depois que o programa já começou. O construtor agora apenas zera
   contadores.

Verificado ligando os arquivos objeto em três ordens diferentes: o programa
carrega a imagem corretamente nas três.
