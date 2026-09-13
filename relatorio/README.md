# Relatório

## Compilar

```sh
pdflatex relatorio.tex
pdflatex relatorio.tex   # segunda passada, para o sumário
```

Gera `relatorio.pdf`.

## Antes de entregar

Preencher os nomes da equipe no comando `\author{}`, no início de
`relatorio.tex`.

## Conteúdo

```
relatorio.tex     o relatório (7 seções exigidas pelo enunciado)
relatorio.pdf     versão compilada
imagens/          17 figuras, todas geradas pelo próprio programa
```

As figuras em `imagens/` saíram da tecla `s` do programa (pipeline completo
com gravação), convertidas de BMP para PNG. Para regerar: rodar o programa em
`../codigo`, apertar `s` em cada imagem de teste e converter os arquivos de
`codigo/saida/`.
