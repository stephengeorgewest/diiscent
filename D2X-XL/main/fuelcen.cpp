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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "descent.h"
#include "gameseg.h"
#include "error.h"
#include "cockpit.h"
#include "fireball.h"
#include "gamesave.h"
#include "collide.h"
#include "network.h"
#include "multibot.h"
#include "escort.h"
#include "dropobject.h"

// The max number of fuel stations per mine.

// Every time a robot is created in the morphing code, it decreases capacity of the morpher
// by this amount... when capacity gets to 0, no more morphers...

#define	ROBOT_GEN_TIME (I2X (5))
#define	EQUIP_GEN_TIME (I2X (3) * (gameStates.app.nDifficultyLevel + 1))

#define MATCEN_HP_DEFAULT			I2X (500) // Hitpoints
#define MATCEN_INTERVAL_DEFAULT	I2X (5)	//  5 seconds

//------------------------------------------------------------
// Resets all fuel center info
void ResetGenerators (void)
{
	int i;

for (i = 0; i < LEVEL_SEGMENTS; i++)
	SEGMENTS [i].m_nType = SEGMENT_IS_NOTHING;
gameData.matCens.nFuelCenters = 0;
gameData.matCens.nBotCenters = 0;
gameData.matCens.nEquipCenters = 0;
gameData.matCens.nRepairCenters = 0;
}

#if DBG		//this is sometimes called by people from the debugger
void reset_allRobot_centers ()
{
	int i;

	// Remove all materialization centers
for (i = 0; i < gameData.segs.nSegments; i++)
	if (SEGMENTS [i].m_nType == SEGMENT_IS_ROBOTMAKER) {
		SEGMENTS [i].m_nType = SEGMENT_IS_NOTHING;
		SEGMENTS [i].m_nMatCen = -1;
		}
}
#endif

//------------------------------------------------------------
// Turns a CSegment into a fully charged up fuel center...
void CSegment::CreateFuelCen (int oldType)
{
	int	i, stationType = m_nType;

switch (stationType) {
	case SEGMENT_IS_NOTHING:
	case SEGMENT_IS_GOAL_BLUE:
	case SEGMENT_IS_GOAL_RED:
	case SEGMENT_IS_TEAM_BLUE:
	case SEGMENT_IS_TEAM_RED:
	case SEGMENT_IS_WATER:
	case SEGMENT_IS_LAVA:
	case SEGMENT_IS_SPEEDBOOST:
	case SEGMENT_IS_BLOCKED:
	case SEGMENT_IS_NODAMAGE:
	case SEGMENT_IS_SKYBOX:
	case SEGMENT_IS_OUTDOOR:
		return;
	case SEGMENT_IS_FUELCEN:
	case SEGMENT_IS_REPAIRCEN:
	case SEGMENT_IS_CONTROLCEN:
	case SEGMENT_IS_ROBOTMAKER:
	case SEGMENT_IS_EQUIPMAKER:
		break;
	default:
		Error ("Segment %d has invalid\nstation nType %d in fuelcen.c\n", Index (), stationType);
	}

switch (oldType) {
	case SEGMENT_IS_FUELCEN:
	case SEGMENT_IS_REPAIRCEN:
	case SEGMENT_IS_ROBOTMAKER:
	case SEGMENT_IS_EQUIPMAKER:
		i = m_value;
		break;
	default:
		Assert (gameData.matCens.nFuelCenters < MAX_FUEL_CENTERS);
		i = gameData.matCens.nFuelCenters;
	}

m_value = i;
gameData.matCens.fuelCenters [i].nType = stationType;
gameData.matCens.origStationTypes [i] = (oldType == stationType) ? SEGMENT_IS_NOTHING : oldType;
gameData.matCens.fuelCenters [i].xMaxCapacity = gameData.matCens.xFuelMaxAmount;
gameData.matCens.fuelCenters [i].xCapacity = gameData.matCens.fuelCenters [i].xMaxCapacity;
gameData.matCens.fuelCenters [i].nSegment = Index ();
gameData.matCens.fuelCenters [i].xTimer = -1;
gameData.matCens.fuelCenters [i].bFlag = 0;
//	gameData.matCens.fuelCenters [i].NextRobotType = -1;
//	gameData.matCens.fuelCenters [i].last_created_obj=NULL;
//	gameData.matCens.fuelCenters [i].last_created_sig = -1;
gameData.matCens.fuelCenters [i].vCenter = Center ();
if (oldType == SEGMENT_IS_NOTHING)
	gameData.matCens.nFuelCenters++;
if (oldType == SEGMENT_IS_EQUIPMAKER)
	gameData.matCens.nEquipCenters++;
else if (oldType == SEGMENT_IS_ROBOTMAKER) {
	gameData.matCens.origStationTypes [i] = SEGMENT_IS_NOTHING;
	if (m_nMatCen < --gameData.matCens.nBotCenters)
		gameData.matCens.botGens [m_nMatCen] = gameData.matCens.botGens [gameData.matCens.nBotCenters];
	}
}

//------------------------------------------------------------

void CSegment::CreateMatCen (int nOldType, int nMaxCount)
{
	int	i;

if (m_nMatCen >= nMaxCount) {
	m_nType = SEGMENT_IS_NOTHING;
	m_nMatCen = -1;
	return;
	}
switch (nOldType) {
	case SEGMENT_IS_FUELCEN:
	case SEGMENT_IS_REPAIRCEN:
	case SEGMENT_IS_ROBOTMAKER:
	case SEGMENT_IS_EQUIPMAKER:
		i = m_value;	// index in fuel center array passed from level editor
		break;
	default:
		Assert (gameData.matCens.nFuelCenters < MAX_FUEL_CENTERS);
		m_value = i = gameData.matCens.nFuelCenters;
	}
gameData.matCens.origStationTypes [i] = (nOldType == m_nType) ? SEGMENT_IS_NOTHING : nOldType;
tFuelCenInfo* fuelCenP = &gameData.matCens.fuelCenters [i];
fuelCenP->nType = m_nType;
fuelCenP->xCapacity = I2X (gameStates.app.nDifficultyLevel + 3);
fuelCenP->xMaxCapacity = fuelCenP->xCapacity;
fuelCenP->nSegment = Index ();
fuelCenP->xTimer = -1;
fuelCenP->bFlag = 0;
fuelCenP->vCenter = m_vCenter;
}

//------------------------------------------------------------
// Adds a matcen that already is a special nType into the gameData.matCens.fuelCenters array.
// This function is separate from other fuelcens because we don't want values reset.
void CSegment::CreateBotGen (int nOldType)
{
CreateMatCen (nOldType, gameFileInfo.botGen.count);
int i = gameData.matCens.nBotCenters++;
gameData.matCens.botGens [i].xHitPoints = MATCEN_HP_DEFAULT;
gameData.matCens.botGens [i].xInterval = MATCEN_INTERVAL_DEFAULT;
gameData.matCens.botGens [i].nSegment = Index ();
if (nOldType == SEGMENT_IS_NOTHING)
	gameData.matCens.botGens [i].nFuelCen = gameData.matCens.nFuelCenters;
gameData.matCens.nFuelCenters++;
}


//------------------------------------------------------------
// Adds a matcen that already is a special nType into the gameData.matCens.fuelCenters array.
// This function is separate from other fuelcens because we don't want values reset.
void CSegment::CreateEquipGen (int nOldType)
{
CreateMatCen (nOldType, gameFileInfo.equipGen.count);
int i = gameData.matCens.nEquipCenters++;
gameData.matCens.equipGens [i].xHitPoints = MATCEN_HP_DEFAULT;
gameData.matCens.equipGens [i].xInterval = MATCEN_INTERVAL_DEFAULT;
gameData.matCens.equipGens [i].nSegment = Index ();
if (nOldType == SEGMENT_IS_NOTHING)
	gameData.matCens.equipGens [i].nFuelCen = gameData.matCens.nFuelCenters;
gameData.matCens.nFuelCenters++;
}

//------------------------------------------------------------

void SetEquipGenStates (void)
{
	int	i;

for (i = 0; i < gameData.matCens.nEquipCenters; i++)
	gameData.matCens.fuelCenters [gameData.matCens.equipGens [i].nFuelCen].bEnabled =
		FindTriggerTarget (gameData.matCens.fuelCenters [i].nSegment, -1) == 0;
}
//------------------------------------------------------------
// Adds a CSegment that already is a special nType into the gameData.matCens.fuelCenters array.
void CSegment::CreateGenerator (int nType)
{
m_nType = nType;
if (m_nType == SEGMENT_IS_ROBOTMAKER)
	CreateBotGen (SEGMENT_IS_NOTHING);
else if (m_nType == SEGMENT_IS_EQUIPMAKER)
	CreateEquipGen (SEGMENT_IS_NOTHING);
else {
	CreateFuelCen (SEGMENT_IS_NOTHING);
	if (m_nType == SEGMENT_IS_REPAIRCEN)
		m_nMatCen = gameData.matCens.nRepairCenters++;
	}
}

//	The lower this number is, the more quickly the center can be re-triggered.
//	If it's too low, it can mean all the robots won't be put out, but for about 5
//	robots, that's not real likely.
#define	MATCEN_LIFE (I2X (30 - 2 * gameStates.app.nDifficultyLevel))

//------------------------------------------------------------
//	Trigger (enable) the materialization center in CSegment nSegment
int StartMatCen (short nSegment)
{
	// -- CSegment		*segP = &SEGMENTS [nSegment];
	CSegment*		segP = &SEGMENTS [nSegment];
	CFixVector		pos, delta;
	tFuelCenInfo	*matCenP;
	int				nObject;

#if TRACE
console.printf (CON_DBG, "Trigger matcen, CSegment %i\n", nSegment);
#endif
if (segP->m_nType == SEGMENT_IS_EQUIPMAKER) {	// toggle it on or off
	matCenP = gameData.matCens.fuelCenters + gameData.matCens.equipGens [segP->m_nMatCen].nFuelCen;
	return (matCenP->bEnabled = !matCenP->bEnabled) ? 1 : 2;
	}
matCenP = gameData.matCens.fuelCenters + gameData.matCens.botGens [segP->m_nMatCen].nFuelCen;
if (matCenP->bEnabled)
	return 0;
if (!matCenP->nLives)
	return 0;
//	MK: 11/18/95, At insane, matcens work forever!
if (gameStates.app.bD1Mission || (gameStates.app.nDifficultyLevel + 1 < NDL))
	matCenP->nLives--;

matCenP->xTimer = I2X (1000);	//	Make sure the first robot gets emitted right away.
matCenP->bEnabled = 1;			//	Say this center is enabled, it can create robots.
matCenP->xCapacity = I2X (gameStates.app.nDifficultyLevel + 3);
matCenP->xDisableTime = MATCEN_LIFE;

//	Create a bright CObject in the CSegment.
pos = matCenP->vCenter;
delta = gameData.segs.vertices[SEGMENTS [nSegment].m_verts [0]] - matCenP->vCenter;
pos += delta * (I2X (1)/2);
nObject = CreateLight (SINGLE_LIGHT_ID, nSegment, pos);
if (nObject != -1) {
	OBJECTS [nObject].info.xLifeLeft = MATCEN_LIFE;
	OBJECTS [nObject].cType.lightInfo.intensity = I2X (8);	//	Light cast by a fuelcen.
	}
return 0;
}

//------------------------------------------------------------
//	Trigger (enable) the materialization center in CSegment nSegment
void OperateBotGen (CObject *objP, short nSegment)
{
	CSegment*		segP = &SEGMENTS [nSegment];
	tFuelCenInfo*	matCenP;
	short				nType;

if (nSegment < 0)
	nType = 255;
else {
	matCenP = gameData.matCens.fuelCenters + gameData.matCens.botGens [segP->m_nMatCen].nFuelCen;
	nType = GetMatCenObjType (matCenP, gameData.matCens.botGens [segP->m_nMatCen].objFlags);
	if (nType < 0)
		nType = 255;
	}
objP->BossSpewRobot (NULL, nType, 1);
}

//	----------------------------------------------------------------------------------------------------------

CObject *CreateMorphRobot (CSegment *segP, CFixVector *vObjPosP, ubyte nObjId)
{
	short			nObject;
	CObject		*objP;
	tRobotInfo	*botInfoP;
	ubyte			default_behavior;

LOCALPLAYER.numRobotsLevel++;
LOCALPLAYER.numRobotsTotal++;
nObject = CreateRobot (nObjId, segP->Index (), *vObjPosP);
if (nObject < 0) {
#if TRACE
	console.printf (1, "Can't create morph robot.  Aborting morph.\n");
#endif
	Int3 ();
	return NULL;
	}
objP = OBJECTS + nObject;
//Set polygon-CObject-specific data
botInfoP = &ROBOTINFO (objP->info.nId);
objP->rType.polyObjInfo.nModel = botInfoP->nModel;
objP->rType.polyObjInfo.nSubObjFlags = 0;
//set Physics info
objP->mType.physInfo.mass = botInfoP->mass;
objP->mType.physInfo.drag = botInfoP->drag;
objP->mType.physInfo.flags |= (PF_LEVELLING);
objP->info.xShields = RobotDefaultShields (objP);
default_behavior = botInfoP->behavior;
if (ROBOTINFO (objP->info.nId).bossFlag)
	gameData.bosses.Add (nObject);
InitAIObject (objP->Index (), default_behavior, -1);		//	Note, -1 = CSegment this robot goes to to hide, should probably be something useful
CreateNSegmentPath (objP, 6, -1);		//	Create a 6 CSegment path from creation point.
gameData.ai.localInfo [nObject].mode = AIBehaviorToMode (default_behavior);
return objP;
}

int Num_extryRobots = 15;

#if DBG
int	FrameCount_last_msg = 0;
#endif

//	----------------------------------------------------------------------------------------------------------

void CreateMatCenEffect (tFuelCenInfo *matCenP, ubyte nVideoClip)
{
	CFixVector	vPos;
	CObject		*objP;

vPos = SEGMENTS [matCenP->nSegment].Center ();
// HACK!!!The 10 under here should be something equal to the 1/2 the size of the CSegment.
objP = /*Object*/CreateExplosion ((short) matCenP->nSegment, vPos, I2X (10), nVideoClip);
if (objP) {
	ExtractOrientFromSegment (&objP->info.position.mOrient, SEGMENTS + matCenP->nSegment);
	if (gameData.eff.vClips [0][nVideoClip].nSound > -1)
		audio.CreateSegmentSound (gameData.eff.vClips [0][nVideoClip].nSound, (short) matCenP->nSegment, 0, vPos, 0, I2X (1));
	matCenP->bFlag	= 1;
	matCenP->xTimer = 0;
	}
}

//	----------------------------------------------------------------------------------------------------------

int GetMatCenObjType (tFuelCenInfo *matCenP, int *objFlags)
{
	int				i, nObjIndex, nTypes = 0;
	uint	flags;
	sbyte				objTypes [64];

memset (objTypes, 0, sizeof (objTypes));
for (i = 0; i < 3; i++) {
	nObjIndex = i * 32;
	flags = (uint) objFlags [i];
	while (flags) {
		if (flags & 1)
			objTypes [nTypes++] = nObjIndex;
		flags >>= 1;
		nObjIndex++;
		}
	}
if (!nTypes)
	return -1;
if (nTypes == 1)
	return objTypes [0];
return objTypes [(d_rand () * nTypes) / 32768];
}

//	----------------------------------------------------------------------------------------------------------

void EquipGenHandler (tFuelCenInfo * matCenP)
{
	int			nObject, nMatCen, nType;
	CObject		*objP;
	CFixVector	vPos;
	fix			topTime;

if (!matCenP->bEnabled)
	return;
nMatCen = SEGMENTS [matCenP->nSegment].m_nMatCen;
if (nMatCen == -1) {
#if TRACE
	console.printf (CON_DBG, "Dysfunctional robot generator at %d\n", matCenP->nSegment);
#endif
	return;
	}
matCenP->xTimer += gameData.time.xFrame;
if (!matCenP->bFlag) {
	topTime = EQUIP_GEN_TIME;
	if (matCenP->xTimer < topTime)
		return;
	nObject = SEGMENTS [matCenP->nSegment].m_objects;
	while (nObject >= 0) {
		objP = OBJECTS + nObject;
		if ((objP->info.nType == OBJ_POWERUP) || (objP->info.nId == OBJ_PLAYER)) {
			matCenP->xTimer = 0;
			return;
			}
		nObject = objP->info.nNextInSeg;
		}
	CreateMatCenEffect (matCenP, VCLIP_MORPHING_ROBOT);
	}
else if (matCenP->bFlag == 1) {			// Wait until 1/2 second after VCLIP started.
	if (matCenP->xTimer < (gameData.eff.vClips [0][VCLIP_MORPHING_ROBOT].xTotalTime / 2))
		return;
	matCenP->bFlag = 0;
	matCenP->xTimer = 0;
	nType = GetMatCenObjType (matCenP, gameData.matCens.equipGens [nMatCen].objFlags);
	if (nType < 0)
		return;
	vPos = SEGMENTS [matCenP->nSegment].Center ();
	// If this is the first materialization, set to valid robot.
	nObject = CreatePowerup (nType, -1, (short) matCenP->nSegment, vPos, 1, true);
	if (nObject < 0)
		return;
	objP = OBJECTS + nObject;
	if (IsMultiGame) {
		gameData.multiplayer.maxPowerupsAllowed [nType]++;
		gameData.multigame.create.nObjNums [gameData.multigame.create.nCount++] = nObject;
		}
	objP->rType.vClipInfo.nClipIndex = gameData.objs.pwrUp.info [objP->info.nId].nClipIndex;
	objP->rType.vClipInfo.xFrameTime = gameData.eff.vClips [0][objP->rType.vClipInfo.nClipIndex].xFrameTime;
	objP->rType.vClipInfo.nCurFrame = 0;
	objP->info.nCreator = SEGMENTS [matCenP->nSegment].m_owner;
	objP->info.xLifeLeft = IMMORTAL_TIME;
	}
else {
	matCenP->bFlag = 0;
	matCenP->xTimer = 0;
	}
}

//	----------------------------------------------------------------------------------------------------------

void VirusGenHandler (tFuelCenInfo * matCenP)
{
	int			nObject, nMatCen;
	CObject		*objP;
	CFixVector	vPos;
	fix			topTime;

if (gameStates.entropy.bExitSequence || (SEGMENTS [matCenP->nSegment].m_owner <= 0))
	return;
nMatCen = SEGMENTS [matCenP->nSegment].m_nMatCen;
if (nMatCen == -1) {
#if TRACE
	console.printf (CON_DBG, "Dysfunctional robot generator at %d\n", matCenP->nSegment);
#endif
	return;
	}
matCenP->xTimer += gameData.time.xFrame;
if (!matCenP->bFlag) {
	topTime = I2X (extraGameInfo [1].entropy.nVirusGenTime);
	if (matCenP->xTimer < topTime)
		return;
	nObject = SEGMENTS [matCenP->nSegment].m_objects;
	while (nObject >= 0) {
		objP = OBJECTS + nObject;
		if ((objP->info.nType == OBJ_POWERUP) && (objP->info.nId == POW_ENTROPY_VIRUS)) {
			matCenP->xTimer = 0;
			return;
			}
		nObject = objP->info.nNextInSeg;
		}
	CreateMatCenEffect (matCenP, VCLIP_POWERUP_DISAPPEARANCE);
	}
else if (matCenP->bFlag == 1) {			// Wait until 1/2 second after VCLIP started.
	if (matCenP->xTimer < (gameData.eff.vClips [0][VCLIP_POWERUP_DISAPPEARANCE].xTotalTime / 2))
		return;
	matCenP->bFlag = 0;
	matCenP->xTimer = 0;
	vPos = SEGMENTS [matCenP->nSegment].Center ();
	// If this is the first materialization, set to valid robot.
	nObject = CreatePowerup (POW_ENTROPY_VIRUS, -1, (short) matCenP->nSegment, vPos, 1);
	if (nObject >= 0) {
		objP = OBJECTS + nObject;
		if (IsMultiGame)
			gameData.multigame.create.nObjNums [gameData.multigame.create.nCount++] = nObject;
		objP->rType.vClipInfo.nClipIndex = gameData.objs.pwrUp.info [objP->info.nId].nClipIndex;
		objP->rType.vClipInfo.xFrameTime = gameData.eff.vClips [0][objP->rType.vClipInfo.nClipIndex].xFrameTime;
		objP->rType.vClipInfo.nCurFrame = 0;
		objP->info.nCreator = SEGMENTS [matCenP->nSegment].m_owner;
		objP->info.xLifeLeft = IMMORTAL_TIME;
		}
	}
else {
	matCenP->bFlag = 0;
	matCenP->xTimer = 0;
	}
}

//	----------------------------------------------------------------------------------------------------------

inline int VertigoObjFlags (tMatCenInfo *miP)
{
return miP->objFlags [2] = gameData.objs.nVertigoBotFlags;
}

//	----------------------------------------------------------------------------------------------------------

void BotGenHandler (tFuelCenInfo * matCenP)
{
	fix			xDistToPlayer;
	CFixVector	vPos, vDir;
	int			nMatCen, nSegment, nObject;
	CObject		*objP;
	fix			topTime;
	int			nType, nMyStation, nCount;
	//int			i;

if (gameStates.gameplay.bNoBotAI)
	return;
if (!matCenP->bEnabled)
	return;
if (matCenP->xDisableTime > 0) {
	matCenP->xDisableTime -= gameData.time.xFrame;
	if (matCenP->xDisableTime <= 0) {
#if TRACE
		console.printf (CON_DBG, "Robot center #%i gets disabled due to time running out.\n",FUELCEN_IDX (matCenP));
#endif
		matCenP->bEnabled = 0;
		}
	}
//	No robot making in multiplayer mode.
if (IsMultiGame && (!(gameData.app.nGameMode & GM_MULTI_ROBOTS) || !NetworkIAmMaster ()))
	return;
// Wait until transmorgafier has capacity to make a robot...
if (matCenP->xCapacity <= 0)
	return;
nMatCen = SEGMENTS [matCenP->nSegment].m_nMatCen;
if (nMatCen == -1) {
#if TRACE
	console.printf (CON_DBG, "Dysfunctional robot generator at %d\n", matCenP->nSegment);
#endif
	return;
	}
if (!(gameData.matCens.botGens [nMatCen].objFlags [0] ||
		gameData.matCens.botGens [nMatCen].objFlags [1] ||
		VertigoObjFlags (gameData.matCens.botGens + nMatCen)))
	return;

// Wait until we have a free slot for this puppy...
if ((LOCALPLAYER.numRobotsLevel -
	  LOCALPLAYER.numKillsLevel) >=
	 (nGameSaveOrgRobots + Num_extryRobots)) {
#if DBG
	if (gameData.app.nFrameCount > FrameCount_last_msg + 20) {
#if TRACE
		console.printf (CON_DBG, "Cannot morph until you kill one!\n");
#endif
		FrameCount_last_msg = gameData.app.nFrameCount;
		}
#endif
	return;
	}
matCenP->xTimer += gameData.time.xFrame;
if (!matCenP->bFlag) {
	if (IsMultiGame)
		topTime = ROBOT_GEN_TIME;
	else {
		xDistToPlayer = CFixVector::Dist(gameData.objs.consoleP->info.position.vPos, matCenP->vCenter);
		topTime = xDistToPlayer / 64 + d_rand () * 2 + I2X (2);
		if (topTime > ROBOT_GEN_TIME)
			topTime = ROBOT_GEN_TIME + d_rand ();
		if (topTime < I2X (2))
			topTime = I2X (3)/2 + d_rand ()*2;
		}
	if (matCenP->xTimer < topTime)
		return;
	nMyStation = FUELCEN_IDX (matCenP);

	//	Make sure this robotmaker hasn't put out its max without having any of them killed.
	nCount = 0;
	FORALL_ROBOT_OBJS (objP, i)
		if ((objP->info.nCreator ^ 0x80) == nMyStation)
			nCount++;
	if (nCount > gameStates.app.nDifficultyLevel + 3) {
#if TRACE
		console.printf (CON_DBG, "Cannot morph: center %i has already put out %i robots.\n", nMyStation, nCount);
#endif
		matCenP->xTimer /= 2;
		return;
		}
		//	Whack on any robot or CPlayerData in the matcen CSegment.
	nCount = 0;
	nSegment = matCenP->nSegment;
	for (nObject = SEGMENTS [nSegment].m_objects; nObject != -1; nObject = OBJECTS [nObject].info.nNextInSeg) {
		nCount++;
		if (nCount > LEVEL_OBJECTS) {
#if TRACE
			console.printf (CON_DBG, "Object list in CSegment %d is circular.", nSegment);
#endif
			Int3 ();
			return;
			}
		if (OBJECTS [nObject].info.nType == OBJ_ROBOT) {
			OBJECTS [nObject].CollideRobotAndMatCen ();
			matCenP->xTimer = topTime / 2;
			return;
			}
		else if (OBJECTS [nObject].info.nType == OBJ_PLAYER) {
			OBJECTS [nObject].CollidePlayerAndMatCen ();
			matCenP->xTimer = topTime / 2;
			return;
			}
		}
	CreateMatCenEffect (matCenP, VCLIP_MORPHING_ROBOT);
	}
else if (matCenP->bFlag == 1) {			// Wait until 1/2 second after VCLIP started.
	if (matCenP->xTimer <= (gameData.eff.vClips [0][VCLIP_MORPHING_ROBOT].xTotalTime / 2))
		return;
	matCenP->xCapacity -= gameData.matCens.xEnergyToCreateOneRobot;
	matCenP->bFlag = 0;
	matCenP->xTimer = 0;
	vPos = SEGMENTS [matCenP->nSegment].Center ();
	// If this is the first materialization, set to valid robot.
	nType = (int) GetMatCenObjType (matCenP, gameData.matCens.botGens [nMatCen].objFlags);
	if (nType < 0)
		return;
#if TRACE
	console.printf (CON_DBG, "Morph: (nType = %i) (seg = %i) (capacity = %08x)\n", nType, matCenP->nSegment, matCenP->xCapacity);
#endif
	if (!(objP = CreateMorphRobot (SEGMENTS + matCenP->nSegment, &vPos, nType))) {
#if TRACE
		console.printf (CON_DBG, "Warning: CreateMorphRobot returned NULL (no OBJECTS left?)\n");
#endif
		return;
		}
	if (IsMultiGame)
		MultiSendCreateRobot (FUELCEN_IDX (matCenP), objP->Index (), nType);
	objP->info.nCreator = (FUELCEN_IDX (matCenP)) | 0x80;
	// Make object face player...
	vDir = gameData.objs.consoleP->info.position.vPos - objP->info.position.vPos;
	objP->info.position.mOrient = CFixMatrix::CreateFU(vDir, objP->info.position.mOrient.UVec ());
	//objP->info.position.mOrient = CFixMatrix::CreateFU(vDir, &objP->info.position.mOrient.UVec (), NULL);
	objP->MorphStart ();
	}
else {
	matCenP->bFlag = 0;
	matCenP->xTimer = 0;
	}
}


//-------------------------------------------------------------
// Called once per frame, replenishes fuel supply.
void FuelcenUpdateAll (void)
{
	int				i, t;
	tFuelCenInfo*	fuelCenP = &gameData.matCens.fuelCenters [0];
	fix				xAmountToReplenish = FixMul (gameData.time.xFrame,gameData.matCens.xFuelRefillSpeed);

for (i = 0; i < gameData.matCens.nFuelCenters; i++, fuelCenP++) {
	t = fuelCenP->nType;
	if (t == SEGMENT_IS_ROBOTMAKER) {
		if (IsMultiGame && (gameData.app.nGameMode & GM_ENTROPY))
			VirusGenHandler (gameData.matCens.fuelCenters + i);
		else if (!(gameStates.app.bGameSuspended & SUSP_ROBOTS))
			BotGenHandler (gameData.matCens.fuelCenters + i);
		}
	else if (t == SEGMENT_IS_EQUIPMAKER) {
		if (!(gameStates.app.bGameSuspended & SUSP_ROBOTS))
			EquipGenHandler (gameData.matCens.fuelCenters + i);
		}
	else if (t == SEGMENT_IS_CONTROLCEN) {
		//controlcen_proc (gameData.matCens.fuelCenters + i);
		}
	else if ((fuelCenP->xMaxCapacity > 0) &&
				(gameData.matCens.playerSegP != SEGMENTS + fuelCenP->nSegment)) {
		if (fuelCenP->xCapacity < fuelCenP->xMaxCapacity) {
 			fuelCenP->xCapacity += xAmountToReplenish;
			if (fuelCenP->xCapacity >= fuelCenP->xMaxCapacity) {
				fuelCenP->xCapacity = fuelCenP->xMaxCapacity;
				//gauge_message ("Fuel center is fully recharged!   ");
				}
			}
		}
	}
}

#define FUELCEN_SOUND_DELAY (I2X (1)/4)		//play every half second

//-------------------------------------------------------------

fix CSegment::Damage (fix xMaxDamage)
{
	static fix last_playTime=0;
	fix amount;

if (!(gameData.app.nGameMode & GM_ENTROPY))
	return 0;
gameData.matCens.playerSegP = this;
if ((m_owner < 1) || (m_owner == GetTeam (gameData.multiplayer.nLocalPlayer) + 1))
	return 0;
amount = FixMul (gameData.time.xFrame, I2X (extraGameInfo [1].entropy.nShieldDamageRate));
if (amount > xMaxDamage)
	amount = xMaxDamage;
if (last_playTime > gameData.time.xGame)
	last_playTime = 0;
if (gameData.time.xGame > last_playTime + FUELCEN_SOUND_DELAY) {
	audio.PlaySound (SOUND_PLAYER_GOT_HIT, SOUNDCLASS_GENERIC, I2X (1) / 2);
	if (IsMultiGame)
		MultiSendPlaySound (SOUND_PLAYER_GOT_HIT, I2X (1) / 2);
	last_playTime = gameData.time.xGame;
	}
return amount;
}

//-------------------------------------------------------------

fix CSegment::Refuel (fix nMaxFuel)
{
	fix	amount;

	static fix last_playTime = 0;

gameData.matCens.playerSegP = this;
if ((gameData.app.nGameMode & GM_ENTROPY) && ((m_owner < 0) ||
	 ((m_owner > 0) && (m_owner != GetTeam (gameData.multiplayer.nLocalPlayer) + 1))))
	return 0;
if (m_nType != SEGMENT_IS_FUELCEN)
	return 0;
DetectEscortGoalAccomplished (-4);	//	UGLY!Hack!-4 means went through fuelcen.
#if 0
if (gameData.matCens.fuelCenters [segP->value].xMaxCapacity <= 0) {
	HUDInitMessage ("Fuelcenter %d is destroyed.", segP->value);
	return 0;
	}
if (gameData.matCens.fuelCenters [segP->value].xCapacity <= 0) {
	HUDInitMessage ("Fuelcenter %d is empty.", segP->value);
	return 0;
	}
#endif
if (nMaxFuel <= 0)
	return 0;
if (gameData.app.nGameMode & GM_ENTROPY)
	amount = FixMul (gameData.time.xFrame, I2X (gameData.matCens.xFuelGiveAmount));
else
	amount = FixMul (gameData.time.xFrame, gameData.matCens.xFuelGiveAmount);
if (amount > nMaxFuel)
	amount = nMaxFuel;
if (last_playTime > gameData.time.xGame)
	last_playTime = 0;
if (gameData.time.xGame > last_playTime + FUELCEN_SOUND_DELAY) {
	audio.PlaySound (SOUND_REFUEL_STATION_GIVING_FUEL, SOUNDCLASS_GENERIC, I2X (1) / 2);
	if (IsMultiGame)
		MultiSendPlaySound (SOUND_REFUEL_STATION_GIVING_FUEL, I2X (1) / 2);
	last_playTime = gameData.time.xGame;
	}
//HUDInitMessage ("Fuelcen %d has %d/%d fuel", segP->value,X2I (gameData.matCens.fuelCenters [segP->value].xCapacity),X2I (gameData.matCens.fuelCenters [segP->value].xMaxCapacity));
return amount;
}

//-------------------------------------------------------------
// DM/050904
// Repair centers
// use same values as fuel centers
fix CSegment::Repair (fix nMaxShields)
{
	static fix last_playTime=0;
	fix amount;

if (gameOpts->legacy.bFuelCens)
	return 0;
gameData.matCens.playerSegP = this;
if ((gameData.app.nGameMode & GM_ENTROPY) && ((m_owner < 0) ||
	 ((m_owner > 0) && (m_owner != GetTeam (gameData.multiplayer.nLocalPlayer) + 1))))
	return 0;
if (m_nType != SEGMENT_IS_REPAIRCEN)
	return 0;
if (nMaxShields <= 0) {
	return 0;
}
amount = FixMul (gameData.time.xFrame, I2X (extraGameInfo [IsMultiGame].entropy.nShieldFillRate));
if (amount > nMaxShields)
	amount = nMaxShields;
if (last_playTime > gameData.time.xGame)
	last_playTime = 0;
if (gameData.time.xGame > last_playTime + FUELCEN_SOUND_DELAY) {
	audio.PlaySound (SOUND_REFUEL_STATION_GIVING_FUEL, SOUNDCLASS_GENERIC, I2X (1)/2);
	if (IsMultiGame)
		MultiSendPlaySound (SOUND_REFUEL_STATION_GIVING_FUEL, I2X (1)/2);
	last_playTime = gameData.time.xGame;
	}
return amount;
}

//	--------------------------------------------------------------------------------------------

void DisableMatCens (void)
{
	int	i;

for (i = 0; i < gameData.matCens.nBotCenters; i++) {
	if (gameData.matCens.fuelCenters [i].nType != SEGMENT_IS_EQUIPMAKER) {
		gameData.matCens.fuelCenters [i].bEnabled = 0;
		gameData.matCens.fuelCenters [i].xDisableTime = 0;
		}
	}
}

//	--------------------------------------------------------------------------------------------
//	Initialize all materialization centers.
//	Give them all the right number of lives.
void InitAllMatCens (void)
{
	int	i;
#if DBG
	int	j, nFuelCen;
#endif

for (i = 0; i < gameData.matCens.nFuelCenters; i++)
	if (gameData.matCens.fuelCenters [i].nType == SEGMENT_IS_ROBOTMAKER) {
		 gameData.matCens.fuelCenters [i].nLives = 3;
		 gameData.matCens.fuelCenters [i].bEnabled = 0;
		 gameData.matCens.fuelCenters [i].xDisableTime = 0;
		 }

#if DBG

for (i = 0; i < gameData.matCens.nFuelCenters; i++)
	if (gameData.matCens.fuelCenters [i].nType == SEGMENT_IS_ROBOTMAKER) {
		//	Make sure this fuelcen is pointed at by a matcen.
		for (j = 0; j < gameData.matCens.nBotCenters; j++)
			if (gameData.matCens.botGens [j].nFuelCen == i)
				break;
#	if 1
		if (j == gameData.matCens.nBotCenters)
			j = j;
#	else
		Assert (j != gameData.matCens.nBotCenters);
#	endif
		}
	else if (gameData.matCens.fuelCenters [i].nType == SEGMENT_IS_EQUIPMAKER) {
		//	Make sure this fuelcen is pointed at by a matcen.
		for (j = 0; j < gameData.matCens.nEquipCenters; j++)
			if (gameData.matCens.equipGens [j].nFuelCen == i)
				break;
#	if 1
		if (j == gameData.matCens.nEquipCenters)
			j = j;
#	else
		Assert (j != gameData.matCens.nEquipCenters);
#	endif
		}

//	Make sure all matcens point at a fuelcen
for (i = 0; i < gameData.matCens.nBotCenters; i++) {
	nFuelCen = gameData.matCens.botGens [i].nFuelCen;
	Assert (nFuelCen < gameData.matCens.nFuelCenters);
#	if 1
	if (gameData.matCens.fuelCenters [nFuelCen].nType != SEGMENT_IS_ROBOTMAKER)
		i = i;
#	else
	Assert (gameData.matCens.fuelCenters [nFuelCen].nType == SEGMENT_IS_ROBOTMAKER);
#	endif
	}

for (i = 0; i < gameData.matCens.nEquipCenters; i++) {
	nFuelCen = gameData.matCens.equipGens [i].nFuelCen;
	Assert (nFuelCen < gameData.matCens.nFuelCenters);
#	if 1
	if (gameData.matCens.fuelCenters [nFuelCen].nType != SEGMENT_IS_EQUIPMAKER)
		i = i;
#	else
	Assert (gameData.matCens.fuelCenters [nFuelCen].nType == SEGMENT_IS_EQUIPMAKER);
#	endif
	}
#endif

}

//-----------------------------------------------------------------------------

short flagGoalList [MAX_SEGMENTS_D2X];
short flagGoalRoots [2] = {-1, -1};
short blueFlagGoals = -1;

// create linked lists of all segments with special nType blue and red goal

int GatherFlagGoals (void)
{
	int			h, i, j;
	CSegment*	segP = SEGMENTS.Buffer ();

memset (flagGoalList, 0xff, sizeof (flagGoalList));
for (h = i = 0; i <= gameData.segs.nLastSegment; i++, segP++) {
	if (segP->m_nType == SEGMENT_IS_GOAL_BLUE) {
		j = 0;
		h |= 1;
		}
	else if (segP->m_nType == SEGMENT_IS_GOAL_RED) {
		h |= 2;
		j = 1;
		}
	else
		continue;
	flagGoalList [i] = flagGoalRoots [j];
	flagGoalRoots [j] = i;
	}
return h;
}

//--------------------------------------------------------------------

void MultiSendCaptureBonus (char);

int FlagAtHome (int nFlagId)
{
	int		i, j;
	CObject	*objP;

for (i = flagGoalRoots [nFlagId - POW_BLUEFLAG]; i >= 0; i = flagGoalList [i])
	for (j = SEGMENTS [i].m_objects; j >= 0; j = objP->info.nNextInSeg) {
		objP = OBJECTS + j;
		if ((objP->info.nType == OBJ_POWERUP) && (objP->info.nId == nFlagId))
			return 1;
		}
return 0;
}

//--------------------------------------------------------------------

int CSegment::CheckFlagDrop (int nTeamId, int nFlagId, int nGoalId)
{
if (m_nType != nGoalId)
	return 0;
if (GetTeam (gameData.multiplayer.nLocalPlayer) != nTeamId)
	return 0;
if (!(LOCALPLAYER.flags & PLAYER_FLAGS_FLAG))
	return 0;
if (gameStates.app.bHaveExtraGameInfo [1] && extraGameInfo [1].bEnhancedCTF &&
	 !FlagAtHome ((nFlagId == POW_BLUEFLAG) ? POW_REDFLAG : POW_BLUEFLAG))
	return 0;
MultiSendCaptureBonus (char (gameData.multiplayer.nLocalPlayer));
LOCALPLAYER.flags &= (~(PLAYER_FLAGS_FLAG));
MaybeDropNetPowerup (-1, nFlagId, FORCE_DROP);
return 1;
}

//--------------------------------------------------------------------

void CSegment::CheckForGoal (void)
{
	Assert (gameData.app.nGameMode & GM_CAPTURE);

#if 1
CheckFlagDrop (TEAM_BLUE, POW_REDFLAG, SEGMENT_IS_GOAL_BLUE);
CheckFlagDrop (TEAM_RED, POW_BLUEFLAG, SEGMENT_IS_GOAL_RED);
#else
if (!(LOCALPLAYER.flags & PLAYER_FLAGS_FLAG))
	return;
if (segP->m_nType == SEGMENT_IS_GOAL_BLUE) {
	if (GetTeam (gameData.multiplayer.nLocalPlayer) == TEAM_BLUE) && FlagAtHome (POW_BLUEFLAG)) {
		MultiSendCaptureBonus (gameData.multiplayer.nLocalPlayer);
		LOCALPLAYER.flags &= (~(PLAYER_FLAGS_FLAG);
		MaybeDropNetPowerup (-1, POW_REDFLAG, FORCE_DROP);
		}
	}
else if (segP->m_nType == SEGMENT_IS_GOAL_RED) {
	if (GetTeam (gameData.multiplayer.nLocalPlayer) == TEAM_RED) && FlagAtHome (POW_REDFLAG)) {
		MultiSendCaptureBonus (gameData.multiplayer.nLocalPlayer);
		LOCALPLAYER.flags &= (~(PLAYER_FLAGS_FLAG);
		MaybeDropNetPowerup (-1, POW_BLUEFLAG, FORCE_DROP);
		}
	}
#endif
}

//--------------------------------------------------------------------

void CSegment::CheckForHoardGoal (void)
{
Assert (gameData.app.nGameMode & GM_HOARD);
if (gameStates.app.bPlayerIsDead)
	return;
if ((m_nType != SEGMENT_IS_GOAL_BLUE) && (m_nType != SEGMENT_IS_GOAL_RED))
	return;
if (!LOCALPLAYER.secondaryAmmo [PROXMINE_INDEX])
	return;
#if TRACE
console.printf (CON_DBG,"In orb goal!\n");
#endif
MultiSendOrbBonus ((char) gameData.multiplayer.nLocalPlayer);
LOCALPLAYER.flags &= (~(PLAYER_FLAGS_FLAG));
LOCALPLAYER.secondaryAmmo [PROXMINE_INDEX]=0;
}

//--------------------------------------------------------------------
#if 1//ndef FAST_FILE_IO /*permanently enabled for a reason!*/
/*
 * reads an old_tMatCenInfo structure from a CFile
 */
void OldMatCenInfoRead (old_tMatCenInfo *mi, CFile& cf)
{
mi->objFlags = cf.ReadInt ();
mi->xHitPoints = cf.ReadFix ();
mi->xInterval = cf.ReadFix ();
mi->nSegment = cf.ReadShort ();
mi->nFuelCen = cf.ReadShort ();
}

/*
 * reads a tMatCenInfo structure from a CFile
 */
void MatCenInfoRead (tMatCenInfo *mi, CFile& cf)
{
mi->objFlags [0] = cf.ReadInt ();
mi->objFlags [1] = cf.ReadInt ();
mi->xHitPoints = cf.ReadFix ();
mi->xInterval = cf.ReadFix ();
mi->nSegment = cf.ReadShort ();
mi->nFuelCen = cf.ReadShort ();
}
#endif
