// **********************************************************************
//  BmpLib1.cpp
//  Produced by ...
//  Incremented by Diogenes Furlan
//
//  Le bitmaps 24 bits e 8 bits
//  Grava bitmaps 24 bits
// **********************************************************************

#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include "bmp_lib2.h"

#define BMP_TYPE 19778      //0x4D42
#define BMP_OFFSET 54       //0x36
#define BMP_HEADERSIZE 40   //0x28

// estruturas

typedef struct
{
    unsigned short int fileid;
    uint32_t filesize;
    unsigned short int reserved1;
    unsigned short int reserved2;
    uint32_t imgoffset;
} bmpinfoheader ;

typedef struct
{
    uint32_t headersize;
    int32_t imgwidth;
    int32_t imgheight;
    unsigned short int numplanes;
    unsigned short int pixeldepth;
    uint32_t compression;
    uint32_t bitmapsize;
    int32_t hresolution;
    int32_t vresolution;
    uint32_t usedcolors;
    uint32_t significantcolors;
} imginfoheader;

typedef struct
{
    unsigned char red;
    unsigned char green;
    unsigned char blue;
    unsigned char unused;
} bmppalete ;

// Os dois structs acima sao lidos e gravados byte a byte a partir do arquivo,
// entao seu tamanho precisa bater exatamente com o do cabecalho BMP. Usar
// larguras fixas (uint32_t/int32_t) garante isso em qualquer plataforma - com
// 'long' o segundo struct passaria de 40 para 80 bytes no Linux 64 bits. A
// verificacao abaixo falha na compilacao caso algum compilador insira
// preenchimento entre os campos.
static_assert(sizeof(bmpinfoheader) == 16, "cabecalho BMP com tamanho inesperado");
static_assert(sizeof(imginfoheader) == 40, "cabecalho DIB com tamanho inesperado");

// variaveis

static unsigned char *red, *green, *blue;
static int width, height;

static imginfoheader imgheader;
static bmpinfoheader bmpheader;
static bmppalete palete[256];
// **********************************************************************
//
//
// **********************************************************************
bool BMP::load(const char* bmp, unsigned int &sizeX, unsigned int &sizeY)
{
    FILE *fptr;
    long x,y;
    unsigned int padbytes=0;

    if((fptr=fopen(bmp,"rb"))==NULL)
    {
        printf("\nFile not found.");
        return false;
    }
    else
    {
        fread(&bmpheader.fileid,2,1,fptr);   // 14 bytes
        fread(&bmpheader.filesize,4,1,fptr);
        fread(&bmpheader.reserved1,2,1,fptr);
        fread(&bmpheader.reserved2,2,1,fptr);
        fread(&bmpheader.imgoffset,4,1,fptr);
        fread(&imgheader,sizeof(imgheader),1,fptr);  // 40 bytes

        if (    (bmpheader.fileid != BMP_TYPE) ||
                //(bmpheader.imgoffset != 54) ||   //1078  0x436
                (imgheader.headersize != 40) ||
                (imgheader.numplanes != 1) ||
                //(imgheader.pixeldepth != 24) ||  //8
                (imgheader.compression != 0)
           )
        {
            printf("\nError: Invalid bitmap image.");
            return false;
        };


        red = (unsigned char *) malloc(imgheader.imgheight * imgheader.imgwidth);
        green = (unsigned char *) malloc(imgheader.imgheight * imgheader.imgwidth);
        blue = (unsigned char *) malloc(imgheader.imgheight * imgheader.imgwidth);

        if (red == NULL || green == NULL || blue == NULL)
        {
            printf("\nOut of memory.");
            return false;
        }

        if( imgheader.pixeldepth == 24 )
        {
            printf("24 bits. ");

            padbytes = (imgheader.imgwidth*3)%4;
            if(padbytes!=0)
                padbytes = 4-padbytes;

            for(x=imgheader.imgheight-1; x>-1; x--)
            {
                for(y=0; y<imgheader.imgwidth; y++)
                {
                    blue[(x*imgheader.imgwidth+y)] = getc(fptr);
                    green[(x*imgheader.imgwidth+y)] = getc(fptr);
                    red[(x*imgheader.imgwidth+y)] = getc(fptr);
                }
                for(y=0; y<(long)padbytes; y++)
                    getc(fptr);
            }
        }
        else if( imgheader.pixeldepth == 8 )
        {
            printf("8 bits. ");

            padbytes = (imgheader.imgwidth)%4;
            if(padbytes!=0)
                padbytes = 4-padbytes;

            // le palete
            int num_posicoes = (bmpheader.imgoffset - 54) / 4;
            for(int i=0; i<num_posicoes; i++)
            {
                fread(&palete[i].blue,1,1,fptr);
                fread(&palete[i].green,1,1,fptr);
                fread(&palete[i].red,1,1,fptr);
                fread(&palete[i].unused,1,1,fptr);
            }
            // atualiza dados
            unsigned char dado;
            for(x=imgheader.imgheight-1; x>-1; x--)
            {
                for(y=0; y<imgheader.imgwidth; y++)
                {
                    dado = getc(fptr);
                    blue[(x*imgheader.imgwidth+y)] = palete[dado].blue;
                    green[(x*imgheader.imgwidth+y)] = palete[dado].green;
                    red[(x*imgheader.imgwidth+y)] = palete[dado].red;
                }
                for(y=0; y<(long)padbytes; y++)
                    getc(fptr);
            }
        }
        else
        {
            printf("\nError: Size of bitmap image non tratable: %d bits.", imgheader.pixeldepth);
            fclose(fptr);
            return false;
        }
        sizeX = width = imgheader.imgwidth;
        sizeY = height = imgheader.imgheight;
        fclose(fptr);
    }
    return true;
}
// **********************************************************************
//
//
// **********************************************************************
void BMP::copy_to_ImageClass(unsigned char* data)
{
    long x,y, i;
    for(x=0; x<width; x++)
    {
        for(y=0; y<height; y++)
        {
            unsigned long addr;
            i = (y*width+ x);
            addr = (unsigned long)( (height-y-1) *(width)* 3 + x * 3 );
            data[addr++] = red[i];
            data[addr++] = green[i];
            data[addr] = blue[i];
        }
    }
}
// **********************************************************************
//
//
// **********************************************************************
void BMP::free_memory()
{
    free(red);
    free(green);
    free(blue);
    red = green = blue = NULL;
}
// **********************************************************************
//
//
// **********************************************************************
bool BMP::copy_from_ImageClass(unsigned char* data)
{
    long x,y, i;
    long tam = height*width;

    red = (unsigned char *) malloc(tam);
    green = (unsigned char *) malloc(tam);
    blue = (unsigned char *) malloc(tam);
    if (red == NULL || green == NULL || blue == NULL)
    {
        printf("\nOut of memory - CopyImageClassToBmp.");
        return false;
    }

    for(x=0; x<width; x++)
    {
        for(y=0; y<height; y++)
        {
            unsigned long addr;
            i = (y*width+ x);
            addr = (unsigned long)( (height-y-1) *(width)* 3 + x * 3 );
            red[i] = data[addr++];
            green[i] = data[addr++];
            blue[i] = data[addr];
        }
    }
    return true;
}
// **********************************************************************
//
//
// **********************************************************************
bool BMP::save(const char* bmp, unsigned char* data, unsigned int sizeX, unsigned int sizeY)
{
    FILE *fptr;
    long x,y;
    unsigned int padbytes=0;

    // copia os dados da ImageClass para o BMP
    width = sizeX;
    height = sizeY;
    if (!BMP::copy_from_ImageClass(data))
        return false; // erro
// ----

    // Cada linha ocupa width*3 bytes arredondados para cima ate' o proximo
    // multiplo de 4 - o BMP exige esse alinhamento. O tamanho declarado no
    // cabecalho precisa bater com o tamanho real do arquivo, ou visualizadores
    // mais rigorosos recusam a imagem.
    unsigned int bytesLinha = (width * 3 + 3) / 4 * 4;

    bmpheader.fileid = BMP_TYPE;
    bmpheader.filesize = 54 + bytesLinha * height;
    bmpheader.reserved1 = 0;
    bmpheader.reserved2 = 0;
    bmpheader.imgoffset = 54;
    imgheader.headersize = 40;
    imgheader.imgwidth = width;
    imgheader.imgheight = height;
    imgheader.numplanes = 1;
    imgheader.pixeldepth = 24;
    imgheader.compression = 0;
    imgheader.bitmapsize = bytesLinha * height;
    imgheader.hresolution = 0;
    imgheader.vresolution = 0;
    imgheader.usedcolors = 0;
    imgheader.significantcolors = 0;

    if((fptr=fopen(bmp,"wb"))==NULL)
    {
        printf("\nCould not write to file.");
        return false;
    }
    else
    {
        fwrite(&bmpheader.fileid,2,1,fptr);
        fwrite(&bmpheader.filesize,4,1,fptr);
        fwrite(&bmpheader.reserved1,2,1,fptr);
        fwrite(&bmpheader.reserved2,2,1,fptr);
        fwrite(&bmpheader.imgoffset,4,1,fptr);
        fwrite(&imgheader,sizeof(imgheader),1,fptr);


        padbytes = (imgheader.imgwidth*3)%4;
        if(padbytes!=0)
            padbytes = 4-padbytes;
        for(x=imgheader.imgheight-1; x>-1; x--)
        {
            for(y=0; y<imgheader.imgwidth; y++)
            {
                putc(blue[(x*imgheader.imgwidth+y)],fptr);
                putc(green[(x*imgheader.imgwidth+y)],fptr);
                putc(red[(x*imgheader.imgwidth+y)],fptr);
            }
            for(y=0; y< (long)padbytes; y++)
                putc(0,fptr);
        }
        fclose(fptr);
    }

    BMP::free_memory();
    return true;
}
