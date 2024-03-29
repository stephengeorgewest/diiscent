#ifndef _TGA_H
#define _TGA_H

#ifdef HAVE_CONFIG_H
#	include <conf.h>
#endif

typedef struct {
    char  identSize;         // size of ID field that follows 18 char header (0 usually)
    char  colorMapType;      // nType of colour map 0=none, 1=has palette
    char  imageType;         // nType of image 0=none,1=indexed,2=rgb,3=grey,+8=rle packed

    short colorMapStart;     // first colour map entry in palette
    short colorMapLength;    // number of colours in palette
    char  colorMapBits;      // number of bits per palette entry 15,16,24,32

    ushort xStart;            // image x origin
    ushort yStart;            // image y origin
    ushort width;             // image width in pixels
    ushort height;            // image height in pixels
    char   bits;              // image bits per pixel 8,16,24,32
    char   descriptor;        // image descriptor bits (vh flip bits)
} __pack__ tTgaHeader;

class CModelTextures {
	public:
		int						m_nBitmaps;
		CArray<CCharArray>	m_names;
		CArray<CBitmap>		m_bitmaps;
		CArray<ubyte>			m_nTeam;

	public:
		CModelTextures () { Init (); } 
		void Init (void) { m_nBitmaps = 0; }
		int Bind (int bCustom);
		void Release (void);
		int Read (int bCustom);
		int ReadBitmap (int i, int bCustom);
		bool Create (int nBitmaps);
		void Destroy (void);
};

int ShrinkTGA (CBitmap *bm, int xFactor, int yFactor, int bRealloc);
int ReadTGAHeader (CFile& cf, tTgaHeader *ph, CBitmap *pb);
int ReadTGAImage (CFile& cf, tTgaHeader *ph, CBitmap *pb, int alpha, 
						double brightness, int bGrayScale, int bRedBlueFlip);
int LoadTGA (CFile& cf, CBitmap *pb, int alpha, double brightness, 
				 int bGrayScale, int bRedBlueFlip);
int ReadTGA (const char* pszFile, const char* pszFolder, CBitmap* bmP, int alpha = -1, double brightness = 1.0, int bGrayScale = 0, int bRedBlueFlip = 0);
CBitmap *CreateAndReadTGA (const char *szFile);
int SaveTGA (const char *pszFile, const char *pszFolder, tTgaHeader *ph, CBitmap *bmP);
double TGABrightness (CBitmap *bmP);
void TGAChangeBrightness (CBitmap *bmP, double dScale, int bInverse, int nOffset, int bSkipAlpha);
int TGAInterpolate (CBitmap *bmP, int nScale);
int TGAMakeSquare (CBitmap *bmP);
int ReadModelTGA (const char *pszFile, CBitmap *bmP, int bCustom);
int CompressTGA (CBitmap *bmP);

#endif //_TGA_H
