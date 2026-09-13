#include "pdi.h"
#include <dirent.h>
#include <algorithm>   // std::sort, usado pela mediana
#include <cstdlib>
#include <cctype>
#include <queue>
#include <cmath>
// Criacao de diretorio: a funcao e o cabecalho mudam de nome entre os
// sistemas, e no Windows ela nao recebe as permissoes.
#ifdef _WIN32
  #include <direct.h>
  #define CRIA_PASTA(p) _mkdir(p)
#else
  #include <sys/stat.h>
  #define CRIA_PASTA(p) mkdir(p, 0755)
#endif

// Pasta das imagens.
//
// E' um vetor de char, e nao um std::string global, de proposito. Um
// std::string global precisa ser CONSTRUIDO em tempo de execucao, e a ordem
// em que os objetos globais de arquivos .cpp diferentes sao construidos nao
// e' definida pela linguagem. Como o objeto global "pdi" (em main.cpp)
// consultava esta variavel durante a propria construcao, o programa podia ler
// uma string ainda nao inicializada e abortar - dependendo apenas da ordem em
// que o compilador resolvesse ligar os arquivos.
//
// Um vetor de char com iniciador constante ja esta pronto antes de qualquer
// construtor rodar, o que elimina o problema.
const char PASTA[] = "ImagensGL/";

PDI::PDI()
{
    // O construtor nao le o disco. Antes ele chamava carregaNomesImagens(),
    // mas como "pdi" e' um objeto global, isso acontecia antes de main() e
    // dependia da ordem de inicializacao entre arquivos. A leitura da pasta
    // passou para Init(), que roda depois que o programa ja comecou.
    imgAtual  = 0;
    imgLimite = 0;
    W = H = 0;
}

PDI::~PDI()
{
    //dtor
}
void PDI::alocaBuffers()
{
    W = Imagem->getSizeX();
    H = Imagem->getSizeY();
    cinza.assign(W*H, 0);
    bin.assign(W*H, 0);
    rotulo.assign(W*H, 0);
    objetos.clear();
}

void PDI::paraCinza()
{
    for(int y = 0; y < H; y++)
        for(int x = 0; x < W; x++)
            cinza[y*W + x] = Imagem->GetPointIntensity(x, y);
}

void PDI::mostraBuffer(const vector<unsigned char>& buf, int escala)
{
    for(int y = 0; y < H; y++)
        for(int x = 0; x < W; x++) {
            unsigned char v = buf[y*W + x] * escala;
            NovaImagem->DrawPixel(x, y, v, v, v);
        }
}

void PDI::calculaHistograma()
{
    for(int i = 0; i < 256; i++)
        histograma[i] = 0;

    for(int y = 0; y < H; y++)
        for(int x = 0; x < W; x++)
            histograma[ cinza[y*W + x] ]++;
}

void PDI::desenhaHistograma()
{
    // limpa o fundo
    for(int y = 0; y < H; y++)
        for(int x = 0; x < W; x++)
            NovaImagem->DrawPixel(x, y, 255, 255, 255);

    // maior contagem, para normalizar a altura
    int maior = 1;
    for(int i = 0; i < 256; i++)
        if(histograma[i] > maior) maior = histograma[i];

    int largura = W / 256;
    if(largura < 1) largura = 1;

    for(int i = 0; i < 256; i++)
    {
        int altura = (int)((double)histograma[i] / maior * (H - 1));
        for(int dx = 0; dx < largura; dx++)
        {
            int x = i * largura + dx;
            if(x >= W) break;
            for(int y = 0; y < altura; y++)
                NovaImagem->DrawPixel(x, y, 0, 0, 0);
        }
    }
}

void PDI::filtroMedia3x3()
{
    vector<unsigned char> orig = cinza;   // le daqui, escreve em cinza

    for(int y = 1; y < H-1; y++)
        for(int x = 1; x < W-1; x++) {
            int soma = 0;
            for(int dy = -1; dy <= 1; dy++)
                for(int dx = -1; dx <= 1; dx++)
                    soma += orig[(y+dy)*W + (x+dx)];
            cinza[y*W + x] = soma / 9;
        }
}

void PDI::filtroMediana3x3()
{
    vector<unsigned char> orig = cinza;
    unsigned char v[9];

    for(int y = 1; y < H-1; y++)
        for(int x = 1; x < W-1; x++) {
            int n = 0;
            for(int dy = -1; dy <= 1; dy++)
                for(int dx = -1; dx <= 1; dx++)
                    v[n++] = orig[(y+dy)*W + (x+dx)];

            sort(v, v+9);
            cinza[y*W + x] = v[4];   // elemento central de 9 ordenados
        }
}

void PDI::adicionaRuido(int pct)
{
    int n = (W * H * pct) / 100;
    for(int i = 0; i < n; i++) {
        int x = rand() % W;
        int y = rand() % H;
        cinza[y*W + x] = (rand() % 2) ? 0 : 255;
    }
}

void PDI::limiariza(int T, bool objetoClaro)
{
    for(int i = 0; i < W*H; i++) {
        bool objeto = objetoClaro ? (cinza[i] > T) : (cinza[i] <= T);
        bin[i] = objeto ? 1 : 0;
    }
}

int PDI::otsu()
{
    long long total = 0, somaTotal = 0;
    for(int i = 0; i < 256; i++) {
        total     += histograma[i];
        somaTotal += (long long)i * histograma[i];
    }

    long long pesoFundo = 0, somaFundo = 0;
    double melhorVar = -1.0;
    int melhorT = 0;

    for(int t = 0; t < 256; t++) {
        pesoFundo += histograma[t];
        if(pesoFundo == 0) continue;

        long long pesoObjeto = total - pesoFundo;
        if(pesoObjeto == 0) break;

        somaFundo += (long long)t * histograma[t];

        double mediaFundo  = (double)somaFundo / pesoFundo;
        double mediaObjeto = (double)(somaTotal - somaFundo) / pesoObjeto;

        // variancia entre classes
        double var = (double)pesoFundo * pesoObjeto *
                     (mediaFundo - mediaObjeto) * (mediaFundo - mediaObjeto);

        if(var > melhorVar) {
            melhorVar = var;
            melhorT = t;
        }
    }
    return melhorT;
}

void PDI::erosao()
{
    vector<unsigned char> orig = bin;

    for(int y = 1; y < H-1; y++)
        for(int x = 1; x < W-1; x++) {
            unsigned char menor = 1;              // AND da vizinhanca
            for(int dy = -1; dy <= 1; dy++)
                for(int dx = -1; dx <= 1; dx++)
                    if(orig[(y+dy)*W + (x+dx)] == 0)
                        menor = 0;
            bin[y*W + x] = menor;
        }
}

void PDI::dilatacao()
{
    vector<unsigned char> orig = bin;

    for(int y = 1; y < H-1; y++)
        for(int x = 1; x < W-1; x++) {
            unsigned char maior = 0;              // OR da vizinhanca
            for(int dy = -1; dy <= 1; dy++)
                for(int dx = -1; dx <= 1; dx++)
                    if(orig[(y+dy)*W + (x+dx)] == 1)
                        maior = 1;
            bin[y*W + x] = maior;
        }
}

void PDI::abertura()
{
    erosao();
    dilatacao();
}

void PDI::fechamento()
{
    dilatacao();
    erosao();
}

int PDI::rotulaComponentes()
{
    rotulo.assign(W*H, 0);
    objetos.clear();   // a lista de objetos descreve a rotulacao anterior

    int proximo = 0;
    for(int y = 0; y < H; y++)
        for(int x = 0; x < W; x++)
            if(bin[y*W + x] == 1 && rotulo[y*W + x] == 0) {
                proximo++;
                floodFill(x, y, proximo);
            }

    return proximo;
}

void PDI::floodFill(int x0, int y0, int rot)
{
    queue< pair<int,int> > fila;

    rotulo[y0*W + x0] = rot;
    fila.push(make_pair(x0, y0));

    while(!fila.empty()) {
        int x = fila.front().first;
        int y = fila.front().second;
        fila.pop();

        for(int dy = -1; dy <= 1; dy++)          // conectividade-8
            for(int dx = -1; dx <= 1; dx++) {
                int nx = x + dx;
                int ny = y + dy;

                if(nx < 0 || ny < 0 || nx >= W || ny >= H)
                    continue;

                if(bin[ny*W + nx] == 1 && rotulo[ny*W + nx] == 0) {
                    rotulo[ny*W + nx] = rot;
                    fila.push(make_pair(nx, ny));
                }
            }
    }
}
int PDI::calculaPerimetro(int id)
{
    int per = 0;
    for(int y = 0; y < H; y++)
        for(int x = 0; x < W; x++) {
            if(rotulo[y*W + x] != id) continue;

            // vizinhanca-4: se algum vizinho nao e' do mesmo objeto, e' borda
            bool borda = false;
            if(x == 0 || y == 0 || x == W-1 || y == H-1) borda = true;
            else if(rotulo[y*W + (x-1)] != id) borda = true;
            else if(rotulo[y*W + (x+1)] != id) borda = true;
            else if(rotulo[(y-1)*W + x] != id) borda = true;
            else if(rotulo[(y+1)*W + x] != id) borda = true;

            if(borda) per++;
        }
    return per;
}

int PDI::analisaObjetos(int areaMinima)
{
    objetos.clear();

    // quantos rotulos existem
    int maxRot = 0;
    for(int i = 0; i < W*H; i++)
        if(rotulo[i] > maxRot) maxRot = rotulo[i];
    if(maxRot == 0) return 0;

    // uma passada acumulando area, soma de coordenadas e bounding box
    vector<int>    area(maxRot+1, 0);
    vector<double> somaX(maxRot+1, 0.0), somaY(maxRot+1, 0.0);
    vector<int>    xmin(maxRot+1, W), ymin(maxRot+1, H);
    vector<int>    xmax(maxRot+1, -1), ymax(maxRot+1, -1);

    for(int y = 0; y < H; y++)
        for(int x = 0; x < W; x++) {
            int r = rotulo[y*W + x];
            if(r == 0) continue;
            area[r]++;
            somaX[r] += x;
            somaY[r] += y;
            if(x < xmin[r]) xmin[r] = x;
            if(y < ymin[r]) ymin[r] = y;
            if(x > xmax[r]) xmax[r] = x;
            if(y > ymax[r]) ymax[r] = y;
        }

    // descarta os pequenos e renumera os que ficam
    vector<int> novoId(maxRot+1, 0);
    int n = 0;
    for(int r = 1; r <= maxRot; r++)
        if(area[r] >= areaMinima)
            novoId[r] = ++n;

    for(int i = 0; i < W*H; i++)
        rotulo[i] = novoId[rotulo[i]];

    // com os rotulos ja renumerados, monta a lista
    objetos.resize(n);
    for(int r = 1; r <= maxRot; r++) {
        if(novoId[r] == 0) continue;

        Objeto o;
        o.id   = novoId[r];
        o.area = area[r];
        o.cx   = somaX[r] / area[r];
        o.cy   = somaY[r] / area[r];
        o.xmin = xmin[r];
        o.ymin = ymin[r];
        o.xmax = xmax[r];
        o.ymax = ymax[r];
        o.perimetro = calculaPerimetro(o.id);

        // circularidade = 4*PI*A / P^2 ; vale 1 no circulo perfeito
        if(o.perimetro > 0)
            o.circularidade = 4.0 * M_PI * o.area / ((double)o.perimetro * o.perimetro);
        else
            o.circularidade = 0.0;

        objetos[o.id - 1] = o;
    }

    return n;
}

void PDI::imprimeRelatorio(const string& nomeImagem)
{
    printf("\n========================================\n");
    printf("  ANALISE DA IMAGEM\n");
    printf("========================================\n\n");
    printf("Imagem: %s\n\n", nomeImagem.c_str());
    printf("Dimensoes: %d x %d\n\n", W, H);
    printf("Quantidade de objetos: %d\n\n", (int)objetos.size());

    printf("---------------------------------------------------------------------------\n");
    printf("%-7s %8s %14s %24s %10s %7s\n",
           "OBJETO", "AREA", "CENTROIDE", "BOUNDING BOX", "PERIMETRO", "CIRC");
    printf("---------------------------------------------------------------------------\n");

    for(size_t i = 0; i < objetos.size(); i++) {
        const Objeto& o = objetos[i];
        char cent[32], bbox[48];
        sprintf(cent, "(%.0f, %.0f)", o.cx, o.cy);
        sprintf(bbox, "(%d,%d)-(%d,%d)", o.xmin, o.ymin, o.xmax, o.ymax);
        printf("%-7d %8d %14s %24s %10d %7.3f\n",
               o.id, o.area, cent, bbox, o.perimetro, o.circularidade);
    }
    printf("---------------------------------------------------------------------------\n\n");
}

void PDI::desenhaObjetos()
{
    static const unsigned char cores[12][3] = {
        {230,  25,  75}, { 60, 180,  75}, {255, 225,  25}, {  0, 130, 200},
        {245, 130,  48}, {145,  30, 180}, { 70, 240, 240}, {240,  50, 230},
        {210, 245,  60}, {250, 190, 190}, {  0, 128, 128}, {170, 110,  40}
    };

    for(int y = 0; y < H; y++)
        for(int x = 0; x < W; x++) {
            int r = rotulo[y*W + x];
            if(r == 0)
                NovaImagem->DrawPixel(x, y, 0, 0, 0);
            else {
                const unsigned char* c = cores[(r-1) % 12];
                NovaImagem->DrawPixel(x, y, c[0], c[1], c[2]);
            }
        }

    for(size_t i = 0; i < objetos.size(); i++) {
        const Objeto& o = objetos[i];

        NovaImagem->DrawBox(o.xmin, o.ymin, o.xmax, o.ymax, 255, 255, 255);

        int cx = (int)(o.cx + 0.5);
        int cy = (int)(o.cy + 0.5);
        NovaImagem->DrawLineH(cy, cx-3, cx+3, 255, 255, 255);
        NovaImagem->DrawLineV(cx, cy-3, cy+3, 255, 255, 255);
    }
}

void PDI::salvaBuffer(const string& nome, const vector<unsigned char>& buf, int escala)
{
    CRIA_PASTA("saida");   // nao faz nada se a pasta ja existir

    for(int y = 0; y < H; y++)
        for(int x = 0; x < W; x++) {
            unsigned char v = buf[y*W + x] * escala;
            NovaImagem->DrawPixel(x, y, v, v, v);
        }

    string caminho = "saida/" + nome;
    NovaImagem->Save((char*)caminho.c_str());
}

int PDI::pipelineCompleto(bool objetoClaro, int areaMinima, bool salvar)
{
    paraCinza();
    if(salvar) salvaBuffer("1_original.bmp", cinza, 1);

    filtroMediana3x3();
    if(salvar) salvaBuffer("2_filtrada.bmp", cinza, 1);

    calculaHistograma();
    int T = otsu();
    limiariza(T, objetoClaro);
    if(salvar) salvaBuffer("3_binaria.bmp", bin, 255);

    abertura();
    fechamento();
    if(salvar) salvaBuffer("4_morfologia.bmp", bin, 255);

    rotulaComponentes();
    int n = analisaObjetos(areaMinima);

    imprimeRelatorio(arquivos[imgAtual]);
    printf("Limiar de Otsu: %d | area minima: %d px\n\n", T, areaMinima);

    desenhaObjetos();
    if(salvar) {
        CRIA_PASTA("saida");
        NovaImagem->Save((char*)"saida/5_rotulada.bmp");
    }

    return n;
}

void PDI::geraImagemRuidosa(int pct)
{
    paraCinza();
    adicionaRuido(pct);

    // grava em ImagensGL/ para virar mais uma imagem navegavel
    for(int y = 0; y < H; y++)
        for(int x = 0; x < W; x++) {
            unsigned char v = cinza[y*W + x];
            NovaImagem->DrawPixel(x, y, v, v, v);
        }

    string nome = arquivos[imgAtual];
    size_t ponto = nome.rfind('.');
    if(ponto != string::npos) nome = nome.substr(0, ponto);

    char destino[512];
    sprintf(destino, "%s%s-ruido%d.bmp", PASTA, nome.c_str(), pct);
    NovaImagem->Save(destino);
    printf("\nImagem ruidosa gravada: %s\n", destino);
    printf("Reinicie o programa para ela aparecer na navegacao.\n");
}

bool PDI::polaridadeAutomatica(int T)
{
    long claros = 0, escuros = 0;
    for(int i = 0; i < W*H; i++)
        if(cinza[i] > T) claros++;
        else             escuros++;

    return claros <= escuros;   // objeto = classe minoritaria
}

//-------------------------------------------------------
void PDI::Display()
{
    Imagem->Display();
    NovaImagem->Display();
}

void PDI::Init()
{
    // A leitura da pasta acontece aqui, e nao no construtor: ver o comentario
    // em PDI::PDI().
    carregaNomesImagens();
    imgLimite = arquivos.size();

    if(arquivos.empty()) {
        printf("Nenhuma imagem .bmp encontrada em %s\n", PASTA);
        printf("Execute o programa a partir da pasta 'codigo'.\n");
        exit(1);
    }

    // Cria um objeto imagem
	Imagem = new ImageClass();

	// carrega arquivo
	int r = Imagem->Load( string(PASTA) + arquivos[imgAtual] );

	if (!r)
    {
        printf("Imagem não encontrada. Verifique o nome do Arquivo.\n");
        printf("Pressione ENTRE para encerrar.");
        getchar();
		exit(1);
    }
	else printf ("Imagem carregada!\n");

	// Instacia o objeto que irá exibir a nova imagem
	// Caso precise alterar o tamanho da nova imagem, mude os parâmetros
	// da construtura, na chamada abaixo
	NovaImagem = new ImageClass(Imagem->getSizeX(), Imagem->getSizeY());

	// Posiciona as duas imagens lado a lado, com folga no alto para as legendas
	Imagem->setPos(10, 10);
	NovaImagem->setPos(Imagem->getSizeX() + 20, 10);

	alocaBuffers();
	paraCinza();
}
void PDI::Reload()
{
    // Imagem 1
    Imagem->Delete();
    bool ok = Imagem->Load(string(PASTA) + arquivos[imgAtual]);
    if (!ok)
        printf("\nImagem %s ignorada.\n", arquivos[imgAtual].c_str());
    Imagem->setPos(10, 10);

    // Imagem 2
    NovaImagem->Resize(Imagem->getSizeX(), Imagem->getSizeY());
	NovaImagem->setPos(Imagem->getSizeX() + 20, 10);

	alocaBuffers();
	paraCinza();
}

void PDI::anteImagem()
{
    imgAtual--;
    if(imgAtual < 0)
        imgAtual = imgLimite-1;
}

void PDI::proxImagem()
{
    imgAtual++;
    if(imgAtual >= imgLimite)
        imgAtual = 0;
}

void PDI::carregaNomesImagens()
{
    DIR *dir;
    struct dirent *diread;

    if ((dir = opendir(PASTA)) != NULL) {
        while ((diread = readdir(dir)) != NULL) {
            string nome = diread->d_name;

            // So entram arquivos com extensao conhecida. Isto tambem descarta
            // "." e "..", que antes eram removidos por posicao - o que so
            // funcionava quando readdir os devolvia nos dois primeiros lugares.
            size_t ponto = nome.rfind('.');
            if (ponto == string::npos)
                continue;

            string ext = nome.substr(ponto + 1);
            for (unsigned int i = 0; i < ext.length(); i++)
                ext[i] = tolower(ext[i]);

            if (ext == "bmp")
                arquivos.push_back(nome);
        }
        closedir (dir);
    } else {
        perror ("opendir");
        return;
    }

    printf("\n\n");
    sort(arquivos.begin(), arquivos.end());   // ordem estavel entre execucoes
    for (auto file : arquivos)
        cout << file << "\n";
    cout << endl;
    return;
}
