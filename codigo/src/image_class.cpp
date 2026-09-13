#include <stdio.h>
#include <stdlib.h>

#include <GL/gl.h>

#include "image_class.h"
#include "bmp_lib2.h"
#include <cstring>
#include <cctype>

#define TAM_PIXEL 3

#define COLOR_WHITE 255

// **********************************************************************
string extensao(string arquivo)
{
    int tam = arquivo.length();
    int i;
    for(i=tam-1; arquivo[i]!='.'; i--);

    return arquivo.substr(i+1);
}
// **********************************************************************
//
//	Constructor
// **********************************************************************
ImageClass::ImageClass()
{
	data = NULL;
	setSize(0,0);

	posX = posY = 0;
	zoomH = zoomV = 1;

    // NOVO
    // Isto resolve o problema de ter a imagem com largura múltipla de 4
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
}
// **********************************************************************
//
//	Constructor
// **********************************************************************
ImageClass::ImageClass(int sizeX, int sizeY)
{
    data = NULL;   // PORTE: sem isto Realloc->Delete() faz free() de lixo
    Realloc(sizeX,sizeY);

    posX = posY = 0;
	zoomH = zoomV = 1;

    // NOVO
    // Isto resolve o problema de ter a imagem com largura múltipla de 4
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
}
// **********************************************************************
//
//
// **********************************************************************
void ImageClass::Realloc(int sizeX, int sizeY)
{
	unsigned int tam;

	Delete();

	setSize(sizeX, sizeY);

	tam = sizeof(unsigned char) * sizeX * sizeY * TAM_PIXEL;
	data = (unsigned char *) malloc (tam);
	memset(data,255,tam);
}
// **********************************************************************
//
//
// **********************************************************************
bool ImageClass::Load(string nome)
{
    string ext = (extensao(nome));
    for (unsigned int i = 0; i < ext.length(); i++)
        ext[i] = tolower(ext[i]);          // aceita .BMP alem de .bmp

    if (ext == "bmp")
    {
        if (data) {
            free(data);   // é necessário desalocar a área da imagem
            data = NULL;  // sem isto, Realloc->Delete() libera o mesmo bloco de novo
        }

        if (BMP::load(nome.c_str(), sizeX, sizeY))
        {
            Realloc(sizeX, sizeY);
            BMP::copy_to_ImageClass(data);
            BMP::free_memory();
            return true;
        }
    }
    else
    {
        // futuros tipos de imagens
    }

    // Falhou: zera as dimensoes. Se elas continuassem com os valores da
    // imagem anterior, quem chama leria data (agora nulo) achando que a
    // imagem ainda tem aquele tamanho.
    setSize(0, 0);
    printf("\nErro: nao foi possivel carregar %s", nome.c_str());
    return false;
}
// **********************************************************************
//
//
// **********************************************************************
void ImageClass::Save(char *nome)
{
    if( BMP::save(nome, data, sizeX, sizeY) )
        printf("\nArquivo salvo com sucesso");
}
// **********************************************************************
//
//
// **********************************************************************
void ImageClass::Display()
{
	glPixelZoom(zoomH , zoomV);
	glRasterPos2f(posX, posY);
	glDrawPixels(sizeX, sizeY, GL_RGB, GL_UNSIGNED_BYTE, data);
//  glDrawPixels(sizeX, sizeY, GL_BGR_EXT, GL_UNSIGNED_BYTE, data);
}
// **********************************************************************
//
//
// **********************************************************************
void ImageClass::Delete()
{
	// Cleanup
	if (data)
	{
		free(data);
		data = NULL;
    }
}
// **********************************************************************
//
//
// **********************************************************************
void ImageClass::DrawPixel(GLint x, GLint y, unsigned char r, unsigned char g, unsigned char b)
{
	unsigned long addr;

	addr = (unsigned long)( y *(sizeX)* TAM_PIXEL + x * TAM_PIXEL );
	data[addr++] = r;
	data[addr++] = g;
	data[addr] = b;
}
// **********************************************************************
//
//
//
// **********************************************************************
void ImageClass::DrawLineH(int y, int x1, int x2,unsigned char r, unsigned char g, unsigned char b )
{
	int x;
	if (x1 <= x2)
		for (x = x1; x<=x2; x++)
		{
			DrawPixel(x,y,r,g,b);
		}
    else
		for (x = x2; x<=x1; x++)
		{
			DrawPixel(x,y,r,g,b);
		}

}
// **********************************************************************
//
//
//
// **********************************************************************
void ImageClass::DrawLineV(int x, int y1, int y2,unsigned char r, unsigned char g, unsigned char b )
{
	int y;
	for (y = y1; y<=y2; y++)
	{
		DrawPixel(x,y,r,g,b);
	}
}

// **********************************************************************
//
//
// **********************************************************************
void ImageClass::ReadPixel(GLint x, GLint y, unsigned char &r, unsigned char &g, unsigned char &b)
{
	unsigned long addr;

	addr = (unsigned long)( y *(sizeX)* TAM_PIXEL + x * TAM_PIXEL );
	r = data[addr++];
	g = data[addr++];
	b = data[addr];
}


// **********************************************************************
//
//
//
// **********************************************************************
double ImageClass::GetPointIntensity(int x, int y)
{
	unsigned char r,g,b;
	double i;

	ReadPixel(x,y,r,g,b);
	i = (0.3 * r + 0.59 * g + 0.11 * b);
	return i;
}

// **************************************************************
//
// **************************************************************
void ImageClass::DrawBox(int x1,int y1,int x2,int y2,unsigned char r, unsigned char g, unsigned char b )
{
    DrawLineH(y1, x1, x2, r,g,b);
    DrawLineH(y2, x1, x2, r,g,b);
    DrawLineV(x1, y1, y2, r,g,b);
    DrawLineV(x2, y1, y2, r,g,b);
}
// **********************************************************************
//
//
// **********************************************************************
void ImageClass::setPos(int X, int Y)
{
	this->posX = X;
	this->posY = Y;
}
// **********************************************************************
//
//
// **********************************************************************
void ImageClass::setSize(int X, int Y)
{
	this->sizeX = X;
	this->sizeY = Y;
}
// **********************************************************************
// Funcoes do Diogenes
// **********************************************************************
int ImageClass::size()
{
    return sizeX * sizeY;
}
#define COLOR_WHITE 255

void ImageClass::Resize(int auxX, int auxY)
{
    unsigned int tam;

    if(data)
        free(data);

    sizeX = auxX;
    sizeY = auxY;

    tam = sizeof(unsigned char) * sizeX * sizeY * TAM_PIXEL;
    data = (unsigned char *) malloc (tam);
    memset(data, COLOR_WHITE, tam);
}
