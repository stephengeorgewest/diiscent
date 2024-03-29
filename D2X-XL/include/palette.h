/*
THE COMPUTER CODE CONTAINED HEREIN IS THE SOLE PROPERTY OF PARALLAX
SOFTWARE CORPORATION ("PARALLAX").  PARALLAX, IN DISTRIBUTING THE CODE TO
END-USERS, AND SUBJECT TO ALL OF THE TERMS AND CONDITIONS HEREIN, GRANTS A
ROYALTY-FREE, PERPETUAL LICENSE TO SUCH END-USERS FOR USE BY SUCH END-USERS
IN USING, DISPLAYING,  AND CREATING DERIVATIVE WORKS THEREOF, SO LONG AS
SUCH USE, DISPLAY OR CREATION IS FOR NON-COMMERCIAL, ROYALTY OR REVENUE
FREE PURPOSES.  IN NO EVENT SHALL THE END-USER USE THE COMPUTER CODE
CONTAINED HEREIN FOR REVENUE-BEARING PURPOSES.  THE END-USER UNDERSTANDS
AND AGREES TO THE TERMS HEREIN AND ACCEPTS THE SAME BY USE OF THIS FILE.
COPYRIGHT 1993-1998 PARALLAX SOFTWARE CORPORATION.  ALL RIGHTS RESERVED.
*/

#ifndef _PALETTE_H
#define _PALETTE_H

#define DEFAULT_LEVEL_PALETTE "groupa.256" //don't confuse with DEFAULT_PALETTE
#define D1_PALETTE "palette.256"

#include "color.h"
#include "cfile.h"
#include "cstack.h"

#define PALETTE_SIZE				256
#define MAX_COMPUTED_COLORS	64
#define MAX_FADE_LEVELS			34
#define FADE_LEVELS				31
#define FADE_RATE					16		//	gots to be a power of 2, else change the code in DiminishPaletteTowardsNormal

typedef struct tBGR {
	ubyte	b,g,r;
} __pack__ tBGR;

typedef struct tRGB {
	ubyte	r,g,b;
} __pack__ tRGB;

typedef union tPalette {
	ubyte			raw [PALETTE_SIZE * 3];
	tRgbColorb	rgb [PALETTE_SIZE];
} __pack__ tPalette;

typedef struct tComputedColor {
	ubyte			nIndex;
	tRgbColorb	color;
} __pack__ tComputedColor;

class CPalette {
	private:
		tPalette			m_data;
		tComputedColor	m_computedColors [MAX_COMPUTED_COLORS];
		short				m_nComputedColors;

	public:
		CPalette () {};
		~CPalette () {};
		void Init (void);
		void ClearStep ();
		bool Read (CFile& cf);
		bool Write (CFile& cf);
		int ClosestColor (int r, int g, int b);
		inline int ClosestColor (tRgbColorb* colorP)
		 { return ClosestColor ((int) colorP->red, (int) colorP->green, (int) colorP->blue); }
		inline int ClosestColor (tRgbColorf* colorP)
		 { return ClosestColor ((int) (colorP->red * 63.0f), (int) (colorP->green * 63.0f), (int) (colorP->blue * 63.0f)); }
		void SwapTransparency (void);
		void AddComputedColor (int r, int g, int b, int nIndex);
		void InitComputedColors (void);
		void ToRgbaf (ubyte nColor, tRgbaColorf& color);

		inline tPalette& Data (void) { return m_data; }
		inline tRgbColorb *Color (void) { return m_data.rgb; }
		inline ubyte *Raw (void) { return m_data.raw; }
		inline void Skip (CFile& cf) { cf.Seek (sizeof (m_data), SEEK_CUR); }
		inline size_t Size (void) { return sizeof (m_data); }
		inline void SetBlack (ubyte r, ubyte g, ubyte b) 
		 { m_data.rgb [0].red = r, m_data.rgb [0].green = g, m_data.rgb [0].blue = b; }
		inline void SetTransparency (ubyte r, ubyte g, ubyte b) 
		 { m_data.rgb [PALETTE_SIZE - 1].red = r, m_data.rgb [PALETTE_SIZE - 1].green = g, m_data.rgb [PALETTE_SIZE - 1].blue = b; }
		inline CPalette& operator= (CPalette& source) { 
			memcpy (&Data (), &source.Data (), sizeof (tPalette));
			Init ();
			return *this;
			}
		inline bool operator== (CPalette& source) { return !memcmp (&Data (), &source.Data (), sizeof (tPalette)); }
		inline tRgbColorb& operator[] (const int i) { return m_data.rgb [i]; }
	};

//------------------------------------------------------------------------------

typedef struct tPaletteList {
	struct tPaletteList	*next;
	CPalette					palette;
} tPaletteList;

//------------------------------------------------------------------------------

class CPaletteData {
	public:
		CPalette*		deflt;
		CPalette*		fade;
		CPalette*		game;
		CPalette*		last;
		CPalette*		D1;
		CPalette*		texture;
		CPalette*		current;
		CPalette*		prev;
		tPaletteList*	list;
		char				szLastLoaded [FILENAME_LEN];
		char				szLastPig [FILENAME_LEN];
		int				nPalettes;
		int				nGamma;
		int				nLastGamma;
		fix				xFlashDuration;
		fix				xLastDuration;
		fix				xLastEffectTime;

		ubyte				fadeTable [PALETTE_SIZE * MAX_FADE_LEVELS];
		tRgbColorf		flash;
		tRgbColorf		effect;
		tRgbColorf		lastEffect;
		bool				bDoEffect;
		int				nSuspended;
	};


class CPalette;

class CPaletteManager {
	private:
		CPaletteData	m_data;

		CStack< CPalette* > m_save;

	public:
		CPaletteManager () { Init (); };
		~CPaletteManager () { Destroy (); }
		void Init (void);
		void Destroy (void); 
		CPalette *Find (CPalette& palette);
		CPalette *Add (CPalette& palette);
		CPalette *Add (ubyte* buffer);
		CPalette *Load (const char *pszPaletteName, const char *pszLevelName, int nUsedForLevel, int bNoScreenChange, int bForce);
		CPalette *Load (const char* filename, const char* levelname);
		CPalette* LoadD1 (void);
		int FindClosestColor15bpp (int rgb);
		void SetGamma (int gamma);
		int GetGamma (void);
		void Flash (void);

		void ClearStep (void);
		void StepUp (int r, int g, int b);
		void StepUp (void);
		void RenderEffect (void);
		void FadeEffect (void);
		void ResetEffect (void);
		void SaveEffect (void);
		void SuspendEffect (bool bCond = true);
		void ResumeEffect (bool bCond = true);
		void SetEffect (int red, int green, int blue, bool bForce = false);
		void SetEffect (float red, float green, float blue, bool bForce = false);
		void BumpEffect (int red, int green, int blue);
		void BumpEffect (float red, float green, float blue);
		inline void SetRedEffect (int color) { m_data.effect.red = float (color) / 64.0f; }
		inline void SetGreenEffect (int color) { m_data.effect.green = float (color) / 64.0f; }
		inline void SetBlueEffect (int color) { m_data.effect.blue = float (color) / 64.0f; }
		void SetEffect (bool bForce = false);
		void ClearEffect (void);
		int ClearEffect (CPalette* palette);
		int EnableEffect (bool bReset = false);
		int DisableEffect (void);
		bool EffectEnabled (void) { return m_data.nSuspended <= 0; }
		bool EffectDisabled (void) { return m_data.nSuspended > 0; }
		inline bool FadedOut (void) { return m_data.nSuspended <= 0; }
		void SetPrev (CPalette *palette) { m_data.prev = palette; }

		 void Push (void) { 
			m_save.Push (m_data.current); 
			}
		 void Pop (void) { 
			m_data.current = m_save.Pop (); 
			}

		inline CPalette* Activate (CPalette* palette) {
			if (palette && (m_data.current != palette)) {
				m_data.current = palette;
				m_data.current->Init ();
				}
			return m_data.current;
			}

		inline sbyte RedEffect (void) { return sbyte (m_data.effect.red * 64.0f); }
		inline sbyte GreenEffect (void) { return sbyte (m_data.effect.green * 64.0f); }
		inline sbyte BlueEffect (void) { return sbyte (m_data.effect.blue * 64.0f); }
		inline void SetRedEffect (sbyte color) {  m_data.effect.red = float (color) / 64.0f; }
		inline void SetGreenEffect (sbyte color) {  m_data.effect.green = float (color) / 64.0f; }
		inline void SetBlueEffect (sbyte color) {  m_data.effect.blue = float (color) / 64.0f; }
		inline void SetFlashDuration (fix xDuration) { m_data.xFlashDuration = xDuration; }
		inline void SetLastEffectTime (fix xTime) { m_data.xLastEffectTime = xTime; }
		inline fix FlashDuration (void) { return m_data.xFlashDuration; }
		inline fix LastEffectTime (void) { return m_data.xLastEffectTime; }

		inline void SetDefault (CPalette* palette) { m_data.deflt = palette; }
		inline void SetCurrent (CPalette* palette) { m_data.current = palette; }
		inline void SetGame (CPalette* palette) { m_data.game = palette; }
		inline void SetFade (CPalette* palette) { m_data.fade = palette; }
		inline void SetTexture (CPalette* palette) { m_data.texture = palette; }
		inline void SetD1 (CPalette* palette) { m_data.D1 = palette; }

		inline CPalette* Default (void) { return m_data.deflt; }
		inline CPalette* Current (void) { return m_data.current ? m_data.current : m_data.deflt; }
		inline CPalette* Game (void) { return m_data.game ? m_data.game : Current (); }
		inline CPalette* Texture (void) { return m_data.texture ? m_data.texture : Current (); };
		inline CPalette* D1 (void) { return m_data.D1 ? m_data.D1 : Current (); }
		inline CPalette* Fade (void) { return m_data.fade; }
		inline ubyte* FadeTable (void) { return m_data.fadeTable; }
		inline bool DoEffect (void) { return m_data.bDoEffect; }

		inline char* LastLoaded (void) { return m_data.szLastLoaded; }
		inline char* LastPig (void) { return m_data.szLastPig; }
		inline void SetLastLoaded (const char *name) { strncpy (m_data.szLastLoaded, name, sizeof (m_data.szLastLoaded)); }
		inline void SetLastPig (const char *name) { strncpy (m_data.szLastPig, name, sizeof (m_data.szLastPig)); }

		inline int ClosestColor (int r, int g, int b)
		 { return m_data.current ? m_data.current->ClosestColor (r, g, b) : 0; }
	};

extern CPaletteManager paletteManager;
extern char szCurrentLevelPalette [SHORT_FILENAME_LEN];

#endif //_PALETTE_H
