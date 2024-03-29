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

#ifndef _AUTOMAP_H
#define _AUTOMAP_H

#include "carray.h"
#include "player.h"

#define AM_SHOW_PLAYERS				(!IsMultiGame || (gameData.app.nGameMode & (GM_TEAM | GM_MULTI_COOP)) || (netGame.m_info.gameFlags & NETGAME_FLAG_SHOW_MAP))
#define AM_SHOW_PLAYER(_i)			(!IsMultiGame || \
											 (gameData.app.nGameMode & GM_MULTI_COOP) || \
											 (netGame.m_info.gameFlags & NETGAME_FLAG_SHOW_MAP) || \
											 (GetTeam (gameData.multiplayer.nLocalPlayer) == GetTeam (_i)))
#define AM_SHOW_ROBOTS				EGI_FLAG (bRobotsOnRadar, 0, 1, 0)
#ifdef DBG
#	define AM_SHOW_POWERUPS(_i)	(EGI_FLAG (bPowerupsOnRadar, 0, 1, 0) >= (_i))
#else
#	define AM_SHOW_POWERUPS(_i)	((EGI_FLAG (bPowerupsOnRadar, 0, 1, 0) >= (_i)) && (!IsMultiGame || IsCoopGame))
#endif

//------------------------------------------------------------------------------

#define MAX_EDGES 65536 // Determined by loading all the levels by John & Mike, Feb 9, 1995

typedef struct tEdgeInfo {
	short		verts [2];     // 4 bytes
	ubyte		sides [4];     // 4 bytes
	short		nSegment [4];  // 8 bytes  // This might not need to be stored... If you can access the normals of a CSide.
	uint		color;			// 4 bytes
	ubyte		nFaces;			// 1 bytes  // 19 bytes...
	ubyte		flags;			// 1 bytes  // See the EF_??? defines above.
} __pack__ tEdgeInfo;

//------------------------------------------------------------------------------

typedef struct tAutomapWallColors {
	uint	nNormal;
	uint	nDoor;
	uint	nDoorBlue;
	uint	nDoorGold;
	uint	nDoorRed;
	uint	nRevealed;
} tAutomapWallColors;

typedef struct tAutomapColors {
	tAutomapWallColors	walls;
	uint						nHostage;
	uint						nMonsterball;
	uint						nWhite;
	uint						nMedGreen;
	uint						nLgtBlue;
	uint						nLgtRed;
	uint						nDkGray;
} __pack__ tAutomapColors;

typedef struct tAutomapData {
	int			bCheat;
	int			bHires;
	fix			nViewDist;
	fix			nMaxDist;
	fix			nZoom;
	CFixVector	viewPos;
	CFixVector	viewTarget;
	CFixMatrix	viewMatrix;
} __pack__ tAutomapData;

//------------------------------------------------------------------------------

class CAutomap {
	private:
		tAutomapData			m_data;
		tAutomapColors			m_colors;
		CArray<tEdgeInfo>		m_edges;
		CArray<tEdgeInfo*>	m_brightEdges;
		int						m_nEdges;
		int						m_nMaxEdges;
		int						m_nLastEdge;
		int						m_nWidth;
		int						m_nHeight;
		int						m_bFade;
		int						m_nColor;
		float						m_fScale;
		tRgbaColorf				m_color;
		int						m_bChaseCam;
		int						m_bFreeCam;
		char						m_szLevelNum [200];
		char						m_szLevelName [200];
		CBitmap					m_background;
		CAngleVector			m_vTAngles;
		bool						m_bDrawBuffers;
		int						m_nVerts;

	public:
		CArray<ushort>			m_visited [2];
		CArray<ushort>			m_visible;
		int						m_bRadar;
		bool						m_bFull;
		int						m_bDisplay;
		int						m_nSegmentLimit;
		int						m_nMaxSegsAway;

	public:
		CAutomap () { Init (); }
		~CAutomap () {}
		void Init (void);
		void InitColors (void);
		bool InitBackground (void);
		int Setup (int bPauseGame, fix& xEntryTime);
		int Update (void);
		void Render (fix xStereoSeparation = 0);
		void DoFrame (int nKeyCode, int bRadar);
		void ClearVisited (void);
		int ReadControls (int nLeaveMode, int bDone, int& bPauseGame);

		inline int Radar (void) { return m_bRadar; }
		inline int SegmentLimit (void) { return m_nSegmentLimit; }
		inline int MaxSegsAway (void) { return m_nMaxSegsAway; }
		inline int Visible (int nSegment) { return m_bFull || m_visited [0][nSegment]; }
		inline int Display (void) { return m_bDisplay; }

	private:
		void AdjustSegmentLimit (int nSegmentLimit, CArray<ushort>& visited);
		void DrawEdges (void);
		void DrawPlayer (CObject* objP);
		void DrawObjects (void);
		void DrawLevelId (void);
		void CreateNameCanvas (void);
		int GameFrame (int bPauseGame, int bDone);
		int FindEdge (int v0, int v1, tEdgeInfo*& edgeP);
		void BuildEdgeList (void);
		void AddEdge (int va, int vb, uint color, ubyte CSide, short nSegment, int bHidden, int bGrate, int bNoFade);
		void AddUnknownEdge (int va, int vb);
		void AddSegmentEdges (CSegment *segP);
		void AddUnknownSegmentEdges (CSegment* segP);
		void SetEdgeColor (int nColor, int bFade, float fScale = 1.e10f);
		void DrawLine (short v0, short v1);
};

//------------------------------------------------------------------------------

extern CAutomap automap;

//------------------------------------------------------------------------------

#endif //_AUTOMAP_H
