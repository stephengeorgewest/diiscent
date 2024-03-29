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
COPYRIGHT 1993-1999 PARALLAX SOFTWARE CORPORATION.  ALL RIGHTS RESERVED.
*/


#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#ifdef _WIN32
#	include <windows.h>
#endif

#include <stdio.h>
#include <string.h>

#include "pstypes.h"
#include "strutil.h"
#include "descent.h"
#include "gr.h"
#include "u_mem.h"
#include "iff.h"
#include "mono.h"
#include "error.h"
#include "sounds.h"
#include "songs.h"
#include "loadgamedata.h"
#include "bmread.h"
#include "hash.h"
#include "args.h"
#include "palette.h"
#include "gamefont.h"
#include "rle.h"
#include "screens.h"
#include "gamemine.h"
#include "textures.h"
#include "texmerge.h"
#include "paging.h"
#include "game.h"
#include "text.h"
#include "cfile.h"
#include "menu.h"
#include "byteswap.h"
#include "findfile.h"
#include "makesig.h"
#include "effects.h"
#include "wall.h"
#include "weapon.h"
#include "error.h"
#include "grdef.h"
#include "gamepal.h"
#include "piggy.h"

//#define NO_DUMP_SOUNDS        1   //if set, dump bitmaps but not sounds

#if DBG
#	define PIGGY_MEM_QUOTA	8 //4
#else
#	define PIGGY_MEM_QUOTA	8
#endif

short *d1_tmap_nums = NULL;

CBitmap bogusBitmap;
CSoundSample bogusSound;

#define RLE_REMAP_MAX_INCREASE 132 /* is enough for d1 pc registered */

static int bMustWriteHamFile = 0;
static int nBitmapFilesNew = 0;

static CHashTable bitmapNames [2];
size_t bitmapCacheUsed = 0;
size_t bitmapCacheSize = 0;
int bitmapCacheNext [2] = {0, 0};
int bitmapOffsets [2][MAX_BITMAP_FILES];
#ifndef _WIN32
static ubyte *bitmapBits [2] = {NULL, NULL};
#endif
ubyte d1ColorMap [256];

#if DBG
#	define PIGGY_BUFFER_SIZE ((uint) (512*1024*1024))
#else
#	define PIGGY_BUFFER_SIZE ((uint) 0x7fffffff)
#endif
#define PIGGY_SMALL_BUFFER_SIZE (16*1024*1024)

#define DBM_FLAG_ABM    64 // animated bitmap
#define DBM_NUM_FRAMES  63

#define BM_FLAGS_TO_COPY (BM_FLAG_TRANSPARENT | BM_FLAG_SUPER_TRANSPARENT | BM_FLAG_NO_LIGHTING | BM_FLAG_RLE | BM_FLAG_RLE_BIG)


#define PIGBITMAPHEADER_D1_SIZE 17 // no wh_extra

int RequestCD (void);

CFile cfPiggy [2];

char szCurrentPigFile [2][SHORT_FILENAME_LEN] = {"",""};

int bPigFileInitialized = 0;

ubyte bBigPig = 0;

extern char szLastPalettePig [];
extern char CDROM_dir [];

//------------------------------------------------------------------------------

static char szPigFiles [2][11] = {"groupa.pig", "d2demo.pig"};

char* DefaultPigFile (int bDemoData)
{
return szPigFiles [(bDemoData < 0) ? gameStates.app.bDemoData : bDemoData];
}

//------------------------------------------------------------------------------

static char szHamFiles [2][13] = {"descent2.ham", "d2demo.ham"};

char* DefaultHamFile (void)
{
return szHamFiles [gameStates.app.bDemoData];
}

//------------------------------------------------------------------------------

static char szSndFiles [3][13] = {"descent2.s11", "descent2.s22", "d2demo.ham"};

char* DefaultSoundFile (void)
{
if (gameData.pig.tex.nHamFileVersion < 3) {
	gameOpts->sound.soundSampleRate = SAMPLE_RATE_11K;
	return szSndFiles [2];
	}
if (gameOpts->sound.audioSampleRate == SAMPLE_RATE_11K) {
	gameOpts->sound.soundSampleRate = SAMPLE_RATE_11K;
	return szSndFiles [0];
	}
gameOpts->sound.soundSampleRate = SAMPLE_RATE_22K;
return szSndFiles [1];
}

//------------------------------------------------------------------------------
/*
 * reads a tPIGBitmapHeader structure from a CFile
 */
void PIGBitmapHeaderRead (tPIGBitmapHeader *dbh, CFile& cf)
{
cf.Read (dbh->name, 8, 1);
dbh->dflags = cf.ReadByte ();
dbh->width = cf.ReadByte ();
dbh->height = cf.ReadByte ();
dbh->wh_extra = cf.ReadByte ();
dbh->flags = cf.ReadByte ();
dbh->avgColor = cf.ReadByte ();
dbh->offset = cf.ReadInt ();
}

//------------------------------------------------------------------------------
/*
 * reads a descent 1 tPIGBitmapHeader structure from a CFile
 */
void PIGBitmapHeaderD1Read (tPIGBitmapHeader *dbh, CFile& cf)
{
cf.Read (dbh->name, 8, 1);
dbh->dflags = cf.ReadByte ();
dbh->width = cf.ReadByte ();
dbh->height = cf.ReadByte ();
dbh->wh_extra = 0;
dbh->flags = cf.ReadByte ();
dbh->avgColor = cf.ReadByte ();
dbh->offset = cf.ReadInt ();
}

//------------------------------------------------------------------------------

tBitmapIndex PiggyRegisterBitmap (CBitmap *bmP, const char *name, int bInFile)
{
	tBitmapIndex bmi;
	Assert (gameData.pig.tex.nBitmaps [gameStates.app.bD1Data] < MAX_BITMAP_FILES);

#if DBG
if (strstr (name, "door13"))
	name = name;
#endif
bmi.index = gameData.pig.tex.nBitmaps [gameStates.app.bD1Data];
if (!bInFile) {
	nBitmapFilesNew++;
	}
int i = gameData.pig.tex.nBitmaps [gameStates.app.bD1Data];
strncpy (gameData.pig.tex.bitmapFileP [i].name, name, 12);
bitmapNames [gameStates.app.bD1Data].Insert (gameData.pig.tex.bitmapFileP [i].name, i);
bmP->Clone (gameData.pig.tex.bitmapP [i]);
bmP->SetBuffer (NULL);	//avoid automatic destruction trying to delete the same buffer twice
if (!bInFile) {
	bitmapOffsets [gameStates.app.bD1Data][i] = 0;
	gameData.pig.tex.bitmapFlags [gameStates.app.bD1Data][i] = bmP->Flags ();
	}
gameData.pig.tex.nBitmaps [gameStates.app.bD1Data]++;
return bmi;
}

//------------------------------------------------------------------------------

tBitmapIndex PiggyFindBitmap (const char * pszName, int bD1Data)
{
	tBitmapIndex	bi;
	int				i;
	const char		*t;
	char				name [FILENAME_LEN], alias [FILENAME_LEN];

bi.index = 0;
strcpy (name, pszName);
if ((t = strchr (pszName, '#')))
	name [t - pszName] = '\0';
for (i = 0; i < gameData.pig.tex.nAliases; i++)
	if (!stricmp (name, gameData.pig.tex.aliases [i].aliasname)) {
		if (t) {	//this is a frame of an animated texture, so add the frame number
			_splitpath (gameData.pig.tex.aliases [i].filename, NULL, NULL, alias, NULL);
			strcat (alias, "#");
			strcat (alias, t + 1);
			pszName = alias;
			}
		else
			pszName = gameData.pig.tex.aliases [i].filename;
		break;
		}
i = bitmapNames [bD1Data].Search (pszName);
Assert (i != 0);
bi.index = ushort (i);
return bi;
}

//------------------------------------------------------------------------------

void PiggyCloseFile (void)
{
	int	i;

for (i = 0; i < 2; i++)
	if (cfPiggy [i].File ()) {
		cfPiggy [i].Close ();
		szCurrentPigFile [i][0] = 0;
		}
}

//------------------------------------------------------------------------------
//copies a pigfile from the CD to the current dir
//retuns file handle of new pig
int CopyPigFileFromCD (CFile& cf, char *filename)
{
return cf.Open (filename, gameFolders.szDataDir, "rb", 0);
#if 0
	char name [80];
	FFS ffs;
	int ret;

messageBox.Show ("Copying bitmap data from CD...");
//paletteManager.ResumeEffect ();    //I don't think this line is really needed
//First, delete all PIG files currently in the directory
if (!FFF ("*.pig", &ffs, 0)) {
	do {
		cf.Delete (ffs.name, "");
	} while (!FFN  (&ffs, 0));
	FFC (&ffs);
}
//Now, copy over new pig
redbook.Stop ();           //so we can read off the cd
//new code to unarj file
strcpy (name, CDROM_dir);
strcat (name, "descent2.sow");
do {
//	ret = unarj_specific_file (name,filename,filename);
// DPH:FIXME
	ret = !EXIT_SUCCESS;
	if (ret != EXIT_SUCCESS) {
		//delete file, so we don't leave partial file
		cf.Delete (filename, "");
		if (RequestCD () == -1)
			//NOTE LINK TO ABOVE IF
			Error ("Cannot load file <%s> from CD", filename);
		}
	} while (ret != EXIT_SUCCESS);
mb.Clear ();
return cfPiggy [gameStates.app.bD1Data].Open (filename, gameFolders.szDataDir, "rb", 0);
#endif
}

//------------------------------------------------------------------------------

int PiggyInitMemory (void)
{
	static	int bMemInited = 0;

if (bMemInited)
	return 1;
if (gameStates.app.bUseSwapFile) {
	bitmapCacheSize = 0xFFFFFFFF;
	gameStates.render.nMaxTextureQuality = 3;
	return bMemInited = 1;
	}
#if defined (_WIN32)
 {
	MEMORYSTATUS	memStat;
	GlobalMemoryStatus (&memStat);
	bitmapCacheSize = (int) (memStat.dwAvailPhys / 10) * PIGGY_MEM_QUOTA;
#	if DBG
	gameStates.render.nMaxTextureQuality = 3;
#	else
	if (bitmapCacheSize > 1024 * 1024 * 1024)
		gameStates.render.nMaxTextureQuality = 3;
	else if (bitmapCacheSize > 256 * 1024 * 1024)
		gameStates.render.nMaxTextureQuality = 2;
	else
		gameStates.render.nMaxTextureQuality = 1;
#	endif
	PrintLog ("maximum permissible texture quality: %d\n", gameStates.render.nMaxTextureQuality);
	}
#else
gameStates.render.nMaxTextureQuality = 3;
bitmapCacheSize = PIGGY_BUFFER_SIZE;
// the following loop will scale bitmap memory down by 20% for each iteration
// until memory could be allocated, and then once more to leave enough memory
// for other parts of the program
for (;;) {
	if ((bitmapBits [0] = new ubyte (bitmapCacheSize))) {
		delete[] bitmapBits [0];
		break;
		}
	bitmapCacheSize = (bitmapCacheSize / 10) * PIGGY_MEM_QUOTA;
	if (bitmapCacheSize < PIGGY_SMALL_BUFFER_SIZE) {
		Error ("Not enough memory to load D2 bitmaps\n");
		return 0;
		break;
		}
	}
#endif
return bMemInited = 1;
}

//------------------------------------------------------------------------------
//initialize a pigfile, reading headers
//returns the size of all the bitmap data
void PiggyInitPigFile (char *filename)
{
	CFile					*cfP = cfPiggy;
	char					szName [16];
	char					szNameRead [16];
	int					nHeaderSize, nBitmapNum, nDataSize, nDataStart, i;
	//int					bD1 = gameStates.app.bD1Data;
	bool					bRegister = filename != NULL;
	bool					bReload = true;
	CBitmap				bm;
	tPIGBitmapHeader	bmh;

	static char szPigName [FILENAME_LEN] = {'\0'};

if (filename) {
	if (stricmp (szPigName, filename))
		strcpy (szPigName, filename);
	else if (cfP->File ())
		return;
	else
		bReload = false;
	}
else if (!*szPigName || cfP->File ())
	return;
if (bRegister)
	PiggyCloseFile ();             //close old pig if still open
//rename pigfile for shareware
strlwr (szPigName);
if (stricmp (szPigName, DefaultPigFile (1)) && !CFile::Exist (szPigName, gameFolders.szDataDir, 0))
	strcpy (szPigName, DefaultPigFile (1));
if (!cfP->Open (szPigName, gameFolders.szDataDir, "rb", 0)) {
	if (!CopyPigFileFromCD (*cfP, szPigName))
		return;
	}
if (!bReload)
	return;

int pig_id = cfP->ReadInt ();
int pigVersion = cfP->ReadInt ();
if (pig_id != PIGFILE_ID || pigVersion != PIGFILE_VERSION) {
	cfP->Close ();              //out of date pig
	return;
	}
strncpy (szCurrentPigFile [0], szPigName, sizeof (szCurrentPigFile [0]));
nBitmapNum = cfP->ReadInt ();
nHeaderSize = nBitmapNum * sizeof (tPIGBitmapHeader);
nDataStart = nHeaderSize + cfP->Tell ();
nDataSize = cfP->Length () - nDataStart;
gameData.pig.tex.nBitmaps [0] = 1;
SetDataVersion (0);
for (i = 0; i < nBitmapNum; i++) {
	PIGBitmapHeaderRead (&bmh, *cfP);
	memcpy (szNameRead, bmh.name, 8);
	szNameRead [8] = 0;
	if (bmh.dflags & DBM_FLAG_ABM)
		sprintf (szName, "%s#%d", szNameRead, bmh.dflags & DBM_NUM_FRAMES);
	else
		strcpy (szName, szNameRead);
	memset (&bm, 0, sizeof (CBitmap));
	bm.SetWidth (bmh.width + ((short) (bmh.wh_extra & 0x0f) << 8));
	bm.SetHeight (bmh.height + ((short) (bmh.wh_extra & 0xf0) << 4));
	bm.SetBPP (1);
	bm.SetFlags (BM_FLAG_PAGED_OUT);
	bm.SetAvgColorIndex (bmh.avgColor);
	gameData.pig.tex.bitmapFlags [0][i+1] = bmh.flags & BM_FLAGS_TO_COPY;
	bitmapOffsets [0][i+1] = bmh.offset + nDataStart;
	if (bRegister) {
		Assert ((i+1) == gameData.pig.tex.nBitmaps [0]);
		PiggyRegisterBitmap (&bm, szName, 1);
		}
	}
bPigFileInitialized = 1;
}

//------------------------------------------------------------------------------

int ReadHamFile (bool bDefault)
{
	CFile		cf;
#if 1
	char		szD1PigFileName [FILENAME_LEN];
#endif
	int		nHAMId;
	int		nSoundOffset = 0;
	char		szFile [FILENAME_LEN];
	char*		pszFile, * pszFolder;

if (bDefault) {
	pszFile = DefaultHamFile ();
	pszFolder = gameFolders.szDataDir;
	}
else {
	if (!*gameFolders.szModName)
		return 0;
	sprintf (szFile, "%s.ham", gameFolders.szModName);
	pszFile = szFile;
	pszFolder = gameFolders.szModDir [1];
	}

if (!cf.Open (pszFile, pszFolder, "rb", 0)) {
	bMustWriteHamFile = 1;
	return 0;
	}
//make sure ham is valid nType file & is up-to-date
nHAMId = cf.ReadInt ();
gameData.pig.tex.nHamFileVersion = cf.ReadInt ();
if (nHAMId != HAMFILE_ID)
	Error ("Cannot open ham file %s\n", DefaultHamFile ());
if (gameData.pig.tex.nHamFileVersion < 3) // hamfile contains sound info
	nSoundOffset = cf.ReadInt ();
BMReadAll (cf, bDefault);
/*---*/PrintLog ("      Loading bitmap index translation table\n");
	gameData.pig.tex.bitmapXlat.Read (cf, MAX_BITMAP_FILES);
if (gameData.pig.tex.nHamFileVersion < 3) {
	cf.Seek (nSoundOffset, SEEK_SET);
	int nSoundNum = cf.ReadInt ();
	int nSoundStart = cf.Tell ();
/*---*/PrintLog ("      Loading %d sounds\n", nSoundNum);
	SetupSounds (cf, nSoundNum, nSoundStart);
	}
cf.Close ();
/*---*/PrintLog ("      Looking for Descent 1 data files\n");
strcpy (szD1PigFileName, "descent.pig");
if (cfPiggy [1].File ())
	cfPiggy [1].Seek (0, SEEK_SET);
else
	cfPiggy [1].Open (szD1PigFileName, gameFolders.szDataDir, "rb", 0);
if (cfPiggy [1].File ()) {
	gameStates.app.bHaveD1Data = 1;
/*---*/PrintLog ("      Loading Descent 1 data\n");
	BMReadGameDataD1 (cfPiggy [1]);
	}
return 1;
}

//------------------------------------------------------------------------------

int PiggyInit (void)
{
	int bHamOk = 0, bSoundOk = 0;
	int i;

/*---*/PrintLog ("   Initializing hash tables\n");
bitmapNames [0].Create (MAX_BITMAP_FILES);
bitmapNames [1].Create (D1_MAX_BITMAP_FILES);
soundNames [0].Create (MAX_SOUND_FILES);
soundNames [1].Create (MAX_SOUND_FILES);

#if 0
/*---*/PrintLog ("   Initializing sound data (%d sounds)\n", MAX_SOUND_FILES);
for (i = 0; i < MAX_SOUND_FILES; i++)
	soundOffset [0][i] = 0;
#endif

/*---*/PrintLog ("   Initializing bitmap index (%d indices)\n", MAX_BITMAP_FILES);
for (i = 0; i < MAX_BITMAP_FILES; i++)
	gameData.pig.tex.bitmapXlat [i] = i;

if (!bogusBitmap.FrameSize ()) {
	int i;
	ubyte c;
/*---*/PrintLog ("   Initializing placeholder bitmap\n");
	bogusBitmap.Setup (0, 64, 64, 1, "Bogus Bitmap");
	bogusBitmap.SetBuffer (new ubyte [4096 * 4096]);
	bogusBitmap.SetPalette (paletteManager.Game ());
	c = paletteManager.Game ()->ClosestColor (0, 0, 63);
	memset (bogusBitmap.Buffer (), c, 4096);
	c = paletteManager.Game ()->ClosestColor (63, 0, 0);
	// Make a big red X !
	for (i = 0; i < 1024; i++) {
		bogusBitmap [i * 1024 + i] = c;
		bogusBitmap [i * 1024 + (1023 - i)] = c;
		}
	PiggyRegisterBitmap (&bogusBitmap, "bogus", 1);
	bogusSound.nLength [0] = 1024*1024;
	bogusSound.data [0].ShareBuffer (bogusBitmap);
	bitmapOffsets [0][0] =
	bitmapOffsets [1][0] = 0;
}

if (FindArg ("-bigpig"))
	bBigPig = 1;

/*---*/PrintLog ("   Loading game data\n");
PiggyInitPigFile (DefaultPigFile (-1));
/*---*/PrintLog ("   Loading main ham file\n");
bSoundOk = bHamOk = ReadHamFile ();
gameData.pig.sound.nType = -1; //none loaded
if (gameData.pig.tex.nHamFileVersion >= 3) {
/*---*/PrintLog ("   Loading sound file\n");
	bSoundOk = LoadD2Sounds ();
	}
//if (gameStates.app.bFixModels)
#if 1 //!DBG
gameStates.app.bAltModels =
gameStates.app.bFixModels = gameStates.app.bDemoData ? 0 : LoadRobotReplacements ("d2x-xl", NULL, 0, 1) > 0;
#endif
LoadTextureBrightness ("descent2", gameData.pig.tex.defaultBrightness [0].Buffer ());
LoadTextureBrightness ("descent", gameData.pig.tex.defaultBrightness [1].Buffer ());
LoadTextureColors ("descent2", gameData.render.color.defaultTextures [0].Buffer ());
LoadTextureColors ("descent", gameData.render.color.defaultTextures [1].Buffer ());
atexit (PiggyClose);
return (bHamOk && bSoundOk);               //read ok
}

//------------------------------------------------------------------------------

const char * szCriticalErrors [13] = {
	"Write Protected", "Unknown Unit", "Drive Not Ready", "Unknown Command", "CRC Error",
	"Bad struct length", "Seek Error", "Unknown media nType", "Sector not found", "Printer out of paper",
	"Write Fault",	"Read fault", "General Failure"
	};

void PiggyCriticalError (void)
{
CCanvas::Push ();
//paletteManager.ResumeEffect ();
int i = MsgBox ("Disk Error", NULL, 2, "Retry", "Exit", "%s\non drive %c:", 
					 szCriticalErrors [descent_critical_errcode & 0xf], (descent_critical_deverror & 0xf) + 'A');
CCanvas::Pop ();
if (i == 1)
	exit (1);
}

//------------------------------------------------------------------------------

int IsMacDataFile (CFile *cfP, int bD1)
{
if (cfP == cfPiggy + bD1)
	switch (cfP->Length ()) {
		default:
			if (!FindArg ("-macdata"))
				break;
		case MAC_ALIEN1_PIGSIZE:
		case MAC_ALIEN2_PIGSIZE:
		case MAC_FIRE_PIGSIZE:
		case MAC_GROUPA_PIGSIZE:
		case MAC_ICE_PIGSIZE:
		case MAC_WATER_PIGSIZE:
			return !bD1;
		case D1_MAC_PIGSIZE:
		case D1_MAC_SHARE_PIGSIZE:
			return bD1;
		}
return 0;
}

//------------------------------------------------------------------------------

void PiggyBitmapReadD1 (
	CFile					&cf,
	CBitmap				*bmP, /* read into this bmP */
	int					nBmDataOffs, /* specific to file */
   tPIGBitmapHeader	*bmh, /* header info for bmP */
   ubyte					**pNextBmP, /* where to write it (if 0, use reinterpret_cast<ubyte*> (D2_ALLOC) */
   ubyte					*colorMap) /* how to translate bmP's colors */
{
	int zSize, bSwap0255;

memset (bmP, 0, sizeof (CBitmap));
bmP->SetWidth (bmh->width + ((short) (bmh->wh_extra&0x0f)<<8));
bmP->SetHeight (bmh->height + ((short) (bmh->wh_extra&0xf0)<<4));
bmP->SetBPP (1);
bmP->SetAvgColorIndex (bmh->avgColor);
bmP->AddFlags (bmh->flags & BM_FLAGS_TO_COPY);

cf.Seek (nBmDataOffs + bmh->offset, SEEK_SET);
if (bmh->flags & BM_FLAG_RLE) {
	zSize = cf.ReadInt ();
	cf.Seek (-4, SEEK_CUR);
	}
else
	zSize = bmP->Width () * bmP->Width ();

if (pNextBmP) {
	bmP->SetBuffer (*pNextBmP);
	*pNextBmP += zSize;
	}
else {
	if (bmP->CreateBuffer ())
		UseBitmapCache (bmP, (int) bmP->FrameSize ());
	else
		return;
	}
bmP->Read (cf, zSize);
bSwap0255 = 0;
switch (cf.Length ()) {
	case D1_MAC_PIGSIZE:
	case D1_MAC_SHARE_PIGSIZE:
		if (bmh->flags & BM_FLAG_RLE)
			bSwap0255 = 1;
		else
			bmP->Swap_0_255 ();
		}
if (bmh->flags & BM_FLAG_RLE)
	bmP->RLEExpand (NULL, bSwap0255);
bmP->Remap (paletteManager.D1 (), TRANSPARENCY_COLOR, -1);
}

//------------------------------------------------------------------------------
#define D1_MAX_TEXTURES 800
#define D1_MAX_TMAP_NUM 1630 // 1621 in descent.pig Mac registered

/* the inverse of the gameData.pig.tex.bmIndex array, for descent 1.
 * "gameData.pig.tex.bmIndex" looks up a d2 bitmap index given a d2 nBaseTex
 * "d1_tmap_nums" looks up a d1 nBaseTex given a d1 bitmap. "-1" means "None"
 */
void _CDECL_ FreeD1TMapNums (void)
{
if (d1_tmap_nums) {
	PrintLog ("unloading D1 texture ids\n");
	delete[] d1_tmap_nums;
	d1_tmap_nums = NULL;
	}
}

//------------------------------------------------------------------------------

void BMReadD1TMapNums (CFile& cf)
{
	int i, d1_index;

	FreeD1TMapNums ();
	cf.Seek (8, SEEK_SET);
	d1_tmap_nums = new short [D1_MAX_TMAP_NUM];
	for (i = 0; i < D1_MAX_TMAP_NUM; i++)
		d1_tmap_nums [i] = -1;
	for (i = 0; i < D1_MAX_TEXTURES; i++) {
		d1_index = cf.ReadShort ();
		Assert (d1_index >= 0 && d1_index < D1_MAX_TMAP_NUM);
		d1_tmap_nums [d1_index] = i;
	}
	atexit (FreeD1TMapNums);
}

//------------------------------------------------------------------------------
/* If the given d1_index is the index of a bitmap we have to load
 * (because it is unique to descent 1), then returns the d2_index that
 * the given d1_index replaces.
 * Returns -1 if the given d1_index is not unique to descent 1.
 */
short D2IndexForD1Index (short d1_index)
{
	short d1_tmap_num = d1_tmap_nums ? d1_tmap_nums [d1_index] : -1;

	Assert (d1_index >= 0 && d1_index < D1_MAX_TMAP_NUM);
	if ((d1_tmap_num == -1) || !d1_tmap_num_unique (d1_tmap_num))
  		return -1;
	return gameData.pig.tex.bmIndex [0][ConvertD1Texture (d1_tmap_num, 0)].index;
}

//------------------------------------------------------------------------------

void LoadD1PigHeader (CFile& cf, int *pSoundNum, int *pBmHdrOffs, int *pBmDataOffs, int *pBitmapNum, int bReadTMapNums)
{

#	define D1_PIG_LOAD_FAILED "Failed loading " D1_PIGFILE

	int	nPigDataStart,
			nHeaderSize,
			nBmHdrOffs,
			nBmDataOffs,
			nSoundNum,
			nBitmapNum;

switch (cf.Length ()) {
	case D1_SHARE_BIG_PIGSIZE:
	case D1_SHARE_10_PIGSIZE:
	case D1_SHARE_PIGSIZE:
	case D1_10_BIG_PIGSIZE:
	case D1_10_PIGSIZE:
		nPigDataStart = 0;
		Warning ("%s %s. %s", TXT_LOAD_FAILED, D1_PIGFILE, TXT_D1_SUPPORT);
		return;
	default:
		Warning ("%s %s", TXT_UNKNOWN_SIZE, D1_PIGFILE);
		Int3 ();
		// fall through
	case D1_PIGSIZE:
	case D1_OEM_PIGSIZE:
	case D1_MAC_PIGSIZE:
	case D1_MAC_SHARE_PIGSIZE:
		nPigDataStart = cf.ReadInt ();
		if (bReadTMapNums)
			BMReadD1TMapNums (cf);
		break;
	}
cf.Seek (nPigDataStart, SEEK_SET);
nBitmapNum = cf.ReadInt ();
nSoundNum = int (cf.ReadShort ());
cf.Seek (sizeof (short), SEEK_CUR);	//skip another 2 bytes
nHeaderSize = nBitmapNum * PIGBITMAPHEADER_D1_SIZE + nSoundNum * sizeof (tPIGSoundHeader);
nBmHdrOffs = nPigDataStart + 2 * sizeof (int);
nBmDataOffs = nBmHdrOffs + nHeaderSize;
if (pSoundNum)
	*pSoundNum = nSoundNum;
*pBmHdrOffs = nBmHdrOffs;
*pBmDataOffs = nBmDataOffs;
*pBitmapNum = nBitmapNum;
}

//------------------------------------------------------------------------------

CBitmap bmTemp;

#define D1_BITMAPS_SIZE (128 * 1024 * 1024)

//------------------------------------------------------------------------------

bool LoadD1Sounds (bool bCustom)
{
	int	nBmHdrOffs, nBmDataOffs, nSounds, nBitmaps;

if (cfPiggy [1].File ())
	cfPiggy [1].Seek (0, SEEK_SET);
else if (!cfPiggy [1].Open (D1_PIGFILE, gameFolders.szDataDir, "rb", 0)) {
	Warning (D1_PIG_LOAD_FAILED);
	return false;
	}
LoadD1PigHeader (cfPiggy [1], &nSounds, &nBmHdrOffs, &nBmDataOffs, &nBitmaps, 1);
if (gameStates.app.bD1Mission && gameStates.app.bHaveD1Data) {
	gameStates.app.bD1Data = 1;
	SetDataVersion (1);
	SetD1Sound ();
	if ((gameData.pig.sound.nType != 1) || gameStates.app.bCustomSounds || bCustom) {
		SetupSounds (cfPiggy [1], nSounds, nBmHdrOffs + nBitmaps * PIGBITMAPHEADER_D1_SIZE, bCustom, false);
		LoadSounds (cfPiggy [1]);
		gameData.pig.sound.nType = 1;
		if (gameStates.sound.bD1Sound)
			gameOpts->sound.soundSampleRate = SAMPLE_RATE_11K;
		}
	}
return nSounds > 0;
}

//------------------------------------------------------------------------------

void LoadD1Textures (void)
{
	tPIGBitmapHeader	bmh;
	char					szNameRead [16];
	int					i, nBmHdrOffs, nBmDataOffs, nSounds, nBitmaps;

PiggyInitPigFile (const_cast<char*> ("groupa.pig"));
SetDataVersion (1);
if (cfPiggy [1].File ())
	cfPiggy [1].Seek (0, SEEK_SET);
else if (!cfPiggy [1].Open (D1_PIGFILE, gameFolders.szDataDir, "rb", 0)) {
	Warning (D1_PIG_LOAD_FAILED);
	return;
	}
//first, free up data allocated for old bitmaps
paletteManager.LoadD1 ();

LoadD1PigHeader (cfPiggy [1], &nSounds, &nBmHdrOffs, &nBmDataOffs, &nBitmaps, 1);
gameStates.app.bD1Data = 1;
if (gameStates.app.bD1Mission && gameStates.app.bHaveD1Data && !gameStates.app.bHaveD1Textures) {
	cfPiggy [1].Seek (nBmHdrOffs, SEEK_SET);
	gameData.pig.tex.nBitmaps [1] = 0;
	PiggyRegisterBitmap (&bogusBitmap, "bogus", 1);
	for (i = 0; i < nBitmaps; i++) {
		PIGBitmapHeaderD1Read (&bmh, cfPiggy [1]);
		memcpy (szNameRead, bmh.name, 8);
		szNameRead [8] = 0;
		memset (&bmTemp, 0, sizeof (CBitmap));
		if (bmh.dflags & DBM_FLAG_ABM) {
			char szName [20];
			sprintf (szName, "%s#%d", szNameRead, bmh.dflags & DBM_NUM_FRAMES);
			bmTemp.SetName (szName);
			}
		else
			bmTemp.SetName (szNameRead);
#if DBG
		if (strstr (bmTemp.Name (), "door13"))
			i = i;
#endif
		bmTemp.SetWidth (bmh.width + ((short) (bmh.wh_extra&0x0f)<<8));
		bmTemp.SetHeight (bmh.height + ((short) (bmh.wh_extra&0xf0)<<4));
		bmTemp.SetBPP (1);
		bmTemp.SetFlags (bmh.flags | BM_FLAG_PAGED_OUT);
		bmTemp.SetAvgColorIndex (bmh.avgColor);
		bmTemp.SetBuffer (NULL);
		bmTemp.SetBPP (1);
		bitmapCacheUsed += bmTemp.FrameSize ();
		gameData.pig.tex.bitmapFlags [1][i+1] = bmh.flags & BM_FLAGS_TO_COPY;
		bitmapOffsets [1][i+1] = bmh.offset + nBmDataOffs;
		Assert ((i+1) == gameData.pig.tex.nBitmaps [1]);
		PiggyRegisterBitmap (&bmTemp, bmTemp.Name (), 1);
		}
	gameStates.app.bHaveD1Textures = 1;
	}
paletteManager.SetLastPig ("");  //force pig re-load
TexMergeFlush ();       //for re-merging with new textures
}

//------------------------------------------------------------------------------

/*
 * Find and load the named bitmap from descent.pig
 * similar to LoadExitModelIFF
 */
tBitmapIndex ReadExtraBitmapD1Pig (const char *name)
{
	CFile					cf;
	tPIGBitmapHeader	bmh;
	int					i, nBmHdrOffs, nBmDataOffs, nBitmapNum;
	tBitmapIndex		bmi;
	CBitmap			*newBm = gameData.pig.tex.bitmaps [0] + gameData.pig.tex.nExtraBitmaps;

bmi.index = 0;
if (!cf.Open (D1_PIGFILE, gameFolders.szDataDir, "rb", 0)) {
	Warning (D1_PIG_LOAD_FAILED);
	return bmi;
	}
if (!gameStates.app.bHaveD1Data)
	paletteManager.LoadD1 ();
LoadD1PigHeader (cf, NULL, &nBmHdrOffs, &nBmDataOffs, &nBitmapNum, 0);
for (i = 0; i < nBitmapNum; i++) {
	PIGBitmapHeaderD1Read (&bmh, cf);
	if (!strnicmp (bmh.name, name, 8))
		break;
	}
if (i >= nBitmapNum) {
#if TRACE
	console.printf (CON_DBG, "could not find bitmap %s\n", name);
#endif
	return bmi;
	}
PiggyBitmapReadD1 (cf, newBm, nBmDataOffs, &bmh, 0, d1ColorMap);
cf.Close ();
newBm->SetAvgColorIndex (0);
bmi.index = gameData.pig.tex.nExtraBitmaps;
gameData.pig.tex.bitmapP [gameData.pig.tex.nExtraBitmaps++] = *newBm;
return bmi;
}

//------------------------------------------------------------------------------

#if 1//ndef FAST_FILE_IO /*permanently enabled for a reason!*/
/*
 * reads a tBitmapIndex structure from a CFile
 */
void ReadBitmapIndex (tBitmapIndex *bi, CFile& cf)
{
bi->index = cf.ReadShort ();
}

//------------------------------------------------------------------------------
/*
 * reads n tBitmapIndex structs from a CFile
 */
int ReadBitmapIndices (CArray<tBitmapIndex>& bi, int n, CFile& cf, int o)
{
	int		i;

for (i = 0; i < n; i++)
	bi [i + o].index = cf.ReadShort ();
return i;
}

#endif // FAST_FILE_IO

//------------------------------------------------------------------------------

typedef struct tBitmapFileHeader {
	short	bfType;
	uint bfSize;
	short bfReserved1;
	short bfReserved2;
	uint bfOffBits;
} tBitmapFileHeader;

typedef struct tBitmapInfoHeader {
	uint biSize;
	uint biWidth;
	uint biHeight;
	short biPlanes;
	short biBitCount;
	uint biCompression;
	uint biSizeImage;
	uint biXPelsPerMeter;
	uint biYPelsPerMeter;
	uint biClrUsed;
	uint biClrImportant;
} tBitmapInfoHeader;

CBitmap *PiggyLoadBitmap (const char *pszFile)
{
	CFile					cf;
	CBitmap				*bmP;
	tBitmapFileHeader	bfh;
	tBitmapInfoHeader	bih;

if (!cf.Open (pszFile, gameFolders.szDataDir, "rb", 0))
	return NULL;

bfh.bfType = cf.ReadShort ();
bfh.bfSize = (uint) cf.ReadInt ();
bfh.bfReserved1 = cf.ReadShort ();
bfh.bfReserved2 = cf.ReadShort ();
bfh.bfOffBits = (uint) cf.ReadInt ();

bih.biSize = (uint) cf.ReadInt ();
bih.biWidth = (uint) cf.ReadInt ();
bih.biHeight = (uint) cf.ReadInt ();
bih.biPlanes = cf.ReadShort ();
bih.biBitCount = cf.ReadShort ();
bih.biCompression = (uint) cf.ReadInt ();
bih.biSizeImage = (uint) cf.ReadInt ();
bih.biXPelsPerMeter = (uint) cf.ReadInt ();
bih.biYPelsPerMeter = (uint) cf.ReadInt ();
bih.biClrUsed = (uint) cf.ReadInt ();
bih.biClrImportant = (uint) cf.ReadInt ();

if (!(bmP = CBitmap::Create (0, bih.biWidth, bih.biHeight, 1))) {
	cf.Close ();
	return NULL;
	}
cf.Seek (bfh.bfOffBits, SEEK_SET);
if (bmP->Read (cf, bih.biWidth * bih.biHeight) != bih.biWidth * bih.biHeight) {
	delete bmP;
	return NULL;
	}
cf.Close ();
return bmP;
}

//------------------------------------------------------------------------------

void _CDECL_ PiggyClose (void)
{
	int				i, j;
	CSoundSample*	dsP;

PrintLog ("unloading textures\n");
PiggyCloseFile ();
PrintLog ("unloading sounds\n");
for (i = 0; i < 2; i++) {
	for (j = 0, dsP = gameData.pig.sound.sounds [i].Buffer (); j < MAX_SOUND_FILES; j++, dsP++)
		if (dsP->bHires) {
			dsP->data [0].Destroy ();
			dsP->bHires = 0;
			}
		else if (dsP->bCustom) {
			dsP->data [1].Destroy ();
			dsP->bCustom = 0;
			}
	gameData.pig.sound.data [i].Destroy ();
	bitmapNames [i].Destroy ();
	soundNames [i].Destroy ();
	}
}

//------------------------------------------------------------------------------
//eof
