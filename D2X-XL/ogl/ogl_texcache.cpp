/*
 *
 * Graphics support functions for OpenGL.
 *
 *
 */

#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#ifdef _WIN32
#	include <windows.h>
#	include <stddef.h>
#	include <io.h>
#endif
#include <string.h>
#include <math.h>
#include <fcntl.h>
#include <stdio.h>
//wii edit:
//#ifdef __macosx__
# include <stdlib.h>
# include <SDL/SDL.h>
//#else
//# include <SDL.h>
//#endif

#include "descent.h"
#include "error.h"
#include "error.h"
#include "ogl_defs.h"
#include "ogl_lib.h"
#include "ogl_texture.h"
#include "ogl_texcache.h"
#include "texmerge.h"
#include "renderlib.h"
#include "menu.h"

static int bLoadTextures = 0;

static CStaticArray< bool, MAX_VCLIPS >	bVClipLoaded;

//------------------------------------------------------------------------------

void OglCachePolyModelTextures (int nModel)
{
	CPolyModel* modelP = GetPolyModel (NULL, NULL, nModel, 0);

if (modelP)
	modelP->LoadTextures (NULL);
}

//------------------------------------------------------------------------------

static void OglCacheVClipTextures (tVideoClip* vcP, int nTranspType)
{
	CBitmap*	bmP;
	int		h;

for (int i = 0; i < vcP->nFrameCount; i++) {
#if DBG
	h = vcP->frames [i].index;
	if ((nDbgTexture >= 0) && (h == nDbgTexture))
		nDbgTexture = nDbgTexture;
#endif
	LoadBitmap (h = vcP->frames [i].index, 0);
	bmP = &gameData.pig.tex.bitmaps [0][h];
	bmP->SetTranspType (nTranspType);
	bmP->SetupTexture (1, bLoadTextures);
	if (!i && bmP->Override ())
		SetupHiresVClip (vcP, NULL, bmP->Override ());
	}
}

//------------------------------------------------------------------------------

static void OglCacheVClipTextures (int i, int nTransp)
{
if ((i >= 0) && !bVClipLoaded [i]) {
	bVClipLoaded [i] = true;
#if DBG
	if (i == 19)
		i = i;
#endif
	OglCacheVClipTextures (&gameData.eff.vClips [0][i], nTransp);
	}
}

//------------------------------------------------------------------------------

static void OglCacheWeaponTextures (CWeaponInfo* wi)
{
OglCacheVClipTextures (wi->nFlashVClip, 1);
OglCacheVClipTextures (wi->nRobotHitVClip, 1);
OglCacheVClipTextures (wi->nWallHitVClip, 1);
if (wi->renderType == WEAPON_RENDER_VCLIP)
	OglCacheVClipTextures (wi->nVClipIndex, 3);
else if (wi->renderType == WEAPON_RENDER_POLYMODEL)
	OglCachePolyModelTextures (wi->nModel);
}

//------------------------------------------------------------------------------

#if 0

static CBitmap *OglLoadFaceBitmap (short nTexture, short nFrameIdx, int bLoadTextures)
{
	CBitmap*	bmP, * bmoP, * bmfP;
	int		nFrames;

LoadBitmap (gameData.pig.tex.bmIndexP [nTexture].index, gameStates.app.bD1Mission);
bmP = gameData.pig.tex.bitmapP + gameData.pig.tex.bmIndexP [nTexture].index;
bmP->SetStatic (1);
if (!(bmoP = bmP->Override ()))
	return bmP;
bmoP->SetStatic (1);
if (!bmoP->WallAnim ())
	return bmoP;
if (2 > (nFrames = bmoP->FrameCount ()))
	return bmoP;
bmoP->SetTranspType (3);
bmoP->SetupTexture (1, bLoadTextures);
if (!(bmfP = bmoP->Frames ()))
	return bmoP;
if ((nFrameIdx < 0) && (nFrames >= -nFrameIdx))
	bmfP -= (nFrameIdx + 1);
bmoP->SetCurFrame (bmfP);
bmfP->SetTranspType (3);
bmfP->SetupTexture (1, bLoadTextures);
bmfP->SetStatic (1);
return bmP;
}

#endif

//------------------------------------------------------------------------------

static void CacheSideTextures (int nSeg)
{
	short			nSide, tMap1, tMap2;
	CBitmap*		bmP, * bm2, * bmm;
	CSide*		sideP;

for (nSide = 0; nSide < MAX_SIDES_PER_SEGMENT; nSide++) {
	sideP = SEGMENTS [nSeg].m_sides + nSide;
	tMap1 = sideP->m_nBaseTex;
	if ((tMap1 < 0) || (tMap1 >= gameData.pig.tex.nTextures [gameStates.app.bD1Data]))
		continue;
	bmP = LoadFaceBitmap (tMap1, sideP->m_nFrame, bLoadTextures);
	if ((tMap2 = sideP->m_nOvlTex)) {
		bm2 = LoadFaceBitmap (tMap1, sideP->m_nFrame, bLoadTextures);
		bm2->SetTranspType (3);
		if (/*!(bm2->Flags () & BM_FLAG_SUPER_TRANSPARENT) ||*/ gameOpts->ogl.bGlTexMerge)
			bm2->SetupTexture (1, bLoadTextures);
		else if ((bmm = TexMergeGetCachedBitmap (tMap1, tMap2, sideP->m_nOvlOrient)))
			bmP = bmm;
		else
			bm2->SetupTexture (1, bLoadTextures);
		}
	bmP->SetTranspType (3);
	bmP->SetupTexture (1, bLoadTextures);
	}
}

//------------------------------------------------------------------------------

static int nCacheSeg = 0;
static int nCacheObj = -3;

static int TexCachePoll (CMenu& menu, int& key, int nCurItem, int nState)
{
if (nState)
	return nCurItem;

if (nCacheSeg < gameData.segs.nSegments)
	CacheSideTextures (nCacheSeg++);
else if (nCacheObj <= gameData.objs.nLastObject [0]) 
	CacheSideTextures (nCacheObj++);
else {
	key = -2;
	return nCurItem;
	}
menu [0].m_value++;
menu [0].m_bRebuild = 1;
key = 0;
return nCurItem;
}

//------------------------------------------------------------------------------

int OglCacheTextures (void)
{
	CMenu	m (3);
	int	i;

m.AddGauge ("                    ", -1, gameData.segs.nSegments + gameData.objs.nLastObject [0] + 4); 
nCacheSeg = 0;
nCacheObj = -3;
do {
	i = m.Menu (NULL, "Loading textures", TexCachePoll);
	} while (i >= 0);
return 1;
}

//------------------------------------------------------------------------------

static void CacheAddonTextures (void)
{
	int	i;

for (i = 0; i < MAX_ADDON_BITMAP_FILES; i++) {
	PageInAddonBitmap (-i - 1);
	BM_ADDON (i)->SetTranspType (0);
	BM_ADDON (i)->SetupTexture (1, 1); 
	}
}

//------------------------------------------------------------------------------

int OglCacheLevelTextures (void)
{
	int				i, j, bD1;
	tEffectClip*	ecP;
	int				max_efx = 0, ef;
	int				nSegment, nSide;
	short				nBaseTex, nOvlTex;
	CBitmap*			bmBot,* bmTop, * bmm;
	CSegment*		segP;
	CSide*			sideP;
	CObject*			objP;
	CStaticArray< bool, MAX_POLYGON_MODELS >	bModelLoaded;

if (gameStates.render.bBriefing)
	return 0;
PrintLog ("caching level textures\n");
TexMergeClose ();
TexMergeInit (-1);
PrintLog ("   caching effect textures\n");
for (bD1 = 0; bD1 <= gameStates.app.bD1Data; bD1++) {
	for (i = 0, ecP = gameData.eff.effects [bD1].Buffer (); i < gameData.eff.nEffects [bD1]; i++, ecP++) {
		if ((ecP->changingWallTexture == -1) && (ecP->changingObjectTexture == -1))
			continue;
		if (ecP->vClipInfo.nFrameCount > max_efx)
			max_efx = ecP->vClipInfo.nFrameCount;
		}
	for (ef = 0; ef < max_efx; ef++)
		for (i = 0, ecP = gameData.eff.effects [bD1].Buffer (); i < gameData.eff.nEffects [bD1]; i++, ecP++) {
			if ((ecP->changingWallTexture == -1) && (ecP->changingObjectTexture == -1))
				continue;
			ecP->xTimeLeft = -1;
			}
	}

PrintLog ("   caching geometry textures\n");
bLoadTextures = (ogl.m_states.nPreloadTextures > 0);
for (segP = SEGMENTS.Buffer (), nSegment = 0; nSegment < gameData.segs.nSegments; nSegment++, segP++) {
	for (nSide = 0, sideP = segP->m_sides; nSide < MAX_SIDES_PER_SEGMENT; nSide++, sideP++) {
		nBaseTex = sideP->m_nBaseTex;
		if ((nBaseTex < 0) || (nBaseTex >= gameData.pig.tex.nTextures [gameStates.app.bD1Data]))
			continue;
#if DBG
		if ((nSegment == nDbgSeg) && ((nDbgSide < 0) || (nSide == nDbgSide)))
			nDbgSeg = nDbgSeg;
#endif
		bmBot = LoadFaceBitmap (nBaseTex, sideP->m_nFrame, bLoadTextures);
		if ((nOvlTex = sideP->m_nOvlTex)) {
			bmTop = LoadFaceBitmap (nOvlTex, sideP->m_nFrame, bLoadTextures);
			bmTop->SetTranspType (3);
			if (gameOpts->ogl.bGlTexMerge) // || !(bmTop->Flags () & BM_FLAG_SUPER_TRANSPARENT))
				bmTop->SetupTexture (1, bLoadTextures);
			else if ((bmm = TexMergeGetCachedBitmap (nBaseTex, nOvlTex, sideP->m_nOvlOrient)))
				bmBot = bmm;
			else
				bmTop->SetupTexture (1, bLoadTextures);
			}
		bmBot->SetTranspType (3);
		bmBot->SetupTexture (1, bLoadTextures);
		}
	}

PrintLog ("   caching addon textures\n");
CacheAddonTextures ();

PrintLog ("   caching model textures\n");
bLoadTextures = (ogl.m_states.nPreloadTextures > 1);
bModelLoaded.Clear ();
bVClipLoaded.Clear ();
FORALL_OBJS (objP, i) {
	if (objP->info.renderType != RT_POLYOBJ)
		continue;
	if (bModelLoaded [objP->rType.polyObjInfo.nModel])
		continue;
	bModelLoaded [objP->rType.polyObjInfo.nModel] = true;
	OglCachePolyModelTextures (objP->rType.polyObjInfo.nModel);
	}

PrintLog ("   caching hostage sprites\n");
bLoadTextures = (ogl.m_states.nPreloadTextures > 3);
OglCacheVClipTextures (33, 3);    

PrintLog ("   caching weapon sprites\n");
bLoadTextures = (ogl.m_states.nPreloadTextures > 5);
for (i = 0; i < EXTRA_OBJ_IDS; i++)
	OglCacheWeaponTextures (gameData.weapons.info + i);

PrintLog ("   caching powerup sprites\n");
bLoadTextures = (ogl.m_states.nPreloadTextures > 4);
for (i = 0; i < MAX_POWERUP_TYPES; i++)
	if (i != 9)
		OglCacheVClipTextures (gameData.objs.pwrUp.info [i].nClipIndex, 3);

PrintLog ("   caching effect textures\n");
CacheObjectEffects ();
bLoadTextures = (ogl.m_states.nPreloadTextures > 2);
for (i = 0; i < gameData.eff.nClips [0]; i++)
	OglCacheVClipTextures (i, 1);

PrintLog ("   caching cockpit textures\n");
for (i = 0; i < 2; i++)
	for (j = 0; j < MAX_GAUGE_BMS; j++)
		if (gameData.cockpit.gauges [i][j].index != 0xffff)
			LoadBitmap (gameData.cockpit.gauges [i][j].index, 0);

ResetSpecialEffects ();
InitSpecialEffects ();
DoSpecialEffects (true);
return 0;
}

//------------------------------------------------------------------------------

