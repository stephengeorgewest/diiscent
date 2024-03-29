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

#ifndef _RENDERLIB_H
#define _RENDERLIB_H

#include "descent.h"

//------------------------------------------------------------------------------

#define VISITED(_ch)	(gameData.render.mine.bVisited [_ch] == gameData.render.mine.nVisited)
#define VISIT(_ch) (gameData.render.mine.bVisited [_ch] = gameData.render.mine.nVisited)

//------------------------------------------------------------------------------

typedef struct tFaceProps {
	short			segNum, sideNum;
	short			nBaseTex, nOvlTex, nOvlOrient;
	tUVL			uvls [4];
	short			vp [5];
	tUVL			uvl_lMaps [4];
	CFixVector	vNormal;
	ubyte			nVertices;
	ubyte			widFlags;
	char			nType;
} tFaceProps;

typedef struct tFaceListEntry {
	short			nextFace;
	tFaceProps	props;
} tFaceListEntry;

//------------------------------------------------------------------------------

int LoadExplBlast (void);
void FreeExplBlast (void);
int LoadCorona (void);
int LoadGlare (void);
int LoadHalo (void);
int LoadThruster (int nStyle = -1);
int LoadShield (void);
int LoadDeadzone (void);
void LoadDamageIcons (void);
void FreeDamageIcons (void);
int LoadDamageIcon (int i);
void FreeDamageIcon (int i);
int LoadScope (void);
void FreeScope (void);
void LoadAddonImages (void);
void FreeExtraImages (void);
void FreeDeadzone (void);
int LoadJoyMouse (void);
void FreeJoyMouse (void);
int LoadAddonBitmap (CBitmap **bmPP, const char *pszName, int *bHaveP);

// Given a list of point numbers, rotate any that haven't been rotated
// this frame
g3sPoint *RotateVertex (int i);
g3sCodes RotateVertexList (int nVerts, short* vertexIndexP);
void RotateSideNorms (void);
// Given a list of point numbers, project any that haven't been projected
void ProjectVertexList (int nv, short *pointIndex);

void TransformSideCenters (void);
#if USE_SEGRADS
void TransformSideCenters (void);
#endif

int IsTransparentTexture (short nTexture);
int SetVertexColor (int nVertex, tFaceColor *pc);
int SetVertexColors (tFaceProps *propsP);
fix SetVertexLight (int nSegment, int nSide, int nVertex, tFaceColor *pc, fix light);
int SetFaceLight (tFaceProps *propsP);
void AdjustVertexColor (CBitmap *bmP, tFaceColor *pc, fix xLight);
char IsColoredSegFace (short nSegment, short nSide);
tRgbaColorf *ColoredSegmentColor (int nSegment, int nSide, char nColor);
int IsMonitorFace (short nSegment, short nSide, int bForce);
float WallAlpha (short nSegment, short nSide, short nWall, ubyte widFlags, int bIsMonitor, ubyte bAdditive,
					  tRgbaColorf *pc, int *bCloaking, ubyte *bTextured, ubyte* bCloaked, ubyte* bTransparent);
int SetupMonitorFace (short nSegment, short nSide, short nCamera, CSegFace *faceP);
CBitmap *LoadFaceBitmap (short nTexture, short nFrameIdx, int bLoadTextures = 1);
void DrawOutline (int nVertices, g3sPoint **pointList);
int ToggleOutlineMode (void);
int ToggleShowOnlyCurSide (void);
void RotateTexCoord2f (tTexCoord2f& dest, tTexCoord2f& src, ubyte nOrient);
int FaceIsVisible (short nSegment, short nSide);
int SegmentMayBeVisible (short nStartSeg, short nRadius, int nMaxDist, int nThread = 0);
ubyte BumpVisitedFlag (void);
ubyte BumpProcessedFlag (void);
ubyte BumpVisibleFlag (void);
void SetupMineRenderer (void);
void ComputeMineLighting (short nStartSeg, fix xStereoSeparation, int nWindow);

#if DBG
void OutlineSegSide (CSegment *seg, int _side, int edge, int vert);
void DrawWindowBox (uint color, short left, short top, short right, short bot);
#endif

//------------------------------------------------------------------------------

extern CBitmap *bmpCorona;
extern CBitmap *bmpGlare;
extern CBitmap *bmpHalo;
extern CBitmap *bmpThruster [2][2];
extern CBitmap *bmpShield;
extern CBitmap *bmpExplBlast;
extern CBitmap *bmpSparks;
extern CBitmap *bmpDeadzone;
extern CBitmap *bmpScope;
extern CBitmap *bmpJoyMouse;
extern CBitmap *bmpDamageIcon [3];
extern int bHaveDamageIcon [3];
extern const char* szDamageIcon [3];

extern int bHaveDeadzone;
extern int bHaveJoyMouse;

extern tRgbaColorf segmentColors [4];

#if DBG
extern short nDbgSeg;
extern short nDbgSide;
extern int nDbgVertex;
extern int nDbgBaseTex;
extern int nDbgOvlTex;
#endif

extern int bOutLineMode, bShowOnlyCurSide;

//------------------------------------------------------------------------------

static inline int IsTransparentFace (tFaceProps *propsP)
{
return IsTransparentTexture (SEGMENTS [propsP->segNum].m_sides [propsP->sideNum].m_nBaseTex);
}

//------------------------------------------------------------------------------

static inline int IsWaterTexture (short nTexture)
{
return ((nTexture >= 399) && (nTexture <= 403));
}

//------------------------------------------------------------------------------

#endif // _RENDERLIB_H
