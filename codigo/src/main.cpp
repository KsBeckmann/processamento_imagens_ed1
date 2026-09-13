// **********************************************************************
// COMPUTAÇÃO GRÁFICA
//
// **********************************************************************
//
// ImageTest.cpp
//
// Programa de testes para manipulação de Imagens
//
//		Este programa deve ser compilador junto com a classe "ImageClass",
//		que está implementada no arquivo "ImageClass.cpp"
//
//		- para compilar no Visual C ou Visual Studio acrescente as seguintes
//        bibliotecas:
//					 opengl32.lib glu32.lib glut32.lib
//
//		- para compilar no DEVCPP ou CodeBlocks (ou no g++) acrescente as seguintes
//        bibliotecas:
//		      -lopengl32 -lglut32 -lglu32
//
//		- para alterar a imagem que é carregada pelo programa, olhe a
//		  rotina 'init' e altere a linha:
//							r = Image->Load("    ");
//
// **********************************************************************


// No Windows, <GL/gl.h> depende de tipos declarados em <windows.h> (APIENTRY,
// WINGDIAPI) e nao compila se este vier depois. No Linux o cabecalho nao
// existe nem e' necessario.
#ifdef _WIN32
#include <windows.h>
#endif

#include <stdio.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>

#include <GL/gl.h>
#include <GL/glut.h>

#include "pdi.h"

int LIMIAR = 100;
bool OBJETO_CLARO = true;   // polaridade usada pelo pipeline completo
int  AREA_MINIMA  = 50;     // componentes menores que isto sao descartados
bool AUTO_POLARIDADE = true; // deduzir a polaridade no inicio de cada roteiro

PDI pdi;

// ---------------------------------------------------------------------------
//  ROTEIROS DE DEMONSTRACAO
//
//  Cada roteiro e' uma sequencia de passos executados automaticamente, com
//  1 segundo de intervalo, disparados pelo menu do botao direito. A ideia e'
//  poder conduzir a apresentacao sem precisar lembrar de tecla nenhuma.
// ---------------------------------------------------------------------------

int  demoAtual   = 0;     // roteiro em execucao (0 = nenhum)
int  demoPasso   = 0;     // passo corrente dentro do roteiro
bool demoTerminou = false;// ultimo passo ja foi executado
unsigned demoSemente = 0; // semente do rand(), para o roteiro ser reproduzivel

char demoTitulo[128] = "";   // desenhado no alto da janela
char demoLegenda[192] = "";  // idem, linha de baixo

vector<unsigned char> demoGuardado;   // copia de 'cinza' para repetir um trecho

int  janelaLargura = 800;
int  janelaAltura  = 400;

void iniciaDemo(int op);
void MenuFuncoes(int op);
void posicionaImagens();
void avancaDemo();
void voltaDemo();

// Coloca as duas imagens lado a lado, centradas na largura e logo abaixo da
// faixa reservada as legendas. E' recalculado a cada reshape e a cada troca de
// imagem, de modo que o layout funciona em qualquer tamanho de janela -
// inclusive sob gerenciadores tiling, que ignoram glutReshapeWindow.
void posicionaImagens()
{
    if(pdi.Imagem == NULL || pdi.NovaImagem == NULL)
        return;

    int iw = pdi.Imagem->getSizeX();
    int ih = pdi.Imagem->getSizeY();
    if(iw <= 0 || ih <= 0) return;

    // Espaco util: metade da largura para cada imagem, menos a faixa de
    // legendas no alto. Se as imagens nao couberem, sao reduzidas por zoom
    // ate caber - nunca ampliadas.
    float dispW = (janelaLargura - 30) / 2.0f;
    float dispH = janelaAltura - 88.0f;

    float zoom = 1.0f;
    if(iw * zoom > dispW) zoom = dispW / iw;
    if(ih * zoom > dispH) zoom = dispH / ih;
    if(zoom > 1.0f) zoom = 1.0f;
    if(zoom <= 0.0f) zoom = 0.05f;

    pdi.Imagem->SetZoomH(zoom);     pdi.Imagem->SetZoomV(zoom);
    pdi.NovaImagem->SetZoomH(zoom); pdi.NovaImagem->SetZoomV(zoom);

    int dw = (int)(iw * zoom);
    int dh = (int)(ih * zoom);

    int x = (janelaLargura - (2*dw + 20)) / 2;
    if(x < 5) x = 5;

    int y = janelaAltura - 78 - dh;   // faixa no alto para as tres linhas de texto
    if(y < 5) y = 5;

    pdi.Imagem->setPos(x, y);
    pdi.NovaImagem->setPos(x + dw + 20, y);
}

// Pede a janela do tamanho da imagem. Gerenciadores tiling ignoram este
// pedido, mas posicionaImagens() cuida do layout de qualquer maneira.
void ajustaJanela()
{
    int w = pdi.Imagem->getSizeX() * 2 + 30;
    int h = pdi.Imagem->getSizeY() + 88;
    if(w < 560) w = 560;
    if(h < 240) h = 240;
    glutReshapeWindow(w, h);
    posicionaImagens();
}
// **********************************************************************
//  void init(void)
//		Inicializa os parâmetros globais de OpenGL
//      Cria os objetos que representam as imagens
//
// **********************************************************************
void init(void)
{
    glClearColor(0.0f, 0.0f, 1.0f, 1.0f); // Fundo de tela azul

    pdi.Init();
}
// **********************************************************************
//  void reshape( int w, int h )
//		trata o redimensionamento da janela OpenGL
//
// **********************************************************************
void reshape( int w, int h )
{
    // Reset the coordinate system before modifying
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    // Set the viewport to be the entire window
    janelaLargura = w;
    janelaAltura  = h;
    glViewport(0, 0, w, h);
    gluOrtho2D(0,w,0,h);
    posicionaImagens();

    // Set the clipping volume
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}
// **********************************************************************
//  void display( void )
//      Esta rotina é chamada toda vez que a tela precisa ser
//      redesenhada e toda vez qua a rotina 'glutPostRedisplay' é chamada
//
// **********************************************************************
void desenhaTexto(int x, int y, void* fonte, const char* txt)
{
    glRasterPos2i(x, y);
    for(const char* c = txt; *c; c++)
        glutBitmapCharacter(fonte, *c);
}

void display( void )
{
    glClear(GL_COLOR_BUFFER_BIT);

    glMatrixMode(GL_MODELVIEW);
    pdi.Display();

    // legendas do roteiro, no alto da janela
    if(demoTitulo[0]) {
        glColor3f(1.0f, 1.0f, 1.0f);
        desenhaTexto(12, janelaAltura - 24, GLUT_BITMAP_HELVETICA_18, demoTitulo);
        glColor3f(1.0f, 0.9f, 0.3f);
        desenhaTexto(12, janelaAltura - 46, GLUT_BITMAP_HELVETICA_18, demoLegenda);

        if(demoAtual != 0) {
            glColor3f(0.75f, 0.75f, 0.75f);
            desenhaTexto(12, janelaAltura - 64, GLUT_BITMAP_HELVETICA_12,
                         demoTerminou ? "[P] etapa anterior"
                                      : "[N] proxima etapa    [P] etapa anterior");
        }
    }

    glutSwapBuffers();
}
// **********************************************************************
//  void keyboard ( unsigned char key, int x, int y )
//
//
// **********************************************************************
void keyboard ( unsigned char key, int x, int y )
{
    switch ( key )
    {
    case 27:        // Termina o programa qdo
        exit ( 0 );   // a tecla ESC for pressionada
        break;
    case '4':
        pdi.calculaHistograma();
        pdi.desenhaHistograma();
        glutPostRedisplay();
        break;
    case '5':
        pdi.filtroMedia3x3();
        pdi.mostraBuffer(pdi.cinza);
        glutPostRedisplay();
        break;
    case '6':
        pdi.filtroMediana3x3();
        pdi.mostraBuffer(pdi.cinza);
        glutPostRedisplay();
        break;
    case '7':
        pdi.adicionaRuido(5);
        pdi.mostraBuffer(pdi.cinza);
        glutPostRedisplay();
        break;
    case 'r':                    // recarrega o cinza original
        pdi.paraCinza();
        pdi.mostraBuffer(pdi.cinza);
        glutPostRedisplay();
        break;
    case '+':
        LIMIAR += 10;
        if(LIMIAR > 255) LIMIAR = 255;
        printf("LIMIAR = %d\n", LIMIAR);
        pdi.limiariza(LIMIAR, true);
        pdi.mostraBuffer(pdi.bin, 255);
        glutPostRedisplay();
        break;
    case '-':
        LIMIAR -= 10;
        if(LIMIAR < 0) LIMIAR = 0;
        printf("LIMIAR = %d\n", LIMIAR);
        pdi.limiariza(LIMIAR, true);
        pdi.mostraBuffer(pdi.bin, 255);
        glutPostRedisplay();
        break;
    case '8':                       // limiar automatico (Otsu)
        pdi.calculaHistograma();
        LIMIAR = pdi.otsu();
        printf("Otsu: LIMIAR = %d\n", LIMIAR);
        pdi.limiariza(LIMIAR, true);
        pdi.mostraBuffer(pdi.bin, 255);
        glutPostRedisplay();
        break;
    case 'i':                       // inverte a polaridade
        pdi.limiariza(LIMIAR, false);
        pdi.mostraBuffer(pdi.bin, 255);
        glutPostRedisplay();
        break;
    case 'e':                       // erosao
        pdi.erosao();
        pdi.mostraBuffer(pdi.bin, 255);
        glutPostRedisplay();
        break;
    case 'd':                       // dilatacao
        pdi.dilatacao();
        pdi.mostraBuffer(pdi.bin, 255);
        glutPostRedisplay();
        break;
    case 'a':                       // abertura  = erosao + dilatacao
        pdi.abertura();
        pdi.mostraBuffer(pdi.bin, 255);
        glutPostRedisplay();
        break;
    case 'f':                       // fechamento = dilatacao + erosao
        pdi.fechamento();
        pdi.mostraBuffer(pdi.bin, 255);
        glutPostRedisplay();
        break;
    case 'l':                       // rotula os componentes conexos
    {
        int n = pdi.rotulaComponentes();
        printf("Objetos encontrados: %d\n", n);
        pdi.desenhaObjetos();
        glutPostRedisplay();
        break;
    }
    case 'o':                       // analisa os objetos e imprime a tabela
    {
        int n = pdi.analisaObjetos(AREA_MINIMA);
        printf("Objetos apos filtro de area (>= %d px): %d\n", AREA_MINIMA, n);
        pdi.imprimeRelatorio(pdi.arquivos[pdi.imgAtual]);
        pdi.desenhaObjetos();
        glutPostRedisplay();
        break;
    }
    case '0':                       // PIPELINE COMPLETO (sem gravar)
        pdi.pipelineCompleto(OBJETO_CLARO, AREA_MINIMA, false);
        glutPostRedisplay();
        break;
    case 's':                       // PIPELINE COMPLETO + grava saida/
        pdi.pipelineCompleto(OBJETO_CLARO, AREA_MINIMA, true);
        glutPostRedisplay();
        break;
    case 'v':                       // alterna (inverte) a polaridade
        OBJETO_CLARO = !OBJETO_CLARO;
        AUTO_POLARIDADE = false;    // escolha manual desliga a deteccao
        printf("Polaridade: objeto %s (deteccao automatica desligada)\n",
               OBJETO_CLARO ? "CLARO" : "ESCURO");
        break;
    case 'n':                       // proxima etapa do roteiro
    case 'N':
        if(demoAtual == 0)
            printf("Nenhum roteiro em andamento. Abra o menu com o botao direito.\n");
        else if(demoTerminou)
            printf("Roteiro terminado. Use P para voltar ou escolha outro no menu.\n");
        else
            avancaDemo();
        break;
    case 'p':                       // etapa anterior do roteiro
    case 'P':
        if(demoAtual == 0)
            printf("Nenhum roteiro em andamento. Abra o menu com o botao direito.\n");
        else if(demoPasso <= 1)
            printf("Ja esta na primeira etapa do roteiro.\n");
        else
            voltaDemo();
        break;
    case ']':                       // area minima +25
        AREA_MINIMA += 25;
        printf("Area minima = %d px\n", AREA_MINIMA);
        break;
    case '[':                       // area minima -25
        AREA_MINIMA -= 25;
        if(AREA_MINIMA < 0) AREA_MINIMA = 0;
        printf("Area minima = %d px\n", AREA_MINIMA);
        break;
    case 'g':                       // gera a imagem de teste com ruido
        pdi.geraImagemRuidosa(5);
        glutPostRedisplay();
        break;
    default:
        break;
    }
}
// **********************************************************************
//  void arrow_keys ( int a_keys, int x, int y )
//
//
// **********************************************************************
void arrow_keys ( int a_keys, int x, int y )
{
    switch ( a_keys )
    {
    case GLUT_KEY_UP:       // When Up Arrow Is Pressed...
        glutFullScreen ( ); // Go Into Full Screen Mode
        break;
    case GLUT_KEY_LEFT:
        pdi.anteImagem();
        pdi.Reload();
        ajustaJanela();
        pdi.mostraBuffer(pdi.cinza);
        glutPostRedisplay();
        break;
    case GLUT_KEY_RIGHT:
        pdi.proxImagem();
        pdi.Reload();
        ajustaJanela();
        pdi.mostraBuffer(pdi.cinza);
        glutPostRedisplay();
        break;
    default:
        break;
    }
}
// **********************************************************************
//  MENU E ROTEIROS DE DEMONSTRACAO
// **********************************************************************

// Executa um passo do roteiro. Devolve false quando o roteiro acabou.
// 'legenda' recebe o texto que vai aparecer na tela.
bool passoDemo(int demo, int passo, char* titulo, char* legenda)
{
    int n;

    switch(demo)
    {
    // -------------------------------------------------------------------
    case 1:   // PIPELINE COMPLETO
        strcpy(titulo, "Pipeline completo");
        switch(passo) {
        case 0:
            strcpy(legenda, "1/6  Imagem original convertida para tons de cinza");
            pdi.paraCinza();
            pdi.mostraBuffer(pdi.cinza);
            return true;
        case 1:
            strcpy(legenda, "2/6  Pre-processamento: filtro de mediana 3x3");
            pdi.filtroMediana3x3();
            pdi.mostraBuffer(pdi.cinza);
            return true;
        case 2:
            strcpy(legenda, "3/6  Histograma: onde esta o vale entre fundo e objetos");
            pdi.calculaHistograma();
            pdi.desenhaHistograma();
            return true;
        case 3:
            pdi.calculaHistograma();
            sprintf(legenda, "4/6  Limiarizacao automatica (Otsu): limiar = %d", pdi.otsu());
            pdi.limiariza(pdi.otsu(), OBJETO_CLARO);
            pdi.mostraBuffer(pdi.bin, 255);
            return true;
        case 4:
            strcpy(legenda, "5/6  Morfologia: abertura + fechamento");
            pdi.abertura();
            pdi.fechamento();
            pdi.mostraBuffer(pdi.bin, 255);
            return true;
        case 5:
            pdi.rotulaComponentes();
            n = pdi.analisaObjetos(AREA_MINIMA);
            sprintf(legenda, "6/6  Rotulacao e analise: %d objetos (tabela no terminal)", n);
            pdi.imprimeRelatorio(pdi.arquivos[pdi.imgAtual]);
            pdi.desenhaObjetos();
            return true;
        }
        return false;

    // -------------------------------------------------------------------
    case 2:   // MEDIA x MEDIANA
        strcpy(titulo, "Ruido: filtro de media x filtro de mediana");
        switch(passo) {
        case 0:
            strcpy(legenda, "1/5  Imagem limpa");
            pdi.paraCinza();
            pdi.mostraBuffer(pdi.cinza);
            return true;
        case 1:
            strcpy(legenda, "2/5  Com 5% de ruido sal-e-pimenta");
            pdi.adicionaRuido(5);
            demoGuardado = pdi.cinza;          // guarda para repetir o trecho
            pdi.mostraBuffer(pdi.cinza);
            return true;
        case 2:
            strcpy(legenda, "3/5  Filtro de MEDIA: cada ponto vira uma mancha 3x3");
            pdi.filtroMedia3x3();
            pdi.mostraBuffer(pdi.cinza);
            return true;
        case 3:
            strcpy(legenda, "4/5  Voltando a mesma imagem ruidosa...");
            pdi.cinza = demoGuardado;
            pdi.mostraBuffer(pdi.cinza);
            return true;
        case 4:
            strcpy(legenda, "5/5  Filtro de MEDIANA: o ruido some e a borda fica intacta");
            pdi.filtroMediana3x3();
            pdi.mostraBuffer(pdi.cinza);
            return true;
        }
        return false;

    // -------------------------------------------------------------------
    case 3:   // POR QUE A MORFOLOGIA IMPORTA
        strcpy(titulo, "Por que a morfologia e o filtro de area importam");
        switch(passo) {
        case 0:
            strcpy(legenda, "1/5  Imagem limpa com 5% de ruido");
            pdi.paraCinza();
            pdi.adicionaRuido(5);
            pdi.mostraBuffer(pdi.cinza);
            return true;
        case 1:
            pdi.calculaHistograma();
            pdi.limiariza(pdi.otsu(), OBJETO_CLARO);
            n = pdi.rotulaComponentes();
            sprintf(legenda, "2/5  Limiarizando direto: %d componentes (deveriam ser poucos!)", n);
            pdi.mostraBuffer(pdi.bin, 255);
            return true;
        case 2:
            strcpy(legenda, "3/5  Mediana antes de limiarizar");
            pdi.paraCinza();
            pdi.adicionaRuido(5);
            pdi.filtroMediana3x3();
            pdi.calculaHistograma();
            pdi.limiariza(pdi.otsu(), OBJETO_CLARO);
            n = pdi.rotulaComponentes();
            sprintf(legenda, "3/5  Com mediana antes: caiu para %d componentes", n);
            pdi.mostraBuffer(pdi.bin, 255);
            return true;
        case 3:
            pdi.abertura();
            pdi.fechamento();
            n = pdi.rotulaComponentes();
            sprintf(legenda, "4/5  Abertura + fechamento: %d componentes", n);
            pdi.mostraBuffer(pdi.bin, 255);
            return true;
        case 4:
            n = pdi.analisaObjetos(AREA_MINIMA);
            sprintf(legenda, "5/5  Descartando area < %d px: %d objetos", AREA_MINIMA, n);
            pdi.imprimeRelatorio(pdi.arquivos[pdi.imgAtual]);
            pdi.desenhaObjetos();
            return true;
        }
        return false;

    // -------------------------------------------------------------------
    case 4:   // EROSAO x ABERTURA
        strcpy(titulo, "Erosao pura x abertura");
        switch(passo) {
        case 0:
            strcpy(legenda, "1/6  Imagem binarizada, com ruido");
            pdi.paraCinza();
            pdi.adicionaRuido(5);
            pdi.calculaHistograma();
            pdi.limiariza(pdi.otsu(), OBJETO_CLARO);
            demoGuardado = pdi.bin;
            pdi.mostraBuffer(pdi.bin, 255);
            return true;
        case 1:
            strcpy(legenda, "2/6  Erosao 1x: o ruido sai, mas os objetos encolhem");
            pdi.erosao();
            pdi.mostraBuffer(pdi.bin, 255);
            return true;
        case 2:
            strcpy(legenda, "3/6  Erosao 2x: encolhendo mais");
            pdi.erosao();
            pdi.mostraBuffer(pdi.bin, 255);
            return true;
        case 3:
            strcpy(legenda, "4/6  Erosao 3x: os objetos estao sendo destruidos");
            pdi.erosao();
            pdi.mostraBuffer(pdi.bin, 255);
            return true;
        case 4:
            strcpy(legenda, "5/6  Voltando a mesma imagem binaria...");
            pdi.bin = demoGuardado;
            pdi.mostraBuffer(pdi.bin, 255);
            return true;
        case 5:
            strcpy(legenda, "6/6  ABERTURA: limpa o ruido e devolve o tamanho original");
            pdi.abertura();
            pdi.mostraBuffer(pdi.bin, 255);
            return true;
        }
        return false;

    // -------------------------------------------------------------------
    case 5:   // LIMIAR
        strcpy(titulo, "A escolha do limiar");
        if(passo == 0) {
            strcpy(legenda, "1/5  Histograma da imagem");
            pdi.paraCinza();
            pdi.calculaHistograma();
            pdi.desenhaHistograma();
            return true;
        }
        if(passo >= 1 && passo <= 3) {
            int t[] = {40, 120, 200};
            sprintf(legenda, "%d/5  Limiar manual = %d", passo+1, t[passo-1]);
            pdi.limiariza(t[passo-1], OBJETO_CLARO);
            pdi.mostraBuffer(pdi.bin, 255);
            return true;
        }
        if(passo == 4) {
            pdi.calculaHistograma();
            sprintf(legenda, "5/5  Limiar automatico de Otsu = %d", pdi.otsu());
            pdi.limiariza(pdi.otsu(), OBJETO_CLARO);
            pdi.mostraBuffer(pdi.bin, 255);
            return true;
        }
        return false;
    }
    return false;
}

// Executa a proxima etapa do roteiro. Chamado pela tecla N.
//
// O avanco e' manual, e nao por temporizador: numa apresentacao voce precisa
// poder parar numa etapa enquanto explica ou responde uma pergunta, sem que a
// tela mude sozinha.
void avancaDemo()
{
    if(demoAtual == 0 || demoTerminou)
        return;

    if(!passoDemo(demoAtual, demoPasso, demoTitulo, demoLegenda)) {
        strcat(demoLegenda, "   [fim do roteiro]");
        demoTerminou = true;
        glutPostRedisplay();
        return;
    }

    demoPasso++;
    glutPostRedisplay();
}

// Volta uma etapa. Chamado pela tecla P.
//
// As etapas nao sao reversiveis - cada uma transforma os buffers em cima do
// resultado da anterior, e nao ha como desfazer uma erosao ou um filtro. Por
// isso voltar significa re-executar o roteiro desde o inicio ate a etapa
// desejada. Como e' tudo em memoria, o custo e' irrelevante e o resultado e'
// exatamente o mesmo de antes: srand() com uma semente fixa garante que o
// ruido sorteado se repita igual em cada re-execucao.
void voltaDemo()
{
    if(demoAtual == 0 || demoPasso <= 1)
        return;                     // ja esta na primeira etapa

    int alvo = demoPasso - 2;       // indice da etapa anterior a atual

    srand(demoSemente);
    pdi.paraCinza();
    for(int i = 0; i <= alvo; i++)
        passoDemo(demoAtual, i, demoTitulo, demoLegenda);

    demoPasso = alvo + 1;
    demoTerminou = false;
    glutPostRedisplay();
}

void iniciaDemo(int op)
{
    demoAtual = op;
    demoPasso = 0;
    demoTerminou = false;
    demoTitulo[0] = 0;
    demoLegenda[0] = 0;

    demoSemente = 20260928;        // semente fixa: o roteiro se repete igual
    srand(demoSemente);
    pdi.paraCinza();

    if(AUTO_POLARIDADE) {          // descobre sozinho de que lado esta o objeto
        pdi.calculaHistograma();
        OBJETO_CLARO = pdi.polaridadeAutomatica(pdi.otsu());
        printf("Polaridade detectada: objeto %s\n", OBJETO_CLARO ? "CLARO" : "ESCURO");
    }
    avancaDemo();                  // ja mostra a primeira etapa
}

// Menu de ajustes e de etapas avulsas
void MenuFuncoes(int op)
{
    switch(op)
    {
    case 100: OBJETO_CLARO = !OBJETO_CLARO;
              AUTO_POLARIDADE = false;   // escolha manual desliga a deteccao
              printf("Polaridade: objeto %s (deteccao automatica desligada)\n",
                     OBJETO_CLARO ? "CLARO" : "ESCURO");
              break;
    case 103: AUTO_POLARIDADE = !AUTO_POLARIDADE;
              printf("Deteccao automatica de polaridade: %s\n",
                     AUTO_POLARIDADE ? "LIGADA" : "desligada");
              break;
    case 101: AREA_MINIMA += 25; printf("Area minima = %d px\n", AREA_MINIMA); break;
    case 102: AREA_MINIMA -= 25;
              if(AREA_MINIMA < 0) AREA_MINIMA = 0;
              printf("Area minima = %d px\n", AREA_MINIMA); break;

    case 110: pdi.anteImagem(); pdi.Reload(); ajustaJanela();
              pdi.mostraBuffer(pdi.cinza); demoTitulo[0] = 0; break;
    case 111: pdi.proxImagem(); pdi.Reload(); ajustaJanela();
              pdi.mostraBuffer(pdi.cinza); demoTitulo[0] = 0; break;

    case 120: pdi.pipelineCompleto(OBJETO_CLARO, AREA_MINIMA, true);
              strcpy(demoTitulo, "Pipeline completo");
              strcpy(demoLegenda, "5 imagens gravadas em saida/");
              break;
    case 121: pdi.geraImagemRuidosa(5);
              strcpy(demoTitulo, "Imagem de teste com ruido");
              strcpy(demoLegenda, "gravada em ImagensGL/ - reinicie para navegar ate ela");
              break;

    case 130: pdi.paraCinza();          pdi.mostraBuffer(pdi.cinza); break;
    case 131: pdi.calculaHistograma();  pdi.desenhaHistograma();     break;
    case 132: pdi.filtroMedia3x3();     pdi.mostraBuffer(pdi.cinza); break;
    case 133: pdi.filtroMediana3x3();   pdi.mostraBuffer(pdi.cinza); break;
    case 134: pdi.adicionaRuido(5);     pdi.mostraBuffer(pdi.cinza); break;
    case 135: pdi.calculaHistograma();
              pdi.limiariza(pdi.otsu(), OBJETO_CLARO);
              pdi.mostraBuffer(pdi.bin, 255); break;
    case 136: pdi.erosao();             pdi.mostraBuffer(pdi.bin, 255); break;
    case 137: pdi.dilatacao();          pdi.mostraBuffer(pdi.bin, 255); break;
    case 138: pdi.abertura();           pdi.mostraBuffer(pdi.bin, 255); break;
    case 139: pdi.fechamento();         pdi.mostraBuffer(pdi.bin, 255); break;
    case 140: printf("Objetos: %d\n", pdi.rotulaComponentes());
              pdi.desenhaObjetos(); break;
    case 141: pdi.analisaObjetos(AREA_MINIMA);
              pdi.imprimeRelatorio(pdi.arquivos[pdi.imgAtual]);
              pdi.desenhaObjetos(); break;
    }
    glutPostRedisplay();
}

void CriaMenu()
{
    int roteiros = glutCreateMenu(iniciaDemo);
    glutAddMenuEntry("1. Pipeline completo, etapa por etapa", 1);
    glutAddMenuEntry("2. Ruido: filtro de media x mediana",   2);
    glutAddMenuEntry("3. Por que a morfologia importa",       3);
    glutAddMenuEntry("4. Erosao pura x abertura",             4);
    glutAddMenuEntry("5. A escolha do limiar",                5);

    int etapas = glutCreateMenu(MenuFuncoes);
    glutAddMenuEntry("Tons de cinza (recomecar)", 130);
    glutAddMenuEntry("Histograma",                131);
    glutAddMenuEntry("Adicionar 5% de ruido",     134);
    glutAddMenuEntry("Filtro de media 3x3",       132);
    glutAddMenuEntry("Filtro de mediana 3x3",     133);
    glutAddMenuEntry("Limiarizar (Otsu)",         135);
    glutAddMenuEntry("Erosao",                    136);
    glutAddMenuEntry("Dilatacao",                 137);
    glutAddMenuEntry("Abertura",                  138);
    glutAddMenuEntry("Fechamento",                139);
    glutAddMenuEntry("Rotular componentes",       140);
    glutAddMenuEntry("Analisar objetos + tabela", 141);

    int ajustes = glutCreateMenu(MenuFuncoes);
    glutAddMenuEntry("Inverter polaridade (objeto claro/escuro)", 100);
    glutAddMenuEntry("Deteccao automatica de polaridade on/off",   103);
    glutAddMenuEntry("Area minima +25 px",                        101);
    glutAddMenuEntry("Area minima -25 px",                        102);

    int imagens = glutCreateMenu(MenuFuncoes);
    glutAddMenuEntry("Imagem anterior", 110);
    glutAddMenuEntry("Proxima imagem",  111);

    int arquivos = glutCreateMenu(MenuFuncoes);
    glutAddMenuEntry("Rodar pipeline e gravar em saida/",   120);
    glutAddMenuEntry("Gerar imagem de teste com ruido",     121);

    glutCreateMenu(MenuFuncoes);
    glutAddSubMenu("Demonstracoes", roteiros);
    glutAddSubMenu("Etapas avulsas", etapas);
    glutAddSubMenu("Imagens",        imagens);
    glutAddSubMenu("Ajustes",        ajustes);
    glutAddSubMenu("Gravar",         arquivos);
    glutAttachMenu(GLUT_RIGHT_BUTTON);
}


// **********************************************************************
//  void main ( int argc, char** argv )
//
//
// **********************************************************************
int main ( int argc, char** argv )
{
    glutInit            ( &argc, argv );
    glutInitDisplayMode (GLUT_DOUBLE | GLUT_DEPTH | GLUT_RGB );
    glutInitWindowPosition (100,100);

    // Define o tamanho da janela gráfica do programa
    glutInitWindowSize  (800, 400);
    glutCreateWindow    ( "Image Loader" );

    init ();
    //glutFullScreen();

    glutDisplayFunc ( display );
    glutReshapeFunc ( reshape );
    glutKeyboardFunc ( keyboard );
    glutSpecialFunc ( arrow_keys );

    CriaMenu();     // menu criado UMA vez; antes era recriado a cada clique
    ajustaJanela(); // janela do tamanho da imagem carregada

    glutMainLoop ( );
    return 0;
}
