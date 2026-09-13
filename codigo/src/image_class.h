// *****************************************************************************************
//  ImageClassNEW2.h
// *****************************************************************************************
// Ver comentario em main.cpp: no Windows, <windows.h> precisa vir antes de
// <GL/gl.h>.
#ifdef _WIN32
#include <windows.h>
#endif

#include <GL/gl.h>
#include <string>
using namespace std;

#define uchar unsigned char

class ImageClass
{

protected:
	int posX, posY;
	float zoomH, zoomV;
	unsigned char *data;
	unsigned int sizeX, sizeY;

public:

	ImageClass(void);
	ImageClass(int sizeX, int sizeY);

	// arquivo
	bool Load(string);
	void Save(char *);
	void Delete(void);

	// desenho
	void Display(void);
	void DrawPixel(int x, int y, uchar r, uchar g, uchar b);
	void DrawLineH(int y, int x1, int x2, uchar r, uchar g, uchar b);
	void DrawLineV(int x, int y1, int y2, uchar r, uchar g, uchar b );
	void ReadPixel(GLint x, GLint y, uchar &r, uchar &g, uchar &b);

	double GetPointIntensity(int x, int y);
	int getSizeX() { return sizeX; };
	int getSizeY() { return sizeY; };
	void SetZoomH(float H) { zoomH = H; };
	void SetZoomV(float V) { zoomV = V; };

	void Realloc(int sizeX, int sizeY);
	void setPos(int X, int Y);
	void setSize(int X, int Y);

	void DrawBox(int x1,int y1,int x2,int y2,unsigned char r, unsigned char g, unsigned char b);

    // Diogenes
    int size();
	void Resize(int,int);

};
