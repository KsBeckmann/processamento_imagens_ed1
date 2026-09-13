# Processamento de Imagens — Avaliação Bimestral (1º Bimestre)

Sistema de análise automática de objetos em uma imagem.
UTP — Ciência da Computação — Prof. Diógenes Furlan.

## Como compilar e executar

O mesmo código-fonte compila nos dois sistemas. O `Makefile` detecta a
plataforma e troca as bibliotecas de ligação sozinho; as diferenças de API
entre os sistemas estão isoladas em blocos `#ifdef _WIN32` no código.

---

### Linux

**1. Instalar as dependências**

Arch / Manjaro:

```sh
sudo pacman -S --needed base-devel freeglut glu mesa
```

Debian / Ubuntu / Mint:

```sh
sudo apt install build-essential freeglut3-dev libglu1-mesa-dev
```

Fedora:

```sh
sudo dnf install gcc-c++ make freeglut-devel mesa-libGLU-devel
```

**2. Compilar e executar**

Dentro da pasta `codigo`:

```sh
make
./pdi
```

Ou, em um comando só:

```sh
make run
```

Para recompilar do zero: `make clean && make`.

---

### Windows

O ambiente usado na disciplina é **Code::Blocks com MinGW**. O projeto
`pdi.cbp` já vem configurado, com os diretórios de inclusão e as três
bibliotecas necessárias (`freeglut`, `opengl32`, `glu32`).

**1. Instalar o freeglut**

O MinGW não traz o freeglut por padrão. Duas formas de obtê-lo:

*Opção A --- reaproveitar o da disciplina.* O arquivo `PDI-Aula-2026.zip`
distribuído em aula já contém `lib/libfreeglut.a` e `bin/Debug/freeglut.dll`.
Copie:

- `libfreeglut.a` para a pasta `lib` da sua instalação do MinGW;
- os cabeçalhos do freeglut (`glut.h`, `freeglut.h`, `freeglut_std.h`,
  `freeglut_ext.h`) para a pasta `include\GL` do MinGW;
- `freeglut.dll` para **a mesma pasta do `pdi.exe`**.

*Opção B --- MSYS2.* Se você usa MSYS2, basta:

```sh
pacman -S mingw-w64-x86_64-freeglut mingw-w64-x86_64-gcc mingw-w64-x86_64-make
```

**2. Compilar**

Pelo Code::Blocks: abrir `pdi.cbp` e usar *Build → Rebuild* (ou `Ctrl+F11`).

Pela linha de comando, no prompt do MinGW ou do MSYS2, dentro da pasta
`codigo`:

```sh
mingw32-make
pdi.exe
```

**3. Se der erro**

| Mensagem | Causa provável |
|---|---|
| `GL/glut.h: No such file or directory` | cabeçalhos do freeglut não estão em `include\GL` |
| `cannot find -lfreeglut` | `libfreeglut.a` não está na pasta `lib` do MinGW |
| `freeglut.dll not found` ao executar | falta a DLL ao lado do `pdi.exe` |
| `undefined reference to glClear...` | faltou ligar `opengl32`/`glu32` (use o `pdi.cbp`) |

---

### Nos dois sistemas

**Execute a partir da pasta `codigo`.** O caminho das imagens é relativo
(`ImagensGL/`). Rodando de outro diretório, o programa avisa e encerra:

```
Nenhuma imagem .bmp encontrada em ImagensGL/
Execute o programa a partir da pasta 'codigo'.
```

No Code::Blocks, confira em *Project → Properties → Build targets* se o
diretório de execução aponta para a pasta do projeto.

**Saída esperada na primeira execução.** O terminal lista as imagens
encontradas e confirma a carga da primeira:

```
abbey.bmp
C1.bmp
...
shells.bmp

8 bits. Imagem carregada!
```

e abre a janela **Image Loader** com a imagem original à esquerda e a área de
resultado à direita. A partir daí, o botão direito abre o menu.

**Teste rápido de que está tudo certo:** navegue até `Circulos4.bmp` com a
seta direita e aperte `0`. O terminal deve imprimir uma tabela com
**4 objetos**.

## Menu (botão direito)

Tudo o que o programa faz está no menu, organizado em cinco grupos. As
**Demonstrações** são roteiros encadeados: escolha um no menu e caminhe por
ele no seu ritmo, com `N` para avançar e `P` para voltar. Cada etapa mostra na própria janela o
que está acontecendo, com os números calculados na hora.

| Menu | Conteúdo |
|---|---|
| Demonstrações | 5 roteiros automáticos (ver abaixo) |
| Etapas avulsas | cada operação do pipeline, uma de cada vez |
| Imagens | navegar entre as imagens de `ImagensGL/` |
| Ajustes | polaridade e área mínima |
| Gravar | rodar o pipeline salvando em `saida/`, gerar imagem com ruído |

### Roteiros

1. **Pipeline completo, etapa por etapa** — cinza → mediana → histograma → Otsu → morfologia → rotulação
2. **Ruído: filtro de média x mediana** — mostra a média borrando e a mediana preservando a borda
3. **Por que a morfologia importa** — a contagem de objetos caindo de milhares para o número certo
4. **Erosão pura x abertura** — três erosões destruindo os objetos, contra uma abertura que preserva
5. **A escolha do limiar** — três limiares manuais e depois o de Otsu

A polaridade (objetos claros ou escuros) é **detectada automaticamente** no
início de cada roteiro, então não é preciso ajustar nada ao trocar de imagem.

## Controles

**Pipeline completo**

| Tecla | Efeito |
|---|---|
| `0` | Roda o pipeline inteiro e imprime a tabela de objetos |
| `s` | Idem, gravando as 5 imagens intermediárias em `saida/` |
| `v` | Alterna a polaridade (objeto claro / escuro) |
| `n` / `p` | Próxima etapa / etapa anterior do roteiro |
| `]` / `[` | Área mínima +25 / -25 px |
| `g` | Gera uma cópia da imagem atual com 5% de ruído em `ImagensGL/` |

**Etapas isoladas**

| Tecla | Efeito |
|---|---|
| `r` | Restaura a imagem em tons de cinza original |
| `4` | Histograma |
| `5` / `6` | Filtro de média / mediana 3x3 |
| `7` | Adiciona 5% de ruído sal-e-pimenta |
| `8` | Limiariza com o limiar de Otsu |
| `+` / `-` | Limiar manual, de 10 em 10 |
| `i` | Inverte a polaridade da limiarização |
| `e` / `d` | Erosão / dilatação |
| `a` / `f` | Abertura / fechamento |
| `l` | Rotula os componentes conexos |
| `o` | Analisa os objetos e imprime a tabela |

**Navegação**

| Tecla | Efeito |
|---|---|
| `←` `→` | Imagem anterior / próxima de `ImagensGL/` |
| `↑` | Tela cheia |
| botão direito | Menu |
| `ESC` | Sai |

## Estrutura

```
src/main.cpp        main, callbacks GLUT (display, keyboard, reshape)
src/pdi.cpp/.h      algoritmos de processamento
src/image_class.*   classe de imagem: carga, acesso a pixel, exibição via glDrawPixels
src/bmp_lib2.*      leitura/escrita de arquivos BMP (24 e 8 bits)
Makefile            compila no Linux e no Windows
pdi.cbp             projeto do Code::Blocks (Windows)
ImagensGL/          imagens de teste
saida/              imagens geradas pelo pipeline (criada na execução)
PORTE.md            notas do porte entre os dois sistemas
```

## Portabilidade

O código base foi fornecido para Windows/Code::Blocks e portado para Linux,
mantendo a compatibilidade com os dois sistemas. As alterações necessárias ---
separadores de caminho, cabeçalhos condicionais, criação de diretório,
tamanhos de tipos inteiros e ordem de inicialização de objetos globais ---
estão descritas em `PORTE.md`.
