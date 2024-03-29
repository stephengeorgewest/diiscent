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

//#define DOORDBGGING

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#ifndef _WIN32
#	include <unistd.h>
#endif

#include "descent.h"
#include "console.h"
#include "key.h"
#include "menu.h"
#include "physics.h"
#include "error.h"
#include "joy.h"
#include "mono.h"
#include "iff.h"
#include "timer.h"
#include "rendermine.h"
#include "transprender.h"
#include "screens.h"
#include "slew.h"
#include "cockpit.h"
#include "texmap.h"
#include "gameseg.h"
#include "u_mem.h"
#include "light.h"
#include "newdemo.h"
#include "automap.h"
#include "text.h"
#include "network_lib.h"
#include "gamefont.h"
#include "gamepal.h"
#include "kconfig.h"
#include "mouse.h"
#include "playsave.h"
#include "piggy.h"
#include "ai.h"
#include "rbaudio.h"
#include "trigger.h"
#include "ogl_defs.h"
#include "object.h"
#include "rendermine.h"
#include "marker.h"
#include "systemkeys.h"
#include "songs.h"

#if defined (TACTILE)
#	include "tactile.h"
#endif

char *pszPauseMsg = NULL;

//------------------------------------------------------------------------------
//#define TEST_TIMER    1		//if this is set, do checking on timer

#ifdef SDL_INPUT
#ifdef __macosx__
# include <SDL/SDL.h>
#else
# include <SDL.h>
#endif
#endif

//	Function prototypes --------------------------------------------------------

void SpeedtestInit(void);
void SpeedtestFrame(void);
void PlayTestSound(void);

// Functions ------------------------------------------------------------------

#define CONVERTER_RATE  20		//10 units per second xfer rate
#define CONVERTER_SCALE  2		//2 units energy -> 1 unit shields

#define CONVERTER_SOUND_DELAY (I2X (1)/2)		//play every half second

void TransferEnergyToShield (fix time)
{
	fix e;		//how much energy gets transferred
	static fix last_playTime = 0;

if (time <= 0)
	return;
e = min (time * CONVERTER_RATE, LOCALPLAYER.energy - INITIAL_ENERGY);
e = min (e, (MAX_SHIELDS - LOCALPLAYER.shields) * CONVERTER_SCALE);
if (e <= 0) {
	if (LOCALPLAYER.energy <= INITIAL_ENERGY)
		HUDInitMessage (TXT_TRANSFER_ENERGY, X2I(INITIAL_ENERGY));
	else
		HUDInitMessage (TXT_TRANSFER_SHIELDS);
	return;
}

LOCALPLAYER.energy -= e;
LOCALPLAYER.shields += e / CONVERTER_SCALE;
OBJECTS [gameData.multiplayer.nLocalPlayer].ResetDamage ();
MultiSendShields ();
gameStates.app.bUsingConverter = 1;
if (last_playTime > gameData.time.xGame)
	last_playTime = 0;

if (gameData.time.xGame > last_playTime+CONVERTER_SOUND_DELAY) {
	audio.PlaySound (SOUND_CONVERT_ENERGY);
	last_playTime = gameData.time.xGame;
	}
}

//------------------------------------------------------------------------------

void formatTime(char *str, int secs_int)
{
	int h, m, s;

h = secs_int/3600;
s = secs_int%3600;
m = s / 60;
s = s % 60;
sprintf(str, "%1d:%02d:%02d", h, m, s );
}

//------------------------------------------------------------------------------

void PauseGame (void)
{
if (!gameData.app.bGamePaused) {
	gameData.app.bGamePaused = 1;
	audio.PauseAll ();
	rba.Pause ();
	StopTime ();
	paletteManager.DisableEffect ();
	GameFlushInputs ();
#if defined (TACTILE)
	if (TactileStick)
		DisableForces();
#endif
	}
}

//------------------------------------------------------------------------------

void ResumeGame (void)
{
GameFlushInputs ();
paletteManager.EnableEffect ();
StartTime (0);
if (redbook.Playing ())
	rba.Resume ();
audio.ResumeAll ();
gameData.app.bGamePaused = 0;
}

//------------------------------------------------------------------------------

void DoShowNetGameHelp (void);

//Process selected keys until game unpaused. returns key that left pause (p or esc)
int DoGamePause (void)
{
	int			key = 0;
	int			bScreenChanged;
	char			msg [1000];
	char			totalTime [9], xLevelTime [9];

if (gameData.app.bGamePaused) {		//unpause!
	gameData.app.bGamePaused = 0;
	gameStates.app.bEnterGame = 1;
#if defined (TACTILE)
	if (TactileStick)
		EnableForces();
#endif
	return KEY_PAUSE;
	}

if (gameData.app.nGameMode & GM_NETWORK) {
	 DoShowNetGameHelp();
    return (KEY_PAUSE);
	}
else if (gameData.app.nGameMode & GM_MULTI) {
	HUDInitMessage (TXT_MODEM_PAUSE);
	return (KEY_PAUSE);
	}
PauseGame ();
SetPopupScreenMode ();
//paletteManager.ResumeEffect ();
formatTime (totalTime, X2I (LOCALPLAYER.timeTotal) + LOCALPLAYER.hoursTotal * 3600);
formatTime (xLevelTime, X2I (LOCALPLAYER.timeLevel) + LOCALPLAYER.hoursLevel * 3600);
  if (gameData.demo.nState!=ND_STATE_PLAYBACK)
	sprintf (msg, TXT_PAUSE_MSG1, GAMETEXT (332 + gameStates.app.nDifficultyLevel), 
			   LOCALPLAYER.hostages.nOnBoard, xLevelTime, totalTime);
   else
	  	sprintf (msg, TXT_PAUSE_MSG2, GAMETEXT (332 +  gameStates.app.nDifficultyLevel), 
				   LOCALPLAYER.hostages.nOnBoard);

if (!gameOpts->menus.nStyle) {
	gameStates.menus.nInMenu++;
	GameRenderFrame ();
	gameStates.menus.nInMenu--;
	}
messageBox.Show (pszPauseMsg = msg, false);	
GrabMouse (0, 0);
while (gameData.app.bGamePaused) {
	if (!(gameOpts->menus.nStyle && gameStates.app.bGameRunning))
		key = KeyGetChar();
	else {
		gameStates.menus.nInMenu++;
		while (!(key = KeyInKey ())) {
			GameRenderFrame ();
			messageBox.Render ();
			G3_SLEEP (1);
			}
		gameStates.menus.nInMenu--;
		}
#if DBG
		HandleTestKey(key);
#endif
		bScreenChanged = HandleSystemKey (key);
		HandleVRKey (key);
		if (bScreenChanged) {
			GameRenderFrame ();
			messageBox.Render ();
#if 0		
			show_extraViews ();
			if ((gameStates.render.cockpit.nType == CM_FULL_COCKPIT) || (gameStates.render.cockpit.nType == CM_STATUS_BAR))
				RenderGauges();
#endif			
			}
	}
GrabMouse (1, 0);
messageBox.Clear ();
ResumeGame ();
return key;
}

//------------------------------------------------------------------------------
//switch a cockpit window to the next function
int SelectNextWindowFunction (int nWindow)
{
switch (gameStates.render.cockpit.n3DView [nWindow]) {
	case CV_NONE:
		gameStates.render.cockpit.n3DView [nWindow] = CV_REAR;
		break;

	case CV_REAR:
		if (!(gameStates.app.bNostalgia || COMPETITION) && EGI_FLAG (bRadarEnabled, 0, 1, 0) &&
		    (!(gameData.app.nGameMode & GM_MULTI) || (netGame.m_info.gameFlags & NETGAME_FLAG_SHOW_MAP))) {
			gameStates.render.cockpit.n3DView [nWindow] = CV_RADAR_TOPDOWN;
			break;
			}

	case CV_RADAR_TOPDOWN:
		if (!(gameStates.app.bNostalgia || COMPETITION) && EGI_FLAG (bRadarEnabled, 0, 1, 0) &&
		    (!(gameData.app.nGameMode & GM_MULTI) || (netGame.m_info.gameFlags & NETGAME_FLAG_SHOW_MAP))) {
			gameStates.render.cockpit.n3DView [nWindow] = CV_RADAR_HEADSUP;
			break;
			}

	case CV_RADAR_HEADSUP:
		if (FindEscort()) {
			gameStates.render.cockpit.n3DView [nWindow] = CV_ESCORT;
			break;
			}

		//if no ecort, fall through
	case CV_ESCORT:
		gameStates.render.cockpit.nCoopPlayerView [nWindow] = -1;		//force first CPlayerData
		//fall through

	case CV_COOP:
		gameData.marker.viewers [nWindow] = -1;
		if ((gameData.app.nGameMode & GM_MULTI_COOP) || (gameData.app.nGameMode & GM_TEAM)) {
			gameStates.render.cockpit.n3DView [nWindow] = CV_COOP;
			while (1) {
				gameStates.render.cockpit.nCoopPlayerView [nWindow]++;
				if (gameStates.render.cockpit.nCoopPlayerView [nWindow] == gameData.multiplayer.nPlayers) {
					gameStates.render.cockpit.n3DView [nWindow] = CV_MARKER;
					goto case_marker;
					}
				if (gameStates.render.cockpit.nCoopPlayerView [nWindow]==gameData.multiplayer.nLocalPlayer)
					continue;

				if (gameData.app.nGameMode & GM_MULTI_COOP)
					break;
				else if (GetTeam(gameStates.render.cockpit.nCoopPlayerView [nWindow]) == GetTeam(gameData.multiplayer.nLocalPlayer))
					break;
				}
			break;
			}
		//if not multi, fall through
	case CV_MARKER:
	case_marker:;
		if (!IsMultiGame || IsCoopGame || netGame.m_info.bAllowMarkerView) {	//anarchy only
			gameStates.render.cockpit.n3DView [nWindow] = CV_MARKER;
			if (gameData.marker.viewers [nWindow] == -1)
				gameData.marker.viewers [nWindow] = gameData.multiplayer.nLocalPlayer * 3;
			else if (gameData.marker.viewers [nWindow] < gameData.multiplayer.nLocalPlayer * 3 + MaxDrop ())
				gameData.marker.viewers [nWindow]++;
			else
				gameStates.render.cockpit.n3DView [nWindow] = CV_NONE;
		}
		else
			gameStates.render.cockpit.n3DView [nWindow] = CV_NONE;
		break;
	}
SavePlayerProfile();
return 1;	 //bScreenChanged
}

//------------------------------------------------------------------------------
//switch a cockpit window to the next function
int GatherWindowFunctions (int* nWinFuncs)
{
	int	i = 0;

nWinFuncs [i++] = CV_NONE;
if (FindEscort())
	nWinFuncs [i++] = CV_ESCORT;
nWinFuncs [i++] = CV_REAR;
if ((gameData.app.nGameMode & GM_MULTI_COOP) || (gameData.app.nGameMode & GM_TEAM)) 
	nWinFuncs [i++] = CV_COOP;
if (!IsMultiGame || IsCoopGame || netGame.m_info.bAllowMarkerView)
	nWinFuncs [i++] = CV_MARKER;
if (!(gameStates.app.bNostalgia || COMPETITION) && EGI_FLAG (bRadarEnabled, 0, 1, 0) &&
	 (!(gameData.app.nGameMode & GM_MULTI) || (netGame.m_info.gameFlags & NETGAME_FLAG_SHOW_MAP))) {
	nWinFuncs [i++] = CV_RADAR_TOPDOWN;
	nWinFuncs [i++] = CV_RADAR_HEADSUP;
	}
return i;
}

//	Testing functions ----------------------------------------------------------

#if DBG
void SpeedtestInit(void)
{
	gameData.speedtest.nStartTime = TimerGetFixedSeconds();
	gameData.speedtest.bOn = 1;
	gameData.speedtest.nSegment = 0;
	gameData.speedtest.nSide = 0;
	gameData.speedtest.nFrameStart = gameData.app.nFrameCount;
#if TRACE
	console.printf (CON_DBG, "Starting speedtest.  Will be %i frames.  Each . = 10 frames.\n", gameData.segs.nLastSegment+1);
#endif
}

//------------------------------------------------------------------------------

void SpeedtestFrame(void)
{
	CFixVector	view_dir, center_point;

	gameData.speedtest.nSide=gameData.speedtest.nSegment % MAX_SIDES_PER_SEGMENT;

	gameData.objs.viewerP->info.position.vPos = SEGMENTS [gameData.speedtest.nSegment].Center ();
	gameData.objs.viewerP->info.position.vPos[X] += 0x10;	
	gameData.objs.viewerP->info.position.vPos[Y] -= 0x10;	
	gameData.objs.viewerP->info.position.vPos[Z] += 0x17;

	gameData.objs.viewerP->RelinkToSeg (gameData.speedtest.nSegment);
	center_point = SEGMENTS [gameData.speedtest.nSegment].SideCenter (gameData.speedtest.nSide);
	CFixVector::NormalizedDir(view_dir, center_point, gameData.objs.viewerP->info.position.vPos);
	//gameData.objs.viewerP->info.position.mOrient = CFixMatrix::Create(view_dir, NULL, NULL);
	gameData.objs.viewerP->info.position.mOrient = CFixMatrix::CreateF(view_dir);
	if (((gameData.app.nFrameCount - gameData.speedtest.nFrameStart) % 10) == 0) {
#if TRACE
		console.printf (CON_DBG, ".");
#endif
		}
	gameData.speedtest.nSegment++;

	if (gameData.speedtest.nSegment > gameData.segs.nLastSegment) {
		char    msg[128];

		sprintf(msg, TXT_SPEEDTEST, 
			gameData.app.nFrameCount-gameData.speedtest.nFrameStart, 
			X2F(TimerGetFixedSeconds() - gameData.speedtest.nStartTime), 
			(double) (gameData.app.nFrameCount-gameData.speedtest.nFrameStart) / X2F(TimerGetFixedSeconds() - gameData.speedtest.nStartTime));
#if TRACE
		console.printf (CON_DBG, "%s", msg);
#endif
		HUDInitMessage(msg);

		gameData.speedtest.nCount--;
		if (gameData.speedtest.nCount == 0)
			gameData.speedtest.bOn = 0;
		else
			SpeedtestInit();
	}

}
#endif
//------------------------------------------------------------------------------
//eof
