#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#include "pstypes.h"
#include "mono.h"

#include "descent.h"
#include "error.h"
#include "text.h"
#include "segment.h"
#include "textures.h"
#include "wall.h"
#include "object.h"
#include "gamemine.h"
#include "gameseg.h"
#include "trigger.h"
#include "ogl_defs.h"
#include "oof.h"
#include "lightmap.h"
#include "rendermine.h"
#include "gameseg.h"

#include "game.h"
#include "menu.h"
#include "menu.h"
#include "createmesh.h"

#include "cfile.h"
#include "fuelcen.h"

#include "hash.h"
#include "key.h"
#include "piggy.h"

#include "byteswap.h"
#include "gamesave.h"
#include "u_mem.h"
#include "vecmat.h"
#include "gamepal.h"
#include "paging.h"
#include "maths.h"
#include "network.h"
#include "light.h"
#include "dynlight.h"
#include "renderlib.h"
#include "netmisc.h"
#include "createmesh.h"

using namespace Mesh;

void LoadFaceBitmaps (CSegment *segP, CSegFace *faceP);

//------------------------------------------------------------------------------

#define	MAX_EDGE_LEN(nMeshQuality)	fMaxEdgeLen [nMeshQuality]

#define MESH_DATA_VERSION 11

//------------------------------------------------------------------------------

typedef struct tMeshDataHeader {
	int	nVersion;
	int	nCheckSum;
	int	nSegments;
	int	nVertices;
	int	nFaceVerts;
	int	nFaces;
	int	nTris;
	} tMeshHeaderData;

//------------------------------------------------------------------------------

float fMaxEdgeLen [] = {1e30f, 30.9f, 20.9f, 19.9f, 9.9f};

CQuadMeshBuilder meshBuilder;

//------------------------------------------------------------------------------

void CTriMeshBuilder::FreeData (void)
{
m_triangles.Destroy ();
m_edges.Destroy ();
}

//------------------------------------------------------------------------------

int CTriMeshBuilder::AllocData (void)
{
if (m_nMaxTriangles && m_nMaxEdges) {
	if (!(m_edges.Resize (m_nMaxEdges * 2) && m_triangles.Resize (m_nMaxTriangles * 2))) {
		PrintLog ("      Not enough memory for building the triangle mesh (%d edges, %d tris).\n", m_nMaxEdges * 2, m_nMaxTriangles * 2);
		FreeData ();
		return 0;
		}
	memset (&m_edges [m_nMaxTriangles], 0xff, m_nMaxTriangles * sizeof (tEdge));
	memset (&m_triangles [m_nMaxEdges], 0xff, m_nMaxTriangles * sizeof (tTriangle));
	m_nMaxTriangles *= 2;
	m_nMaxEdges *= 2;
	}
else {
	m_nMaxTriangles = gameData.segs.nFaces * 4;
	m_nMaxEdges = gameData.segs.nFaces * 4;
	if (!m_edges.Create (m_nMaxEdges)) {
		PrintLog ("      Not enough memory for building the triangle mesh (%d edges).\n", m_nMaxEdges);
		return 0;
		}
	if (!m_triangles.Create (m_nMaxTriangles)) {
		PrintLog ("      Not enough memory for building the triangle mesh (%d tris).\n", m_nMaxTriangles);
		FreeData ();
		return 0;
		}
	m_edges.Clear (0xff);
	m_triangles.Clear (0xff);
	}
return 1;
}

//------------------------------------------------------------------------------

tEdge *CTriMeshBuilder::FindEdge (ushort nVert1, ushort nVert2, int i)
{
	tEdge	*edgeP = &m_edges [++i];

for (; i < m_nEdges; i++, edgeP++)
	if ((edgeP->verts [0] == nVert1) && (edgeP->verts [1] == nVert2))
		return edgeP;
return NULL;
}

//------------------------------------------------------------------------------

int CTriMeshBuilder::AddEdge (int nTri, ushort nVert1, ushort nVert2)
{
if (nVert2 < nVert1) {
	ushort h = nVert1;
	nVert1 = nVert2;
	nVert2 = h;
	}
#if DBG
if ((nTri < 0) || (nTri >= m_nTriangles))
	return -1;
#endif
tEdge *edgeP;
for (int i = -1;;) {
	if (!(edgeP = FindEdge (nVert1, nVert2, i)))
		break;
	i = int (edgeP - &m_edges [0]);
	if (edgeP->tris [0] < 0) {
		edgeP->tris [0] = nTri;
		return i;
		}
	if (edgeP->tris [1] < 0) {
		edgeP->tris [1] = nTri;
		return i;
		}
	}
if (m_nFreeEdges >= 0) {
	edgeP = &m_edges [m_nFreeEdges];
	m_nFreeEdges = edgeP->nNext;
	edgeP->nNext = -1;
	}
else {
	if ((m_nEdges == m_nMaxEdges - 1) && !AllocData ())
		return -1;
	edgeP = &m_edges [m_nEdges++];
	}
edgeP->tris [0] = nTri;
edgeP->verts [0] = nVert1;
edgeP->verts [1] = nVert2;
edgeP->fLength = CFloatVector::Dist(gameData.segs.fVertices [nVert1], gameData.segs.fVertices [nVert2]);
return m_edges.Index (edgeP);
}

//------------------------------------------------------------------------------

tTriangle *CTriMeshBuilder::CreateTriangle (tTriangle *triP, ushort index [], int nFace, int nIndex)
{
	int	h, i;

if (triP)
	triP->nIndex = nIndex;
else {
	if ((m_nTriangles == m_nMaxTriangles - 1) && !AllocData ())
		return NULL;
	triP = &m_triangles [m_nTriangles++];
	triP->nIndex = nIndex;
	if (nIndex >= 0) {
		i = TRIANGLES [nIndex].nIndex;
		memcpy (triP->texCoord, FACES.texCoord + i, sizeof (triP->texCoord));
		memcpy (triP->ovlTexCoord, FACES.ovlTexCoord + i, sizeof (triP->ovlTexCoord));
		memcpy (triP->color, FACES.color + i, sizeof (triP->color));
		}
	}
nIndex = m_triangles.Index (triP);
for (i = 0; i < 3; i++) {
	if (0 > (h = AddEdge (nIndex, index [i], index [(i + 1) % 3])))
		return NULL;
	triP = &m_triangles [nIndex];
	triP->lines [i] = h;
	}
triP->nFace = nFace;
memcpy (triP->index, index, sizeof (triP->index));
return triP;
}

//------------------------------------------------------------------------------

tTriangle *CTriMeshBuilder::AddTriangle (tTriangle *triP, ushort index [], tFaceTriangle *grsTriP)
{
return CreateTriangle (triP, index, grsTriP->nFace, grsTriP - TRIANGLES);
}

//------------------------------------------------------------------------------

void CTriMeshBuilder::DeleteEdge (tEdge *edgeP)
{
#if 1
edgeP->nNext = m_nFreeEdges;
m_nFreeEdges = m_edges.Index (edgeP);
#else
	tTriangle	*triP;
	int		h = edgeP - m_edges, i, j;

if (h < --m_nEdges) {
	*edgeP = m_edges [m_nEdges];
	for (i = 0; i < 2; i++) {
		if (edgeP->tris [i] >= 0) {
			triP = m_triangles + edgeP->tris [i];
			for (j = 0; j < 3; j++)
				if (triP->lines [j] == m_nEdges)
					triP->lines [j] = h;
			}
		}
	}
#endif
}

//------------------------------------------------------------------------------

void CTriMeshBuilder::DeleteTriangle (tTriangle *triP)
{
	tEdge	*edgeP;
	int	i, nTri = m_triangles.Index (triP);

for (i = 0; i < 3; i++) {
	edgeP = &m_edges [triP->lines [i]];
	if (edgeP->tris [0] == nTri)
		edgeP->tris [0] = edgeP->tris [1];
	edgeP->tris [1] = -1;
	if (edgeP->tris [0] < 0)
		DeleteEdge (edgeP);
	}
}

//------------------------------------------------------------------------------

int CTriMeshBuilder::CreateTriangles (void)
{
PrintLog ("   adding existing triangles\n");
m_nEdges = 0;
m_nTriangles = 0;
m_nMaxTriangles = 0;
m_nMaxTriangles = 0;
m_nFreeEdges = -1;
m_nVertices = gameData.segs.nVertices;
if (!AllocData ())
	return 0;

CSegFace *faceP;
tFaceTriangle *grsTriP;
tTriangle *triP;
int i, nFace = -1;
short nId = 0;

#if 0
if (gameStates.render.nMeshQuality) {
	i = LEVEL_VERTICES + ((gameData.segs.nTris ? gameData.segs.nTris / 2 : gameData.segs.nFaces) << (abs (gameStates.render.nMeshQuality)));
	if (!(gameData.segs.fVertices.Resize (i) && gameData.segs.vertices.Resize (i) && gameData.segs.points.Resize (i)))
		return 0;
	}
#endif
for (i = gameData.segs.nTris, grsTriP = TRIANGLES.Buffer (); i; i--, grsTriP++) {
	if (!(triP = AddTriangle (NULL, grsTriP->index, grsTriP))) {
		FreeData ();
		return 0;
		}
	if (nFace == grsTriP->nFace)
		nId++;
	else {
		nFace = grsTriP->nFace;
		nId = 0;
		}
	triP->nId = nId;
	faceP = FACES.faces + grsTriP->nFace;
#if DBG
	if ((faceP->m_info.nSegment == nDbgSeg) && ((nDbgSide < 0) || (faceP->m_info.nSide == nDbgSide)))
		nDbgSeg = nDbgSeg;
#endif
	if (faceP->m_info.bSlide || (faceP->m_info.nCamera >= 0))
		triP->nPass = -2;
	}
return m_nTris = m_nTriangles;
}

//------------------------------------------------------------------------------

int CTriMeshBuilder::SplitTriangleByEdge (int nTri, ushort nVert1, ushort nVert2, short nPass)
{
if (nTri < 0)
	return 1;

tTriangle *triP = &m_triangles [nTri];

if (triP->nPass < -1)
	return 1;

CSegFace *faceP = FACES.faces + triP->nFace;

#if DBG
if ((faceP->m_info.nSegment == nDbgSeg) && ((nDbgSide < 0) || (faceP->m_info.nSide == nDbgSide)))
	nDbgSeg = nDbgSeg;
#endif

	int			h, i, nIndex = triP->nIndex;
	ushort		nFace = triP->nFace, *indexP = triP->index, index [4];
	tTexCoord2f	texCoord [4], ovlTexCoord [4];
	tRgbaColorf	color [4];

// find insertion point for the new vertex
for (i = 0; i < 3; i++) {
	h = indexP [i];
	if ((h == nVert1) || (h == nVert2))
		break;
	}
if (i == 3) {
	PrintLog ("      Internal error during construction of the triangle mesh.\n");
	return 0;
	}

h = indexP [(i + 1) % 3]; //check next vertex index
if ((h == nVert1) || (h == nVert2))
	h = (i + 1) % 3; //insert before i+1
else
	h = i; //insert before i

// build new quad index containing the new vertex
// the two triangle indices will be derived from it (indices 0,1,2 and 1,2,3)
index [0] = gameData.segs.nVertices - 1;
for (i = 1; i < 4; i++) {
	index [i] = indexP [h];
	texCoord [i] = triP->texCoord [h];
	ovlTexCoord [i] = triP->ovlTexCoord [h];
	color [i] = triP->color [h++];
	h %= 3;
	}

// interpolate texture coordinates and color for the new vertex
texCoord [0].v.v = (texCoord [1].v.v + texCoord [3].v.v) / 2;
texCoord [0].v.u = (texCoord [1].v.u + texCoord [3].v.u) / 2;
ovlTexCoord [0].v.v = (ovlTexCoord [1].v.v + ovlTexCoord [3].v.v) / 2;
ovlTexCoord [0].v.u = (ovlTexCoord [1].v.u + ovlTexCoord [3].v.u) / 2;
color [0].red = (color [1].red + color [3].red) / 2;
color [0].green = (color [1].green + color [3].green) / 2;
color [0].blue = (color [1].blue + color [3].blue) / 2;
color [0].alpha = (color [1].alpha + color [3].alpha) / 2;
DeleteTriangle (triP); //remove any references to this triangle
if (!(triP = CreateTriangle (triP, index, nFace, nIndex))) //create a new triangle at this location (insert)
	return 0;
triP->nPass = nPass;
triP->nId = (faceP->m_info.nTris)++;
memcpy (triP->color, color, sizeof (triP->color));
memcpy (triP->texCoord, texCoord, sizeof (triP->texCoord));
memcpy (triP->ovlTexCoord, ovlTexCoord, sizeof (triP->ovlTexCoord));

index [1] = index [0];
if (!(triP = CreateTriangle (NULL, index + 1, nFace, -1))) //create a new triangle (append)
	return 0;
triP->nPass = nPass;
triP->nId = (faceP->m_info.nTris)++;
triP->texCoord [0] = texCoord [0];
triP->ovlTexCoord [0] = ovlTexCoord [0];
triP->color [0] = color [0];
memcpy (triP->color + 1, color + 2, 2 * sizeof (triP->color [0]));
memcpy (triP->texCoord + 1, texCoord + 2, 2 * sizeof (triP->texCoord [0]));
memcpy (triP->ovlTexCoord + 1, ovlTexCoord + 2, 2 * sizeof (triP->ovlTexCoord [0]));
FACES.faces [triP->nFace].m_info.nVerts++;
return 1;
}

//------------------------------------------------------------------------------

float CTriMeshBuilder::NewEdgeLen (int nTri, int nVert1, int nVert2)
{
	tTriangle	*triP = &m_triangles [nTri];
	int			h, i;

for (i = 0; i < 3; i++) {
	h = triP->index [i];
	if ((h != nVert1) && (h != nVert2))
		break;
	}
return CFloatVector::Dist(gameData.segs.fVertices [h], gameData.segs.fVertices [gameData.segs.nVertices]);
}

//------------------------------------------------------------------------------

int CTriMeshBuilder::SplitEdge (tEdge *edgeP, short nPass)
{
	int		tris [2];
	ushort	verts [2];

memcpy (tris, edgeP->tris, sizeof (tris));
memcpy (verts, edgeP->verts, sizeof (verts));
int i = gameData.segs.fVertices.Length ();
if (gameData.segs.nVertices >= i) {
	i *= 2;
	if (!(gameData.segs.fVertices.Resize (i) && gameData.segs.vertices.Resize (i) && gameData.segs.points.Resize (i)))
		return 0;
	}
gameData.segs.fVertices [gameData.segs.nVertices] = 
	CFloatVector::Avg (gameData.segs.fVertices [verts [0]], gameData.segs.fVertices [verts [1]]);
gameData.segs.vertices [gameData.segs.nVertices].Assign (gameData.segs.fVertices [gameData.segs.nVertices]);
#if 0
if (tris [1] >= 0) {
	if (NewEdgeLen (tris [0], verts [0], verts [1]) + NewEdgeLen (tris [1], verts [0], verts [1]) < MAX_EDGE_LEN (m_nQuality))
		return -1;
	}
#endif
gameData.segs.nVertices++;
if (!SplitTriangleByEdge (tris [0], verts [0], verts [1], nPass))
	return 0;
if (!SplitTriangleByEdge (tris [1], verts [0], verts [1], nPass))
	return 0;
return 1;
}

//------------------------------------------------------------------------------

int CTriMeshBuilder::SplitTriangle (tTriangle *triP, short nPass)
{
	int	h = 0, i;
	float	l, lMax = 0;

for (i = 0; i < 3; i++) {
	l = m_edges [triP->lines [i]].fLength;
	if (lMax < l) {
		lMax = l;
		h = i;
		}
	}
if (lMax <= MAX_EDGE_LEN (m_nQuality))
	return -1;
return SplitEdge (&m_edges [triP->lines [h]], nPass);
}

//------------------------------------------------------------------------------

int CTriMeshBuilder::SplitTriangles (void)
{
	int	bSplit = 0, h, i, j, nSplitRes;
	short	nPass = 0, nMaxPasses = 10 * m_nQuality;

h = 0;
do {
	bSplit = 0;
	j = m_nTriangles;
	PrintLog ("   splitting triangles (pass %d)\n", nPass);
	for (i = h, h = 0; i < j; i++) {
		if (m_triangles [i].nPass != nPass - 1)
			continue;
#if DBG
		CSegFace *faceP = FACES.faces + m_triangles [i].nFace;
		if ((faceP->m_info.nSegment == nDbgSeg) && ((nDbgSide < 0) || (faceP->m_info.nSide == nDbgSide)))
			nDbgSeg = nDbgSeg;
#endif
		nSplitRes = SplitTriangle (&m_triangles [i], nPass);
		if (gameData.segs.nVertices == 65536) {
			PrintLog ("      Level too big for requested triangle mesh quality.\n");
			return 0;
			}
		if (!nSplitRes)
			return 0;
		if (nSplitRes < 0)
			m_triangles [i].nPass = -2;
		else {
			bSplit = 1;
			if (!h)
				h = i;
			}
		}
	nPass++;
	} while (bSplit && (nPass < nMaxPasses));
return 1;
}

//------------------------------------------------------------------------------

void CTriMeshBuilder::QSortTriangles (int left, int right)
{
	int	l = left,
			r = right,
			m = m_triangles [(l + r) / 2].nFace;
	short i = m_triangles [(l + r) / 2].nId;

do {
	while ((m_triangles [l].nFace < m) || ((m_triangles [l].nFace == m) && (m_triangles [l].nId < i)))
		l++;
	while ((m_triangles [r].nFace > m) || ((m_triangles [r].nFace == m) && (m_triangles [r].nId > i)))
		r--;
	if (l <= r) {
		if (l < r)
			::Swap (m_triangles [l], m_triangles [r]);
		l++;
		r--;
		}
	} while (l <= r);
if (l < right)
	QSortTriangles (l, right);
if (left < r)
	QSortTriangles (left, r);
}

//------------------------------------------------------------------------------

void CTriMeshBuilder::SetupVertexNormals (void)
{
	tFaceTriangle*	triP;
	g3sPoint*		pointP;
	int				h, i, nVertex;

for (i = gameData.segs.nVertices, pointP = gameData.segs.points.Buffer (); i; i--, pointP++) {
/*
	(*pointP->p3_normal.vNormal.XYZ ()) [X] =
	(*pointP->p3_normal.vNormal.XYZ ()) [Y] =
	(*pointP->p3_normal.vNormal.XYZ ()) [Z] = 0;
*/
	pointP->p3_normal.vNormal.XYZ ()->SetZero ();

	pointP->p3_normal.nFaces = 0;
	}
for (h = 0, triP = FACES.tris.Buffer (); h < gameData.segs.nTris; h++, triP++) {
	for (i = 0; i < 3; i++) {
		nVertex = triP->index [i];
#if DBG
		if (nVertex == nDbgVertex)
			nVertex = nVertex;
#endif
		*gameData.segs.points [nVertex].p3_normal.vNormal.XYZ () += FACES.normals [3 * h];
		gameData.segs.points [nVertex].p3_normal.nFaces++;
		}
	}
ComputeVertexNormals ();
}

//------------------------------------------------------------------------------

int CTriMeshBuilder::InsertTriangles (void)
{
	tTriangle*		triP = &m_triangles [0];
	tFaceTriangle*	grsTriP = TRIANGLES.Buffer ();
	CSegFace*		m_faceP = NULL;
	CFixVector		vNormal;
	int				h, i, nFace = -1;
	GLuint			nIndex = 0;

PrintLog ("   inserting new triangles\n");
QSortTriangles (0, m_nTriangles - 1);
ResetVertexNormals ();
for (h = 0; h < m_nTriangles; h++, triP++, grsTriP++) {
	grsTriP->nFace = triP->nFace;
	if (grsTriP->nFace == nFace)
		m_faceP->m_info.nTris++;
	else {
		if (m_faceP)
			m_faceP++;
		else
			m_faceP = FACES.faces.Buffer ();
		nFace = grsTriP->nFace;
#if DBG
		if (m_faceP - FACES.faces != nFace)
			return 0;
#endif
		m_faceP->m_info.nFrame = -1;
		m_faceP->m_info.nIndex = nIndex;
		m_faceP->m_info.nTriIndex = h;
		m_faceP->m_info.nTris = 1;
#if USE_RANGE_ELEMENTS
		m_faceP->m_info.vertIndex = FACES.vertIndex + nIndex;
#endif
		}
	grsTriP->nIndex = nIndex;
	memcpy (grsTriP->index, triP->index, sizeof (triP->index));
	for (i = 0; i < 3; i++)
		FACES.vertices [nIndex + i].Assign (gameData.segs.fVertices [triP->index [i]]);
	FACES.normals [nIndex] = CFloatVector3::Normal (FACES.vertices [nIndex], FACES.vertices [nIndex + 1], FACES.vertices [nIndex + 2]);
#if DBG
	if (FACES.normals [nIndex].Mag () == 0)
		m_faceP = m_faceP;
#endif
	vNormal.Assign (FACES.normals [nIndex]);
	for (i = 1; i < 3; i++)
		FACES.normals [nIndex + i] = FACES.normals [nIndex];
	memcpy (FACES.texCoord + nIndex, triP->texCoord, sizeof (triP->texCoord));
	memcpy (FACES.ovlTexCoord + nIndex, triP->ovlTexCoord, sizeof (triP->ovlTexCoord));
	memcpy (FACES.color + nIndex, triP->color, sizeof (triP->color));
#if USE_RANGE_ELEMENTS
	for (i = 0; i < 3; i++, nIndex++)
		FACES.vertIndex [nIndex] = nIndex;
#else
	nIndex += 3;
#endif
	}
gameData.segs.nTris = m_nTriangles;
SetupVertexNormals ();
FreeData ();
PrintLog ("   created %d new triangles and %d new vertices\n",
			 m_nTriangles - m_nTris, gameData.segs.nVertices - m_nVertices);
CreateFaceVertLists ();
return 1;
}

//------------------------------------------------------------------------------

void CTriMeshBuilder::SortFaceVertList (ushort *vertList, int left, int right)
{
	int	l = left,
			r = right,
			m = vertList [(l + r) / 2];

do {
	while (vertList [l] < m)
		l++;
	while (vertList [r] > m)
		r--;
	if (l <= r) {
		if (l < r) {
			int h = vertList [l];
			vertList [l] = vertList [r];
			vertList [r] = h;
			}
		l++;
		r--;
		}
	} while (l <= r);
if (l < right)
	SortFaceVertList (vertList, l, right);
if (left < r)
	SortFaceVertList (vertList, left, r);
}

//------------------------------------------------------------------------------

void CTriMeshBuilder::CreateSegFaceList (void)
{
	int			h = 0;
	tSegFaces*	segFaceP = SEGFACES.Buffer ();

for (int i = gameData.segs.nSegments; i; i--, segFaceP++) {
	segFaceP->faceP = &FACES.faces [h];
	h += segFaceP->nFaces;
	}
}

//------------------------------------------------------------------------------

void CTriMeshBuilder::CreateFaceVertLists (void)
{
	int*				bTags = new int [gameData.segs.nVertices];
	CSegFace*		faceP;
	tFaceTriangle*	triP;
	int				h, i, j, k, nFace;

//count the vertices of each face
memset (bTags, 0xFF, gameData.segs.nVertices * sizeof (bTags [0]));
gameData.segs.nFaceVerts = 0;
for (i = gameData.segs.nFaces, faceP = FACES.faces.Buffer (), nFace = 0; i; i--, faceP++, nFace++) {
	faceP->m_info.nVerts = 0;
	for (j = faceP->m_info.nTris, triP = TRIANGLES + faceP->m_info.nTriIndex; j; j--, triP++) {
		for (k = 0; k < 3; k++) {
			h = triP->index [k];
			if (bTags [h] != nFace) {
				bTags [h] = nFace;
				faceP->m_info.nVerts++;
				gameData.segs.nFaceVerts++;
				}
			}
		}
	}
//insert each face's vertices' indices in the vertex index buffer
memset (bTags, 0xFF, gameData.segs.nVertices * sizeof (bTags [0]));
gameData.segs.nFaceVerts = 0;
for (i = gameData.segs.nFaces, faceP = FACES.faces.Buffer (), nFace = 0; i; i--, faceP++, nFace++) {
	faceP->triIndex = FACES.faceVerts + gameData.segs.nFaceVerts;
#if DBG
	if (faceP->m_info.nSegment == nDbgSeg)
		nDbgSeg = nDbgSeg;
#endif
	for (j = faceP->m_info.nTris, triP = TRIANGLES + faceP->m_info.nTriIndex; j; j--, triP++) {
		for (k = 0; k < 3; k++) {
			h = triP->index [k];
			if (bTags [h] != nFace) {
				bTags [h] = nFace;
				FACES.faceVerts [gameData.segs.nFaceVerts++] = h;
				}
			}
		}
	}
#if 1
//sort each face's vertex index list
for (i = gameData.segs.nFaces, faceP = FACES.faces.Buffer (); i; i--, faceP++)
	SortFaceVertList (faceP->triIndex, 0, faceP->m_info.nVerts - 1);
#endif
}

//------------------------------------------------------------------------------

char *CTriMeshBuilder::DataFilename (char *pszFilename, int nLevel)
{
return GameDataFilename (pszFilename, "mesh", nLevel,
								 (gameStates.render.bTriangleMesh < 0) ? -1 : m_nQuality);
}

//------------------------------------------------------------------------------

bool CTriMeshBuilder::Load (int nLevel, bool bForce)
{
	CFile					cf;
	tMeshDataHeader	mdh;
	int					nSize;
	bool					bOk;
	char					szFilename [FILENAME_LEN];
	char					*ioBuffer = NULL, *bufP;

if (!(gameStates.render.bTriangleMesh && (gameStates.app.bCacheMeshes || bForce)))
	return false;
if (!cf.Open (DataFilename (szFilename, nLevel), gameFolders.szCacheDir, "rb", 0))
	return false;
bOk = (cf.Read (&mdh, sizeof (mdh), 1) == 1);
if (bOk)
	bOk = (mdh.nVersion == MESH_DATA_VERSION) &&
			(mdh.nCheckSum == CalcSegmentCheckSum ()) &&
			(mdh.nSegments == gameData.segs.nSegments) &&
			(mdh.nFaces == gameData.segs.nFaces);
	uint	nTriVerts = mdh.nTris * 3;
if (bOk)
	nSize =
		(sizeof (gameData.segs.vertices [0]) + sizeof (gameData.segs.fVertices [0])) * mdh.nVertices +
		 sizeof (CSegFaceInfo) * mdh.nFaces +
		 sizeof (FACES.tris [0]) * mdh.nTris +
		(sizeof (FACES.vertices [0]) + sizeof (FACES.normals [0]) + sizeof (FACES.texCoord [0]) + 
		 sizeof (FACES.ovlTexCoord [0]) + sizeof (FACES.color [0]) + sizeof (FACES.lMapTexCoord [0])) * nTriVerts +
		 sizeof (FACES.faceVerts [0]) * mdh.nFaceVerts;
if (bOk)
	bOk = ((ioBuffer = new char [nSize]) != NULL);
if (bOk)
	bOk = cf.Read (ioBuffer, nSize, 1) == 1;
if (bOk) {
	bufP = ioBuffer;
	FACES.Destroy ();
	gameData.segs.vertices.Create (mdh.nVertices);
	memcpy (gameData.segs.vertices.Buffer (), bufP, nSize = sizeof (gameData.segs.vertices [0]) * mdh.nVertices);
	bufP += nSize;
	gameData.segs.fVertices.Create (mdh.nVertices);
	memcpy (gameData.segs.fVertices.Buffer (), bufP, nSize = sizeof (gameData.segs.fVertices [0]) * mdh.nVertices);
	bufP += nSize;
	FACES.faces.Create (mdh.nFaces);
	FACES.faces.Clear ();
	for (int i = 0; i < mdh.nFaces; i++, bufP += sizeof (CSegFaceInfo))
		memcpy (&FACES.faces [i].m_info, bufP, sizeof (CSegFaceInfo));
	FACES.tris.Create (mdh.nTris);
	memcpy (FACES.tris.Buffer (), bufP, nSize = sizeof (FACES.tris [0]) * mdh.nTris);
	bufP += nSize;
	FACES.vertices.Create (nTriVerts);
	memcpy (FACES.vertices.Buffer (), bufP, nSize = sizeof (FACES.vertices [0]) * nTriVerts);
	bufP += nSize;
	FACES.normals.Create (nTriVerts);
	memcpy (FACES.normals.Buffer (), bufP, nSize = sizeof (FACES.normals [0]) * nTriVerts);
	bufP += nSize;
	FACES.texCoord.Create (nTriVerts);
	memcpy (FACES.texCoord.Buffer (), bufP, nSize = sizeof (FACES.texCoord [0]) * nTriVerts);
	bufP += nSize;
	FACES.ovlTexCoord.Create (nTriVerts);
	memcpy (FACES.ovlTexCoord.Buffer (), bufP, nSize = sizeof (FACES.ovlTexCoord [0]) * nTriVerts);
	bufP += nSize;
	FACES.color.Create (nTriVerts);
	memcpy (FACES.color.Buffer (), bufP, nSize = sizeof (FACES.color [0]) * nTriVerts);
	bufP += nSize;
	FACES.lMapTexCoord.Create (nTriVerts);
	memcpy (FACES.lMapTexCoord.Buffer (), bufP, nSize = sizeof (FACES.lMapTexCoord [0]) * nTriVerts);
	bufP += nSize;
	FACES.faceVerts.Create (mdh.nFaceVerts);
	memcpy (FACES.faceVerts.Buffer (), bufP, nSize = sizeof (FACES.faceVerts [0]) * mdh.nFaceVerts);
	gameData.segs.points.Resize (mdh.nVertices);
	}
if (ioBuffer) {
	delete[] ioBuffer;
	ioBuffer = NULL;
	}
if (bOk) {
	gameData.segs.nVertices = mdh.nVertices;
	gameData.segs.nTris = mdh.nTris;
	SetupVertexNormals ();
	}
cf.Close ();
if (!gameStates.app.bCacheMeshes)
	cf.Delete (DataFilename (szFilename, nLevel), gameFolders.szCacheDir);
CreateSegFaceList ();
CreateFaceVertLists ();
return bOk;
}

//------------------------------------------------------------------------------

bool CTriMeshBuilder::Save (int nLevel)
{
	tMeshDataHeader mdh = {MESH_DATA_VERSION,
								  CalcSegmentCheckSum (),
								  gameData.segs.nSegments,
								  gameData.segs.nVertices,
								  gameData.segs.nFaceVerts,
								  gameData.segs.nFaces,
								  gameData.segs.nTris};

	CFile					cf;
	bool					bOk;
	char					szFilename [FILENAME_LEN];
	uint					nTriVerts = uint (mdh.nTris * 3);

if (!(gameStates.render.bTriangleMesh /*&& gameStates.app.bCacheMeshes*/))
	return 0;
if (!cf.Open (DataFilename (szFilename, nLevel), gameFolders.szCacheDir, "wb", 0))
	return 0;
bOk = (cf.Write (&mdh, sizeof (mdh), 1) == 1) &&
		(gameData.segs.vertices.Write (cf, mdh.nVertices) == uint (mdh.nVertices)) &&
		(gameData.segs.fVertices.Write (cf, mdh.nVertices) == uint (mdh.nVertices));
if (bOk) {
	for (int i = 0; i < mdh.nFaces; i++) {
		if (cf.Write (&FACES.faces [i].m_info, sizeof (CSegFaceInfo), 1) != 1) {
			bOk = false;
			break;
			}
		}
	}
if (bOk)
	bOk = (FACES.tris.Write (cf, mdh.nTris) == uint (mdh.nTris)) &&
			(FACES.vertices.Write (cf, nTriVerts) == nTriVerts) &&
			(FACES.normals.Write (cf, nTriVerts) == nTriVerts) &&
			(FACES.texCoord.Write (cf, nTriVerts) == nTriVerts) &&
			(FACES.ovlTexCoord.Write (cf, nTriVerts) == nTriVerts) &&
			(FACES.color.Write (cf, nTriVerts) == nTriVerts) &&
			(FACES.lMapTexCoord.Write (cf, nTriVerts) == nTriVerts) &&
			(FACES.faceVerts.Write (cf, mdh.nFaceVerts) == uint (mdh.nFaceVerts));
cf.Close ();
return bOk;
}

//------------------------------------------------------------------------------

int CTriMeshBuilder::Build (int nLevel, int nQuality)
{
PrintLog ("creating triangle mesh\n");
m_nQuality = nQuality;
if (Load (nLevel))
	return 1;
if (!CreateTriangles ()) {
	gameData.segs.nVertices = m_nVertices;
	return 0;
	}
if ((gameStates.render.bTriangleMesh > 0) && !SplitTriangles ()) {
	gameData.segs.nVertices = m_nVertices;
	return 0;
	}
if (!InsertTriangles ()) {
	gameData.segs.nVertices = m_nVertices;
	return 0;
	}
Save (nLevel);
return Load (nLevel, true); //Load will rebuild all face data buffers, reducing their memory footprint
}

//------------------------------------------------------------------------------

int CQuadMeshBuilder::IsBigFace (ushort *sideVerts)
{
for (int i = 0; i < 4; i++)
	if (CFloatVector::Dist (gameData.segs.fVertices [sideVerts [i]], gameData.segs.fVertices [sideVerts [(i + 1) % 4]]) > MAX_EDGE_LEN (gameStates.render.nMeshQuality))
		return 1;
return 0;
}

//------------------------------------------------------------------------------

CFloatVector3 *CQuadMeshBuilder::SetTriNormals (tFaceTriangle *triP, CFloatVector3 *m_normalP)
{
	CFloatVector	vNormalf;

vNormalf = CFloatVector::Normal (gameData.segs.fVertices [triP->index [0]],
				 gameData.segs.fVertices [triP->index [1]], gameData.segs.fVertices [triP->index [2]]);
*m_normalP++ = *vNormalf.XYZ ();
*m_normalP++ = *vNormalf.XYZ ();
*m_normalP++ = *vNormalf.XYZ ();
return m_normalP;
}

//------------------------------------------------------------------------------

void CQuadMeshBuilder::InitFace (short nSegment, ubyte nSide, bool bRebuild)
{
if (bRebuild)
	m_faceP->m_info.nTris = 0;
else
	memset (m_faceP, 0, sizeof (*m_faceP));
m_faceP->m_info.nSegment = nSegment;
m_faceP->m_info.nVerts = 4;
m_faceP->m_info.nFrame = -1;
m_faceP->m_info.nIndex = m_vertexP - FACES.vertices;
if (gameStates.render.bTriangleMesh)
	m_faceP->m_info.nTriIndex = m_triP - TRIANGLES;
memcpy (m_faceP->m_info.index, m_sideVerts, sizeof (m_faceP->m_info.index));
m_faceP->m_info.nType = gameStates.render.bTriangleMesh ? m_sideP->m_nType : -1;
m_faceP->m_info.nSegment = nSegment;
m_faceP->m_info.nSide = nSide;
m_faceP->m_info.nWall = gameStates.app.bD2XLevel ? m_nWall : IS_WALL (m_nWall) ? m_nWall : (ushort) -1;
m_faceP->m_info.bAnimation = IsAnimatedTexture (m_faceP->m_info.nBaseTex) || IsAnimatedTexture (m_faceP->m_info.nOvlTex);
m_faceP->m_info.bHasColor = 0;
m_faceP->m_info.fRads [0] = X2F (SEGMENTS [nSegment].Side (nSide)->m_rads [0]); //(float) sqrt ((rMinf * rMinf + rMaxf * rMaxf) / 2);
m_faceP->m_info.fRads [1] = X2F (SEGMENTS [nSegment].Side (nSide)->m_rads [1]); //(float) sqrt ((rMinf * rMinf + rMaxf * rMaxf) / 2);
}

//------------------------------------------------------------------------------

void CQuadMeshBuilder::InitTexturedFace (void)
{
	static char szEmpty [] = "";
	
m_faceP->m_info.nBaseTex = m_sideP->m_nBaseTex;
if ((m_faceP->m_info.nOvlTex = m_sideP->m_nOvlTex))
	m_nOvlTexCount++;
m_faceP->m_info.bSlide = (gameData.pig.tex.tMapInfoP [m_faceP->m_info.nBaseTex].slide_u || gameData.pig.tex.tMapInfoP [m_faceP->m_info.nBaseTex].slide_v);
m_faceP->m_info.nCamera = IsMonitorFace (m_faceP->m_info.nSegment, m_faceP->m_info.nSide, 1);
m_faceP->m_info.bIsLight = IsLight (m_faceP->m_info.nBaseTex) || (m_faceP->m_info.nOvlTex && IsLight (m_faceP->m_info.nOvlTex));
m_faceP->m_info.nOvlOrient = (ubyte) m_sideP->m_nOvlOrient;
m_faceP->m_info.bTextured = 1;
m_faceP->m_info.bTransparent = 0;
int nTexture = m_faceP->m_info.nOvlTex ? m_faceP->m_info.nOvlTex : m_faceP->m_info.nBaseTex;
char *pszName = (nTexture < MAX_WALL_TEXTURES)
					 ? gameData.pig.tex.bitmapFiles [gameStates.app.bD1Mission][gameData.pig.tex.bmIndexP [nTexture].index].name
					 : szEmpty;
if (strstr (pszName, "misc17") != NULL)
	m_faceP->m_info.bSparks = (nTexture == m_faceP->m_info.nBaseTex) ? 1 : 2;
if (m_nWallType < 2)
	m_faceP->m_info.bAdditive = 0;
else if (WALLS [m_nWall].flags & WALL_RENDER_ADDITIVE)
	m_faceP->m_info.bAdditive = 2;
else if (strstr (pszName, "lava"))
	m_faceP->m_info.bAdditive = 2;
else
	m_faceP->m_info.bAdditive = (strstr (pszName, "force") || m_faceP->m_info.bSparks) ? 1 : 0;
}

//------------------------------------------------------------------------------

void CQuadMeshBuilder::InitColoredFace (short nSegment)
{
m_faceP->m_info.nBaseTex = -1;
m_faceP->m_info.bTransparent = 1;
m_faceP->m_info.bSegColor = 1;
m_faceP->m_info.bAdditive = SEGMENTS [nSegment].m_nType >= SEGMENT_IS_LAVA;
}

//------------------------------------------------------------------------------

void CQuadMeshBuilder::SetupLMapTexCoord (tTexCoord2f *texCoordP)
{
#define	LMAP_SIZE	0.0f //(1.0f / 16.0f)
#if 0
	static tTexCoord2f lMapTexCoord [4] = {
	 {{LMAP_SIZE, LMAP_SIZE}},
	 {{1.0f - LMAP_SIZE, LMAP_SIZE}},
	 {{1.0f - LMAP_SIZE, 1.0f - LMAP_SIZE}},
	 {{LMAP_SIZE, 1.0f - LMAP_SIZE}}
	};
#endif
int i = m_faceP->m_info.nLightmap % LIGHTMAP_BUFSIZE;
float x = (float) (i % LIGHTMAP_ROWSIZE);
float y = (float) (i / LIGHTMAP_ROWSIZE);
texCoordP [0].v.u = x / (float) LIGHTMAP_ROWSIZE + 1.0f / (float) (LIGHTMAP_ROWSIZE * LIGHTMAP_WIDTH * 2);
texCoordP [0].v.v = y / (float) LIGHTMAP_ROWSIZE + 1.0f / (float) (LIGHTMAP_ROWSIZE * LIGHTMAP_WIDTH * 2);
texCoordP [1].v.u =
texCoordP [2].v.u = (x + 1) / (float) LIGHTMAP_ROWSIZE - 1.0f / (float) (LIGHTMAP_ROWSIZE * LIGHTMAP_WIDTH * 2);
texCoordP [1].v.v = y / (float) LIGHTMAP_ROWSIZE + 1.0f / (float) (LIGHTMAP_ROWSIZE * LIGHTMAP_WIDTH * 2);
texCoordP [2].v.v = (y + 1) / (float) LIGHTMAP_ROWSIZE - 1.0f / (float) (LIGHTMAP_ROWSIZE * LIGHTMAP_WIDTH * 2);
texCoordP [3].v.u = x / (float) LIGHTMAP_ROWSIZE + 1.0f / (float) (LIGHTMAP_ROWSIZE * LIGHTMAP_WIDTH * 2);
texCoordP [3].v.v = (y + 1) / (float) LIGHTMAP_ROWSIZE - 1.0f / (float) (LIGHTMAP_ROWSIZE * LIGHTMAP_WIDTH * 2);
}

//------------------------------------------------------------------------------

void CQuadMeshBuilder::SetupFace (void)
{
	int				i, j;
	CFixVector		vNormal;
	CFloatVector3	vNormalf;

vNormal = m_sideP->m_normals [0] + m_sideP->m_normals [1];
vNormal *= I2X (1) / 2;
vNormalf.Assign (vNormal);
for (i = 0; i < 4; i++) {
	j = m_sideVerts [i];
	*m_vertexP++ = *gameData.segs.fVertices [j].XYZ ();
	*m_normalP++ = vNormalf;
	m_texCoordP->v.u = X2F (m_sideP->m_uvls [i].u);
	m_texCoordP->v.v = X2F (m_sideP->m_uvls [i].v);
	RotateTexCoord2f (*m_ovlTexCoordP, *m_texCoordP, (ubyte) m_sideP->m_nOvlOrient);
#if DBG
	if (m_texCoordP > FACES.texCoord.Buffer () + FACES.texCoord.Length ())
		return;
	if (m_ovlTexCoordP > FACES.ovlTexCoord.Buffer () + FACES.ovlTexCoord.Length ())
		return;
#endif
	m_texCoordP++;
	m_ovlTexCoordP++;
	if (!gameStates.app.bNostalgia)
		*m_faceColorP = gameData.render.color.ambient [j].color;
	else {
		m_faceColorP->red = 
		m_faceColorP->green = 
		m_faceColorP->blue = X2F (m_sideP->m_uvls [i].l);
		m_faceColorP->alpha = 1;
		}
	m_faceColorP++;
	}
SetupLMapTexCoord (m_lMapTexCoordP);
m_lMapTexCoordP += 4;
}

//------------------------------------------------------------------------------

void CQuadMeshBuilder::SplitIn2Tris (void)
{
	static short	n2TriVerts [2][2][3] = {{{0,1,2},{0,2,3}},{{0,1,3},{1,2,3}}};

	int			h, i, j, k, v;
	short			*triVertP;
	tTexCoord2f	lMapTexCoord [4];

#if DBG
if ((m_faceP->m_info.nSegment == nDbgSeg) && ((nDbgSide < 0) || (m_faceP->m_info.nSide == nDbgSide)))
	nDbgSeg = nDbgSeg;
#endif
SetupLMapTexCoord (lMapTexCoord);
h = (m_sideP->m_nType == SIDE_IS_TRI_13);
for (i = 0; i < 2; i++, m_triP++) {
	gameData.segs.nTris++;
	m_faceP->m_info.nTris++;
	m_triP->nFace = m_faceP - FACES.faces;
	m_triP->nIndex = m_vertexP - FACES.vertices;
	triVertP = n2TriVerts [h][i];
	for (j = 0; j < 3; j++) {
		k = triVertP [j];
		v = m_sideVerts [k];
		m_triP->index [j] = v;
		*m_vertexP++ = *gameData.segs.fVertices [v].XYZ ();
		m_texCoordP->v.u = X2F (m_sideP->m_uvls [k].u);
		m_texCoordP->v.v = X2F (m_sideP->m_uvls [k].v);
		RotateTexCoord2f (*m_ovlTexCoordP, *m_texCoordP, (ubyte) m_sideP->m_nOvlOrient);
		*m_lMapTexCoordP = lMapTexCoord [k];
		m_texCoordP++;
		m_ovlTexCoordP++;
		m_lMapTexCoordP++;
		m_colorP = gameData.render.color.ambient + v;
		*m_faceColorP++ = m_colorP->color;
		}
	m_normalP = SetTriNormals (m_triP, m_normalP);
	}
}

//------------------------------------------------------------------------------

int CQuadMeshBuilder::CompareFaceKeys (const CSegFace** pf, const CSegFace** pm)
{
#if 1
int i = (*pf)->m_info.nKey;
int m = (*pm)->m_info.nKey;
#else
short i = (*pf)->m_info.nBaseTex;
short m = (*pm)->m_info.nBaseTex;
if (i < m) 
	return -1;
if (i > m)
	return 1;
i = (*pf)->m_info.nOvlTex;
m = (*pm)->m_info.nOvlTex;
#endif
return (i < m) ? -1 : (i > m) ? 1 : 0;
}

//------------------------------------------------------------------------------
// Create a linearized set of keys for all faces, where each unique combination of 
// a face's base and overlay texture has a unique key.
// The renderer will use these to quickly access all faces bearing the same textures.

void CQuadMeshBuilder::ComputeFaceKeys (void)
{
	CSegFace	*faceP = FACES.faces.Buffer ();
	CArray<CSegFace*>	keyFaceRef;

keyFaceRef.Create (gameData.segs.nFaces);
for (int i = 0; i < gameData.segs.nFaces; i++, faceP++) {
	keyFaceRef [i] = faceP;
	if (faceP->m_info.nBaseTex < 0)
		faceP->m_info.nKey = 0x7FFFFFFF;
	else if (faceP->m_info.nOvlTex <= 0)
		faceP->m_info.nKey = int (faceP->m_info.nBaseTex);
	else {
		LoadFaceBitmaps (SEGMENTS + faceP->m_info.nSegment, faceP);
		if (faceP->bmTop && (faceP->bmTop->Flags () & BM_FLAG_SUPER_TRANSPARENT))
			faceP->m_info.nKey = int (faceP->m_info.nBaseTex) + int (faceP->m_info.nOvlTex) * MAX_WALL_TEXTURES * MAX_WALL_TEXTURES;
		else
			faceP->m_info.nKey = int (faceP->m_info.nBaseTex) + int (faceP->m_info.nOvlTex) * MAX_WALL_TEXTURES;
		}
	}

CQuickSort<CSegFace*> qs;
qs.SortAscending (keyFaceRef.Buffer (), 0, gameData.segs.nFaces - 1, reinterpret_cast<CQuickSort<CSegFace*>::comparator> (CQuadMeshBuilder::CompareFaceKeys));

int i, nKey = -1;
gameData.segs.nFaceKeys = -1;

for (i = 0; i < gameData.segs.nFaces; i++, faceP++) {
	faceP = keyFaceRef [i];
	if (nKey != faceP->m_info.nKey) {
		nKey = faceP->m_info.nKey;
		++gameData.segs.nFaceKeys;
		}
	faceP->m_info.nKey = gameData.segs.nFaceKeys;
	}
++gameData.segs.nFaceKeys;
gameData.render.faceIndex.Create ();
}

//------------------------------------------------------------------------------

void CQuadMeshBuilder::BuildSlidingFaceList (void)
{
	CSegFace	*faceP = FACES.faces.Buffer ();

FACES.slidingFaces = NULL;
for (int i = gameData.segs.nFaces; i; i--, faceP++)
	if (faceP->m_info.bSlide) {
		faceP->nextSlidingFace = FACES.slidingFaces;
		FACES.slidingFaces = faceP;
		}
}

//------------------------------------------------------------------------------

void CQuadMeshBuilder::RebuildLightmapTexCoord (void)
{
	static short	n2TriVerts [2][2][3] = {{{0,1,2},{0,2,3}},{{0,1,3},{1,2,3}}};

	int			h, i, j, k, nFace;
	short			*triVertP;
	tTexCoord2f	lMapTexCoord [4];

m_faceP = FACES.faces.Buffer ();
m_lMapTexCoordP = FACES.lMapTexCoord.Buffer ();
for (nFace = gameData.segs.nFaces; nFace; nFace--, m_faceP++) {
	SetupLMapTexCoord (lMapTexCoord);
	h = (SEGMENTS [m_faceP->m_info.nSegment].Type (m_faceP->m_info.nSide) == SIDE_IS_TRI_13);
	for (i = 0; i < 2; i++, m_triP++) {
		triVertP = n2TriVerts [h][i];
		for (j = 0; j < 3; j++) {
			k = triVertP [j];
			*m_lMapTexCoordP = lMapTexCoord [k];
			m_lMapTexCoordP++;
			}
		}
	}
}

//------------------------------------------------------------------------------

void CQuadMeshBuilder::SplitIn4Tris (void)
{
	static short	n4TriVerts [4][3] = {{0,1,4},{1,2,4},{2,3,4},{3,0,4}};

	CFloatVector	vSide [4];
	tRgbaColorf		color;
	tTexCoord2f		texCoord;
	short*			triVertP;
	int				h, i, j, k, v;

texCoord.v.u = texCoord.v.v = 0;
color.red = color.green = color.blue = color.alpha = 0;
for (i = 0; i < 4; i++) {
	j = (i + 1) % 4;
	texCoord.v.u += X2F (m_sideP->m_uvls [i].u + m_sideP->m_uvls [j].u) / 8;
	texCoord.v.v += X2F (m_sideP->m_uvls [i].v + m_sideP->m_uvls [j].v) / 8;
	h = m_sideVerts [i];
	k = m_sideVerts [j];
	color.red += (gameData.render.color.ambient [h].color.red + gameData.render.color.ambient [k].color.red) / 8;
	color.green += (gameData.render.color.ambient [h].color.green + gameData.render.color.ambient [k].color.green) / 8;
	color.blue += (gameData.render.color.ambient [h].color.blue + gameData.render.color.ambient [k].color.blue) / 8;
	color.alpha += (gameData.render.color.ambient [h].color.alpha + gameData.render.color.ambient [k].color.alpha) / 8;
	}
vSide [0] = CFloatVector::Avg (gameData.segs.fVertices [m_sideVerts [0]], gameData.segs.fVertices [m_sideVerts [1]]);
vSide [2] = CFloatVector::Avg (gameData.segs.fVertices [m_sideVerts [2]], gameData.segs.fVertices [m_sideVerts [3]]);
vSide [1] = CFloatVector::Avg (gameData.segs.fVertices [m_sideVerts [1]], gameData.segs.fVertices [m_sideVerts [2]]);
vSide [3] = CFloatVector::Avg (gameData.segs.fVertices [m_sideVerts [3]], gameData.segs.fVertices [m_sideVerts [0]]);

VmLineLineIntersection (vSide [0], vSide [2], vSide [1], vSide [3],
								gameData.segs.fVertices [gameData.segs.nVertices],
								gameData.segs.fVertices [gameData.segs.nVertices]);
gameData.segs.vertices [gameData.segs.nVertices].Assign (gameData.segs.fVertices [gameData.segs.nVertices]);
m_sideVerts [4] = gameData.segs.nVertices++;
m_faceP->m_info.nVerts++;
for (i = 0; i < 4; i++, m_triP++) {
	gameData.segs.nTris++;
	m_faceP->m_info.nTris++;
	m_triP->nFace = m_faceP - FACES.faces;
	m_triP->nIndex = m_vertexP - FACES.vertices;
	triVertP = n4TriVerts [i];
	for (j = 0; j < 3; j++) {
		k = triVertP [j];
		v = m_sideVerts [k];
		m_triP->index [j] = v;
		*m_vertexP++ = *gameData.segs.fVertices [v].XYZ ();
		if (j == 2) {
			m_texCoordP [2] = texCoord;
			m_faceColorP [2] = color;
			}
		else {
			m_texCoordP [j].v.u = X2F (m_sideP->m_uvls [k].u);
			m_texCoordP [j].v.v = X2F (m_sideP->m_uvls [k].v);
			m_colorP = gameData.render.color.ambient + v;
			m_faceColorP [j] = m_colorP->color;
			}
		RotateTexCoord2f (*m_ovlTexCoordP, m_texCoordP [j], (ubyte) m_sideP->m_nOvlOrient);
		m_ovlTexCoordP++;
		}
	m_normalP = SetTriNormals (m_triP, m_normalP);
	m_texCoordP += 3;
	m_faceColorP += 3;
	}
}

//------------------------------------------------------------------------------

bool CQuadMeshBuilder::BuildVBOs (void)
{
#if GEOMETRY_VBOS
if (!ogl.m_states.bHaveVBOs)
	return false;
DestroyVBOs ();
int h, i;
glGenBuffersARB (1, &FACES.vboDataHandle);
if ((i = glGetError ())) {
	glGenBuffersARB (1, &FACES.vboDataHandle);
	if ((i = glGetError ())) {
#	if DBG
		HUDMessage (0, "glGenBuffersARB failed (%d)", i);
#	endif
		ogl.m_states.bHaveVBOs = 0;
		return false;
		}
	}
glBindBufferARB (GL_ARRAY_BUFFER_ARB, FACES.vboDataHandle);
if ((i = glGetError ())) {
#	if DBG
	HUDMessage (0, "glBindBufferARB failed (%d)", i);
#	endif
	ogl.m_states.bHaveVBOs = 0;
	return false;
	}
FACES.nVertices = gameStates.render.bTriangleMesh ? gameData.segs.nTris * 3 : gameData.segs.nFaces * 4;
glBufferDataARB (GL_ARRAY_BUFFER, FACES.nVertices * sizeof (tFaceRenderVertex), NULL, GL_STATIC_DRAW_ARB));
FACES.vertexP = reinterpret_cast<ubyte*> (glMapBufferARB (GL_ARRAY_BUFFER_ARB, GL_WRITE_ONLY_ARB));
FACES.vboIndexHandle = 0;
glGenBuffersARB (1, &FACES.vboIndexHandle);
if (FACES.vboIndexHandle) {
	glBindBufferARB (GL_ELEMENT_ARRAY_BUFFER_ARB, FACES.vboIndexHandle);
	glBufferDataARB (GL_ELEMENT_ARRAY_BUFFER_ARB, FACES.nVertices * sizeof (ushort), NULL, GL_STATIC_DRAW_ARB);
	FACES.indexP = reinterpret_cast<ushort*> (glMapBufferARB (GL_ELEMENT_ARRAY_BUFFER_ARB, GL_WRITE_ONLY_ARB);
	}
else
	FACES.indexP = NULL;

ubyte *vertexP = FACES.vertexP;
i = 0;
FACES.iVertices = 0;
memcpy (vertexP, FACES.vertices, h = FACES.nVertices * sizeof (FACES.vertices [0]));
i += h;
FACES.iNormals = i;
memcpy (vertexP + i, FACES.normals, h = FACES.nVertices * sizeof (FACES.normals [0]));
i += h;
FACES.iColor = i;
memcpy (vertexP + i, FACES.color, h = FACES.nVertices * sizeof (FACES.color [0]));
i += h;
FACES.iTexCoord = i;
memcpy (vertexP + i, FACES.texCoord, h = FACES.nVertices * sizeof (FACES.texCoord [0]));
i += h;
FACES.iOvlTexCoord = i;
memcpy (vertexP + i, FACES.ovlTexCoord, h = FACES.nVertices * sizeof (FACES.ovlTexCoord [0]));
i += h;
FACES.iLMapTexCoord = i;
memcpy (vertexP + i, FACES.lMapTexCoord, h = FACES.nVertices * sizeof (FACES.lMapTexCoord [0]));
for (int i = 0; i < FACES.nVertices; i++)
	FACES.indexP [i] = i;

glUnmapBufferARB (GL_ARRAY_BUFFER_ARB);
glBindBufferARB (GL_ARRAY_BUFFER_ARB, 0);
glUnmapBufferARB (GL_ELEMENT_ARRAY_BUFFER_ARB);
glBindBufferARB (GL_ELEMENT_ARRAY_BUFFER_ARB, 0);
return true;
#else
return false;
#endif
}

//------------------------------------------------------------------------------

void CQuadMeshBuilder::DestroyVBOs (void)
{
#if GEOMETRY_VBOS
if (FACES.vboDataHandle) {
	glDeleteBuffersARB (1, &FACES.vboDataHandle);
	FACES.vboDataHandle = 0;
	}
if (FACES.vboIndexHandle) {
	glDeleteBuffersARB (1, &FACES.vboIndexHandle);
	FACES.vboIndexHandle = 0;
	}
#endif
}

//------------------------------------------------------------------------------

#define FACE_VERTS	6

int CQuadMeshBuilder::Build (int nLevel, bool bRebuild)
{
m_faceP = FACES.faces.Buffer ();
m_triP = TRIANGLES.Buffer ();
m_vertexP = FACES.vertices.Buffer ();
m_normalP = FACES.normals.Buffer ();
m_texCoordP = FACES.texCoord.Buffer ();
m_ovlTexCoordP = FACES.ovlTexCoord.Buffer ();
m_lMapTexCoordP = FACES.lMapTexCoord.Buffer ();
m_faceColorP = FACES.color.Buffer ();
m_colorP = gameData.render.color.ambient.Buffer ();
m_segP = SEGMENTS.Buffer ();
m_segFaceP = SEGFACES.Buffer ();
FACES.slidingFaces = NULL;

	short			nSegment, i;
	ubyte			nSide;

#if !DBG
if (gameStates.render.nMeshQuality > 3)
	gameStates.render.nMeshQuality = 3;
#endif
lightManager.SetMethod ();
if (gameStates.render.nLightingMethod) {
	if (gameStates.render.bPerPixelLighting) {
		gameStates.render.bTriangleMesh = -1;
		gameStates.render.nMeshQuality = 0;
		}
	else 
		gameStates.render.bTriangleMesh = (gameStates.render.nMeshQuality > 0) ? 1 : -1;
	}
else
	gameStates.render.bTriangleMesh = 0;
gameStates.render.nFacePrimitive = gameStates.render.bTriangleMesh ? GL_TRIANGLES : GL_TRIANGLE_FAN;
if (gameStates.render.bSplitPolys)
	gameStates.render.bSplitPolys = (gameStates.render.bPerPixelLighting || !gameStates.render.nMeshQuality) ? 1 : -1;
if (gameStates.render.bTriangleMesh)
	cameraManager.Create ();
PrintLog ("   Creating face list\n");
gameData.segs.nFaces = 0;
gameData.segs.nTris = 0;

// the mesh builder can theoretically add one vertex per segment, so resize the vertex buffers
gameData.segs.vertices.Resize (LEVEL_VERTICES + LEVEL_SIDES);
gameData.segs.fVertices.Resize (LEVEL_VERTICES + LEVEL_SIDES);
gameData.segs.points.Resize (LEVEL_VERTICES + LEVEL_SIDES);

for (nSegment = 0; nSegment < gameData.segs.nSegments; nSegment++, m_segP++, m_segFaceP++) {
	m_bColoredSeg = ((SEGMENTS [nSegment].m_nType >= SEGMENT_IS_WATER) &&
					     (SEGMENTS [nSegment].m_nType <= SEGMENT_IS_TEAM_RED)) ||
					     (SEGMENTS [nSegment].m_group >= 0);
#if DBG
	if (nSegment == nDbgSeg)
		m_faceP = m_faceP;
	if (m_faceP >= FACES.faces.Buffer () + FACES.faces.Length ())
		return 0;
	if (m_segFaceP >= SEGFACES.Buffer () + SEGFACES.Length ())
		return 0;
//	if (nSegment == nDbgSeg)
//		FACES.Destroy ();
#endif
	m_faceP->m_info.nSegment = nSegment;
	m_nOvlTexCount = 0;
	m_segFaceP->nFaces = 0;
	for (nSide = 0, m_sideP = m_segP->m_sides; nSide < 6; nSide++, m_sideP++) {
#if DBG
		if ((nSegment == nDbgSeg) && ((nDbgSide < 0) || (nSide == nDbgSide)))
			nDbgSeg = nDbgSeg;
#endif
		m_nWall = SEGMENTS [nSegment].WallNum (nSide);
		m_nWallType = IS_WALL (m_nWall) ? WALLS [m_nWall].IsInvisible () ? 0 : 2 : (m_segP->m_children [nSide] == -1) ? 1 : 0;
		if (m_bColoredSeg || m_nWallType) {
#if DBG
			if ((nSegment == nDbgSeg) && ((nDbgSide < 0) || (nSide == nDbgSide)))
				nDbgSeg = nDbgSeg;
#endif
			memcpy (m_sideVerts, SEGMENTS [nSegment].Corners (nSide), 4 * sizeof (ushort));
			InitFace (nSegment, nSide, bRebuild);
			if (m_nWallType)
				InitTexturedFace ();
			else if (m_bColoredSeg)
				InitColoredFace (nSegment);
			if (gameStates.render.bTriangleMesh) {
				// split in four triangles, using the quad's center of gravity as additional vertex
				if (!gameStates.render.bPerPixelLighting && (m_sideP->m_nType == SIDE_IS_QUAD) &&
					 !m_faceP->m_info.bSlide && (m_faceP->m_info.nCamera < 0) && IsBigFace (m_sideVerts))
					SplitIn4Tris ();
				else // split in two triangles, regarding any non-planarity
					SplitIn2Tris ();
				}
			else
				SetupFace ();
#if DBG
			if ((nSegment == nDbgSeg) && ((nDbgSide < 0) || (nSide == nDbgSide)))
				m_faceP = m_faceP;
#endif
			if (!m_segFaceP->nFaces++)
				m_segFaceP->faceP = m_faceP;
			m_faceP++;
			gameData.segs.nFaces++;
			}
		else {
			m_colorP += FACE_VERTS;
			}
		}
	}
#if 1
// any additional vertices have been stored, so prune the buffers to the minimally required size
if (!(gameData.segs.Resize () && gameData.render.lights.Resize () && gameData.render.color.Resize ()))
	return 0;
#endif
for (m_colorP = gameData.render.color.ambient.Buffer (), i = gameData.segs.nVertices; i; i--, m_colorP++)
	if (m_colorP->color.alpha > 1) {
		m_colorP->color.red /= m_colorP->color.alpha;
		m_colorP->color.green /= m_colorP->color.alpha;
		m_colorP->color.blue /= m_colorP->color.alpha;
		m_colorP->color.alpha = 1;
		}
if (gameStates.render.bTriangleMesh && !m_triMeshBuilder.Build (nLevel, gameStates.render.nMeshQuality))
	return 0;
#if 1
if (!(gameData.render.lights.Resize () && gameData.render.color.Resize ()))
	return 0;
#endif
BuildSlidingFaceList ();
if (gameStates.render.bTriangleMesh)
	cameraManager.Destroy ();
return 1;
}

//------------------------------------------------------------------------------
//eof
