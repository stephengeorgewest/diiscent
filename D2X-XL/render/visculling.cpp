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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include "descent.h"
#include "error.h"
#include "gamefont.h"
#include "texmap.h"
#include "rendermine.h"
#include "fastrender.h"
#include "rendershadows.h"
#include "transprender.h"
#include "renderthreads.h"
#include "glare.h"
#include "radar.h"
#include "objrender.h"
#include "textures.h"
#include "screens.h"
#include "segpoint.h"
#include "texmerge.h"
#include "physics.h"
#include "gameseg.h"
#include "light.h"
#include "dynlight.h"
#include "headlight.h"
#include "newdemo.h"
#include "automap.h"
#include "key.h"
#include "u_mem.h"
#include "kconfig.h"
#include "mouse.h"
#include "interp.h"
#include "ogl_lib.h"
#include "ogl_color.h"
#include "ogl_render.h"
#include "ogl_fastrender.h"
#include "cockpit.h"
#include "input.h"
#include "shadows.h"
#include "textdata.h"
#include "sparkeffect.h"
#include "createmesh.h"

//------------------------------------------------------------------------------

#define bPreDrawSegs			0

#define VIS_CULLING			1
#define OLD_SEGLIST			1
#define SORT_RENDER_SEGS	0

//------------------------------------------------------------------------------

typedef struct tPortal {
	fix	left, right, top, bot;
	char  bProjected;
	ubyte bVisible;
} tPortal;

// ------------------------------------------------------------------------------

void SortSidesByDist (double *sideDists, char *sideNums, int left, int right)
{
	int		l = left;
	int		r = right;
	int		h, m = (l + r) / 2;
	double	hd, md = sideDists [m];

do {
	while (sideDists [l] < md)
		l++;
	while (sideDists [r] > md)
		r--;
	if (l <= r) {
		if (l < r) {
			h = sideNums [l];
			sideNums [l] = sideNums [r];
			sideNums [r] = h;
			hd = sideDists [l];
			sideDists [l] = sideDists [r];
			sideDists [r] = hd;
			}
		++l;
		--r;
		}
	} while (l <= r);
if (right > l)
   SortSidesByDist (sideDists, sideNums, l, right);
if (r > left)
   SortSidesByDist (sideDists, sideNums, left, r);
}

//------------------------------------------------------------------------------

ubyte CodePortalPoint (fix x, fix y, tPortal *p)
{
	ubyte code = 0;

if (x <= p->left)
	code |= 1;
else if (x >= p->right)
	code |= 2;
if (y <= p->top)
	code |= 4;
else if (y >= p->bot)
	code |= 8;
return code;
}

//------------------------------------------------------------------------------

ubyte CodePortal (tPortal *s, tPortal *p)
{
	ubyte code = 0;

if (s->right <= p->left)
	code |= 1;
else if (s->left >= p->right)
	code |= 2;
if (s->bot <= p->top)
	code |= 4;
else if (s->top >= p->bot)
	code |= 8;
return code;
}

//------------------------------------------------------------------------------

tPortal renderPortals [MAX_SEGMENTS_D2X];
#if !OLD_SEGLIST
tPortal sidePortals [MAX_SEGMENTS_D2X * 6];
#endif
ubyte bVisible [MAX_SEGMENTS_D2X];

//Given two sides of CSegment, tell the two verts which form the
//edge between them
short edgeBetweenTwoSides [6][6][2] = {
	{{-1, -1}, {3, 7}, {-1, -1}, {2, 6}, {6, 7}, {2, 3}},
	{{3, 7}, {-1, -1}, {0, 4}, {-1, -1}, {4, 7}, {0, 3}},
	{{-1, -1}, {0, 4}, {-1, -1}, {1, 5}, {4, 5}, {0, 1}},
	{{2, 6}, {-1, -1}, {1, 5}, {-1, -1}, {5, 6}, {1, 2}},
	{{6, 7}, {4, 7}, {4, 5}, {5, 6}, {-1, -1}, {-1, -1}},
	{{2, 3}, {0, 3}, {0, 1}, {1, 2}, {-1, -1}, {-1, -1}}
	};

//given an edge specified by two verts, give the two sides on that edge
int edgeToSides [8][8][2] = {
	{{-1, -1}, {2, 5}, {-1, -1}, {1, 5}, {1, 2}, {-1, -1}, {-1, -1}, {-1, -1}},
	{{2, 5}, {-1, -1}, {3, 5}, {-1, -1}, {-1, -1}, {2, 3}, {-1, -1}, {-1, -1}},
	{{-1, -1}, {3, 5}, {-1, -1}, {0, 5}, {-1, -1}, {-1, -1}, {0, 3}, {-1, -1}},
	{{1, 5}, {-1, -1}, {0, 5}, {-1, -1}, {-1, -1}, {-1, -1}, {-1, -1}, {0, 1}},
	{{1, 2}, {-1, -1}, {-1, -1}, {-1, -1}, {-1, -1}, {2, 4}, {-1, -1}, {1, 4}},
	{{-1, -1}, {2, 3}, {-1, -1}, {-1, -1}, {2, 4}, {-1, -1}, {3, 4}, {-1, -1}},
	{{-1, -1}, {-1, -1}, {0, 3}, {-1, -1}, {-1, -1}, {3, 4}, {-1, -1}, {0, 4}},
	{{-1, -1}, {-1, -1}, {-1, -1}, {0, 1}, {1, 4}, {-1, -1}, {0, 4}, {-1, -1}},
	};

//------------------------------------------------------------------------------
//given an edge and one side adjacent to that edge, return the other adjacent CSide

#if SORT_RENDER_SEGS

int FindOtherSideOnEdge (CSegment *segP, short *verts, int oppSide)
{
	int	i;
	int	i0 = -1, i1 = -1;
	int	side0, side1;
	int	*eptr;
	int	sv, v0, v1;
	short	*vp;

//@@	check_check();

v0 = verts [0];
v1 = verts [1];
vp = seg->verts;
for (i = 0; i < 8; i++) {
	sv = *vp++;	// seg->verts [i];
	if (sv == v0) {
		i0 = i;
		if (i1 != -1)
			break;
		}
	if (sv == v1) {
		i1 = i;
		if (i0 != -1)
			break;
		}
	}
eptr = edgeToSides [i0][i1];
side0 = eptr [0];
side1 = eptr [1];
return (side0 == oppSide) ? side1 : side0;
}

//------------------------------------------------------------------------------
//find the two segments that join a given seg through two sides, and
//the sides of those segments the abut.

typedef struct tSideNormData {
	CFixVector	n [2];
	CFixVector	*p;
	short			t;
} tSideNormData;

int FindAdjacentSideNorms (CSegment *segP, short s0, short s1, tSideNormData *s)
{
	CSegment	*seg0, *seg1;
	CSide		*side0, *side1;
	short		edgeVerts [2];
	int		oppSide0, oppSide1;
	int		otherSide0, otherSide1;

Assert(s0 != -1 && s1 != -1);
seg0 = SEGMENTS + segP->m_children [s0];
seg1 = SEGMENTS + segP->m_children [s1];
edgeVerts [0] = segP->m_verts [edgeBetweenTwoSides [s0][s1][0]];
edgeVerts [1] = segP->m_verts [edgeBetweenTwoSides [s0][s1][1]];
Assert(edgeVerts [0] != -1 && edgeVerts [1] != -1);
oppSide0 = segP->ConnectedSide (seg0);
Assert (oppSide0 != -1);
oppSide1 = segP->ConnectedSide (seg1);
Assert (oppSide1 != -1);
otherSide0 = FindOtherSideOnEdge (seg0, edgeVerts, oppSide0);
otherSide1 = FindOtherSideOnEdge (seg1, edgeVerts, oppSide1);
side0 = seg0->m_sides + otherSide0;
side1 = seg1->m_sides + otherSide1;
memcpy (s [0].n, side0->m_normals, 2 * sizeof (CFixVector));
memcpy (s [1].n, side1->m_normals, 2 * sizeof (CFixVector));
s [0].p = gameData.segs.vertices + seg0->m_verts [sideVertIndex [otherSide0][(s [0].t = side0->m_nType) == 3]];
s [1].p = gameData.segs.vertices + seg1->m_verts [sideVertIndex [otherSide1][(s [1].t = side1->m_nType) == 3]];
return 1;
}

//------------------------------------------------------------------------------
//see if the order matters for these two children.
//returns 0 if order doesn't matter, 1 if c0 before c1, -1 if c1 before c0
static int CompareChildren (CSegment *segP, short c0, short c1)
{
	tSideNormData	s [2];
	CFixVector		temp;
	fix				d0, d1;

if (sideOpposite [c0] == c1)
	return 0;
//find normals of adjoining sides
FindAdjacentSideNorms (segP, c0, c1, s);
temp = gameData.render.mine.viewerEye - *s [0].p;
d0 = CFixVector::Dot (s [0].n [0], temp);
if (s [0].t != 1)	// triangularized face -> 2 different normals
	d0 |= CFixVector::Dot (s [0].n [1], temp);	// we only need the sign, so a bitwise or does the trick
temp = gameData.render.mine.viewerEye - *s [1].p;
d1 = CFixVector::Dot (s [1].n [0], temp);
if (s [1].t != 1)
	d1 |= CFixVector::Dot (s [1].n [1], temp);
if ((d0 & d1) < 0)	// only < 0 if both negative due to bitwise and
	return 0;
if (d0 < 0)
	return 1;
 if (d1 < 0)
	return -1;
return 0;
}

//------------------------------------------------------------------------------

int QuickSortSegChildren (CSegment *segP, short left, short right, short *childList)
{
	short	h,
			l = left,
			r = right,
			m = (l + r) / 2,
			median = childList [m],
			bSwap = 0;

do {
	while ((l < m) && CompareChildren (segP, childList [l], median) >= 0)
		l++;
	while ((r > m) && CompareChildren (segP, childList [r], median) <= 0)
		r--;
	if (l <= r) {
		if (l < r) {
			h = childList [l];
			childList [l] = childList [r];
			childList [r] = h;
			bSwap = 1;
			}
		l++;
		r--;
		}
	} while (l <= r);
if (l < right)
	bSwap |= QuickSortSegChildren (segP, l, right, childList);
if (left < r)
	bSwap |= QuickSortSegChildren (segP, left, r, childList);
return bSwap;
}

//------------------------------------------------------------------------------

//short the children of CSegment to render in the correct order
//returns non-zero if swaps were made
static inline int SortSegChildren (CSegment *segP, int nChildren, short *childList)
{
#if 1

if (nChildren < 2)
	return 0;
if (nChildren == 2) {
	if (CompareChildren (segP, childList [0], childList [1]) >= 0)
		return 0;
	short h = childList [0];
	childList [0] = childList [1];
	childList [1] = h;
	return 1;
	}
return QuickSortSegChildren (segP, (short) 0, (short) (nChildren - 1), childList);

#else
	int i, j;
	int r;
	int made_swaps, count;

if (nChildren < 2)
	return 0;
	//for each child,  compare with other children and see if order matters
	//if order matters, fix if wrong

count = 0;

do {
	made_swaps = 0;

	for (i=0;i<nChildren-1;i++)
		for (j=i+1;childList [i]!=-1 && j<nChildren;j++)
			if (childList [j]!=-1) {
				r = CompareChildren(segP, childList [i], childList [j]);

				if (r == 1) {
					int temp = childList [i];
					childList [i] = childList [j];
					childList [j] = temp;
					made_swaps=1;
				}
			}

} while (made_swaps && ++count<nChildren);
return count;
#endif
}

#endif //SORT_RENDER_SEGS

//------------------------------------------------------------------------------

void UpdateRenderedData (int nWindow, CObject *viewer, int rearViewFlag, int user)
{
Assert(nWindow < MAX_RENDERED_WINDOWS);
windowRenderedData [nWindow].nFrame = gameData.app.nFrameCount;
windowRenderedData [nWindow].viewerP = viewer;
windowRenderedData [nWindow].bRearView = rearViewFlag;
windowRenderedData [nWindow].nUser = user;
}

//------------------------------------------------------------------------------

void AddObjectToSegList (short nObject, short nSegment)
{
	tObjRenderListItem *pi = gameData.render.mine.renderObjs.objs + gameData.render.mine.renderObjs.nUsed;

#if DBG
if (nObject == nDbgObj)
	nDbgObj = nDbgObj;
#endif
pi->nNextItem = gameData.render.mine.renderObjs.ref [nSegment];
gameData.render.mine.renderObjs.ref [nSegment] = gameData.render.mine.renderObjs.nUsed++;
pi->nObject = nObject;
pi->xDist = CFixVector::Dist (OBJECTS [nObject].info.position.vPos, gameData.render.mine.viewerEye);
}

//------------------------------------------------------------------------------

void BuildRenderObjLists (int nSegCount)
{
PROF_START
	CObject*		objP;
	CSegment*	segP;
	CSegMasks	mask;
	short			nSegment, nNewSeg, nChild, nSide, sideFlag;
	int			nListPos;
	short			nObject;

gameData.render.mine.renderObjs.ref.Clear (char (0xff));
gameData.render.mine.renderObjs.nUsed = 0;

for (nListPos = 0; nListPos < nSegCount; nListPos++) {
	nSegment = gameData.render.mine.nSegRenderList [0][nListPos];
	if (nSegment == -0x7fff)
		continue;
#if DBG
	if (nSegment == nDbgSeg)
		nSegment = nSegment;
#endif
	for (nObject = SEGMENTS [nSegment].m_objects; nObject != -1; nObject = objP->info.nNextInSeg) {
#if DBG
		if (nObject == nDbgObj)
			nDbgObj = nDbgObj;
#endif
		objP = OBJECTS + nObject;
		//Assert (objP->info.nSegment == nSegment);
		if (objP->info.nFlags & OF_ATTACHED)
			continue;		//ignore this CObject
		nNewSeg = nSegment;
		if ((objP->info.nType != OBJ_REACTOR) && ((objP->info.nType != OBJ_ROBOT) || (objP->info.nId == 65))) { //don't migrate controlcen
			mask = SEGMENTS [nNewSeg].Masks (OBJPOS (objP)->vPos, objP->info.xSize);
			if (mask.m_side) {
				for (nSide = 0, sideFlag = 1; nSide < 6; nSide++, sideFlag <<= 1) {
					if (!(mask.m_side & sideFlag))
						continue;
					segP = SEGMENTS + nNewSeg;
					if (segP->IsDoorWay (nSide, NULL) & WID_FLY_FLAG) {	//can explosion migrate through
						nChild = segP->m_children [nSide];
						if (gameData.render.mine.bVisible [nChild] == gameData.render.mine.nVisible)
							nNewSeg = nChild;	// only migrate to segment in render list
						}
					}
				}
			}
		AddObjectToSegList (nObject, nNewSeg);
		}
	}
PROF_END(ptBuildObjList)
}

//------------------------------------------------------------------------------
//build a list of segments to be rendered
//fills in gameData.render.mine.nSegRenderList & gameData.render.mine.nRenderSegs

typedef struct tSegZRef {
	fix	z;
	//fix	d;
	short	nSegment;
} tSegZRef;

static tSegZRef segZRef [2][MAX_SEGMENTS_D2X];

void QSortSegZRef (short left, short right)
{
	tSegZRef	*ps = segZRef [0];
	tSegZRef	m = ps [(left + right) / 2];
	tSegZRef	h;
	short		l = left,
				r = right;
do {
	while ((ps [l].z > m.z))// || ((segZRef [l].z == m.z) && (segZRef [l].d > m.d)))
		l++;
	while ((ps [r].z < m.z))// || ((segZRef [r].z == m.z) && (segZRef [r].d < m.d)))
		r--;
	if (l <= r) {
		if (l < r) {
			h = ps [l];
			ps [l] = ps [r];
			ps [r] = h;
			}
		l++;
		r--;
		}
	}
while (l < r);
if (l < right)
	QSortSegZRef (l, right);
if (left < r)
	QSortSegZRef (left, r);
}

//------------------------------------------------------------------------------

void InitSegZRef (int i, int j, int nThread)
{
	tSegZRef*		ps = segZRef [0] + i;
	CFloatVector	vViewer, vCenter;
	int				r, z, zMin = 0x7fffffff, zMax = -0x7fffffff;
	CSegment*		segP;

vViewer.Assign (gameData.render.mine.viewerEye);
for (; i < j; i++, ps++) {
	segP = SEGMENTS + gameData.render.mine.nSegRenderList [0][i];
#if TRANSP_DEPTH_HASH
	vCenter.Assign (segP->Center ());
	float d = CFloatVector::Dist (vCenter, vViewer);
	z = F2X (d);
	r = segP->MaxRad ();
	if (zMax < z + r)
		zMax = z + r;
	if (zMin > z - r)
		zMin = z - r;
	ps->z = z;
#else
	CFixVector v = segP->Center ();
	transformation.Transform (v, v, 0);
	v [Z] += segP->MaxRad ();
	if (zMin > v [Z])
		zMin = v [Z];
	if (zMax < v [Z])
		zMax = v [Z];
	ps->z = v [Z];
#endif
	ps->nSegment = gameData.render.mine.nSegRenderList [0][i];
	}
tiRender.zMin [nThread] = zMin;
tiRender.zMax [nThread] = zMax;
}

//------------------------------------------------------------------------------

void MergeSegZRefs (void)
{
	tSegZRef	*ps, *pi, *pj;
	int		h, i, j;

h = gameData.render.mine.nRenderSegs;
for (i = h / 2, j = h - i, ps = segZRef [1], pi = segZRef [0], pj = pi + h / 2; h; h--) {
	if (i && (!j || (pi->z < pj->z))) {
		*ps++ = *pi++;
		i--;
		}
	else if (j) {
		*ps++ = *pj++;
		j--;
		}
	}
}

//------------------------------------------------------------------------------

void GetMaxDepth (void)
{
gameData.render.zMax = 0;
for (int i = 0; i < gameStates.app.nThreads; i++) {
	if (gameData.render.zMax < tiRender.zMax [i])
		gameData.render.zMax = tiRender.zMax [i];
	if (gameData.render.zMin > tiRender.zMin [i])
		gameData.render.zMin = tiRender.zMin [i];
	}
}

//------------------------------------------------------------------------------

void SortRenderSegs (void)
{
if (gameData.render.mine.nRenderSegs < 2)
	return;

#if USE_OPENMP > 1

	int h, i, j;

if (gameStates.app.nThreads < 2) {
	InitSegZRef (0, gameData.render.mine.nRenderSegs, 0);
	gameData.render.zMin = tiRender.zMin [0];
	gameData.render.zMax = tiRender.zMax [0];
	QSortSegZRef (0, gameData.render.mine.nRenderSegs - 1);
	}
else
#pragma omp parallel
	{
	#pragma omp for private (i, j)
	for (h = 0; h < gameStates.app.nThreads; h++) {
		ComputeThreadRange (h, gameData.render.mine.nRenderSegs, i, j);
		InitSegZRef (i, j, h);
		}
	}
GetMaxDepth ();
#else
if (RunRenderThreads (rtInitSegZRef)) 
	GetMaxDepth ();
else {
	InitSegZRef (0, gameData.render.mine.nRenderSegs, 0);
	gameData.render.zMin = tiRender.zMin [0];
	gameData.render.zMax = tiRender.zMax [0];
	}
#endif
}

//------------------------------------------------------------------------------

void CalcRenderDepth (void)
{
CFixVector vCenter;
transformation.Transform (vCenter, SEGMENTS [gameData.objs.viewerP->info.nSegment].Center (), 0);
CFixVector v;
transformation.Transform (v, gameData.segs.vMin, 0);
fix d1 = CFixVector::Dist (v, vCenter);
transformation.Transform (v, gameData.segs.vMax, 0);
fix d2 = CFixVector::Dist (v, vCenter);

if (d1 < d2)
	d1 = d2;
fix r = gameData.segs.segRads [1][gameData.objs.viewerP->info.nSegment];
gameData.render.zMin = 0;
gameData.render.zMax = vCenter [Z] + d1 + r;
}

//-----------------------------------------------------------------

void BuildRenderSegList (short nStartSeg, int nWindow, bool bIgnoreDoors, int nThread)
{
	int			nCurrent, nHead, nTail, nStart, nSide;
	int			l, i, j;
	short			nChild;
	short			nChildSeg;
	short*		sv;
	int*			s2v;
	ubyte			andCodes, andCodes3D;
	int			bRotated, nSegment, bProjected;
	tPortal*		curPortal;
	short			childList [MAX_SIDES_PER_SEGMENT];		//list of ordered sides to process
	int			nChildren, bCullIfBehind;					//how many sides in childList
	CSegment*	segP;
	CFixVector	viewDir, viewPos;

	CShortArray&	segRenderList = gameData.render.mine.nSegRenderList [nThread];
	CShortArray&	renderPos = gameData.render.mine.nRenderPos [nThread];

viewDir = transformation.m_info.view [0].FVec ();
viewPos = transformation.m_info.pos;
gameData.render.zMin = 0x7fffffff;
gameData.render.zMax = -0x7fffffff;
bCullIfBehind = !SHOW_SHADOWS || (gameStates.render.nShadowPass == 1);
renderPos.Clear (char (0xff));
BumpVisitedFlag ();
BumpProcessedFlag ();
BumpVisibleFlag ();

if (automap.Display () && gameOpts->render.automap.bTextured && !automap.Radar ()) {
	int nSegmentLimit = automap.SegmentLimit ();
	int bUnlimited = nSegmentLimit == automap.MaxSegsAway ();
	int bSkyBox = gameOpts->render.automap.bSkybox;

	for (i = gameData.render.mine.nRenderSegs = 0; i < gameData.segs.nSegments; i++)
		if ((automap.Visible (i)) &&
			 (bSkyBox || (SEGMENTS [i].m_nType != SEGMENT_IS_SKYBOX)) &&
			 (bUnlimited || (automap.m_visible [i] <= nSegmentLimit))) {
			segRenderList [gameData.render.mine.nRenderSegs++] = i;
			gameData.render.mine.bVisible [i] = gameData.render.mine.nVisible;
			VISIT (i);
			}
	SortRenderSegs ();
	return;
	}

segRenderList [0] = nStartSeg;
gameData.render.mine.nSegDepth [0] = 0;
VISIT (nStartSeg);
renderPos [nStartSeg] = 0;
nHead = nTail = nStart = 0;
nCurrent = 1;

renderPortals [0].left =
renderPortals [0].top = 0;
renderPortals [0].right = CCanvas::Current ()->Width () - 1;
renderPortals [0].bot = CCanvas::Current ()->Height () - 1;
#if DBG
int nIterations = 0;
#endif
int nRenderDepth = min (gameStates.render.detail.nRenderDepth, gameData.segs.nSegments);
for (l = 0; l < nRenderDepth; l++) {
	for (nHead = nStart, nTail = nCurrent; nHead < nTail; nHead++) {
		if (gameData.render.mine.bProcessed [nHead] == gameData.render.mine.nProcessed)
			continue;
		gameData.render.mine.bProcessed [nHead] = gameData.render.mine.nProcessed;
		nSegment = segRenderList [nHead];
		curPortal = renderPortals + nHead;
		if (nSegment == -1)
			continue;
#if DBG
		if (nSegment == -32767)
			continue;
		if (nSegment == nDbgSeg)
			nSegment = nSegment;
#endif
#if DBG
		nIterations++;
#endif
		segP = SEGMENTS + nSegment;
		sv = segP->m_verts;
		bRotated = 0;
		//look at all sides of this CSegment.
		//tricky code to look at sides in correct order follows
		for (nChild = nChildren = 0; nChild < MAX_SIDES_PER_SEGMENT; nChild++) {		//build list of sides
			nChildSeg = segP->m_children [nChild];
			if (nChildSeg < 0)
				continue;
			if (!(segP->IsDoorWay (nChild, NULL, bIgnoreDoors) & WID_RENDPAST_FLAG))
				continue;
#if DBG
			if (nChildSeg == nDbgSeg)
				nChildSeg = nChildSeg;
#endif
			if (!bRotated) {
				RotateVertexList (8, sv);
				bRotated = 1;
				}
			if (bCullIfBehind) {
				andCodes = 0xff;
				s2v = sideVertIndex [nChild];
				if ((0xff & gameData.segs.points [sv [s2v [0]]].p3_codes
							 & gameData.segs.points [sv [s2v [1]]].p3_codes
					 	    & gameData.segs.points [sv [s2v [2]]].p3_codes
						    & gameData.segs.points [sv [s2v [3]]].p3_codes) & CC_BEHIND)
					continue;
				}
			childList [nChildren++] = nChild;
			}
		//now order the sides in some magical way
		for (nChild = 0; nChild < nChildren; nChild++) {
			nSide = childList [nChild];
			nChildSeg = segP->m_children [nSide];
#if DBG
			if ((nChildSeg == nDbgSeg) && ((nDbgSide < 0) || (nSide == nDbgSide)))
				nChildSeg = nChildSeg;
#endif
			fix x, y;
			tPortal p = {32767, -32767, 32767, -32767};
			bProjected = 4;	//a point wasn't projected
			s2v = sideVertIndex [nSide];
			andCodes3D = 0xff;
			for (j = 0; j < 4; j++) {
				g3sPoint *pnt = gameData.segs.points + sv [s2v [j]];
				if (pnt->p3_codes & CC_BEHIND) {
#if VIS_CULLING == 2
					bProjected--;
#else
					bProjected = 0;
					break;
#endif
					}
				if (!(pnt->p3_flags & PF_PROJECTED))
					G3ProjectPoint (pnt);
				x = pnt->p3_screen.x;
				y = pnt->p3_screen.y;
				andCodes3D &= (pnt->p3_codes & ~CC_BEHIND);
				if (p.left > x)
					p.left = x;
				if (p.right < x)
					p.right = x;
				if (p.top > y)
					p.top = y;
				if (p.bot < y)
					p.bot = y;
				}
#if DBG
			if ((nChildSeg == nDbgSeg) && ((nDbgSide < 0) || (nSide == nDbgSide)))
				nChildSeg = nChildSeg;
#endif
#if VIS_CULLING == 2
			if ((p.left < p.right) && (p.top < p.bot)) {
#else
			if (!bProjected || !(andCodes3D | (0xff & CodePortal (&p, curPortal)))) {	//maybe add this segment
#endif
				int nPos = renderPos [nChildSeg];
				tPortal *newPortal = renderPortals + nCurrent;

				if (!bProjected)
					*newPortal = *curPortal;
				else {
					newPortal->left = max (curPortal->left, p.left);
					newPortal->right = min (curPortal->right, p.right);
					newPortal->top = max (curPortal->top, p.top);
					newPortal->bot = min (curPortal->bot, p.bot);
					}
				//see if this segment has already been visited, and if so, does the current portal expand the old portal?
				if (nPos == -1) {
					renderPos [nChildSeg] = nCurrent;
					segRenderList [nCurrent] = nChildSeg;
					gameData.render.mine.nSegDepth [nCurrent++] = l;
					VISIT (nChildSeg);
					}
				else {
					tPortal *oldPortal = renderPortals + nPos;
					bool bExpand = false;
					int h;
					h = newPortal->left - oldPortal->left;
					if (h > 0)
						newPortal->left = oldPortal->left;
					else if (h < 0)
						bExpand = true;
					h = newPortal->right - oldPortal->right;
					if (h < 0)
						newPortal->right = oldPortal->right;
					else if (h > 0)
						bExpand = true;
					h = newPortal->top - oldPortal->top;
					if (h > 0)
						newPortal->top = oldPortal->top;
					else if (h < 0)
						bExpand = true;
					h = newPortal->bot - oldPortal->bot;
					if (h < 0)
						newPortal->bot = oldPortal->bot;
					else if (h > 0)
						bExpand = true;
					if (bExpand) {
						if (nCurrent < gameData.segs.nSegments)
							segRenderList [nCurrent] = -0x7fff;
						*oldPortal = *newPortal;		//get updated tPortal
						gameData.render.mine.bProcessed [nPos] = gameData.render.mine.nProcessed - 1;		//force reprocess
#if 0
						if (!nStart || (nStart > nPos))
							nStart = nPos;
#endif
						}
					}
				}
			}
		}
	}

gameData.render.mine.lCntSave = nCurrent;
gameData.render.mine.sCntSave = nHead;
gameData.render.nFirstTerminalSeg = nHead;
gameData.render.mine.nRenderSegs = nCurrent;

for (i = 0; i < gameData.render.mine.nRenderSegs; i++) {
#if DBG
	if (segRenderList [i] == nDbgSeg)
		nDbgSeg = nDbgSeg;
#endif
	if (segRenderList [i] >= 0)
		gameData.render.mine.bVisible [segRenderList [i]] = gameData.render.mine.nVisible;
	}
}

//------------------------------------------------------------------------

void BuildRenderSegListFast (short nStartSeg, int nWindow)
{
	int	nSegment;

gameData.render.mine.nRenderSegs = 0;
for (nSegment = 0; nSegment < gameData.segs.nSegments; nSegment++) {
#if DBG
	if (nSegment == nDbgSeg)
		nDbgSeg = nDbgSeg;
#endif
	if (gameData.segs.SegVis (nStartSeg, nSegment)) {
		gameData.render.mine.bVisible [nSegment] = gameData.render.mine.nVisible;
		gameData.render.mine.nSegRenderList [0][gameData.render.mine.nRenderSegs++] = nSegment;
		RotateVertexList (8, SEGMENTS [nSegment].m_verts);
		}
	}
HUDMessage (0, "%d", gameData.render.mine.nRenderSegs);
}

//------------------------------------------------------------------------------
// eof
