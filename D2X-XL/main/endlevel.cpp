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

//#define SLEW_ON 1

//#define _MARK_ON

#include <stdlib.h>


#include <stdio.h>
#include <string.h>
#include <ctype.h> // for isspace

#include "descent.h"
#include "error.h"
#include "palette.h"
#include "iff.h"
#include "texmap.h"
#include "u_mem.h"
#include "endlevel.h"
#include "objrender.h"
#include "transprender.h"
#include "screens.h"
#include "cockpit.h"
#include "terrain.h"
#include "newdemo.h"
#include "fireball.h"
#include "network.h"
#include "text.h"
#include "compbit.h"
#include "movie.h"
#include "rendermine.h"
#include "gameseg.h"
#include "key.h"
#include "joy.h"
#include "songs.h"

//------------------------------------------------------------------------------

typedef struct tExitFlightData {
	CObject		*objP;
	CAngleVector	angles;			//orientation in angles
	CFixVector	step;				//how far in a second
	CFixVector	angstep;			//rotation per second
	fix			speed;			//how fast CObject is moving
	CFixVector 	headvec;			//where we want to be pointing
	int			firstTime;		//flag for if first time through
	fix			offset_frac;	//how far off-center as portion of way
	fix			offsetDist;	//how far currently off-center
} tExitFlightData;

//endlevel sequence states

#define SHORT_SEQUENCE	1		//if defined, end sequnce when panning starts
//#define STATION_ENABLED	1		//if defined, load & use space station model

#define FLY_SPEED I2X (50)

void DoEndLevelFlyThrough (int n);
void DrawStars ();
int FindExitSide (CObject *objP);
void GenerateStarfield ();
void StartEndLevelFlyThrough (int n, CObject *objP, fix speed);
void StartRenderedEndLevelSequence ();

char movieTable [2][30] = {
 {'a', 'b', 'c',
	'a',
	'd', 'f', 'd', 'f',
	'g', 'h', 'i', 'g',
	'j', 'k', 'l', 'j',
	'm', 'o', 'm', 'o',
	'p', 'q', 'p', 'q',
	0, 0, 0, 0, 0, 0
	},
 {'f', 'f', 'f', 'a', 'a', 'b', 'b', 'c', 'c',
	 'c', 'g', 'f', 'g', 'g', 'f', 'g', 'f', 'g',
	 'f', 'f', 'f', 'g', 'f', 'g', 'd', 'd', 'f',
	 'e', 'e', 'e'
	}
};

#define FLY_ACCEL I2X (5)

//static tUVL satUVL [4] = {{0,0,I2X (1)},{I2X (1),0,I2X (1)},{I2X (1),I2X (1),I2X (1)},{0,I2X (1),I2X (1)}};
static tUVL satUVL [4] = {{0,0,I2X (1)},{I2X (1),0,I2X (1)},{I2X (1),I2X (1),I2X (1)},{0,I2X (1),I2X (1)}};

extern int ELFindConnectedSide (int seg0, int seg1);

//------------------------------------------------------------------------------

#define MAX_EXITFLIGHT_OBJS 2

tExitFlightData exitFlightObjects [MAX_EXITFLIGHT_OBJS];
tExitFlightData *exitFlightDataP;

//------------------------------------------------------------------------------

void InitEndLevelData (void)
{
gameData.endLevel.station.vPos [X] = 0xf8c4 << 10;
gameData.endLevel.station.vPos [Y] = 0x3c1c << 12;
gameData.endLevel.station.vPos [Z] = 0x0372 << 10;
}

//------------------------------------------------------------------------------
//find delta between two angles
inline fixang DeltaAng (fixang a, fixang b)
{
fixang delta0, delta1;

return (abs (delta0 = a - b) < abs (delta1 = b - a)) ? delta0 : delta1;
}

//------------------------------------------------------------------------------
//return though which CSide of seg0 is seg1
int ELFindConnectedSide (int seg0, int seg1)
{
	CSegment *segP = SEGMENTS + seg0;
	int		i;

for (i = MAX_SIDES_PER_SEGMENT;i--; )
	if (segP->m_children [i] == seg1)
		return i;
return -1;
}

//------------------------------------------------------------------------------

#define MOVIE_REQUIRED 1

//returns movie played status.  see movie.h
int StartEndLevelMovie (void)
{
	char szMovieName [SHORT_FILENAME_LEN];
	int r;

strcpy (szMovieName, gameStates.app.bD1Mission ? "exita.mve" : "esa.mve");
if (!IS_D2_OEM)
	if (gameData.missions.nCurrentLevel == gameData.missions.nLastLevel)
		return 1;   //don't play movie
if (gameData.missions.nCurrentLevel > 0)
	if (gameStates.app.bD1Mission)
		szMovieName [4] = movieTable [1][gameData.missions.nCurrentLevel-1];
	else
		szMovieName [2] = movieTable [0][gameData.missions.nCurrentLevel-1];
else if (gameStates.app.bD1Mission) {
	szMovieName [4] = movieTable [1][26 - gameData.missions.nCurrentLevel];
	}
else {
#ifndef SHAREWARE
	return 0;       //no escapes for secret level
#else
	Error ("Invalid level number <%d>", gameData.missions.nCurrentLevel);
#endif
}
#ifndef SHAREWARE
r = movieManager.Play (szMovieName, (gameData.app.nGameMode & GM_MULTI) ? 0 : MOVIE_REQUIRED, 0, gameOpts->movies.bResize);
#else
return 0;	// movie not played for shareware
#endif
if (gameData.demo.nState == ND_STATE_PLAYBACK)
	SetScreenMode (SCREEN_GAME);
gameData.score.bNoMovieMessage = (r == MOVIE_NOT_PLAYED) && IsMultiGame;
return r;
}

//------------------------------------------------------------------------------

void _CDECL_ FreeEndLevelData (void)
{
PrintLog ("unloading endlevel data\n");
if (gameData.endLevel.terrain.bmInstance.Buffer ()) {
	gameData.endLevel.terrain.bmInstance.ReleaseTexture ();
	gameData.endLevel.terrain.bmInstance.DestroyBuffer ();
	}
if (gameData.endLevel.satellite.bmInstance.Buffer ()) {
	gameData.endLevel.satellite.bmInstance.ReleaseTexture ();
	gameData.endLevel.satellite.bmInstance.DestroyBuffer ();
	}
}

//-----------------------------------------------------------------------------

void InitEndLevel (void)
{
#ifdef STATION_ENABLED
	gameData.endLevel.station.bmP = bm_load ("steel3.bbm");
	gameData.endLevel.station.bmList [0] = &gameData.endLevel.station.bmP;

	gameData.endLevel.station.nModel = LoadPolyModel ("station.pof", 1, gameData.endLevel.station.bmList, NULL);
#endif
GenerateStarfield ();
atexit (FreeEndLevelData);
gameData.endLevel.terrain.bmInstance.SetBuffer (NULL);
gameData.endLevel.satellite.bmInstance.SetBuffer (NULL);
}

//------------------------------------------------------------------------------

CObject externalExplosion;

CAngleVector vExitAngles = CAngleVector::Create(-0xa00, 0, 0);

CFixMatrix mSurfaceOrient;

void StartEndLevelSequence (int bSecret)
{
	CObject	*objP;
	int		nMoviePlayed = MOVIE_NOT_PLAYED;
	//int		i;

if (gameData.demo.nState == ND_STATE_RECORDING)		// stop demo recording
	gameData.demo.nState = ND_STATE_PAUSED;
if (gameData.demo.nState == ND_STATE_PLAYBACK) {		// don't do this if in playback mode
	if ((gameData.missions.nCurrentMission == gameData.missions.nBuiltinMission) ||
		 ((gameData.missions.nCurrentMission == gameData.missions.nD1BuiltinMission) &&
		 movieManager.m_bHaveExtras))
		StartEndLevelMovie ();
	paletteManager.SetLastLoaded ("");		//force palette load next time
	return;
	}
if (gameStates.app.bPlayerIsDead || (gameData.objs.consoleP->info.nFlags & OF_SHOULD_BE_DEAD))
	return;				//don't start if dead!
//	Dematerialize Buddy!
FORALL_ROBOT_OBJS (objP, i)
	if (IS_GUIDEBOT (objP)) {
			/*Object*/CreateExplosion (objP->info.nSegment, objP->info.position.vPos, I2X (7)/2, VCLIP_POWERUP_DISAPPEARANCE);
			objP->Die ();
		}
LOCALPLAYER.homingObjectDist = -I2X (1); // Turn off homing sound.
ResetRearView ();		//turn off rear view if set
if (IsMultiGame) {
	MultiSendEndLevelStart (0);
	NetworkDoFrame (1, 1);
	}
if ((gameData.missions.nCurrentMission == gameData.missions.nBuiltinMission) ||
	 ((gameData.missions.nCurrentMission == gameData.missions.nD1BuiltinMission) && movieManager.m_bHaveExtras)) {
	// only play movie for built-in mission
	if (!IsMultiGame)
		nMoviePlayed = StartEndLevelMovie ();
	}
if ((nMoviePlayed == MOVIE_NOT_PLAYED) && gameStates.app.bEndLevelDataLoaded) {   //don't have movie.  Do rendered sequence, if available
	int bExitModelsLoaded = (gameData.pig.tex.nHamFileVersion < 3) ? 1 : LoadExitModels ();
	if (bExitModelsLoaded) {
		StartRenderedEndLevelSequence ();
		return;
		}
	}
//don't have movie or rendered sequence, fade out
paletteManager.DisableEffect ();
if (!bSecret)
	PlayerFinishedLevel (0);		//done with level
}

//------------------------------------------------------------------------------

void StartRenderedEndLevelSequence (void)
{
	int nLastSeg, nExitSide, nTunnelLength;
	int nSegment, nOldSeg, nEntrySide, i;

//count segments in exit tunnel
nOldSeg = gameData.objs.consoleP->info.nSegment;
nExitSide = FindExitSide (gameData.objs.consoleP);
nSegment = SEGMENTS [nOldSeg].m_children [nExitSide];
nTunnelLength = 0;
do {
	nEntrySide = ELFindConnectedSide (nSegment, nOldSeg);
	nExitSide = sideOpposite [nEntrySide];
	nOldSeg = nSegment;
	nSegment = SEGMENTS [nSegment].m_children [nExitSide];
	nTunnelLength++;
	} while (nSegment >= 0);
if (nSegment != -2) {
	PlayerFinishedLevel (0);		//don't do special sequence
	return;
	}
nLastSeg = nOldSeg;
//now pick transition nSegment 1/3 of the way in
nOldSeg = gameData.objs.consoleP->info.nSegment;
nExitSide = FindExitSide (gameData.objs.consoleP);
nSegment = SEGMENTS [nOldSeg].m_children [nExitSide];
i = nTunnelLength / 3;
while (i--) {
	nEntrySide = ELFindConnectedSide (nSegment, nOldSeg);
	nExitSide = sideOpposite [nEntrySide];
	nOldSeg = nSegment;
	nSegment = SEGMENTS [nSegment].m_children [nExitSide];
	}
gameData.endLevel.exit.nTransitSegNum = nSegment;
CGenericCockpit::Save ();
if (IsMultiGame) {
	MultiSendEndLevelStart (0);
	NetworkDoFrame (1, 1);
	}
Assert (nLastSeg == gameData.endLevel.exit.nSegNum);
if (gameData.missions.list [gameData.missions.nCurrentMission].nDescentVersion == 1)
	songManager.Play (SONG_ENDLEVEL, 0);
gameStates.app.bEndLevelSequence = EL_FLYTHROUGH;
gameData.objs.consoleP->info.movementType = MT_NONE;			//movement handled by flythrough
gameData.objs.consoleP->info.controlType = CT_NONE;
gameStates.app.bGameSuspended |= SUSP_ROBOTS;          //robots don't move
gameData.endLevel.xCurFlightSpeed = gameData.endLevel.xDesiredFlightSpeed = FLY_SPEED;
StartEndLevelFlyThrough (0, gameData.objs.consoleP, gameData.endLevel.xCurFlightSpeed);		//initialize
HUDInitMessage (TXT_EXIT_SEQUENCE);
gameStates.render.bOutsideMine = gameStates.render.bExtExplPlaying = 0;
gameStates.render.nFlashScale = I2X (1);
gameStates.gameplay.bMineDestroyed = 0;
}

//------------------------------------------------------------------------------

CAngleVector vPlayerAngles, vPlayerDestAngles;
CAngleVector vDesiredCameraAngles, vCurrentCameraAngles;

#define CHASE_TURN_RATE (0x4000/4)		//max turn per second

//returns bitmask of which angles are at dest. bits 0, 1, 2 = p, b, h
int ChaseAngles (CAngleVector *cur_angles, CAngleVector *desired_angles)
{
	CAngleVector	delta_angs, alt_angles, alt_delta_angs;
	fix			total_delta, altTotal_delta;
	fix			frame_turn;
	int			mask = 0;

delta_angs [PA] = (*desired_angles) [PA] - (*cur_angles) [PA];
delta_angs [HA] = (*desired_angles) [HA] - (*cur_angles) [HA];
delta_angs [BA] = (*desired_angles) [BA] - (*cur_angles) [BA];
total_delta = abs (delta_angs [PA]) + abs (delta_angs [BA]) + abs (delta_angs [HA]);

alt_angles [PA] = I2X (1)/2 - (*cur_angles) [PA];
alt_angles [BA] = (*cur_angles) [BA] + I2X (1)/2;
alt_angles [HA] = (*cur_angles) [HA] + I2X (1)/2;

alt_delta_angs [PA] = (*desired_angles) [PA] - alt_angles [PA];
alt_delta_angs [HA] = (*desired_angles) [HA] - alt_angles [HA];
alt_delta_angs [BA] = (*desired_angles) [BA] - alt_angles [BA];
//alt_delta_angs.b = 0;

altTotal_delta = abs (alt_delta_angs [PA]) + abs (alt_delta_angs [BA]) + abs (alt_delta_angs [HA]);

////printf ("Total delta = %x, alt total_delta = %x\n", total_delta, altTotal_delta);

if (altTotal_delta < total_delta) {
	////printf ("FLIPPING ANGLES!\n");
	*cur_angles = alt_angles;
	delta_angs = alt_delta_angs;
}

frame_turn = FixMul (gameData.time.xFrame, CHASE_TURN_RATE);

if (abs (delta_angs [PA]) < frame_turn) {
	(*cur_angles) [PA] = (*desired_angles) [PA];
	mask |= 1;
	}
else
	if (delta_angs [PA] > 0)
		(*cur_angles) [PA] += (fixang) frame_turn;
	else
		(*cur_angles) [PA] -= (fixang) frame_turn;

if (abs (delta_angs [BA]) < frame_turn) {
	(*cur_angles) [BA] = (fixang) (*desired_angles) [BA];
	mask |= 2;
	}
else
	if (delta_angs [BA] > 0)
		(*cur_angles) [BA] += (fixang) frame_turn;
	else
		(*cur_angles) [BA] -= (fixang) frame_turn;
if (abs (delta_angs [HA]) < frame_turn) {
	(*cur_angles) [HA] = (fixang) (*desired_angles) [HA];
	mask |= 4;
	}
else
	if (delta_angs [HA] > 0)
		(*cur_angles) [HA] += (fixang) frame_turn;
	else
		(*cur_angles) [HA] -= (fixang) frame_turn;
return mask;
}

//------------------------------------------------------------------------------

void StopEndLevelSequence (void)
{
	gameStates.render.nInterpolationMethod = 0;

paletteManager.DisableEffect ();
CGenericCockpit::Restore ();
gameStates.app.bEndLevelSequence = EL_OFF;
PlayerFinishedLevel (0);
}

//------------------------------------------------------------------------------

#define VCLIP_BIG_PLAYER_EXPLOSION	58

//--unused-- CFixVector upvec = {0, I2X (1), 0};

//find the angle between the CPlayerData's heading & the station
inline void GetAnglesToObject (CAngleVector *av, CFixVector *targ_pos, CFixVector *cur_pos)
{
	CFixVector tv = *targ_pos - *cur_pos;
	*av = tv.ToAnglesVec ();
}

//------------------------------------------------------------------------------

void DoEndLevelFrame (void)
{
	static fix timer;
	static fix bank_rate;
	CFixVector vLastPosSave;
	static fix explosion_wait1=0;
	static fix explosion_wait2=0;
	static fix ext_expl_halflife;

vLastPosSave = gameData.objs.consoleP->info.vLastPos;	//don't let move code change this
UpdateAllObjects ();
gameData.objs.consoleP->info.vLastPos = vLastPosSave;

if (gameStates.render.bExtExplPlaying) {
	externalExplosion.info.xLifeLeft -= gameData.time.xFrame;
	externalExplosion.DoExplosionSequence ();
	if (externalExplosion.info.xLifeLeft < ext_expl_halflife)
		gameStates.gameplay.bMineDestroyed = 1;
	if (externalExplosion.info.nFlags & OF_SHOULD_BE_DEAD)
		gameStates.render.bExtExplPlaying = 0;
	}

if (gameData.endLevel.xCurFlightSpeed != gameData.endLevel.xDesiredFlightSpeed) {
	fix delta = gameData.endLevel.xDesiredFlightSpeed - gameData.endLevel.xCurFlightSpeed;
	fix frame_accel = FixMul (gameData.time.xFrame, FLY_ACCEL);

	if (abs (delta) < frame_accel)
		gameData.endLevel.xCurFlightSpeed = gameData.endLevel.xDesiredFlightSpeed;
	else if (delta > 0)
		gameData.endLevel.xCurFlightSpeed += frame_accel;
	else
		gameData.endLevel.xCurFlightSpeed -= frame_accel;
	}

//do big explosions
if (!gameStates.render.bOutsideMine) {
	if (gameStates.app.bEndLevelSequence == EL_OUTSIDE) {
		CFixVector tvec;

		tvec = gameData.objs.consoleP->info.position.vPos - gameData.endLevel.exit.vSideExit;
		if (CFixVector::Dot (tvec, gameData.endLevel.exit.mOrient.FVec ()) > 0) {
			CObject *objP;
			gameStates.render.bOutsideMine = 1;
			objP = /*Object*/CreateExplosion (gameData.endLevel.exit.nSegNum, gameData.endLevel.exit.vSideExit, I2X (50), VCLIP_BIG_PLAYER_EXPLOSION);
			if (objP) {
				externalExplosion = *objP;
				objP->Die ();
				gameStates.render.nFlashScale = 0;	//kill lights in mine
				ext_expl_halflife = objP->info.xLifeLeft;
				gameStates.render.bExtExplPlaying = 1;
				}
			audio.CreateSegmentSound (SOUND_BIG_ENDLEVEL_EXPLOSION, gameData.endLevel.exit.nSegNum, 0, gameData.endLevel.exit.vSideExit, 0, I2X (3)/4);
			}
		}

	//do explosions chasing CPlayerData
	if ((explosion_wait1 -= gameData.time.xFrame) < 0) {
		CFixVector	tpnt;
		short			nSegment;
		CObject		*expl;
		static int	soundCount;

		tpnt = gameData.objs.consoleP->info.position.vPos + gameData.objs.consoleP->info.position.mOrient.FVec () * (-gameData.objs.consoleP->info.xSize * 5);
		tpnt += gameData.objs.consoleP->info.position.mOrient.RVec () * ((d_rand ()- RAND_MAX / 2) * 15);
		tpnt += gameData.objs.consoleP->info.position.mOrient.UVec () * ((d_rand ()- RAND_MAX / 2) * 15);
		nSegment = FindSegByPos (tpnt, gameData.objs.consoleP->info.nSegment, 1, 0);
		if (nSegment != -1) {
			expl = /*Object*/CreateExplosion (nSegment, tpnt, I2X (20), VCLIP_BIG_PLAYER_EXPLOSION);
			if ((d_rand () < 10000) || (++soundCount == 7)) {		//pseudo-random
				audio.CreateSegmentSound (SOUND_TUNNEL_EXPLOSION, nSegment, 0, tpnt, 0, I2X (1));
				soundCount=0;
				}
			}
		explosion_wait1 = 0x2000 + d_rand ()/4;
		}
	}

//do little explosions on walls
if ((gameStates.app.bEndLevelSequence >= EL_FLYTHROUGH) && (gameStates.app.bEndLevelSequence < EL_OUTSIDE))
	if ((explosion_wait2 -= gameData.time.xFrame) < 0) {
		CFixVector tpnt;
		CHitQuery fq;
		CHitData hitData;
		//create little explosion on CWall
		tpnt = gameData.objs.consoleP->info.position.mOrient.RVec () * ((d_rand () - RAND_MAX / 2) * 100);
		tpnt += gameData.objs.consoleP->info.position.mOrient.UVec () * ((d_rand () - RAND_MAX / 2) * 100);
		tpnt += gameData.objs.consoleP->info.position.vPos;
		if (gameStates.app.bEndLevelSequence == EL_FLYTHROUGH)
			tpnt += gameData.objs.consoleP->info.position.mOrient.FVec () * (d_rand ()*200);
		else
			tpnt += gameData.objs.consoleP->info.position.mOrient.FVec () * (d_rand ()*60);
		//find hit point on CWall
		fq.p0					= &gameData.objs.consoleP->info.position.vPos;
		fq.p1					= &tpnt;
		fq.startSeg			= gameData.objs.consoleP->info.nSegment;
		fq.radP0				=
		fq.radP1				= 0;
		fq.thisObjNum		= 0;
		fq.ignoreObjList	= NULL;
		fq.flags				= 0;
		fq.bCheckVisibility = false;
		FindHitpoint (&fq, &hitData);
		if ((hitData.hit.nType == HIT_WALL) && (hitData.hit.nSegment != -1))
			/*Object*/CreateExplosion ((short) hitData.hit.nSegment, hitData.hit.vPoint, I2X (3)+d_rand ()*6, VCLIP_SMALL_EXPLOSION);
		explosion_wait2 = (0xa00 + d_rand ()/8)/2;
		}

switch (gameStates.app.bEndLevelSequence) {
	case EL_OFF:
		return;

	case EL_FLYTHROUGH: {
		DoEndLevelFlyThrough (0);
		if (gameData.objs.consoleP->info.nSegment == gameData.endLevel.exit.nTransitSegNum) {
			if ((gameData.missions.nCurrentMission == gameData.missions.nBuiltinMission) &&
					(StartEndLevelMovie () != MOVIE_NOT_PLAYED))
				StopEndLevelSequence ();
			else {
				gameStates.app.bEndLevelSequence = EL_LOOKBACK;
				int nObject = CreateCamera (gameData.objs.consoleP);
				if (nObject == -1) { //can't get CObject, so abort
#if TRACE
					console.printf (1, "Can't get CObject for endlevel sequence.  Aborting endlevel sequence.\n");
#endif
					StopEndLevelSequence ();
					return;
					}
				gameData.objs.viewerP = gameData.objs.endLevelCamera = OBJECTS + nObject;
				cockpit->Activate (CM_LETTERBOX);
				exitFlightObjects [1] = exitFlightObjects [0];
				exitFlightObjects [1].objP = gameData.objs.endLevelCamera;
				exitFlightObjects [1].speed = (5 * gameData.endLevel.xCurFlightSpeed) / 4;
				exitFlightObjects [1].offset_frac = 0x4000;
				gameData.objs.endLevelCamera->info.position.vPos += gameData.objs.endLevelCamera->info.position.mOrient.FVec () * (I2X (7));
				timer=0x20000;
				}
			}
		break;
		}

	case EL_LOOKBACK: {
		DoEndLevelFlyThrough (0);
		DoEndLevelFlyThrough (1);
		if (timer > 0) {
			timer -= gameData.time.xFrame;
			if (timer < 0)		//reduce speed
				exitFlightObjects [1].speed = exitFlightObjects [0].speed;
			}
		if (gameData.objs.endLevelCamera->info.nSegment == gameData.endLevel.exit.nSegNum) {
			CAngleVector cam_angles, exit_seg_angles;
			gameStates.app.bEndLevelSequence = EL_OUTSIDE;
			timer = I2X (2);
			gameData.objs.endLevelCamera->info.position.mOrient.FVec () = -gameData.objs.endLevelCamera->info.position.mOrient.FVec ();
			gameData.objs.endLevelCamera->info.position.mOrient.RVec () = -gameData.objs.endLevelCamera->info.position.mOrient.RVec ();
			cam_angles = gameData.objs.endLevelCamera->info.position.mOrient.ExtractAnglesVec ();
			exit_seg_angles = gameData.endLevel.exit.mOrient.ExtractAnglesVec ();
			bank_rate = (-exit_seg_angles [BA] - cam_angles [BA])/2;
			gameData.objs.consoleP->info.controlType = gameData.objs.endLevelCamera->info.controlType = CT_NONE;
#ifdef SLEW_ON
			slewObjP = gameData.objs.endLevelCamera;
#endif
			}
		break;
		}

	case EL_OUTSIDE: {
#ifndef SLEW_ON
		CAngleVector cam_angles;
#endif
		gameData.objs.consoleP->info.position.vPos += gameData.objs.consoleP->info.position.mOrient.FVec () * (FixMul (gameData.time.xFrame, gameData.endLevel.xCurFlightSpeed));
#ifndef SLEW_ON
		gameData.objs.endLevelCamera->info.position.vPos +=
							gameData.objs.endLevelCamera->info.position.mOrient.FVec () *
							(FixMul (gameData.time.xFrame, -2*gameData.endLevel.xCurFlightSpeed));
		gameData.objs.endLevelCamera->info.position.vPos +=
							gameData.objs.endLevelCamera->info.position.mOrient.UVec () *
							(FixMul (gameData.time.xFrame, -gameData.endLevel.xCurFlightSpeed/10));
		cam_angles = gameData.objs.endLevelCamera->info.position.mOrient.ExtractAnglesVec ();
		cam_angles [BA] += (fixang) FixMul (bank_rate, gameData.time.xFrame);
		gameData.objs.endLevelCamera->info.position.mOrient = CFixMatrix::Create (cam_angles);
#endif
		timer -= gameData.time.xFrame;
		if (timer < 0) {
			gameStates.app.bEndLevelSequence = EL_STOPPED;
			vPlayerAngles = gameData.objs.consoleP->info.position.mOrient.ExtractAnglesVec ();
			timer = I2X (3);
			}
		break;
		}

	case EL_STOPPED: {
		GetAnglesToObject (&vPlayerDestAngles, &gameData.endLevel.station.vPos, &gameData.objs.consoleP->info.position.vPos);
		ChaseAngles (&vPlayerAngles, &vPlayerDestAngles);
		gameData.objs.consoleP->info.position.mOrient = CFixMatrix::Create (vPlayerAngles);
		gameData.objs.consoleP->info.position.vPos += gameData.objs.consoleP->info.position.mOrient.FVec () * (FixMul (gameData.time.xFrame, gameData.endLevel.xCurFlightSpeed));
		timer -= gameData.time.xFrame;
		if (timer < 0) {
#ifdef SLEW_ON
			slewObjP = gameData.objs.endLevelCamera;
			DoSlewMovement (gameData.objs.endLevelCamera, 1, 1);
			timer += gameData.time.xFrame;		//make time stop
			break;
#else
#	ifdef SHORT_SEQUENCE
			StopEndLevelSequence ();
#	else
			gameStates.app.bEndLevelSequence = EL_PANNING;
			VmExtractAnglesMatrix (&vCurrentCameraAngles, &gameData.objs.endLevelCamera->info.position.mOrient);
			timer = I2X (3);
			if (gameData.app.nGameMode & GM_MULTI) { // try to skip part of the seq if multiplayer
				StopEndLevelSequence ();
				return;
				}
#	endif		//SHORT_SEQUENCE
#endif		//SLEW_ON
			}
		break;
		}
	#ifndef SHORT_SEQUENCE
	case EL_PANNING: {
#ifndef SLEW_ON
		int mask;
#endif
		GetAnglesToObject (&vPlayerDestAngles, &gameData.endLevel.station.vPos, &gameData.objs.consoleP->info.position.vPos);
		ChaseAngles (&vPlayerAngles, &vPlayerDestAngles);
		VmAngles2Matrix (&gameData.objs.consoleP->info.position.mOrient, &vPlayerAngles);
		VmVecScaleInc (&gameData.objs.consoleP->info.position.vPos, &gameData.objs.consoleP->info.position.mOrient.FVec (), FixMul (gameData.time.xFrame, gameData.endLevel.xCurFlightSpeed);

#ifdef SLEW_ON
		DoSlewMovement (gameData.objs.endLevelCamera, 1, 1);
#else
		GetAnglesToObject (&vDesiredCameraAngles, &gameData.objs.consoleP->info.position.vPos, &gameData.objs.endLevelCamera->info.position.vPos);
		mask = ChaseAngles (&vCurrentCameraAngles, &vDesiredCameraAngles);
		VmAngles2Matrix (&gameData.objs.endLevelCamera->info.position.mOrient, &vCurrentCameraAngles);
		if ((mask & 5) == 5) {
			CFixVector tvec;
			gameStates.app.bEndLevelSequence = EL_CHASING;
			VmVecNormalizedDir (&tvec, &gameData.endLevel.station.vPos, &gameData.objs.consoleP->info.position.vPos);
			VmVector2Matrix (&gameData.objs.consoleP->info.position.mOrient, &tvec, &mSurfaceOrient.UVec (), NULL);
			gameData.endLevel.xDesiredFlightSpeed *= 2;
			}
#endif
		break;
	}

	case EL_CHASING:
	 {
		fix d, speed_scale;

#ifdef SLEW_ON
		DoSlewMovement (gameData.objs.endLevelCamera, 1, 1);
#endif
		GetAnglesToObject (&vDesiredCameraAngles, &gameData.objs.consoleP->info.position.vPos, &gameData.objs.endLevelCamera->info.position.vPos);
		ChaseAngles (&vCurrentCameraAngles, &vDesiredCameraAngles);
#ifndef SLEW_ON
		VmAngles2Matrix (&gameData.objs.endLevelCamera->info.position.mOrient, &vCurrentCameraAngles);
#endif
		d = VmVecDistQuick (&gameData.objs.consoleP->info.position.vPos, &gameData.objs.endLevelCamera->info.position.vPos);
		speed_scale = FixDiv (d, I2X (0x20);
		if (d < I2X (1))
			d = I2X (1);
		GetAnglesToObject (&vPlayerDestAngles, &gameData.endLevel.station.vPos, &gameData.objs.consoleP->info.position.vPos);
		ChaseAngles (&vPlayerAngles, &vPlayerDestAngles);
		VmAngles2Matrix (&gameData.objs.consoleP->info.position.mOrient, &vPlayerAngles);
		VmVecScaleInc (&gameData.objs.consoleP->info.position.vPos, &gameData.objs.consoleP->info.position.mOrient.FVec (), FixMul (gameData.time.xFrame, gameData.endLevel.xCurFlightSpeed);
#ifndef SLEW_ON
		VmVecScaleInc (&gameData.objs.endLevelCamera->info.position.vPos, &gameData.objs.endLevelCamera->info.position.mOrient.FVec (), FixMul (gameData.time.xFrame, FixMul (speed_scale, gameData.endLevel.xCurFlightSpeed));
		if (VmVecDist (&gameData.objs.consoleP->info.position.vPos, &gameData.endLevel.station.vPos) < I2X (10))
			StopEndLevelSequence ();
#endif
		break;
		}
#endif		//ifdef SHORT_SEQUENCE
	}
}

//------------------------------------------------------------------------------

#define MIN_D 0x100

//find which CSide to fly out of
int FindExitSide (CObject *objP)
{
	CFixVector	vPreferred, vSegCenter, vSide;
	fix			d, xBestVal = -I2X (2);
	int			nBestSide, i;
	CSegment		*segP = SEGMENTS + objP->info.nSegment;

//find exit CSide
CFixVector::NormalizedDir (vPreferred, objP->info.position.vPos, objP->info.vLastPos);
vSegCenter = segP->Center ();
nBestSide = -1;
for (i = MAX_SIDES_PER_SEGMENT; --i >= 0;) {
	if (segP->m_children [i] != -1) {
		vSide = segP->SideCenter (i);
		CFixVector::NormalizedDir (vSide, vSide, vSegCenter);
		d = CFixVector::Dot (vSide, vPreferred);
		if (labs (d) < MIN_D)
			d = 0;
		if (d > xBestVal) {
			xBestVal = d;
			nBestSide = i;
			}
		}
	}
Assert (nBestSide!=-1);
return nBestSide;
}

//------------------------------------------------------------------------------

void DrawExitModel (void)
{
	CFixVector	vModelPos;
	int			f = 15, u = 0;	//21;

vModelPos = gameData.endLevel.exit.vMineExit + gameData.endLevel.exit.mOrient.FVec () * (I2X (f));
vModelPos += gameData.endLevel.exit.mOrient.UVec () * (I2X (u));
gameStates.app.bD1Model = gameStates.app.bD1Mission && gameStates.app.bD1Data;
DrawPolyModel (NULL, &vModelPos, &gameData.endLevel.exit.mOrient, NULL,
					gameStates.gameplay.bMineDestroyed ? gameData.endLevel.exit.nDestroyedModel : gameData.endLevel.exit.nModel,
					0, I2X (1), NULL, NULL, NULL);
gameStates.app.bD1Model = 0;
}

//------------------------------------------------------------------------------

int nExitPointBmX, nExitPointBmY;

fix xSatelliteSize = I2X (400);

#define SATELLITE_DIST		I2X (1024)
#define SATELLITE_WIDTH		xSatelliteSize
#define SATELLITE_HEIGHT	 ((xSatelliteSize*9)/4)		// ((xSatelliteSize*5)/2)

void RenderExternalScene (fix xEyeOffset)
{
	CFixVector vDelta;
	g3sPoint p, pTop;

gameData.render.mine.viewerEye = gameData.objs.viewerP->info.position.vPos;
if (xEyeOffset)
	gameData.render.mine.viewerEye += gameData.objs.viewerP->info.position.mOrient.RVec () * (xEyeOffset);
G3SetViewMatrix (gameData.objs.viewerP->info.position.vPos, gameData.objs.viewerP->info.position.mOrient, gameStates.render.xZoom, 1);
CCanvas::Current ()->Clear (BLACK_RGBA);
transformation.Begin (CFixVector::ZERO, mSurfaceOrient);
DrawStars ();
transformation.End ();
//draw satellite
G3TransformAndEncodePoint (&p, gameData.endLevel.satellite.vPos);
transformation.RotateScaled (vDelta, gameData.endLevel.satellite.vUp);
G3AddDeltaVec (&pTop, &p, &vDelta);
if (!(p.p3_codes & CC_BEHIND) && !(p.p3_flags & PF_OVERFLOW)) {
	int imSave = gameStates.render.nInterpolationMethod;
	gameStates.render.nInterpolationMethod = 0;
	gameData.endLevel.satellite.bmP->SetTranspType (0);
	if (!gameData.endLevel.satellite.bmP->Bind (1))
		G3DrawRodTexPoly (gameData.endLevel.satellite.bmP, &p, SATELLITE_WIDTH, &pTop, SATELLITE_WIDTH, I2X (1), satUVL);
	gameStates.render.nInterpolationMethod = imSave;
	}
#ifdef STATION_ENABLED
DrawPolyModel (NULL, &gameData.endLevel.station.vPos, &vmdIdentityMatrix, NULL,
						gameData.endLevel.station.nModel, 0, I2X (1), NULL, NULL);
#endif
RenderTerrain (&gameData.endLevel.exit.vGroundExit, nExitPointBmX, nExitPointBmY);
DrawExitModel ();
if (gameStates.render.bExtExplPlaying)
	DrawFireball (&externalExplosion);
int nLighting = gameStates.render.nLighting;
gameStates.render.nLighting = 0;
RenderObject (gameData.objs.consoleP, 0, 0);
gameStates.render.nState = 0;
transparencyRenderer.Render (0);
gameStates.render.nLighting = nLighting;
}

//------------------------------------------------------------------------------

#define MAX_STARS 500

CFixVector stars [MAX_STARS];

void GenerateStarfield (void)
{
	int i;

for (i = 0; i < MAX_STARS; i++) {
	stars [i][X] = (d_rand () - RAND_MAX/2) << 14;
	stars [i][Z] = (d_rand () - RAND_MAX/2) << 14;
	stars [i][Y] = (d_rand ()/2) << 14;
	}
}

//------------------------------------------------------------------------------

void DrawStars ()
{
	int i;
	int intensity = 31;
	g3sPoint p;

for (i = 0; i < MAX_STARS; i++) {
	if ((i&63) == 0) {
		CCanvas::Current ()->SetColorRGBi (RGB_PAL (intensity, intensity, intensity));
		intensity-=3;
		}
	transformation.RotateScaled (p.p3_vec, stars [i]);
	G3EncodePoint (&p);
	if (p.p3_codes == 0) {
		p.p3_flags &= ~PF_PROJECTED;
		G3ProjectPoint (&p);
		DrawPixelClipped (p.p3_screen.x, p.p3_screen.y);
		}
	}
}

//------------------------------------------------------------------------------

void RenderEndLevelMine (fix xEyeOffset, int nWindowNum)
{
	short nStartSeg;

gameData.render.mine.viewerEye = gameData.objs.viewerP->info.position.vPos;
if (gameData.objs.viewerP->info.nType == OBJ_PLAYER)
	gameData.render.mine.viewerEye += gameData.objs.viewerP->info.position.mOrient.FVec () * ((gameData.objs.viewerP->info.xSize * 3) / 4);
if (xEyeOffset)
	gameData.render.mine.viewerEye += gameData.objs.viewerP->info.position.mOrient.RVec () * (xEyeOffset);
if (gameStates.app.bEndLevelSequence >= EL_OUTSIDE) {
	nStartSeg = gameData.endLevel.exit.nSegNum;
	}
else {
	nStartSeg = FindSegByPos (gameData.render.mine.viewerEye, gameData.objs.viewerP->info.nSegment, 1, 0);
	if (nStartSeg == -1)
		nStartSeg = gameData.objs.viewerP->info.nSegment;
	}
if (gameStates.app.bEndLevelSequence == EL_LOOKBACK) {
	CFixMatrix headm, viewm;
	CAngleVector angles = CAngleVector::Create(0, 0, 0x7fff);
	headm = CFixMatrix::Create (angles);
	viewm = gameData.objs.viewerP->info.position.mOrient * headm;
	G3SetViewMatrix (gameData.render.mine.viewerEye, viewm, gameStates.render.xZoom, 1);
	}
else
	G3SetViewMatrix (gameData.render.mine.viewerEye, gameData.objs.viewerP->info.position.mOrient, gameStates.render.xZoom, 1);
RenderMine (nStartSeg, xEyeOffset, nWindowNum);
transparencyRenderer.Render (0);
}

//-----------------------------------------------------------------------------

void RenderEndLevelFrame (fix xStereoSeparation, int nWindowNum)
{
G3StartFrame (0, !nWindowNum, xStereoSeparation);
//gameStates.render.nFrameFlipFlop = !gameStates.render.nFrameFlipFlop;
if (gameStates.app.bEndLevelSequence < EL_OUTSIDE)
	RenderEndLevelMine (xStereoSeparation, nWindowNum);
else if (!nWindowNum)
	RenderExternalScene (xStereoSeparation);
G3EndFrame ();
}

//------------------------------------------------------------------------------
///////////////////////// copy of flythrough code for endlevel

int ELFindConnectedSide (int seg0, int seg1);

fixang DeltaAng (fixang a, fixang b);

#define DEFAULT_SPEED I2X (16)

#define MIN_D 0x100

//if speed is zero, use default speed
void StartEndLevelFlyThrough (int n, CObject *objP, fix speed)
{
exitFlightDataP = exitFlightObjects + n;
exitFlightDataP->objP = objP;
exitFlightDataP->firstTime = 1;
exitFlightDataP->speed = speed ? speed : DEFAULT_SPEED;
exitFlightDataP->offset_frac = 0;
songManager.Play (SONG_INTER, 0);
}

//------------------------------------------------------------------------------

static CAngleVector *angvec_add2_scale (CAngleVector *dest, CFixVector *src, fix s)
{
(*dest) [PA] += (fixang) FixMul ((*src) [X], s);
(*dest) [BA] += (fixang) FixMul ((*src) [Z], s);
(*dest) [HA] += (fixang) FixMul ((*src) [Y], s);
return dest;
}

//-----------------------------------------------------------------------------

#define MAX_ANGSTEP	0x4000		//max turn per second

#define MAX_SLIDE_PER_SEGMENT 0x10000

void DoEndLevelFlyThrough (int n)
{
	CObject *objP;
	CSegment *segP;
	int nOldPlayerSeg;

exitFlightDataP = exitFlightObjects + n;
objP = exitFlightDataP->objP;
nOldPlayerSeg = objP->info.nSegment;

//move the CPlayerData for this frame

if (!exitFlightDataP->firstTime) {
	objP->info.position.vPos += exitFlightDataP->step * gameData.time.xFrame;
	angvec_add2_scale (&exitFlightDataP->angles, &exitFlightDataP->angstep, gameData.time.xFrame);
	objP->info.position.mOrient = CFixMatrix::Create (exitFlightDataP->angles);
	}
//check new CPlayerData seg
if (UpdateObjectSeg (objP, false)) {
	segP = SEGMENTS + objP->info.nSegment;
	if (exitFlightDataP->firstTime || (objP->info.nSegment != nOldPlayerSeg)) {		//moved into new seg
		CFixVector curcenter, nextcenter;
		fix xStepSize, xSegTime;
		short nEntrySide, nExitSide = -1;//what sides we entry and leave through
		CFixVector vDest;		//where we are heading (center of nExitSide)
		CAngleVector aDest;		//where we want to be pointing
		CFixMatrix mDest;
		int nUpSide = 0;

	#if DBG
		if (n && objP->info.nSegment == nDbgSeg)
			nDbgSeg = nDbgSeg;
	#endif
		nEntrySide = 0;
		//find new exit CSide
		if (!exitFlightDataP->firstTime) {
			nEntrySide = ELFindConnectedSide (objP->info.nSegment, nOldPlayerSeg);
			nExitSide = sideOpposite [nEntrySide];
			}
		if (exitFlightDataP->firstTime || (nEntrySide == -1) || (segP->m_children [nExitSide] == -1))
			nExitSide = FindExitSide (objP);
		if ((nExitSide >= 0) && (segP->m_children [nExitSide] >= 0)) {
			fix d, dLargest = -I2X (1);
			for (int i = 0; i < 6; i++) {
				d = CFixVector::Dot (segP->m_sides [i].m_normals [0], exitFlightDataP->objP->info.position.mOrient.UVec ());
				if (d > dLargest) {
					dLargest = d; 
					nUpSide = i;
					}
				}
			//update target point & angles
			vDest = segP->SideCenter (nExitSide);
			//update target point and movement points
			//offset CObject sideways
			if (exitFlightDataP->offset_frac) {
				int s0 = -1, s1 = 0, i;
				CFixVector s0p, s1p;
				fix dist;

				for (i = 0; i < 6; i++)
					if (i != nEntrySide && i != nExitSide && i != nUpSide && i != sideOpposite [nUpSide]) {
						if (s0 == -1)
							s0 = i;
						else
							s1 = i;
						}
				s0p = segP->SideCenter (s0);
				s1p = segP->SideCenter (s1);
				dist = FixMul (CFixVector::Dist (s0p, s1p), exitFlightDataP->offset_frac);
				if (dist-exitFlightDataP->offsetDist > MAX_SLIDE_PER_SEGMENT)
					dist = exitFlightDataP->offsetDist + MAX_SLIDE_PER_SEGMENT;
				exitFlightDataP->offsetDist = dist;
				vDest += objP->info.position.mOrient.RVec () * dist;
				}
			exitFlightDataP->step = vDest - objP->info.position.vPos;
			xStepSize = CFixVector::Normalize (exitFlightDataP->step);
			exitFlightDataP->step *= exitFlightDataP->speed;
			curcenter = segP->Center ();
			nextcenter = SEGMENTS [segP->m_children [nExitSide]].Center ();
			exitFlightDataP->headvec = nextcenter - curcenter;
			mDest = CFixMatrix::CreateFU (exitFlightDataP->headvec, segP->m_sides [nUpSide].m_normals [0]);
			aDest = mDest.ExtractAnglesVec ();
			if (exitFlightDataP->firstTime)
				exitFlightDataP->angles = objP->info.position.mOrient.ExtractAnglesVec ();
			xSegTime = FixDiv (xStepSize, exitFlightDataP->speed);	//how long through seg
			if (xSegTime) {
				exitFlightDataP->angstep [X] = max (-MAX_ANGSTEP, min (MAX_ANGSTEP, FixDiv (DeltaAng (exitFlightDataP->angles [PA], aDest [PA]), xSegTime)));
				exitFlightDataP->angstep [Z] = max (-MAX_ANGSTEP, min (MAX_ANGSTEP, FixDiv (DeltaAng (exitFlightDataP->angles [BA], aDest [BA]), xSegTime)));
				exitFlightDataP->angstep [Y] = max (-MAX_ANGSTEP, min (MAX_ANGSTEP, FixDiv (DeltaAng (exitFlightDataP->angles [HA], aDest [HA]), xSegTime)));
				}
			else {
				exitFlightDataP->angles = aDest;
				exitFlightDataP->angstep.SetZero ();
				}
			}
		}
	}
exitFlightDataP->firstTime = 0;
}

//------------------------------------------------------------------------------

#ifdef SLEW_ON		//this is a special routine for slewing around external scene
int DoSlewMovement (CObject *objP, int check_keys, int check_joy)
{
	int moved = 0;
	CFixVector svel, movement;				//scaled velocity (per this frame)
	CFixMatrix rotmat, new_pm;
	int joy_x, joy_y, btns;
	int joyx_moved, joyy_moved;
	CAngleVector rotang;

if (gameStates.input.keys [PA]ressed [KEY_PAD5])
	VmVecZero (&objP->physInfo.velocity);

if (check_keys) {
	objP->physInfo.velocity.x += VEL_SPEED * (KeyDownTime (KEY_PAD9) - KeyDownTime (KEY_PAD7);
	objP->physInfo.velocity.y += VEL_SPEED * (KeyDownTime (KEY_PADMINUS) - KeyDownTime (KEY_PADPLUS);
	objP->physInfo.velocity.z += VEL_SPEED * (KeyDownTime (KEY_PAD8) - KeyDownTime (KEY_PAD2);

	rotang [PA]itch = (KeyDownTime (KEY_LBRACKET) - KeyDownTime (KEY_RBRACKET))/ROT_SPEED;
	rotang.bank  = (KeyDownTime (KEY_PAD1) - KeyDownTime (KEY_PAD3))/ROT_SPEED;
	rotang.head  = (KeyDownTime (KEY_PAD6) - KeyDownTime (KEY_PAD4))/ROT_SPEED;
	}
	else
		rotang [PA]itch = rotang.bank  = rotang.head  = 0;
//check for joystick movement
if (check_joy && bJoyPresent) {
	JoyGetpos (&joy_x, &joy_y);
	btns=JoyGetBtns ();
	joyx_moved = (abs (joy_x - old_joy_x)>JOY_NULL);
	joyy_moved = (abs (joy_y - old_joy_y)>JOY_NULL);
	if (abs (joy_x) < JOY_NULL) joy_x = 0;
	if (abs (joy_y) < JOY_NULL) joy_y = 0;
	if (btns)
		if (!rotang [PA]itch) rotang [PA]itch = FixMul (-joy_y * 512, gameData.time.xFrame); else;
	else
		if (joyy_moved) objP->physInfo.velocity.z = -joy_y * 8192;
	if (!rotang.head) rotang.head = FixMul (joy_x * 512, gameData.time.xFrame);
	if (joyx_moved)
		old_joy_x = joy_x;
	if (joyy_moved)
		old_joy_y = joy_y;
	}
moved = rotang [PA]itch | rotang.bank | rotang.head;
VmAngles2Matrix (&rotmat, &rotang);
VmMatMul (&new_pm, &objP->info.position.mOrient, &rotmat);
objP->info.position.mOrient = new_pm;
VmTransposeMatrix (&new_pm);		//make those columns rows
moved |= objP->physInfo.velocity.x | objP->physInfo.velocity.y | objP->physInfo.velocity.z;
svel = objP->physInfo.velocity;
VmVecScale (&svel, gameData.time.xFrame);		//movement in this frame
VmVecRotate (&movement, &svel, &new_pm);
VmVecInc (&objP->info.position.vPos, &movement);
moved |= (movement.x || movement.y || movement.z);
return moved;
}
#endif

//-----------------------------------------------------------------------------

#define LINE_LEN	80
#define NUM_VARS	8

#define STATION_DIST	I2X (1024)

int ConvertExt (char *dest, const char *ext)
{
	char *t = strchr (dest, '.');

if (t && (t-dest <= 8)) {
	t [1] = ext [0];
	t [2] = ext [1];
	t [3] = ext [2];
	return 1;
	}
return 0;
}

//------------------------------------------------------------------------------

extern char szAutoMission [255];
//called for each level to load & setup the exit sequence
void LoadEndLevelData (int nLevel)
{
	char		filename [13];
	char		line [LINE_LEN], *p;
	CFile		cf;
	int		var, nSegment, nSide;
	int		nExitSide = 0, i;
	int		bHaveBinary = 0;

gameStates.app.bEndLevelDataLoaded = 0;		//not loaded yet
if (!gameOpts->movies.nLevel)
	return;

try_again:

if (gameStates.app.bAutoRunMission)
	strcpy (filename, szAutoMission);
else if (nLevel < 0)		//secret level
	strcpy (filename, gameData.missions.szSecretLevelNames [-nLevel-1]);
else					//Normal level
	strcpy (filename, gameData.missions.szLevelNames [nLevel-1]);
if (!ConvertExt (filename, "end"))
	Error ("Error converting filename\n'<%s>'\nfor endlevel data\n", filename);
if (!cf.Open (filename, gameFolders.szDataDir, "rb", gameStates.app.bD1Mission)) {
	ConvertExt (filename, "txb");
	if (!cf.Open (filename, gameFolders.szDataDir, "rb", gameStates.app.bD1Mission)) {
		if (nLevel == 1) {
#if TRACE
			console.printf (CON_DBG, "Cannot load file text\nof binary version of\n'<%s>'\n", filename);
#endif
			gameStates.app.bEndLevelDataLoaded = 0; // won't be able to play endlevel sequence
			return;
			}
		else {
			nLevel = 1;
			goto try_again;
			}
		}
	bHaveBinary = 1;
	}

//ok...this parser is pretty simple.  It ignores comments, but
//everything else must be in the right place
var = 0;
PrintLog ("      parsing endlevel description\n");
while (cf.GetS (line, LINE_LEN)) {
	if (bHaveBinary) {
		int l = (int) strlen (line);
		for (i = 0; i < l; i++)
			line [i] = EncodeRotateLeft ((char) (EncodeRotateLeft (line [i]) ^ BITMAP_TBL_XOR));
		p = line;
		}
	if ((p = strchr (line, ';')))
		*p = 0;		//cut off comment
	for (p = line + strlen (line) - 1; (p > line) && ::isspace ((ubyte) *p); *p-- = 0)
		;
	for (p = line; ::isspace ((ubyte) *p); p++)
		;
	if (!*p)		//empty line
		continue;
	switch (var) {
		case 0: {						//ground terrain
			CIFF iff;
			int iff_error;

			PrintLog ("         loading terrain bitmap\n");
			if (gameData.endLevel.terrain.bmInstance.Buffer ()) {
				gameData.endLevel.terrain.bmInstance.ReleaseTexture ();
				gameData.endLevel.terrain.bmInstance.DestroyBuffer ();
				}
			Assert (gameData.endLevel.terrain.bmInstance.Buffer () == NULL);
			iff_error = iff.ReadBitmap (p, &gameData.endLevel.terrain.bmInstance, BM_LINEAR);
			if (iff_error != IFF_NO_ERROR) {
#if DBG
				PrintLog (TXT_EXIT_TERRAIN, p, iff.ErrorMsg (iff_error));
				PrintLog ("\n");
#endif
				gameStates.app.bEndLevelDataLoaded = 0; // won't be able to play endlevel sequence
				return;
				}
			gameData.endLevel.terrain.bmP = &gameData.endLevel.terrain.bmInstance;
			//GrRemapBitmapGood (gameData.endLevel.terrain.bmP, NULL, iff_transparent_color, -1);
			break;
		}

		case 1:							//height map
			PrintLog ("         loading endlevel terrain\n");
			LoadTerrain (p);
			break;

		case 2:
			PrintLog ("         loading exit point\n");
			sscanf (p, "%d, %d", &nExitPointBmX, &nExitPointBmY);
			break;

		case 3:							//exit heading
			PrintLog ("         loading exit angle\n");
			vExitAngles [HA] = I2X (atoi (p))/360;
			break;

		case 4: {						//planet bitmap
			CIFF iff;
			int iff_error;

			PrintLog ("         loading satellite bitmap\n");
			if (gameData.endLevel.satellite.bmInstance.Buffer ()) {
				gameData.endLevel.satellite.bmInstance.ReleaseTexture ();
				gameData.endLevel.satellite.bmInstance.DestroyBuffer ();
				}
			iff_error = iff.ReadBitmap (p, &gameData.endLevel.satellite.bmInstance, BM_LINEAR);
			if (iff_error != IFF_NO_ERROR) {
				Warning (TXT_SATELLITE, p, iff.ErrorMsg (iff_error));
				gameStates.app.bEndLevelDataLoaded = 0; // won't be able to play endlevel sequence
				return;
			}
			gameData.endLevel.satellite.bmP = &gameData.endLevel.satellite.bmInstance;
			//GrRemapBitmapGood (gameData.endLevel.satellite.bmP, NULL, iff_transparent_color, -1);
			break;
		}

		case 5:							//earth pos
		case 7: {						//station pos
			CFixMatrix tm;
			CAngleVector ta;
			int pitch, head;

			PrintLog ("         loading satellite and station position\n");
			sscanf (p, "%d, %d", &head, &pitch);
			ta [HA] = I2X (head)/360;
			ta [PA] = -I2X (pitch)/360;
			ta [BA] = 0;
			tm = CFixMatrix::Create(ta);
			if (var == 5)
				gameData.endLevel.satellite.vPos = tm.FVec ();
				//VmVecCopyScale (&gameData.endLevel.satellite.vPos, &tm.FVec (), SATELLITE_DIST);
			else
				gameData.endLevel.station.vPos = tm.FVec ();
			break;
		}

		case 6:						//planet size
			PrintLog ("         loading satellite size\n");
			xSatelliteSize = I2X (atoi (p));
			break;
	}
	var++;
}

Assert (var == NUM_VARS);
// OK, now the data is loaded.  Initialize everything
//find the exit sequence by searching all segments for a CSide with
//children == -2
for (nSegment = 0, gameData.endLevel.exit.nSegNum = -1;
	  (gameData.endLevel.exit.nSegNum == -1) && (nSegment <= gameData.segs.nLastSegment);
	  nSegment++)
	for (nSide = 0; nSide < 6; nSide++)
		if (SEGMENTS [nSegment].m_children [nSide] == -2) {
			gameData.endLevel.exit.nSegNum = nSegment;
			nExitSide = nSide;
			break;
			}

if (gameData.endLevel.exit.nSegNum == -1) {
#if DBG
	Warning (TXT_EXIT_TERRAIN, p, 12);
#endif
	gameStates.app.bEndLevelDataLoaded = 0; // won't be able to play endlevel sequence
	return;
	}
PrintLog ("      computing endlevel element orientation\n");
gameData.endLevel.exit.vMineExit = SEGMENTS [gameData.endLevel.exit.nSegNum].Center ();
ExtractOrientFromSegment (&gameData.endLevel.exit.mOrient, &SEGMENTS [gameData.endLevel.exit.nSegNum]);
gameData.endLevel.exit.vSideExit = SEGMENTS [gameData.endLevel.exit.nSegNum].SideCenter (nExitSide);
gameData.endLevel.exit.vGroundExit = gameData.endLevel.exit.vMineExit + gameData.endLevel.exit.mOrient.UVec () * (-I2X (20));
//compute orientation of surface
{
	CFixVector tv;
	CFixMatrix exit_orient, tm;

	exit_orient = CFixMatrix::Create (vExitAngles);
	CFixMatrix::Transpose (exit_orient);
	mSurfaceOrient = gameData.endLevel.exit.mOrient * exit_orient;
	tm = mSurfaceOrient.Transpose();
	tv = tm * gameData.endLevel.station.vPos;
	gameData.endLevel.station.vPos = gameData.endLevel.exit.vMineExit + tv * STATION_DIST;
	tv = tm * gameData.endLevel.satellite.vPos;
	gameData.endLevel.satellite.vPos = gameData.endLevel.exit.vMineExit + tv * SATELLITE_DIST;
	tm = CFixMatrix::CreateFU (tv, mSurfaceOrient.UVec ());
	gameData.endLevel.satellite.vUp = tm.UVec () * SATELLITE_HEIGHT;
	}
cf.Close ();
gameStates.app.bEndLevelDataLoaded = 1;
}

//------------------------------------------------------------------------------
//eof
