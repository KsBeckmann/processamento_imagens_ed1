// **********************************************************************
//  BmpLib1.cpp
//  Produced by ...
//  Incremented by Diogenes Furlan
//
//  Le bitmaps 24 bits e 8 bits
//  Grava bitmaps 24 bits
// **********************************************************************

#ifndef BmpLib2_DEF
#define BmpLib2_DEF

class BMP {
public:
    static bool load(const char* bmp, unsigned int &sizeX, unsigned int &sizeY);
    static void copy_to_ImageClass(unsigned char* data);
    static bool save(const char* bmp, unsigned char* data, unsigned int sizeX, unsigned int sizeY);
    static void free_memory();

private:
    static bool copy_from_ImageClass(unsigned char* data);
};


#endif
