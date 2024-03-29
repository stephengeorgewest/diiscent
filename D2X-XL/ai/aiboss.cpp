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

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "descent.h"
#include "error.h"
#include "gameseg.h"
#include "fireball.h"
#include "network.h"
#include "multibot.h"

#if DBG
#include "string.h"
#include <time.h>
#endif

#define	MAX_SPEW_BOT		3

int spewBots [2][NUM_D2_BOSSES][MAX_SPEW_BOT] = {
 {
 { 38, 40, -1},
 { 37, -1, -1},
 { 43, 57, -1},
 { 26, 27, 58},
 { 59, 58, 54},
 { 60, 61, 54},
 { 69, 29, 24},
 { 72, 60, 73}
	},
 {
 {  3,  -1, -1},
 {  9,   0, -1},
 {255, 255, -1},
 { 26,  27, 58},
 { 59,  58, 54},
 { 60,  61, 54},
 { 69,  29, 24},
 { 72,  60, 73}
	}
};

int	maxSpewBots [NUM_D2_BOSSES] = {2, 1, 2, 3, 3, 3, 3, 3};

// --------------------------------------------------------------------------------------------------------------------
// --------------------------------------------------------------------------------------------------------------------
// --------------------------------------------------------------------------------------------------------------------
//	Return nObject if CObject created, else return -1.
//	If pos == NULL, pick random spot in CSegment.
int CObject::CreateGatedRobot (short nSegment, ubyte nObjId, CFixVector* vPos)
{
	int			nObject, nTries = 5;
	CObject		*objP;
	CSegment		*segP = SEGMENTS + nSegment;
	CFixVector	vObjPos;
	tRobotInfo	*botInfoP = &ROBOTINFO (nObjId);
	int			nBoss, count = 0;
	//int			i;
	fix			objsize = gameData.models.polyModels [0][botInfoP->nModel].Rad ();
	ubyte			default_behavior;

#if !DBG
if (gameStates.gameplay.bNoBotAI)
#endif
	return -1;
nBoss = gameData.bosses.Find (OBJ_IDX (this));
if (nBoss < 0)
	return -1;
if (gameData.time.xGame - gameData.bosses [nBoss].m_nLastGateTime < gameData.bosses [nBoss].m_nGateInterval)
	return -1;
FORALL_ROBOT_OBJS (objP, i)
	if (objP->info.nCreator == BOSS_GATE_MATCEN_NUM)
		count++;
if (count > 2 * gameStates.app.nDifficultyLevel + 6) {
	gameData.bosses [nBoss].m_nLastGateTime = gameData.time.xGame - 3 * gameData.bosses [nBoss].m_nGateInterval / 4;
	return -1;
	}
vObjPos = segP->Center ();
for (;;) {
	if (!vPos)
		vObjPos = segP->RandomPoint ();
	else
		vObjPos = *vPos;

	//	See if legal to place CObject here.  If not, move about in CSegment and try again.
	if (CheckObjectObjectIntersection (&vObjPos, objsize, segP)) {
		if (!--nTries) {
			gameData.bosses [nBoss].m_nLastGateTime = gameData.time.xGame - 3 * gameData.bosses [nBoss].m_nGateInterval / 4;
			return -1;
			}
		vPos = NULL;
		}
	else
		break;
	}
nObject = CreateRobot (nObjId, nSegment, vObjPos);
if (nObject < 0) {
	gameData.bosses [nBoss].m_nLastGateTime = gameData.time.xGame - 3 * gameData.bosses [nBoss].m_nGateInterval / 4;
	return -1;
	}
// added lifetime increase depending on difficulty level 04/26/06 DM
gameData.multigame.create.nObjNums [0] = nObject; // A convenient global to get nObject back to caller for multiplayer
objP = OBJECTS + nObject;
objP->info.xLifeLeft = I2X (30) + (I2X (1) / 2) * (gameStates.app.nDifficultyLevel * 15);	//	Gated in robots only live 30 seconds.
//Set polygon-CObject-specific data
objP->rType.polyObjInfo.nModel = botInfoP->nModel;
objP->rType.polyObjInfo.nSubObjFlags = 0;
//set Physics info
objP->mType.physInfo.mass = botInfoP->mass;
objP->mType.physInfo.drag = botInfoP->drag;
objP->mType.physInfo.flags |= (PF_LEVELLING);
objP->info.xShields = botInfoP->strength;
objP->info.nCreator = BOSS_GATE_MATCEN_NUM;	//	flag this robot as having been created by the boss.
default_behavior = ROBOTINFO (objP->info.nId).behavior;
InitAIObject (objP->Index (), default_behavior, -1);		//	Note, -1 = CSegment this robot goes to to hide, should probably be something useful
/*Object*/CreateExplosion (nSegment, vObjPos, I2X (10), VCLIP_MORPHING_ROBOT);
audio.CreateSegmentSound (gameData.eff.vClips [0][VCLIP_MORPHING_ROBOT].nSound, nSegment, 0, vObjPos, 0 , I2X (1));
objP->MorphStart ();
gameData.bosses [nBoss].m_nLastGateTime = gameData.time.xGame;
LOCALPLAYER.numRobotsLevel++;
LOCALPLAYER.numRobotsTotal++;
return objP->Index ();
}

//	----------------------------------------------------------------------------------------------------------
//	objP points at a boss.  He was presumably just hit and he's supposed to create a bot at the hit location *pos.
int CObject::BossSpewRobot (CFixVector *vPos, short objType, int bObjTrigger)
{
	short			nObject, nSegment, maxRobotTypes;
	short			nBossIndex, nBossId = ROBOTINFO (info.nId).bossFlag;
	tRobotInfo	*pri;

if (!bObjTrigger && FindObjTrigger (OBJ_IDX (this), TT_SPAWN_BOT, -1))
	return -1;
nBossIndex = (nBossId >= BOSS_D2) ? nBossId - BOSS_D2 : nBossId;
Assert ((nBossIndex >= 0) && (nBossIndex < NUM_D2_BOSSES));
nSegment = vPos ? FindSegByPos (*vPos, info.nSegment, 1, 0) : info.nSegment;
if (nSegment == -1) {
#if TRACE
	console.printf (CON_DBG, "Tried to spew a bot outside the mine! Aborting!\n");
#endif
	return -1;
	}
if (!vPos) 
	vPos = &SEGMENTS [nSegment].Center ();
if (objType < 0)
	objType = spewBots [gameStates.app.bD1Mission][nBossIndex][(maxSpewBots [nBossIndex] * d_rand ()) >> 15];
if (objType == 255) {	// spawn an arbitrary robot
	maxRobotTypes = gameData.bots.nTypes [gameStates.app.bD1Mission];
	do {
		objType = d_rand () % maxRobotTypes;
		pri = gameData.bots.info [gameStates.app.bD1Mission] + objType;
		} while (pri->bossFlag ||	//well ... don't spawn another boss, huh? ;)
					pri->companion || //the buddy bot isn't exactly an enemy ... ^_^
					(pri->scoreValue < 700)); //avoid spawning a ... spawn nType bot
	}
nObject = CreateGatedRobot (nSegment, (ubyte) objType, vPos);
//	Make spewed robot come tumbling out as if blasted by a flash missile.
if (nObject != -1) {
	CObject	*newObjP = OBJECTS + nObject;
	int		force_val = I2X (1) / (gameData.time.xFrame ? gameData.time.xFrame : 1);
	if (force_val) {
		newObjP->cType.aiInfo.SKIP_AI_COUNT += force_val;
		newObjP->mType.physInfo.rotThrust[X] = ((d_rand () - 16384) * force_val)/16;
		newObjP->mType.physInfo.rotThrust[Y] = ((d_rand () - 16384) * force_val)/16;
		newObjP->mType.physInfo.rotThrust[Z] = ((d_rand () - 16384) * force_val)/16;
		newObjP->mType.physInfo.flags |= PF_USES_THRUST;

		//	Now, give a big initial velocity to get moving away from boss.
		newObjP->mType.physInfo.velocity = *vPos - info.position.vPos;
		CFixVector::Normalize (newObjP->mType.physInfo.velocity);
		newObjP->mType.physInfo.velocity *= I2X (128);
		}
	}
return nObject;
}

// --------------------------------------------------------------------------------------------------------------------
//	Make CObject objP gate in a robot.
//	The process of him bringing in a robot takes one second.
//	Then a robot appears somewhere near the player.
//	Return nObject if robot successfully created, else return -1
int GateInRobot (short nObject, ubyte nType, short nSegment)
{
if (!gameData.bosses.ToS ())
	return -1;
if (nSegment < 0) {
	int nBoss = gameData.bosses.Find (nObject);
	if (nBoss < 0)
		return -1;
	if (!(gameData.bosses [nBoss].m_gateSegs.Buffer () && gameData.bosses [nBoss].m_nGateSegs))
		return -1;
	nSegment = gameData.bosses [nBoss].m_gateSegs [(d_rand () * gameData.bosses [nBoss].m_nGateSegs) >> 15];
	}
Assert ((nSegment >= 0) && (nSegment <= gameData.segs.nLastSegment));
return OBJECTS [nObject].CreateGatedRobot (nSegment, nType, NULL);
}

// --------------------------------------------------------------------------------------------------------------------

int BossFitsInSeg (CObject *bossObjP, int nSegment)
{
	int			nObject = OBJ_IDX (bossObjP);
	int			nPos;
	CFixVector	vSegCenter, vVertPos;

gameData.collisions.nSegsVisited = 0;
vSegCenter = SEGMENTS [nSegment].Center ();
for (nPos = 0; nPos < 9; nPos++) {
	if (!nPos)
		bossObjP->info.position.vPos = vSegCenter;
	else {
		vVertPos = gameData.segs.vertices [SEGMENTS [nSegment].m_verts [nPos-1]];
		bossObjP->info.position.vPos = CFixVector::Avg(vVertPos, vSegCenter);
		}
	OBJECTS [nObject].RelinkToSeg (nSegment);
	if (!ObjectIntersectsWall (bossObjP))
		return 1;
	}
return 0;
}

// --------------------------------------------------------------------------------------------------------------------

int IsValidTeleportDest (CFixVector *vPos, int nMinDist)
{
	CObject		*objP;
	//int			i;
	CFixVector	vOffs;
	fix			xDist;

FORALL_ACTOR_OBJS (objP, i) {
	vOffs = *vPos - objP->info.position.vPos;
	xDist = vOffs.Mag();
	if (xDist > ((nMinDist + objP->info.xSize) * 3 / 2))
		return 1;
	}
return 0;
}

// --------------------------------------------------------------------------------------------------------------------

void TeleportBoss (CObject *objP)
{
	short			i, nAttempts = 5, nRandSeg = 0, nRandIndex, nObject = objP->Index ();
	CFixVector	vBossDir, vNewPos;

//	Pick a random CSegment from the list of boss-teleportable-to segments.
Assert (nObject >= 0);
i = gameData.bosses.Find (nObject);
if (i < 0)
	return;
// do not teleport if less than 1% of initial shield strength left or drives heavily damaged
if ((objP->Damage () < 0.01) || (d_rand () > objP->DriveDamage ()))
	return;
//Assert (gameData.bosses [i].m_nTeleportSegs > 0);
if (gameData.bosses [i].m_nTeleportSegs <= 0)
	return;
if (gameData.bosses [i].m_nDyingStartTime > 0)
	return;
do {
	nRandIndex = (d_rand () * gameData.bosses [i].m_nTeleportSegs) >> 15;
	nRandSeg = gameData.bosses [i].m_teleportSegs [nRandIndex];
	Assert ((nRandSeg >= 0) && (nRandSeg <= gameData.segs.nLastSegment));
	if (IsMultiGame)
		MultiSendBossActions (nObject, 1, nRandSeg, 0);
	vNewPos = SEGMENTS [nRandSeg].Center ();
	if (IsValidTeleportDest (&vNewPos, objP->info.xSize))
		break;
	}
	while (--nAttempts);
if (!nAttempts)
	return;
OBJECTS [nObject].RelinkToSeg (nRandSeg);
gameData.bosses [i].m_nLastTeleportTime = gameData.time.xGame;
//	make boss point right at CPlayerData
objP->info.position.vPos = vNewPos;
vBossDir = OBJECTS [LOCALPLAYER.nObject].info.position.vPos - vNewPos;
objP->info.position.mOrient = CFixMatrix::CreateF(vBossDir);
audio.CreateSegmentSound (gameData.eff.vClips [0][VCLIP_MORPHING_ROBOT].nSound, nRandSeg, 0, objP->info.position.vPos, 0 , I2X (1));
audio.DestroyObjectSound (nObject);
audio.CreateObjectSound (ROBOTINFO (objP->info.nId).seeSound, SOUNDCLASS_ROBOT, objP->Index (), 1, I2X (1), I2X (512));	//	I2X (512) means play twice as loud
//	After a teleport, boss can fire right away.
gameData.ai.localInfo [nObject].nextPrimaryFire = 0;
gameData.ai.localInfo [nObject].nextSecondaryFire = 0;
}

//	----------------------------------------------------------------------

void StartBossDeathSequence (CObject *objP)
{
if (ROBOTINFO (objP->info.nId).bossFlag) {
	int	nObject = objP->Index (),
			i = gameData.bosses.Find (nObject);

	if (i < 0)
		StartRobotDeathSequence (objP);	//kill it anyway, somehow
	else {
		gameData.bosses [i].m_nDying = nObject;
		gameData.bosses [i].m_nDyingStartTime = gameData.time.xGame;
		}
	}
}

//	----------------------------------------------------------------------

void DoBossDyingFrame (CObject *objP)
{
	int	rval, i = gameData.bosses.Find (objP->Index ());

if (i < 0)
	return;
rval = DoRobotDyingFrame (objP, gameData.bosses [i].m_nDyingStartTime, BOSS_DEATH_DURATION,
								  &gameData.bosses [i].m_bDyingSoundPlaying,
								  ROBOTINFO (objP->info.nId).deathrollSound, I2X (4), I2X (4));
if (rval) {
	gameData.bosses.Remove (i);
	if (ROBOTINFO (objP->info.nId).bEndsLevel)
		DoReactorDestroyedStuff (NULL);
	audio.CreateObjectSound (SOUND_BADASS_EXPLOSION, SOUNDCLASS_EXPLOSION, objP->Index (), 0, I2X (4), I2X (512));
	objP->Explode (I2X (1)/4);
	}
}

// --------------------------------------------------------------------------------------------------------------------
//	Do special stuff for a boss.
void DoBossStuff (CObject *objP, int nTargetVisibility)
{
	int	i, nBossId, nBossIndex, nObject;

nObject = objP->Index ();
i = gameData.bosses.Find (nObject);
if (i < 0)
	return;
nBossId = ROBOTINFO (objP->info.nId).bossFlag;
//	Assert ((nBossId >= BOSS_D2) && (nBossId < BOSS_D2 + NUM_D2_BOSSES));
nBossIndex = (nBossId >= BOSS_D2) ? nBossId - BOSS_D2 : nBossId;
#if DBG
if (objP->info.xShields != gameData.bosses [i].m_xPrevShields) {
#if TRACE
	console.printf (CON_DBG, "Boss shields = %7.3f, CObject %i\n", X2F (objP->info.xShields), objP->Index ());
#endif
	gameData.bosses [i].m_xPrevShields = objP->info.xShields;
	}
#endif
	//	New code, fixes stupid bug which meant boss never gated in robots if > 32767 seconds played.
if (gameData.bosses [i].m_nLastTeleportTime > gameData.time.xGame)
	gameData.bosses [i].m_nLastTeleportTime = gameData.time.xGame;

if (gameData.bosses [i].m_nLastGateTime > gameData.time.xGame)
	gameData.bosses [i].m_nLastGateTime = gameData.time.xGame;

//	@mk, 10/13/95:  Reason:
//		Level 4 boss behind locked door.  But he's allowed to teleport out of there.  So he
//		teleports out of there right away, and blasts player right after first door.
if (!gameData.ai.nTargetVisibility && (gameData.time.xGame - gameData.bosses [i].m_nHitTime > I2X (2)))
	return;

if (bossProps [gameStates.app.bD1Mission][nBossIndex].bTeleports) {
	if (objP->cType.aiInfo.CLOAKED == 1) {
		gameData.bosses [i].m_nHitTime = gameData.time.xGame;	//	Keep the cloak:teleport process going.
		if ((gameData.time.xGame - gameData.bosses [i].m_nCloakStartTime > BOSS_CLOAK_DURATION / 3) &&
			 (gameData.bosses [i].m_nCloakEndTime - gameData.time.xGame > BOSS_CLOAK_DURATION / 3) &&
			 (gameData.time.xGame - gameData.bosses [i].m_nLastTeleportTime > gameData.bosses [i].m_nTeleportInterval)) {
			if (AIMultiplayerAwareness (objP, 98)) {
				TeleportBoss (objP);
				if (bossProps [gameStates.app.bD1Mission][nBossIndex].bSpewBotsTeleport) {
					CFixVector	spewPoint;
					spewPoint = objP->info.position.mOrient.FVec () * (objP->info.xSize * 2);
					spewPoint += objP->info.position.vPos;
					if (bossProps [gameStates.app.bD1Mission][nBossIndex].bSpewMore && (d_rand () > 16384) &&
						 (objP->BossSpewRobot (&spewPoint, -1, 0) != -1))
						gameData.bosses [i].m_nLastGateTime = gameData.time.xGame - gameData.bosses [i].m_nGateInterval - 1;	//	Force allowing spew of another bot.
					objP->BossSpewRobot (&spewPoint, -1, 0);
					}
				}
			}
		else if (gameData.time.xGame - gameData.bosses [i].m_nHitTime > I2X (2)) {
			gameData.bosses [i].m_nLastTeleportTime -= gameData.bosses [i].m_nTeleportInterval/4;
			}
		if (!gameData.bosses [i].m_nCloakDuration)
			gameData.bosses [i].m_nCloakDuration = BOSS_CLOAK_DURATION;
		if ((gameData.time.xGame > gameData.bosses [i].m_nCloakEndTime) ||
			 (gameData.time.xGame < gameData.bosses [i].m_nCloakStartTime) || 
			 (objP->Damage () < 0.01) || (d_rand () > objP->DriveDamage ()))
			objP->cType.aiInfo.CLOAKED = 0;
		}
	else if (((gameData.time.xGame - gameData.bosses [i].m_nCloakEndTime > gameData.bosses [i].m_nCloakInterval) ||
				 (gameData.time.xGame - gameData.bosses [i].m_nCloakEndTime < -gameData.bosses [i].m_nCloakDuration)) && 
				 (objP->Damage () >= 0.01) && (d_rand () <= objP->DriveDamage ())) {
		if (AIMultiplayerAwareness (objP, 95)) {
			gameData.bosses [i].m_nCloakStartTime = gameData.time.xGame;
			gameData.bosses [i].m_nCloakEndTime = gameData.time.xGame + gameData.bosses [i].m_nCloakDuration;
			objP->cType.aiInfo.CLOAKED = 1;
			if (IsMultiGame)
				MultiSendBossActions (objP->Index (), 2, 0, 0);
			}
		}
	}
}

//	---------------------------------------------------------------
// eof

