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
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>

#include "descent.h"
#include "screens.h"
#include "error.h"
#include "newdemo.h"
#include "gamefont.h"
#include "text.h"
#include "network.h"
#include "network_lib.h"
#include "rendermine.h"
#include "transprender.h"
#include "gamepal.h"
#include "gr.h"
#include "ogl_lib.h"
#include "ogl_render.h"
#include "ogl_bitmap.h"
#include "ogl_hudstuff.h"
#include "playsave.h"
#include "hud_defs.h"
#include "cockpit.h"
#include "statusbar.h"
#include "slowmotion.h"
#include "automap.h"
#include "hudmsgs.h"
#include "hudicons.h"
#include "radar.h"
#include "visibility.h"
#include "autodl.h"
#include "key.h"

#define SHOW_PLAYER_IP		0

void DrawGuidedCrosshair (fix xStereoSeparation);
void DrawZoomCrosshair (void);

#if 0
CCanvas *Canv_LeftEnergyGauge;
CCanvas *Canv_AfterburnerGauge;
CCanvas *Canv_RightEnergyGauge;
CCanvas *numericalGaugeCanv;

#define COCKPIT_PRIMARY_BOX		 (!gameStates.video.nDisplayMode ? 0 : 4)
#define COCKPIT_SECONDARY_BOX		 (!gameStates.video.nDisplayMode ? 1 : 5)
#define SB_PRIMARY_BOX				 (!gameStates.video.nDisplayMode ? 2 : 6)
#define SB_SECONDARY_BOX			 (!gameStates.video.nDisplayMode ? 3 : 7)
#endif

CHUD			hudCockpit;
CWideHUD		letterboxCockpit;
CCockpit		fullCockpit;
CStatusBar	statusBarCockpit;
CRearView	rearViewCockpit;

CStack<int>	CGenericCockpit::m_save;

CGenericCockpit* cockpit = &fullCockpit;

//	-----------------------------------------------------------------------------
//	-----------------------------------------------------------------------------
//	-----------------------------------------------------------------------------

void CCockpitHistory::Init (void)
{
score = (IsMultiGame && !IsCoopGame) ? -99 : -1;
energy = -1;
shields = -1;
flags = -1;
bCloak = 0;
lives = -1;
afterburner = -1;
bombCount = 0;
laserLevel = 0;
weapon [0] =
weapon [1] = -1;
ammo [0] =
ammo [1] = -1;
xOmegaCharge  = -1;
}

//	-----------------------------------------------------------------------------
//	-----------------------------------------------------------------------------
//	-----------------------------------------------------------------------------

void CCockpitInfo::Init (void)
{
nCloakFadeState = 0;
bLastHomingWarningDrawn [0] =
bLastHomingWarningDrawn [1] = -1;
addedScore [0] =
addedScore [1] = 0;
scoreTime = 0;
lastWarningBeepTime [0] =
lastWarningBeepTime [1] = 0;
bHaveGaugeCanvases = 0;
nInvulnerableFrame = 0;
weaponBoxStates [0] =
weaponBoxStates [1] = 0;
weaponBoxFadeValues [0] =
weaponBoxFadeValues [1] = 0;
weaponBoxUser [0] =
weaponBoxUser [1] = WBU_WEAPON;
bRebuild = false;
}

//	-----------------------------------------------------------------------------
//	-----------------------------------------------------------------------------
//	-----------------------------------------------------------------------------

void CGenericCockpit::Init (void)
{
m_history [0].Init ();
m_history [1].Init ();
m_info.Init ();
if (!m_save.Buffer ())
	m_save.Create (3);
}

//	-----------------------------------------------------------------------------

CBitmap* CGenericCockpit::BitBlt (int nGauge, int x, int y, bool bScalePos, bool bScaleSize, int scale, int orient, CBitmap* bmP, CBitmap* overrideP)
{
	CBitmap*	saveBmP = NULL;

if (nGauge >= 0) {
#if DBG
	if (nGauge == nDbgGauge)
		nDbgGauge = nDbgGauge;
#endif
	PageInGauge (nGauge);
	bmP = gameData.pig.tex.bitmaps [0] + GaugeIndex (nGauge);
	if (overrideP && overrideP->Buffer ())
		saveBmP = bmP->SetOverride (overrideP);
	}
if (bmP) {
#if 0
	CBitmap* bmoP = bmP->HasOverride ();
	if (bmoP)
		bmP = bmoP;
#endif
	if (bScalePos) {
		x = ScaleX (x);
		y = ScaleY (y);
		}
	int w = bmP->Width ();
	int h = bmP->Height ();
	if (bScaleSize) {
		w = ScaleX (w);
		h = ScaleY (h);
		}
	CCanvas::Current ()->SetColorRGBi (m_info.nColor);
	bmP->RenderScaled (x, y, w * (gameStates.app.bDemoData + 1), h * (gameStates.app.bDemoData + 1), scale, orient, &CCanvas::Current ()->Color ());
	CCanvas::Current ()->SetColorRGBi (BLACK_RGBA);
	if (saveBmP)
		bmP->SetOverride (saveBmP);
	}
return bmP;
}

//	-----------------------------------------------------------------------------

int _CDECL_ CGenericCockpit::PrintF (int *idP, int x, int y, const char *pszFmt, ...)
{
	static char szBuf [1000];
	va_list args;

va_start (args, pszFmt);
vsprintf (szBuf, pszFmt, args);
return GrString ((x < 0) ? -x : ScaleX (x), (y < 0) ? -y : ScaleY (y), szBuf, idP);
}

//------------------------------------------------------------------------------

char* CGenericCockpit::ftoa (char *pszVal, fix f)
{
	int decimal, fractional;

decimal = X2I (f);
fractional = ((f & 0xffff) * 100) / 65536;
if (fractional < 0)
	fractional = -fractional;
if (fractional > 99)
	fractional = 99;
sprintf (pszVal, "%d.%02d", decimal, fractional);
return pszVal;
}

//------------------------------------------------------------------------------

char* CGenericCockpit::Convert1s (char* s)
{
char* p = s;
while ((p = strchr (p, '1')))
	*p = char (132);
return s;
}

//------------------------------------------------------------------------------

#define MAX_MARKER_MESSAGE_LEN 120

void CGenericCockpit::DrawMarkerMessage (void)
{
	char szTemp [MAX_MARKER_MESSAGE_LEN+25];

if (gameData.marker.nDefiningMsg) {
	fontManager.SetColorRGBi (GREEN_RGBA, 1, 0, 0);
   sprintf (szTemp, TXT_DEF_MARKER, gameData.marker.szInput);
	DrawCenteredText (CCanvas::Current ()->Height ()/2-16, szTemp);
   }
}

//------------------------------------------------------------------------------

void CGenericCockpit::DrawMultiMessage (void)
{
	char szMessage [MAX_MULTI_MESSAGE_LEN + 25];

if (IsMultiGame && (gameData.multigame.msg.bSending)) {
	fontManager.SetColorRGBi (GREEN_RGBA, 1, 0, 0);
	sprintf (szMessage, "%s: %s_", TXT_MESSAGE, gameData.multigame.msg.szMsg);
	DrawCenteredText (CCanvas::Current ()->Height () / 2 - 16, szMessage);
	}
if (IsMultiGame && gameData.multigame.msg.bDefining) {
	fontManager.SetColorRGBi (GREEN_RGBA, 1, 0, 0);
	sprintf (szMessage, "%s #%d: %s_", TXT_MACRO, gameData.multigame.msg.bDefining, gameData.multigame.msg.szMsg);
	DrawCenteredText (CCanvas::Current ()->Height () / 2 - 16, szMessage);
	}
}

//------------------------------------------------------------------------------

void CGenericCockpit::DrawCountdown (int y)
{
if (gameStates.app.bEndLevelSequence || !gameData.reactor.bDestroyed  || (gameData.reactor.countdown.nSecsLeft < 0))
	return;

if (!IS_D2_OEM && !IS_MAC_SHARE && !IS_SHAREWARE) {    // no countdown on registered only
	//	On last level, we don't want a countdown.
	if ((gameData.missions.nCurrentMission == gameData.missions.nBuiltinMission) &&
		(gameData.missions.nCurrentLevel == gameData.missions.nLastLevel)) {
		if (!IsMultiGame)
			return;
		if (gameData.app.nGameMode & GM_MULTI_ROBOTS)
			return;
			}
		}
fontManager.Push ();
fontManager.SetCurrent (SMALL_FONT);
fontManager.SetColorRGBi (GREEN_RGBA, 1, 0, 0);
GrPrintF (NULL, 0x8000, y, "T-%d s", gameData.reactor.countdown.nSecsLeft);
fontManager.Pop ();
}

//------------------------------------------------------------------------------

void CGenericCockpit::DrawCruise (int x, int y)
{
if (!gameStates.render.bRearView &&
	 (gameStates.input.nCruiseSpeed > 0) &&
	 (gameData.multiplayer.nLocalPlayer > -1) &&
	 (gameData.objs.viewerP->info.nType == OBJ_PLAYER) &&
	 (gameData.objs.viewerP->info.nId == gameData.multiplayer.nLocalPlayer))
	GrPrintF (NULL, x, y, "%s %2d%%", TXT_CRUISE, X2I (gameStates.input.nCruiseSpeed));
}

//------------------------------------------------------------------------------

void CGenericCockpit::DrawRecording (int y)
{
if ((gameData.demo.nState == ND_STATE_PLAYBACK) || (gameData.demo.nState == ND_STATE_RECORDING)) {
#if 0
	char message [128];
	int h, w, aw;

	if (gameData.demo.nState == ND_STATE_PLAYBACK) {
		if (gameData.demo.nVcrState != ND_STATE_PRINTSCREEN) {
			sprintf (message, "%s (%d%%%% %s)",
						gameOpts->demo.bRevertFormat ? TXT_DEMO_CONVERSION : TXT_DEMO_PLAYBACK, int (NDGetPercentDone ()), TXT_DONE);
			}
		else {
			sprintf (message, " ");
			}
		}
	else
		sprintf (message, TXT_DEMO_RECORDING);
	fontManager.SetColorRGBi (RGBA_PAL2 (27, 0, 0), 1, 0, 0);
	fontManager.Current ()->StringSize (message, w, h, aw);
	GrPrintF (NULL, (CCanvas::Current ()->Width () - w) / 2, CCanvas::Current ()->Height () - y - h - 2, message);
#else
	if (gameData.demo.nState == ND_STATE_PLAYBACK) {
		CCanvas::Current ()->SetColorRGB (PAL2RGBA (27), PAL2RGBA (0), PAL2RGBA (0), 255);
		int x = CCanvas::Current ()->Width () / 3;
		int h = int (8 * GLfloat (screen.Height ()) / 480.0f + 0.5f);
		y = CCanvas::Current ()->Height () - y - 2;
		CCanvas::Current ()->SetColorRGB (255, 0, 0, 200);
		OglDrawFilledRect (x, y - h, x + int (NDGetPercentDone () * float (x) / 100.0f), y);
		glLineWidth (GLfloat (screen.Width ()) / 640.0f);
		CCanvas::Current ()->SetColorRGB (255, 0, 0, 100);
		OglDrawEmptyRect (x, y - h, 2 * x, y);
		glLineWidth (1);
		}
	else {
		char message [128];
		int h, w, aw;

		sprintf (message, TXT_DEMO_RECORDING);
		fontManager.SetColorRGBi (RGBA_PAL2 (27, 0, 0), 1, 0, 0);
		fontManager.Current ()->StringSize (message, w, h, aw);
		GrPrintF (NULL, (CCanvas::Current ()->Width () - w) / 2, CCanvas::Current ()->Height () - y - h - 2, message);
		}
#endif
	}
}

//------------------------------------------------------------------------------

void CGenericCockpit::DrawPacketLoss (void)
{
	static int nIdPacketLoss = 0;

if (IsMultiGame && networkData.nTotalMissedPackets && !automap.Display ()) {
		char	szLoss [50];
		int	w, h, aw; // position measured from lower right corner
		int	nLossRate = (1000 * networkData.nTotalMissedPackets) / (networkData.nTotalPacketsGot + networkData.nTotalMissedPackets);

	if (nLossRate > 9) {
		if (nLossRate >= 300)
			fontManager.SetColorRGBi (RED_RGBA, 1, 0, 0);
		else if (nLossRate >= 200)
			fontManager.SetColorRGBi (ORANGE_RGBA, 1, 0, 0);
		else if (nLossRate >= 100)
			fontManager.SetColorRGBi (GOLD_RGBA, 1, 0, 0);
		else if (nLossRate)
			fontManager.SetColorRGBi (GREEN_RGBA, 1, 0, 0);
		sprintf (szLoss, "packet loss: %d.%d%c%c", nLossRate / 10, nLossRate % 10, '%', '%');
		fontManager.Current ()->StringSize (szLoss, w, h, aw);
		nIdPacketLoss = GrPrintF (&nIdPacketLoss,
										 CCanvas::Current ()->Width () - w - LHX (1),
										 CCanvas::Current ()->Height () - 8 * (GAME_FONT->Height () + GAME_FONT->Height () / 4),
										 szLoss);
		}
	}
}

//------------------------------------------------------------------------------

void CGenericCockpit::DrawFrameRate (void)
{
	static fix frameTimeList [8] = {0, 0, 0, 0, 0, 0, 0, 0};
	static fix frameTimeTotal = 0;
	static int frameTimeCounter = 0;
	static int nIdFrameRate = 0;

if (gameStates.render.bShowFrameRate) {
		static time_t t, t0 = -1;

		char	szItem [50];
		int	x = 11, y; // position measured from lower right corner

	frameTimeTotal += gameData.time.xRealFrame - frameTimeList [frameTimeCounter];
	frameTimeList [frameTimeCounter] = gameData.time.xRealFrame;
	frameTimeCounter = (frameTimeCounter + 1) % 8;
	if (gameStates.render.bShowFrameRate == 1) {
		static fix xRate = 0;

		char szRate [20];
		t = SDL_GetTicks ();
		if ((t0 < 0) || (t - t0 >= 500)) {
			t0 = t;
			xRate = frameTimeTotal ? FixDiv (I2X (1) * 8, frameTimeTotal) : 0;
			}
		sprintf (szItem, "FPS: %s", ftoa (szRate, xRate));
		}
	else if (gameStates.render.bShowFrameRate == 2) {
		sprintf (szItem, "Faces: %d ", gameData.render.nTotalFaces);
		}
	else if (gameStates.render.bShowFrameRate == 3) {
		sprintf (szItem, "Transp: %d ", transparencyRenderer.ItemCount ());
		}
	else if (gameStates.render.bShowFrameRate == 4) {
		sprintf (szItem, "Objects: %d/%d ", gameData.render.nTotalObjects, gameData.render.nTotalSprites);
		}
	else if (gameStates.render.bShowFrameRate == 5) {
		sprintf (szItem, "States: %d/%d ", gameData.render.nStateChanges, gameData.render.nShaderChanges);
		}
	else if (gameStates.render.bShowFrameRate == 6) {
		sprintf (szItem, "Lights/Face: %1.2f (%d)",
					(float) gameData.render.nTotalLights / (float) gameData.render.nTotalFaces,
					gameData.render.nMaxLights);
		x = 19;
		}
	if (automap.Display ())
		y = 2;
	else if (IsMultiGame)
		y = 7;
	else
		y = 6;
	fontManager.SetColorRGBi (ORANGE_RGBA, 1, 0, 0);
	nIdFrameRate = GrPrintF (&nIdFrameRate,
									 CCanvas::Current ()->Width () - (x * GAME_FONT->Width ()),
									 CCanvas::Current ()->Height () - y * (GAME_FONT->Height () + GAME_FONT->Height () / 4),
									 szItem);
	}
}

//	-----------------------------------------------------------------------------

void CGenericCockpit::DrawSlowMotion (void)
{
if (Hide ())
	return;
if (gameStates.render.bRearView)
	return;
if ((LOCALPLAYER.energy <= I2X (10)) || !(LOCALPLAYER.flags & PLAYER_FLAGS_SLOWMOTION))
	return;

	char	szScore [40];
	int	w, h, aw;

if ((gameData.hud.msgs [0].nMessages > 0) &&
	 (strlen (gameData.hud.msgs [0].szMsgs [gameData.hud.msgs [0].nFirst]) > 38))
	return;
szScore [0] = (char) 1;
szScore [1] = (char) (127 + 128);
szScore [2] = (char) (127 + 128);
szScore [3] = (char) (0 + 128);
#if !DBG
if (!SlowMotionActive ())
	strcpy (szScore + 4, "          ");
else
#endif
	sprintf (szScore + 4, "M%1.1f S%1.1f ",
				gameStates.gameplay.slowmo [0].fSpeed,
				gameStates.gameplay.slowmo [1].fSpeed);
szScore [14] = (char) 1;
szScore [15] = (char) (0 + 128);
szScore [16] = (char) (127 + 128);
szScore [17] = (char) (0 + 128);
szScore [18] = (char) (0);
fontManager.Current ()->StringSize (szScore, w, h, aw);
fontManager.SetColorRGBi (GREEN_RGBA, 1, 0, 0);
GrPrintF (NULL, CCanvas::Current ()->Width () - 2 * w - LHX (2), 3, szScore);
}

//	-----------------------------------------------------------------------------

void CGenericCockpit::DrawPlayerStats (void)
{
if (Hide ())
	return;
if (gameStates.render.bRearView)
	return;

	int		h, w, aw, y;
	double	p [3], s [3];
	char		szStats [50];

	static int nIdStats = 0;

if (!gameOpts->render.cockpit.bPlayerStats)
	return;
fontManager.SetColorRGBi (ORANGE_RGBA, 1, 0, 0);
if (gameStates.render.cockpit.nType == CM_STATUS_BAR)
	y = 6 + m_info.nLineSpacing;
else
	y = 6 + 2 * m_info.nLineSpacing;
h = (gameData.stats.nDisplayMode - 1) / 2;
if ((gameData.stats.nDisplayMode - 1) % 2 == 0) {
	sprintf (szStats, "%s%d-%d %d-%d %d-%d",
				h ? "T:" : "",
				gameData.stats.player [h].nHits [0],
				gameData.stats.player [h].nMisses [0],
				gameData.stats.player [h].nHits [1],
				gameData.stats.player [h].nMisses [1],
				gameData.stats.player [h].nHits [0] + gameData.stats.player [h].nHits [1],
				gameData.stats.player [h].nMisses [0] + gameData.stats.player [h].nMisses [1]);
	}
else {
	s [0] = gameData.stats.player [h].nHits [0] + gameData.stats.player [h].nMisses [0];
	s [1] = gameData.stats.player [h].nHits [1] + gameData.stats.player [h].nMisses [1];
	s [2] = s [0] + s [1];
	p [0] = s [0] ? (gameData.stats.player [h].nHits [0] / s [0]) * 100 : 0;
	p [1] = s [1] ? (gameData.stats.player [h].nHits [1] / s [1]) * 100 : 0;
	p [2] = s [2] ? ((gameData.stats.player [h].nHits [0] + gameData.stats.player [h].nHits [1]) / s [2]) * 100 : 0;
	sprintf (szStats, "%s%1.1f%c %1.1f%c %1.1f%c", h ? "T:" : "", p [0], '%', p [1], '%', p [2], '%');
	}
fontManager.Current ()->StringSize (szStats, w, h, aw);
int x = CCanvas::Current ()->Width () - w - LHX (2);
if ((extraGameInfo [0].nWeaponIcons >= 3) && (CCanvas::Current ()->Height () < 670))
	x -= HUD_LHX (20);
nIdStats = GrString (x, y, szStats, &nIdStats);
}

//	-----------------------------------------------------------------------------

void CGenericCockpit::DrawTime (void)
{
if (Hide ())
	return;

if (gameStates.render.bShowTime && !(IsMultiGame && gameData.multigame.kills.bShowList)) {
		int secs = X2I (LOCALPLAYER.timeLevel) % 60;
		int mins = X2I (LOCALPLAYER.timeLevel) / 60;

		static int nIdTime = 0;

	fontManager.SetColorRGBi (GREEN_RGBA, 1, 0, 0);
	nIdTime = GrPrintF (&nIdTime, CCanvas::Current ()->Width () - 4 * m_info.fontWidth,
							  CCanvas::Current ()->Height () - 4 * m_info.nLineSpacing,
							  "%d:%02d", mins, secs);
	}
}

//	-----------------------------------------------------------------------------

void CGenericCockpit::DrawTimerCount (void)
{
if (Hide ())
	return;

	char	szScore [20];
	int	w, h, aw, i;
	fix	timevar = 0;

	static int nIdTimer = 0;

if ((gameData.hud.msgs [0].nMessages > 0) && (strlen (gameData.hud.msgs [0].szMsgs [gameData.hud.msgs [0].nFirst]) > 38))
	return;

if ((gameData.app.nGameMode & GM_NETWORK) && (timevar = I2X (netGame.GetPlayTimeAllowed () * 5 * 60))) {
	i = X2I (timevar - gameStates.app.xThisLevelTime) + 1;
	sprintf (szScore, "T - %5d", i);
	fontManager.Current ()->StringSize (szScore, w, h, aw);
	fontManager.SetColorRGBi (GREEN_RGBA, 1, 0, 0);
	if ((i >= 0) && !gameData.reactor.bDestroyed)
		nIdTimer = GrPrintF (&nIdTimer, CCanvas::Current ()->Width () - w - LHX (10), LHX (11), szScore);
	}
}

//	-----------------------------------------------------------------------------

void CGenericCockpit::DrawFlag (int x, int y)
{
if ((gameData.app.nGameMode & GM_CAPTURE) && (LOCALPLAYER.flags & PLAYER_FLAGS_FLAG))
	BitBlt ((GetTeam (gameData.multiplayer.nLocalPlayer) == TEAM_BLUE) ? FLAG_ICON_RED : FLAG_ICON_BLUE, x, y, false, false);
}

//	-----------------------------------------------------------------------------

void CGenericCockpit::DrawOrbs (int x, int y)
{
if (Hide ())
	return;

	static int nIdOrbs = 0, nIdEntropy [2] = {0, 0};

if (gameData.app.nGameMode & (GM_HOARD | GM_ENTROPY)) {
	CBitmap* bmP = BitBlt (-1, x, y, false, false, I2X (1), 0, &gameData.hoard.icon [gameStates.render.fonts.bHires].bm);
	//GrUBitmapM (x, y, bmP);

	x += 3 * bmP->Width () / 2;
	y += gameStates.render.fonts.bHires + 1;
	if (gameData.app.nGameMode & GM_ENTROPY) {
		char	szInfo [20];
		int	w, h, aw;
		if (LOCALPLAYER.secondaryAmmo [PROXMINE_INDEX] >= extraGameInfo [1].entropy.nCaptureVirusLimit)
			fontManager.SetColorRGBi (ORANGE_RGBA, 1, 0, 0);
		else
			fontManager.SetColorRGBi (GREEN_RGBA, 1, 0, 0);
		sprintf (szInfo,
			"x %d [%d]",
			LOCALPLAYER.secondaryAmmo [PROXMINE_INDEX],
			LOCALPLAYER.secondaryAmmo [SMARTMINE_INDEX]);
		nIdEntropy [0] = GrPrintF (nIdEntropy, x, y, szInfo);
		if (gameStates.entropy.bConquering) {
			int t = (extraGameInfo [1].entropy.nCaptureTimeLimit * 1000) -
					   (gameStates.app.nSDLTicks - gameStates.entropy.nTimeLastMoved);

			if (t < 0)
				t = 0;
			fontManager.Current ()->StringSize (szInfo, w, h, aw);
			x += w;
			fontManager.SetColorRGBi (RED_RGBA, 1, 0, 0);
			sprintf (szInfo, " %d.%d", t / 1000, (t % 1000) / 100);
			nIdEntropy [1] = GrPrintF (nIdEntropy + 1, x, y, szInfo);
			}
		}
	else
		nIdOrbs = GrPrintF (&nIdOrbs, x, y, "x %d", LOCALPLAYER.secondaryAmmo [PROXMINE_INDEX]);
	}
}

//	-----------------------------------------------------------------------------

void CGenericCockpit::PlayHomingWarning (void)
{
	fix	xBeepDelay;

if (gameStates.app.bEndLevelSequence || gameStates.app.bPlayerIsDead)
	return;
if (LOCALPLAYER.homingObjectDist >= 0) {
	xBeepDelay = LOCALPLAYER.homingObjectDist/128;
	if (xBeepDelay > I2X (1))
		xBeepDelay = I2X (1);
	else if (xBeepDelay < I2X (1)/8)
		xBeepDelay = I2X (1)/8;
	if (m_info.lastWarningBeepTime [gameStates.render.vr.nCurrentPage] > gameData.time.xGame)
		m_info.lastWarningBeepTime [gameStates.render.vr.nCurrentPage] = 0;
	if (gameData.time.xGame - m_info.lastWarningBeepTime [gameStates.render.vr.nCurrentPage] > xBeepDelay/2) {
		audio.PlaySound (SOUND_HOMING_WARNING);
		m_info.lastWarningBeepTime [gameStates.render.vr.nCurrentPage] = gameData.time.xGame;
		}
	}
}

//	-----------------------------------------------------------------------------

void CGenericCockpit::DrawBombCount (int x, int y, int bgColor, int bShowAlways)
{
	int bomb, nBombs;
	char szBombCount [5], *t;

	static int nIdBombCount = 0;

if (gameData.app.nGameMode & GM_ENTROPY)
	return;
bomb = ArmedBomb ();
if (!bomb)
	return;
if ((gameData.app.nGameMode & GM_HOARD) && (bomb == PROXMINE_INDEX))
	return;
nBombs = LOCALPLAYER.secondaryAmmo [bomb];
if (!(bShowAlways || nBombs))		//no bombs, draw nothing on HUD
	return;
#if DBG
nBombs = min (nBombs, 99);	//only have room for 2 digits - cheating give 200
#endif
sprintf (szBombCount, "B:%02d", nBombs);
for (t = szBombCount; *t; t++)
	if (*t == '1')
		*t = '\x84';	//convert to wide '1'
m_history [gameStates.render.vr.nCurrentPage].bombCount = (bomb == PROXMINE_INDEX) ? nBombs : -nBombs;
//ClearBombCount (bgColor);
if ((gameStates.render.cockpit.nType == CM_FULL_COCKPIT) || (gameStates.render.cockpit.nType == CM_STATUS_BAR))
	fontManager.SetScale (floor (float (CCanvas::Current ()->Width ()) / 640.0f));
nIdBombCount = DrawBombCount (nIdBombCount, x, y, nBombs ? (bomb == PROXMINE_INDEX) ? RED_RGBA : GOLD_RGBA : GREEN_RGBA, szBombCount);
fontManager.SetScale (1.0f);
}

//	-----------------------------------------------------------------------------

void CGenericCockpit::DrawAmmoInfo (int x, int y, int ammoCount, int bPrimary)
{
	int	w;
	char	szAmmo [16];

	static int nIdAmmo [2] = {0, 0};

w = (m_info.fontWidth * (bPrimary ? 7 : 5)) / 2;
CCanvas::Current ()->SetColorRGBi (RGB_PAL (0, 0, 0));
//OglDrawFilledRect (ScaleX (x), ScaleY (y), ScaleX (x+w), ScaleY (y + m_info.fontHeight));
fontManager.SetColorRGBi (RED_RGBA, 1, 0, 0);
sprintf (szAmmo, "%03d", ammoCount);
Convert1s (szAmmo);
nIdAmmo [bPrimary] = PrintF (&nIdAmmo [bPrimary], x, y, szAmmo);
}

//	-----------------------------------------------------------------------------

void CGenericCockpit::CheckForExtraLife (int nPrevScore)
{
if (LOCALPLAYER.score / EXTRA_SHIP_SCORE != nPrevScore / EXTRA_SHIP_SCORE) {
	LOCALPLAYER.lives += LOCALPLAYER.score / EXTRA_SHIP_SCORE - nPrevScore / EXTRA_SHIP_SCORE;
	PowerupBasic (20, 20, 20, 0, TXT_EXTRA_LIFE);
	short nSound = gameData.objs.pwrUp.info [POW_EXTRA_LIFE].hitSound;
	if (nSound > -1)
		audio.PlaySound (nSound);
	}
}

//	-----------------------------------------------------------------------------

void CGenericCockpit::AddPointsToScore (int points)
{
	int nPrevScore;

m_info.scoreTime += I2X (1) * 2;
cockpit->AddScore (0, points);
cockpit->AddScore (1, points);
if (m_info.scoreTime > I2X (1) * 4)
	m_info.scoreTime = I2X (1) * 4;
if (!points || gameStates.app.cheats.bEnabled)
	return;
if (IsMultiGame && !IsCoopGame)
	return;
nPrevScore = LOCALPLAYER.score;
LOCALPLAYER.score += points;
if (gameData.demo.nState == ND_STATE_RECORDING)
	NDRecordPlayerScore (points);
if (IsCoopGame)
	MultiSendScore ();
if (!IsMultiGame)
	CheckForExtraLife (nPrevScore);
}

//	-----------------------------------------------------------------------------

void CGenericCockpit::AddBonusPointsToScore (int points)
{
	int nPrevScore;

if (!points || gameStates.app.cheats.bEnabled)
	return;
nPrevScore = LOCALPLAYER.score;
LOCALPLAYER.score += points;
if (gameData.demo.nState == ND_STATE_RECORDING)
	NDRecordPlayerScore (points);
if (!IsMultiGame)
	CheckForExtraLife (nPrevScore);
}

//	-----------------------------------------------------------------------------

void CGenericCockpit::DrawPlayerShip (int nCloakState, int nOldCloakState, int x, int y)
{
	static fix xCloakFadeTimer = 0;
	static int nCloakFadeValue = FADE_LEVELS - 1;

if (nCloakState) {
	if (nOldCloakState == -1)
		nCloakFadeValue = 0;
	else if (!nOldCloakState)
		m_info.nCloakFadeState = -1;
	}
else {
	nCloakFadeValue = FADE_LEVELS - 1;
	m_info.nCloakFadeState = 0;
	}
if (nCloakState && (LOCALPLAYER.cloakTime != 0x7fffffff) && (gameData.time.xGame > LOCALPLAYER.cloakTime + CLOAK_TIME_MAX - I2X (3)))	{	//doing "about-to-uncloak" effect
	nCloakState = 2;
	if (!m_info.nCloakFadeState)
		m_info.nCloakFadeState = 2;
	}
if (m_info.nCloakFadeState)
	xCloakFadeTimer -= gameData.time.xFrame;
while (m_info.nCloakFadeState && (xCloakFadeTimer < 0)) {
	xCloakFadeTimer += CLOAK_FADE_WAIT_TIME;
	nCloakFadeValue += m_info.nCloakFadeState;
	if (nCloakFadeValue >= FADE_LEVELS - 1) {
		nCloakFadeValue = FADE_LEVELS - 1;
		if (nCloakState && (m_info.nCloakFadeState == 2))
			m_info.nCloakFadeState = -2;
		else
			m_info.nCloakFadeState = 0;
		}
	else if (nCloakFadeValue <= 0) {
		nCloakFadeValue = 0;
		if ((nCloakState == 2) && (m_info.nCloakFadeState == -2))
			m_info.nCloakFadeState = 2;
		else
			m_info.nCloakFadeState = 0;
		}
	}

#if 0
if (gameStates.render.cockpit.nType != CM_FULL_COCKPIT)
	CCanvas::SetCurrent (&gameStates.render.vr.buffers.render [0]);
#endif
m_info.nColor = RGBA (255, 255, 255, int (float (nCloakFadeValue) / float (FADE_LEVELS) * 255));
BitBlt (GAUGE_SHIPS + (IsTeamGame ? GetTeam (gameData.multiplayer.nLocalPlayer) : gameData.multiplayer.nLocalPlayer % MAX_PLAYER_COLORS), x, y);
m_info.nColor = WHITE_RGBA;
gameStates.render.grAlpha = 1.0f;
#if 0
if (gameStates.render.cockpit.nType != CM_FULL_COCKPIT)
	CCanvas::SetCurrent (CurrentGameScreen ());
#endif
}

//	-----------------------------------------------------------------------------

void CGenericCockpit::DrawWeaponInfo (int nWeaponType, int nIndex, tGaugeBox* box, int xPic, int yPic, const char *pszName, int xText, int yText, int orient)
{
	CBitmap*	bmP;
	char		szName [100], *p;
	int		i, color;

	static int nIdWeapon [2][3] = {{0, 0, 0},{0, 0, 0}}, nIdLaser [2] = {0, 0};

i = ((gameData.pig.tex.nHamFileVersion >= 3) && gameStates.video.nDisplayMode)
	 ? gameData.weapons.info [nIndex].hiresPicture.index
	 : gameData.weapons.info [nIndex].picture.index;
LoadBitmap (i, 0);
if (!(bmP = gameData.pig.tex.bitmaps [0] + i))
	return;
color = (m_info.weaponBoxStates [nWeaponType] == WS_SET) ? 255 : int (X2F (m_info.weaponBoxFadeValues [nWeaponType]) / float (FADE_LEVELS) * 255);
m_info.nColor = RGBA (color, color, color, 255);
BitBlt (-1, xPic, yPic, true, true, (gameStates.render.cockpit.nType == CM_FULL_SCREEN) ? I2X (2) : I2X (1), orient, bmP);
m_info.nColor = WHITE_RGBA;
CCanvas::Current ()->SetColorRGB (255, 255, 255, 255);
fontManager.SetColorRGBi (GREEN_RGBA, 1, 0, 0);
if ((p = const_cast<char*> (strchr (pszName, '\n')))) {
	memcpy (szName, pszName, i = int (p - pszName));
	szName [i] = '\0';
	nIdWeapon [nWeaponType][0] = PrintF (&nIdWeapon [nWeaponType][0], xText, yText, szName);
	nIdWeapon [nWeaponType][1] = PrintF (&nIdWeapon [nWeaponType][1], xText, yText + m_info.fontHeight + int (fontManager.Scale ()), p + 1);
	}
else {
	nIdWeapon [nWeaponType][2] = PrintF (&nIdWeapon [nWeaponType][2], xText, yText, pszName);
	}

//	For laser, show level and quadness
if (nIndex == LASER_ID || nIndex == SUPERLASER_ID) {
	sprintf (szName, "%s: 0", TXT_LVL);
	szName [5] = LOCALPLAYER.laserLevel + 1 + '0';
	nIdLaser [0] = PrintF (&nIdLaser [0], xText, yText + m_info.nLineSpacing, szName);
	if (LOCALPLAYER.flags & PLAYER_FLAGS_QUAD_LASERS) {
		strcpy (szName, TXT_QUAD);
		nIdLaser [nWeaponType] = PrintF (&nIdLaser [nWeaponType], xText, yText + 2 * m_info.nLineSpacing, szName);
		}
	}
}

//	-----------------------------------------------------------------------------

void CGenericCockpit::DrawWeaponInfo (int nWeaponType, int nWeaponId, int laserLevel)
{
	int nIndex;

if (nWeaponType == 0) {
	nIndex = primaryWeaponToWeaponInfo [nWeaponId];

	if (nIndex == LASER_ID && laserLevel > MAX_LASER_LEVEL)
		nIndex = SUPERLASER_ID;

	if (gameStates.render.cockpit.nType == CM_STATUS_BAR)
		DrawWeaponInfo (nWeaponType, nIndex,
			hudWindowAreas + SB_PRIMARY_BOX,
			SB_PRIMARY_W_PIC_X, SB_PRIMARY_W_PIC_Y,
			PRIMARY_WEAPON_NAMES_SHORT (nWeaponId),
			SB_PRIMARY_W_TEXT_X, SB_PRIMARY_W_TEXT_Y, 0);
	else
		DrawWeaponInfo (nWeaponType, nIndex,
			hudWindowAreas + COCKPIT_PRIMARY_BOX,
			PRIMARY_W_PIC_X, PRIMARY_W_PIC_Y,
			PRIMARY_WEAPON_NAMES_SHORT (nWeaponId),
			PRIMARY_W_TEXT_X, PRIMARY_W_TEXT_Y, 0);
		}
else {
	nIndex = secondaryWeaponToWeaponInfo [nWeaponId];

	if (gameStates.render.cockpit.nType == CM_STATUS_BAR)
		DrawWeaponInfo (nWeaponType, nIndex,
			hudWindowAreas + SB_SECONDARY_BOX,
			SB_SECONDARY_W_PIC_X, SB_SECONDARY_W_PIC_Y,
			SECONDARY_WEAPON_NAMES_SHORT (nWeaponId),
			SB_SECONDARY_W_TEXT_X, SB_SECONDARY_W_TEXT_Y, 0);
	else
		DrawWeaponInfo (nWeaponType, nIndex,
			hudWindowAreas + COCKPIT_SECONDARY_BOX,
			SECONDARY_W_PIC_X, SECONDARY_W_PIC_Y,
			SECONDARY_WEAPON_NAMES_SHORT (nWeaponId),
			SECONDARY_W_TEXT_X, SECONDARY_W_TEXT_Y, 0);
	}
}

//	-----------------------------------------------------------------------------

//returns true if drew picture
int CGenericCockpit::DrawWeaponDisplay (int nWeaponType, int nWeaponId)
{
int bLaserLevelChanged = ((nWeaponType == 0) &&
								  (nWeaponId == LASER_INDEX) &&
								  ((LOCALPLAYER.laserLevel != m_history [gameStates.render.vr.nCurrentPage].laserLevel)));

if ((m_info.weaponBoxStates [nWeaponType] == WS_SET) &&
	 ((nWeaponId != m_history [gameStates.render.vr.nCurrentPage].weapon [nWeaponType]) || bLaserLevelChanged)) {
	m_info.weaponBoxStates [nWeaponType] = WS_FADING_OUT;
	m_info.weaponBoxFadeValues [nWeaponType] = I2X (FADE_LEVELS - 1);
	}

if (m_info.weaponBoxStates [nWeaponType] == WS_FADING_OUT) {
	if (m_history [gameStates.render.vr.nCurrentPage].weapon [nWeaponType] < 0)
		m_info.weaponBoxFadeValues [nWeaponType] = 0;
	else {
		DrawWeaponInfo (nWeaponType,
							 m_history [gameStates.render.vr.nCurrentPage].weapon [nWeaponType],
							 m_history [gameStates.render.vr.nCurrentPage].laserLevel);
		m_history [gameStates.render.vr.nCurrentPage].ammo [nWeaponType] = -1;
		m_history [gameStates.render.vr.nCurrentPage].xOmegaCharge = -1;
		m_info.weaponBoxFadeValues [nWeaponType] -= gameData.time.xFrame * FADE_SCALE;
		}
	if (m_info.weaponBoxFadeValues [nWeaponType] <= 0) {
		m_info.weaponBoxStates [nWeaponType] = WS_FADING_IN;
		m_history [0].weapon [nWeaponType] =
		m_history [1].weapon [nWeaponType] = nWeaponId;
		m_history [0].laserLevel =
		m_history [1].laserLevel = LOCALPLAYER.laserLevel;
		m_info.weaponBoxFadeValues [nWeaponType] = 0;
		}
	}
else if (m_info.weaponBoxStates [nWeaponType] == WS_FADING_IN) {
	DrawWeaponInfo (nWeaponType, nWeaponId, LOCALPLAYER.laserLevel);
	if ((m_history [gameStates.render.vr.nCurrentPage].weapon [nWeaponType] >= 0) &&
		 (nWeaponId != m_history [gameStates.render.vr.nCurrentPage].weapon [nWeaponType])) {
		m_info.weaponBoxStates [nWeaponType] = WS_FADING_OUT;
		}
	else {
		m_history [gameStates.render.vr.nCurrentPage].weapon [nWeaponType] = nWeaponId;
		m_history [gameStates.render.vr.nCurrentPage].ammo [nWeaponType] = -1;
		m_history [gameStates.render.vr.nCurrentPage].xOmegaCharge = -1;
		m_info.weaponBoxFadeValues [nWeaponType] += gameData.time.xFrame * FADE_SCALE;
		if (m_info.weaponBoxFadeValues [nWeaponType] >= I2X (FADE_LEVELS-1)) {
			m_info.weaponBoxStates [nWeaponType] = WS_SET;
			m_history [!gameStates.render.vr.nCurrentPage].weapon [nWeaponType] = -1;		//force redraw (at full fade-in) of other page
			}
		}
	}
else {
	DrawWeaponInfo (nWeaponType, nWeaponId, LOCALPLAYER.laserLevel);
	m_history [gameStates.render.vr.nCurrentPage].weapon [nWeaponType] = nWeaponId;
	m_history [gameStates.render.vr.nCurrentPage].ammo [nWeaponType] = -1;
	m_history [gameStates.render.vr.nCurrentPage].xOmegaCharge = -1;
	m_history [gameStates.render.vr.nCurrentPage].laserLevel = LOCALPLAYER.laserLevel;
	m_info.weaponBoxStates [nWeaponType] = WS_SET;
	}

return 1;
}

//	-----------------------------------------------------------------------------

fix staticTime [2];

void CGenericCockpit::DrawStatic (int nWindow, int nIndex)
{
	tVideoClip *vc = gameData.eff.vClips [0] + VCLIP_MONITOR_STATIC;
	CBitmap *bmp;
	int framenum;
	int boxofs = (gameStates.render.cockpit.nType == CM_STATUS_BAR) ? SB_PRIMARY_BOX : COCKPIT_PRIMARY_BOX;
	int h, x, y;

staticTime [nWindow] += gameData.time.xFrame;
if (staticTime [nWindow] >= vc->xTotalTime) {
	m_info.weaponBoxUser [nWindow] = WBU_WEAPON;
	return;
	}
framenum = staticTime [nWindow] * vc->nFrameCount / vc->xTotalTime;
LoadBitmap (vc->frames [framenum].index, 0);
bmp = gameData.pig.tex.bitmaps [0] + vc->frames [framenum].index;
//CCanvas::SetCurrent (&gameStates.render.vr.buffers.render [0]);
h = boxofs + nWindow;
for (x = hudWindowAreas [h].left; x < hudWindowAreas [h].right; x += bmp->Width ())
	for (y = hudWindowAreas [h].top; y < hudWindowAreas [h].bot; y += bmp->Height ())
		BitBlt (-1, x, y, true, true, I2X (1), 0, bmp);
//CCanvas::SetCurrent (CurrentGameScreen ());
}

//	-----------------------------------------------------------------------------

void CGenericCockpit::DrawWeapons (void)
{
if ((gameStates.render.cockpit.nType == CM_FULL_COCKPIT) || (gameStates.render.cockpit.nType == CM_STATUS_BAR))
	fontManager.SetScale (floor (float (CCanvas::Current ()->Width ()) / 640.0f));

if (m_info.weaponBoxUser [0] == WBU_WEAPON) {
	if (DrawWeaponDisplay (0, gameData.weapons.nPrimary) && (m_info.weaponBoxStates [0] == WS_SET)) {
		if ((gameData.weapons.nPrimary == VULCAN_INDEX) || (gameData.weapons.nPrimary == GAUSS_INDEX)) {
			if (LOCALPLAYER.primaryAmmo [VULCAN_INDEX] != m_history [gameStates.render.vr.nCurrentPage].ammo [0]) {
				if (gameData.demo.nState == ND_STATE_RECORDING)
					NDRecordPrimaryAmmo (m_history [gameStates.render.vr.nCurrentPage].ammo [0], LOCALPLAYER.primaryAmmo [VULCAN_INDEX]);
				DrawPrimaryAmmoInfo (X2I ((unsigned) VULCAN_AMMO_SCALE * (unsigned) LOCALPLAYER.primaryAmmo [VULCAN_INDEX]));
				m_history [gameStates.render.vr.nCurrentPage].ammo [0] = LOCALPLAYER.primaryAmmo [VULCAN_INDEX];
				}
			}
		else if (gameData.weapons.nPrimary == OMEGA_INDEX) {
			if (gameData.omega.xCharge [IsMultiGame] != m_history [gameStates.render.vr.nCurrentPage].xOmegaCharge) {
				if (gameData.demo.nState == ND_STATE_RECORDING)
					NDRecordPrimaryAmmo (m_history [gameStates.render.vr.nCurrentPage].xOmegaCharge, gameData.omega.xCharge [IsMultiGame]);
				DrawPrimaryAmmoInfo (gameData.omega.xCharge [IsMultiGame] * 100 / MAX_OMEGA_CHARGE);
				m_history [gameStates.render.vr.nCurrentPage].xOmegaCharge = gameData.omega.xCharge [IsMultiGame];
				}
			}
		}
	}
else if (m_info.weaponBoxUser [0] == WBU_STATIC)
	DrawStatic (0);

if (m_info.weaponBoxUser [1] == WBU_WEAPON) {
	if (DrawWeaponDisplay (1, gameData.weapons.nSecondary) && (m_info.weaponBoxStates [1] == WS_SET) &&
		 (LOCALPLAYER.secondaryAmmo [gameData.weapons.nSecondary] != m_history [gameStates.render.vr.nCurrentPage].ammo [1])) {
		m_history [gameStates.render.vr.nCurrentPage].bombCount = 0x7fff;	//force redraw
		if (gameData.demo.nState == ND_STATE_RECORDING)
			NDRecordSecondaryAmmo (m_history [gameStates.render.vr.nCurrentPage].ammo [1], LOCALPLAYER.secondaryAmmo [gameData.weapons.nSecondary]);
		DrawSecondaryAmmoInfo (LOCALPLAYER.secondaryAmmo [gameData.weapons.nSecondary]);
		m_history [gameStates.render.vr.nCurrentPage].ammo [1] = LOCALPLAYER.secondaryAmmo [gameData.weapons.nSecondary];
		}
	}
else if (m_info.weaponBoxUser [1] == WBU_STATIC)
	DrawStatic (1);

fontManager.SetScale (1.0f);
}

//------------------------------------------------------------------------------

static inline bool GuidedMissileActive (void)
{
CObject *gmObjP = gameData.objs.guidedMissile [gameData.multiplayer.nLocalPlayer].objP;
return gmObjP &&
		 (gmObjP->info.nType == OBJ_WEAPON) &&
		 (gmObjP->info.nId == GUIDEDMSL_ID) &&
		 (gmObjP->info.nSignature == gameData.objs.guidedMissile [gameData.multiplayer.nLocalPlayer].nSignature);
}

//------------------------------------------------------------------------------

#if 0

// Draw the reticle in 3D for head tracking
void Draw3DReticle (fix xStereoSeparation)
{
	g3sPoint 	reticlePoints [4];
	tUVL			tUVL [4];
	g3sPoint		*pointList [4];
	int 			i;
	CFixVector	v1, v2;
	int			saved_interp_method;

//	if (!transformation.m_info.bUsePlayerHeadAngles) return;

for (i = 0; i < 4; i++) {
	reticlePoints [i].p3_index = -1;
	pointList [i] = reticlePoints + i;
	tUVL [i].l = MAX_LIGHT;
	}
tUVL [0].u =
tUVL [0].v =
tUVL [1].v =
tUVL [3].u = 0;
tUVL [1].u =
tUVL [2].u =
tUVL [2].v =
tUVL [3].v = I2X (1);

v1 = gameData.objs.viewerP->info.position.vPos + gameData.objs.viewerP->info.position.mOrient.FVec () * (I2X (4));
v1 += gameData.objs.viewerP->info.position.mOrient.RVec () * xStereoSeparation;
v2 = v1 + gameData.objs.viewerP->info.position.mOrient.RVec () * (-I2X (1));
v2 += gameData.objs.viewerP->info.position.mOrient.UVec () * (I2X (1));
G3TransformAndEncodePoint(&reticlePoints [0], v2);
v2 = v1 + gameData.objs.viewerP->info.position.mOrient.RVec () * (+I2X (1));
v2 += gameData.objs.viewerP->info.position.mOrient.UVec () * (I2X (1));
G3TransformAndEncodePoint(&reticlePoints [1], v2);
v2 = v1 + gameData.objs.viewerP->info.position.mOrient.RVec () * (+I2X (1));
v2 += gameData.objs.viewerP->info.position.mOrient.UVec () * (-I2X (1));
G3TransformAndEncodePoint(&reticlePoints [2], v2);
v2 = v1 + gameData.objs.viewerP->info.position.mOrient.RVec () * (-I2X (1));
v2 += gameData.objs.viewerP->info.position.mOrient.UVec () * (-I2X (1));
G3TransformAndEncodePoint(&reticlePoints [3], v2);

if ( reticleCanvas == NULL) {
	reticleCanvas = CCanvas::Create (64, 64);
	if ( !reticleCanvas)
		Error ("Couldn't allocate reticleCanvas");
	//reticleCanvas->Bitmap ().nId = 0;
	reticleCanvas->SetFlags (BM_FLAG_TRANSPARENT);
	}

CCanvas::Push ();
CCanvas::SetCurrent (reticleCanvas);
reticleCanvas->Clear (0);		// Clear to Xparent
cockpit->DrawReticle (1);
CCanvas::Pop ();

saved_interp_method = gameStates.render.nInterpolationMethod;
gameStates.render.nInterpolationMethod	= 3;		// The best, albiet slowest.
G3DrawTexPoly (4, pointList, tUVL, reticleCanvas, NULL, 1, -1);
gameStates.render.nInterpolationMethod	= saved_interp_method;
}

#endif

//	-----------------------------------------------------------------------------

tRgbColorb playerColors [] = {
 {15, 15, 23},
 {27, 0, 0},
 {0, 23, 0},
 {30, 11, 31},
 {31, 16, 0},
 {24, 17, 6},
 {14, 21, 12},
 {29, 29, 0}};

typedef struct {
	int x, y;
} xy;

//offsets for reticle parts: high-big  high-sml  low-big  low-sml
xy crossOffsets [4] = 	{{-8, -5},  {-4, -2},  {-4, -2}, {-2, -1}};
xy primaryOffsets [4] =  {{-30, 14}, {-16, 6},  {-15, 6}, {-8, 2}};
xy secondaryOffsets [4] = {{-24, 2},  {-12, 0}, {-12, 1}, {-6, -2}};

//draw the reticle
void CGenericCockpit::DrawReticle (int bForceBig, fix xStereoSeparation)
{
if (cockpit->Hide ())
	return;
if (!gameOpts->render.cockpit.bReticle
	 || !gameStates.app.bGameRunning 
	 || gameStates.menus.nInMenu 
	 || gameStates.render.bRearView 
	 || gameStates.app.bPlayerIsDead 
	 || gameStates.render.bChaseCam 
	 || gameStates.render.bFreeCam
	 || automap.Display ())
	return;

	int		x, y;
	int		bLaserReady, bMissileReady, bLaserAmmo, bMissileAmmo;
	int		nBmReticle, nCrossBm, nPrimaryBm, nSecondaryBm;
	int		bHiresReticle, bSmallReticle, ofs;

if (((gameOpts->render.cockpit.bGuidedInMainView && GuidedMissileActive ()) ||
	 ((gameData.demo.nState == ND_STATE_PLAYBACK) && gameData.demo.bFlyingGuided))) {
	DrawGuidedCrosshair (m_info.xStereoSeparation);
	return;
	}

if (gameStates.zoom.nFactor > float (gameStates.zoom.nMinFactor)) {
	DrawZoomCrosshair ();
	return;
	}

m_info.xStereoSeparation = xStereoSeparation;
x = CCanvas::Current ()->Width () / 2;
#if 0
if (m_info.xStereoSeparation) {
	ogl.ColorMask (1,1,1,1,1);
	x += ((m_info.xStereoSeparation < 0) ? -1 : 1) * int (float (x) / 640.0f + 1.5f); //(ogl.m_states.bFullScreen ? int (float (x) / 320.0f + 0.5f) : 4);
	}
#endif
y = CCanvas::Current ()->Height () / 2;
bLaserReady = AllowedToFireGun ();
bMissileReady = AllowedToFireMissile (-1, 1);
bLaserAmmo = PlayerHasWeapon (gameData.weapons.nPrimary, 0, -1, 1);
bMissileAmmo = PlayerHasWeapon (gameData.weapons.nSecondary, 1, -1, 1);
nPrimaryBm = bLaserReady && (bLaserAmmo == HAS_ALL);
nSecondaryBm = bMissileReady && (bMissileAmmo == HAS_ALL);
if (nPrimaryBm && (gameData.weapons.nPrimary == LASER_INDEX) &&
	 (LOCALPLAYER.flags & PLAYER_FLAGS_QUAD_LASERS))
	nPrimaryBm++;

if (secondaryWeaponToGunNum [gameData.weapons.nSecondary] == 7)
	nSecondaryBm += 3;		//now value is 0, 1 or 3, 4
else if (nSecondaryBm && !(gameData.laser.nMissileGun & 1))
		nSecondaryBm++;

nCrossBm = ((nPrimaryBm > 0) || (nSecondaryBm > 0));

Assert (nPrimaryBm <= 2);
Assert (nSecondaryBm <= 4);
Assert (nCrossBm <= 1);
#if DBG
if (gameStates.render.bChaseCam)
#else
if (gameStates.render.bChaseCam && (!IsMultiGame || (EGI_FLAG (bEnableCheats, 0, 0, 0) && !COMPETITION)))
#endif
	return;
m_info.xScale *= float (HUD_ASPECT);
bHiresReticle = 1; //(gameStates.render.fonts.bHires != 0) && !gameStates.app.bDemoData;
bSmallReticle = !bForceBig && (CCanvas::Current ()->Width () * 3 <= gameData.render.window.wMax * 2);
ofs = (bHiresReticle ? 0 : 2) + bSmallReticle;
nBmReticle = ((!IsMultiGame || IsCoopGame) && TargetInLineOfFire ()) 
				 ? BM_ADDON_RETICLE_RED 
				 : BM_ADDON_RETICLE_GREEN;
ogl.SetBlendMode (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
glColor3i (1,1,1);
BitBlt ((bSmallReticle ? SML_RETICLE_CROSS : RETICLE_CROSS) + nCrossBm,
		  (x + ScaleX (crossOffsets [ofs].x - 1)), (y + ScaleY (crossOffsets [ofs].y - 1)), false, true,
		  I2X (1), 0, NULL, BM_ADDON (nBmReticle + nCrossBm));
BitBlt ((bSmallReticle ? SML_RETICLE_PRIMARY : RETICLE_PRIMARY) + nPrimaryBm,
		  (x + ScaleX (primaryOffsets [ofs].x - 1)), (y + ScaleY (primaryOffsets [ofs].y - 1)), false, true,
		  I2X (1), 0, NULL, BM_ADDON (nBmReticle + 2 + nPrimaryBm));
BitBlt ((bSmallReticle ? SML_RETICLE_SECONDARY : RETICLE_SECONDARY) + nSecondaryBm,
		  (x + ScaleX (secondaryOffsets [ofs].x - 1)), (y + ScaleY (secondaryOffsets [ofs].y - 1)), false, true,
		  I2X (1), 0, NULL, BM_ADDON (nBmReticle + 5 + nSecondaryBm));

if (!gameStates.app.bNostalgia && gameOpts->input.mouse.bJoystick && gameOpts->render.cockpit.bMouseIndicator)
	OglDrawMouseIndicator ();
m_info.xScale /= float (HUD_ASPECT);
#if 0
if (m_info.xStereoSeparation) {
	ogl.ColorMask (1,1,1,1,0);
	}
#endif
}

//	-----------------------------------------------------------------------------

#if DBG
extern int bSavingMovieFrames;
#else
#define bSavingMovieFrames 0
#endif

//returns true if viewerP can see CObject
//	-----------------------------------------------------------------------------

int CGenericCockpit::CanSeeObject (int nObject, int bCheckObjs)
{
if (nObject < 0)
	return 0;

	CHitQuery fq;
	int nHitType;
	CHitData hitData;

	//see if we can see this CPlayerData

fq.p0 = &gameData.objs.viewerP->info.position.vPos;
fq.p1 = &OBJECTS [nObject].info.position.vPos;
fq.radP0 =
fq.radP1 = 0;
fq.thisObjNum = gameStates.render.cameras.bActive ? -1 : OBJ_IDX (gameData.objs.viewerP);
fq.flags = bCheckObjs ? FQ_CHECK_OBJS | FQ_TRANSWALL : FQ_TRANSWALL;
fq.startSeg = gameData.objs.viewerP->info.nSegment;
fq.ignoreObjList = NULL;
fq.bCheckVisibility = true;
nHitType = FindHitpoint (&fq, &hitData);
return bCheckObjs ? (nHitType == HIT_OBJECT) && (hitData.hit.nObject == nObject) : (nHitType != HIT_WALL);
}

//	-----------------------------------------------------------------------------

//show names of teammates & players carrying flags
void CGenericCockpit::DrawPlayerNames (void)
{
	int			bHasFlag, bShowName, bShowTeamNames, bShowAllNames, bShowFlags, nObject, nTeam;
	int			nPlayer, nState;
	tRgbColorb*	colorP;

	static int nCurColor = 1, tColorChange = 0;
	static tRgbColorb typingColors [2] = {
		{63, 0, 0},
		{63, 63, 0}
	};
	char s [CALLSIGN_LEN + 10];
	int w, h, aw;
	int x0, y0, x1, y1;
	int nColor;
	static int nIdNames [2][MAX_PLAYERS] = {{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}};

bShowAllNames = ((gameData.demo.nState == ND_STATE_PLAYBACK) || (netGame.m_info.bShowAllNames && gameData.multigame.bShowReticleName));
bShowTeamNames = (gameData.multigame.bShowReticleName && (IsCoopGame || IsTeamGame));
bShowFlags = (gameData.app.nGameMode & (GM_CAPTURE | GM_HOARD | GM_ENTROPY));

nTeam = GetTeam (gameData.multiplayer.nLocalPlayer);
for (nPlayer = 0; nPlayer < gameData.multiplayer.nPlayers; nPlayer++) {	//check all players
	if (gameStates.multi.bPlayerIsTyping [nPlayer])
		nState = 1;
	else if (downloadManager.Downloading (nPlayer))
		nState = 2;
	else
		nState = 0;

	bShowName = (nState ||
					 (bShowAllNames && !(gameData.multiplayer.players [nPlayer].flags & PLAYER_FLAGS_CLOAKED)) ||
					 (!IsCoopGame && (bShowTeamNames && GetTeam (nPlayer) == nTeam)));
	bHasFlag = (gameData.multiplayer.players [nPlayer].connected && (gameData.multiplayer.players [nPlayer].flags & PLAYER_FLAGS_FLAG));

	if (gameData.demo.nState != ND_STATE_PLAYBACK)
		nObject = gameData.multiplayer.players [nPlayer].nObject;
	else {
		//if this is a demo, the nObject in the CPlayerData struct is wrong,
		//so we search the CObject list for the nObject
		CObject *objP;
		nObject = -1;
		FORALL_PLAYER_OBJS (objP, nObject)
			if (objP->info.nId == nPlayer) {
				nObject = objP->Index ();
				break;
				}
		if (IS_OBJECT (objP, nObject))		//not in list, thus not visible
			bShowName = !bHasFlag;				//..so don't show name
		}

	if ((bShowName || bHasFlag) && CanSeeObject (nObject, 1)) {
		g3sPoint		vPlayerPos;

#if 0
		G3TransformAndEncodePoint (&vPlayerPos, OBJECTS [nObject].info.position.vPos);
#else
		//transformation.Push ();
		SetRenderView (0, NULL, 0);
		G3TransformAndEncodePoint (&vPlayerPos, OBJECTS [nObject].info.position.vPos);
		ogl.EndFrame ();
		//transformation.Pop ();
#endif
		if (vPlayerPos.p3_codes == 0) {	//on screen
			G3ProjectPoint (&vPlayerPos);
			if (!(vPlayerPos.p3_flags & PF_OVERFLOW)) {
				fix x = vPlayerPos.p3_screen.x;
				fix y = vPlayerPos.p3_screen.y;
				if (bShowName) {				// Draw callsign on HUD
					if (nState) {
						int t = gameStates.app.nSDLTicks;

						if (t - tColorChange > 333) {
							tColorChange = t;
							nCurColor = !nCurColor;
							}
						colorP = typingColors + nCurColor;
						}
					else {
						nColor = IsTeamGame ? GetTeam (nPlayer) : nPlayer % MAX_PLAYER_COLORS;
						colorP = playerColors + nColor;
						}

					sprintf (s, "%s", (nState == 1) ? TXT_TYPING : (nState == 2) ? "Downloading..." : gameData.multiplayer.players [nPlayer].callsign);
					fontManager.Current ()->StringSize (s, w, h, aw);
					fontManager.SetColorRGBi (RGBA_PAL2 (colorP->red, colorP->green, colorP->blue), 1, 0, 0);
					x1 = x - w / 2;
					y1 = y - h / 2;
					nIdNames [nCurColor][nPlayer] = GrString (x1, y1, s, nIdNames [nCurColor] + nPlayer);
					CCanvas::Current ()->SetColorRGBi (RGBA_PAL2 (colorP->red, colorP->green, colorP->blue));
					glLineWidth ((GLfloat) 2.0f);
					OglDrawEmptyRect (x1 - 4, y1 - 3, x1 + w + 2, y1 + h + 3);
					glLineWidth (1);
					}

				if (bHasFlag && (gameStates.app.bNostalgia || !(EGI_FLAG (bTargetIndicators, 0, 1, 0) || EGI_FLAG (bTowFlags, 0, 1, 0)))) {// Draw box on HUD
					fix dy = -FixMulDiv (OBJECTS [nObject].info.xSize, I2X (CCanvas::Current ()->Height ())/2, vPlayerPos.p3_vec[Z]);
//					fix dy = -FixMulDiv (FixMul (OBJECTS [nObject].size, transformation.m_info.scale.y), I2X (CCanvas::Current ()->Height ())/2, vPlayerPos.p3_z);
					fix dx = FixMul (dy, screen.Aspect ());
					fix w = dx / 4;
					fix h = dy / 4;
					if (gameData.app.nGameMode & (GM_CAPTURE | GM_ENTROPY))
						CCanvas::Current ()->SetColorRGBi ((GetTeam (nPlayer) == TEAM_BLUE) ? MEDRED_RGBA :  MEDBLUE_RGBA);
					else if (gameData.app.nGameMode & GM_HOARD) {
						if (gameData.app.nGameMode & GM_TEAM)
							CCanvas::Current ()->SetColorRGBi ((GetTeam (nPlayer) == TEAM_RED) ? MEDRED_RGBA :  MEDBLUE_RGBA);
						else
							CCanvas::Current ()->SetColorRGBi (MEDGREEN_RGBA);
						}
					x0 = x - dx;
					x1 = x + dx;
					y0 = y - dy;
					y1 = y + dy;
					// draw the edges of a rectangle around the player (not a complete rectangle)
#if 1
					// draw the complete rectangle
					OglDrawEmptyRect (x0, y0, x1, y1);
					// now partially erase its sides
					CCanvas::Current ()->SetColorRGBi (0);
					OglDrawLine (x - w, y0, x + w, y0);
					OglDrawLine (x - w, y1, x + w, y1);
					OglDrawLine (x0, y - h, x0, y + h);
					OglDrawLine (x1, y - h, x1, y + h);
#else
					OglDrawLine (x1 - w, y0, x1, y0);	//right
					OglDrawLine (x1, y0, x1, y0 + h);	//down
					OglDrawLine (x0, y0, x0 + w, y0);
					OglDrawLine (x0, y0, x0, y0 + h);
					OglDrawLine (x1 - w, y1, x1, y1);
					OglDrawLine (x1, y1, x1, y1 - h);
					OglDrawLine (x0, y1, x0 + w, y1);
					OglDrawLine (x0, y1, x0, y1 - h);
#endif
					}
				}
			}
		}
	}
}

//	-----------------------------------------------------------------------------

void CGenericCockpit::DrawKillList (int x, int y)
{
if (Hide ())
	return;
if (!(IsMultiGame && gameData.multigame.kills.bShowList))
	return;

	int	nPlayers, playerList [MAX_PLAYERS];
	int	nLeft, nameLen, i, xo = 0, x0, x1, y0, fth;
	int	t = gameStates.app.nSDLTicks;
	int	bGetPing = gameStates.render.cockpit.bShowPingStats && (!networkData.tLastPingStat || (t - networkData.tLastPingStat >= 1000));
	int	faw = int (float (GAME_FONT->TotalWidth ()) / float (GAME_FONT->Range ()) * 1.05f + 0.5f);
	float fScale;


	static int nIdKillList [2][MAX_PLAYERS] = {{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}};

if (bGetPing)
	networkData.tLastPingStat = t;
if (gameData.multigame.kills.xShowListTimer > 0) {
	gameData.multigame.kills.xShowListTimer -= gameData.time.xFrame;
	if (gameData.multigame.kills.xShowListTimer < 0)
		gameData.multigame.kills.bShowList = 0;
	}
nPlayers = (gameData.multigame.kills.bShowList == 3) ? 2 : MultiGetKillList (playerList);
nLeft = /*(nPlayers <= MAXPLAYERS / 2) ? nPlayers :*/ (nPlayers + 1) / 2;
fScale = float (CCanvas::Current ()->Height ()) / 640.0f;
if (fScale < 1.0f)
	fScale = 1.0f;
//If font size changes, this code might not work right anymore
//Assert (GAME_FONT->Height ()==5 && GAME_FONT->Width ()==7);
fth = GAME_FONT->Height ();
y -= nLeft * (fth + 1);
x = CCanvas::Current ()->Width () - LHX (x);
x0 = LHX (1);
nameLen = (9 + (gameData.multigame.kills.bShowList == 3)) * faw;
x1 = x0 + nameLen;
y0 = y;

if (gameStates.render.cockpit.bShowPingStats) {
	if (faw < 0)
		if (gameData.multigame.kills.bShowList == 2)
			xo = faw * 24;//was +25;
		else if (IsCoopGame)
			xo = faw * 14;//was +30;
		else
			xo = faw * 8; //was +20;
	}
for (i = 0; i < nPlayers; i++) {
	int nPlayer;
	char name [80], teamInd [2] = {127, 0};
	int sh, aw, indent = 0;

	if (i >= nLeft) {
		x1 = CCanvas::Current ()->Width () - LHX (1) - 6 * faw; // ((gameData.app.nGameMode & GM_MULTI_COOP) ? LHX (27) : LHX (15));
		x0 = x1 - nameLen;
		if (gameStates.render.cockpit.bShowPingStats) {
			x0 -= xo + 6 * faw;
			x1 -= xo + 6 * faw;
			}
		if (i == nLeft)
			y0 = y;
#if 0
		if (netGame.GetScoreGoal () || netGame.GetPlayTimeAllowed ())
			x1 -= LHX (18);
#endif
		}
#if 0
	else if (netGame.GetScoreGoal () || netGame.GetPlayTimeAllowed ())
		 x1 = int (LHX (43) * fScale - LHX (18));
#endif
	nPlayer = (gameData.multigame.kills.bShowList == 3) ? i : playerList [i];
	if ((gameData.multigame.kills.bShowList == 1) || (gameData.multigame.kills.bShowList == 2)) {
		if (gameData.multiplayer.players [nPlayer].connected != 1)
			fontManager.SetColorRGBi (RGBA_PAL2 (12, 12, 12), 1, 0, 0);
		else {
			int color = IsTeamGame ? GetTeam (nPlayer) : nPlayer % MAX_PLAYER_COLORS;
			fontManager.SetColorRGBi (RGBA_PAL2 (playerColors [color].red, playerColors [color].green, playerColors [color].blue), 1, 0, 0);
			}
		}
	else
		fontManager.SetColorRGBi (RGBA_PAL2 (playerColors [nPlayer].red, playerColors [nPlayer].green, playerColors [nPlayer].blue), 1, 0, 0);
	if (gameData.multigame.kills.bShowList == 3) {
		if (GetTeam (gameData.multiplayer.nLocalPlayer) == i) {
#if 0//DBG
			sprintf (name, "%c%-8s %d.%d.%d.%d:%d",
						teamInd [0], netGame.m_info.szTeamName [i],
						netPlayers.m_info.players [i].network.ipx.node [0],
						netPlayers.m_info.players [i].network.ipx.node [1],
						netPlayers.m_info.players [i].network.ipx.node [2],
						netPlayers.m_info.players [i].network.ipx.node [3],
						netPlayers.m_info.players [i].network.ipx.node [5] +
						 (unsigned) netPlayers.m_info.players [i].network.ipx.node [4] * 256);
#else
			sprintf (name, "%c%s", teamInd [0], netGame.m_info.szTeamName [i]);
#endif
			indent = 0;
			}
		else {
#if SHOW_PLAYER_IP
			sprintf (name, "%-8s %d.%d.%d.%d:%d",
						netGame.m_info.szTeamName [i],
						netPlayers.m_info.players [i].network.ipx.node [0],
						netPlayers.m_info.players [i].network.ipx.node [1],
						netPlayers.m_info.players [i].network.ipx.node [2],
						netPlayers.m_info.players [i].network.ipx.node [3],
						netPlayers.m_info.players [i].network.ipx.node [5] +
						 (unsigned) netPlayers.m_info.players [i].network.ipx.node [4] * 256);
#else
			strcpy (name, netGame.m_info.szTeamName [i]);
#endif
			fontManager.Current ()->StringSize (teamInd, indent, sh, aw);
			}
		}
	else
#if 0//DBG
		sprintf (name, "%-8s %d.%d.%d.%d:%d",
					gameData.multiplayer.players [nPlayer].callsign,
					netPlayers.m_info.players [nPlayer].network.ipx.node [0],
					netPlayers.m_info.players [nPlayer].network.ipx.node [1],
					netPlayers.m_info.players [nPlayer].network.ipx.node [2],
					netPlayers.m_info.players [nPlayer].network.ipx.node [3],
					netPlayers.m_info.players [nPlayer].network.ipx.node [5] +
					 (unsigned) netPlayers.m_info.players [nPlayer].network.ipx.node [4] * 256);
#else
		strcpy (name, gameData.multiplayer.players [nPlayer].callsign);	// Note link to above if!!
#endif
#if 0//DBG
	x1 += LHX (100);
	int l, sw, nWidth = x1 - x0 - LHX (2);
	for (l = (int) strlen (name); l;) {
		fontManager.Current ()->StringSize (name, sw, sh, aw);
		if (sw <= nWidth)
			break;
		name [--l] = '\0';
		}
#endif
	nIdKillList [0][i] = GrPrintF (nIdKillList [0] + i, x0 + indent, y0, "%s", name);

	if (gameData.multigame.kills.bShowList == 2) {
		if (gameData.multiplayer.players [nPlayer].netKilledTotal + gameData.multiplayer.players [nPlayer].netKillsTotal <= 0)
			nIdKillList [1][i] = GrPrintF (nIdKillList [1] + i, x1, y0, TXT_NOT_AVAIL);
		else
			nIdKillList [1][i] = GrPrintF (nIdKillList [1] + i, x1, y0, "%d%%",
													 (int) ((double) gameData.multiplayer.players [nPlayer].netKillsTotal /
															 ((double) gameData.multiplayer.players [nPlayer].netKilledTotal +
															  (double) gameData.multiplayer.players [nPlayer].netKillsTotal) * 100.0));
		}
	else if (gameData.multigame.kills.bShowList == 3) {
		if (gameData.app.nGameMode & GM_ENTROPY)
			nIdKillList [1][i] = GrPrintF (nIdKillList [1] + i, x1, y0, "%3d [%d/%d]",
													 gameData.multigame.kills.nTeam [i], gameData.entropy.nTeamRooms [i + 1],
													 gameData.entropy.nTotalRooms);
		else
			nIdKillList [1][i] = GrPrintF (nIdKillList [1] + i, x1, y0, "%3d", gameData.multigame.kills.nTeam [i]);
		}
	else if (IsCoopGame)
		nIdKillList [1][i] = GrPrintF (nIdKillList [1] + i, x1, y0, "%-6d", gameData.multiplayer.players [nPlayer].score);
   else if (netGame.GetPlayTimeAllowed () || netGame.GetScoreGoal ())
      nIdKillList [1][i] = GrPrintF (nIdKillList [1] + i, x1, y0, "%3d (%d)",
												 gameData.multiplayer.players [nPlayer].netKillsTotal,
												 gameData.multiplayer.players [nPlayer].nScoreGoalCount);
   else
		nIdKillList [1][i] = GrPrintF (nIdKillList [1] + i, x1, y0, "%3d", gameData.multiplayer.players [nPlayer].netKillsTotal);
	if (gameStates.render.cockpit.bShowPingStats && (nPlayer != gameData.multiplayer.nLocalPlayer)) {
		if (bGetPing)
			PingPlayer (nPlayer);
		if (pingStats [nPlayer].sent) {
#if 0//DBG
			nIdKillList [1][i] = GrPrintF (nIdKillList [1] + i, x1 + xo, y0, "%lu %d %d",
						  pingStats [nPlayer].ping,
						  pingStats [nPlayer].sent,
						  pingStats [nPlayer].received);
#else
			nIdKillList [1][i] = GrPrintF (nIdKillList [1] + i, x1 + xo, y0, "%lu %i%%", pingStats [nPlayer].ping,
													 100 - ((pingStats [nPlayer].received * 100) / pingStats [nPlayer].sent));
#endif
			}
		}
	y0 += fth + 1;
	}
}

//	-----------------------------------------------------------------------------

void CGenericCockpit::DrawDamage (void)
{
if (gameStates.app.bNostalgia || !EGI_FLAG (nDamageModel, 0, 0, 0))
	return;
if (cockpit->Hide ())
	return;
if (gameStates.app.bPlayerIsDead)
	return;
#if 0
	static int		nIdDamage [3] = {0, 0, 0};
	static int		nColor [3] = {GOLD_RGBA, ORANGE_RGBA, RED_RGBA};
	static char*	szDamage [3] = {"AIM: %d%c", "DRIVES: %d%c", "GUNS: %d%c"};
	static char*	szId = "ADG";
#endif
	int				i;

	static int		dmgColors [3][4] = {
							{RGBA (64, 16, 64, 96), RGBA (192, 0, 0, 96), RGBA (255, 208, 0, 96), RGBA (0, 192, 0, 96)},
							{RGBA (96, 32, 96, 96), RGBA (255, 0, 0, 96), RGBA (255, 255, 0, 96), RGBA (0, 255, 0, 96)},
							{RGBA (96, 32, 96, 96), RGBA (255, 0, 0, 96), RGBA (255, 255, 0, 96), RGBA (0, 255, 0, 96)}
							};

#if 1 //!DBG
if ((gameStates.app.nSDLTicks - OBJECTS [LOCALPLAYER.nObject].TimeLastRepaired () > 2000) && !OBJECTS [LOCALPLAYER.nObject].CriticalDamage ())
	return;
#endif
	float fScale = float (CCanvas::Current ()->Width ()) / 640.0f;
	int	nRad = int (16.0f * fScale + 0.5f);
	int	y = CCanvas::Current ()->Height () / 2 + 3 * nRad / 2;
	int	x = CCanvas::Current ()->Width () / 2;

if (gameOpts->render.cockpit.bTextGauges) {
	int				nColor, nDamage [3], h [4], w [4], aw [4], tw = 0;
	char				szDamage [3][10];
	tCanvasColor	dmgColor = {-1, 1, {0, 0, 0, 128}};

	fontManager.SetScale (max (1.0f, floor (fScale + 0.5f)));
	for (i = 0; i < 3; i++) {
		nDamage [i] = int (X2F (m_info.nDamage [i]) * 200.0f + 0.5f);
#if 1
		sprintf (szDamage [i], "%d ", nDamage [i]);
#else
		sprintf (szDamage [i], "%c:%d ", szId [i], nDamage [i]);
#endif
		fontManager.Current ()->StringSize (szDamage [i], w [i], h [i], aw [i]);
		tw += w [i];
		}
	fontManager.Current ()->StringSize (" ", w [3], h [3], aw [3]);
	x -= (tw - w [3]) / 2;
	CCanvas::Current ()->SetFontColor (dmgColor, 1);	// black background
	for (i = 0; i < 3; i++) {
		nColor = dmgColors [1][nDamage [i] / 33];
		dmgColor.color.red = RGBA_RED (nColor);
		dmgColor.color.green = RGBA_GREEN (nColor);
		dmgColor.color.blue = RGBA_BLUE (nColor);
		CCanvas::Current ()->SetFontColor (dmgColor, 0);
		if (nDamage [i] < 100)
			szDamage [i][2] = '\0'; // remove the trailing blank, it causes rendering artifacts
		GrPrintF (NULL, x, y, szDamage [i]);
		x += w [i];
		}
	fontManager.SetScale (1.0f);
	}
else {
		static tSinCosf sinCos [32];
		static int bInitSinCos = 1;

	if (bInitSinCos) {
		ComputeSinCosTable (sizeofa (sinCos), sinCos);
		bInitSinCos = 0;
		}

	x -= nRad;
	glLineWidth (fScale);

#if 0
	fontManager.SetColorRGBi (nColor [2], 1, 0, 0);
#	if 0 // rectangular frame
	CCanvas::Current ()->SetColorRGB (64, 32, 0, 64);
	OglDrawFilledRect (x - 7, y - 7, x + 71, y + 71);
	CCanvas::Current ()->SetColorRGB (255, 128, 0, 255);
	OglDrawEmptyRect (x - 7, y - 7, x + 71, y + 71);
#	else // round frame
	glColor4f (1.0f, 0.5f, 0.0f, 1.0f);
	OglDrawEllipse (
		sizeofa (sinCos), GL_LINE_LOOP,
		40.0f / float (screen.Width ()), 0.5f,
		40.0f / float (screen.Height ()), 1.0f - float (CCanvas::Current ()->Top () + y + 32) / float (screen.Height ()), sinCos);
#	endif
	glLineWidth (1);
#endif

	nRad *= 2;
	for (i = 0; i < 3; i++) {
		if (LoadDamageIcon (i)) {
			CCanvas::Current ()->SetColorRGBi (dmgColors [i][int (X2F (m_info.nDamage [i]) * 200.0f + 0.5f) / 33]);
			bmpDamageIcon [i]->RenderScaled (x, y, nRad, nRad, I2X (1), 0, &CCanvas::Current ()->Color ());
			}
		}
	}
}

//	-----------------------------------------------------------------------------

void CGenericCockpit::DrawCockpit (int nCockpit, int y, bool bAlphaTest)
{
if ((gameOpts->render.cockpit.bHUD) || (gameStates.render.cockpit.nType != CM_FULL_SCREEN)) {
	int i = gameData.pig.tex.cockpitBmIndex [nCockpit].index;
	CBitmap *bmP = gameData.pig.tex.bitmaps [0] + i;
	LoadBitmap (gameData.pig.tex.cockpitBmIndex [nCockpit].index, 0, true);
	if (bmP->HasOverride ())
		bmP = bmP->Override (-1);
	ogl.m_states.nTransparencyLimit = 8;	//add transparency to black areas of palettized cockpits (namely the display windows)
	bmP->SetTranspType (3);
	bmP->SetupTexture (0, 1);
	ogl.m_states.nTransparencyLimit = 0;
	CCanvas::Push ();
   CCanvas::SetCurrent (gameStates.render.vr.buffers.screenPages + gameStates.render.vr.nCurrentPage);
	CCanvas::Current ()->SetColorRGBi (WHITE_RGBA);
	bmP->RenderScaled (0, y, -1, CCanvas::Current ()->Height () - y, I2X (1), 0, &CCanvas::Current ()->Color ());
	CCanvas::Current ()->SetColorRGBi (BLACK_RGBA);
	CCanvas::Pop ();
	}
}

//------------------------------------------------------------------------------

#if DBG

fix showViewTextTimer = -1;

void DrawWindowLabel (void)
{
if (showViewTextTimer > 0) {
	const char* viewer_name, * control_name, * viewer_id;
	showViewTextTimer -= gameData.time.xFrame;

	viewer_id = const_cast<char*>("");
	switch (gameData.objs.viewerP->info.nType) {
		case OBJ_FIREBALL:
			viewer_name = const_cast<char*>("Fireball");
			break;
		case OBJ_ROBOT:
			viewer_name = const_cast<char*>("Robot");
			break;
		case OBJ_HOSTAGE:
			viewer_name = const_cast<char*>("Hostage");
			break;
		case OBJ_PLAYER:
			viewer_name = const_cast<char*>("Player");
			break;
		case OBJ_WEAPON:
			viewer_name = const_cast<char*>("Weapon");
			break;
		case OBJ_CAMERA:
			viewer_name = const_cast<char*>("Camera");
			break;
		case OBJ_POWERUP:
			viewer_name = const_cast<char*>("Powerup");
			break;
		case OBJ_DEBRIS:
			viewer_name = const_cast<char*>("Debris");
			break;
		case OBJ_REACTOR:
			viewer_name = const_cast<char*>("Reactor");
			break;
		default:
			viewer_name = const_cast<char*>("Unknown");
			break;
		}

	switch (gameData.objs.viewerP->info.controlType) {
		case CT_NONE:
			control_name = const_cast<char*>("Stopped");
			break;
		case CT_AI:
			control_name = const_cast<char*>("AI");
			break;
		case CT_FLYING:
			control_name = const_cast<char*>("Flying");
			break;
		case CT_SLEW:
			control_name = const_cast<char*>("Slew");
			break;
		case CT_FLYTHROUGH:
			control_name = const_cast<char*>("Flythrough");
			break;
		case CT_MORPH:
			control_name = const_cast<char*>("Morphing");
			break;
		default:
			control_name = const_cast<char*>("Unknown");
			break;
		}
	fontManager.SetColorRGBi (RED_RGBA, 1, 0, 0);
	GrPrintF (NULL, 0x8000, 45, "%i: %s [%s] View - %s", OBJ_IDX (gameData.objs.viewerP), viewer_name, viewer_id, control_name);
	}
}
#endif

//------------------------------------------------------------------------------

void CGenericCockpit::RenderWindows (void)
{
	int		bDidMissileView = 0;
	int		saveNewDemoState = gameData.demo.nState;
	int		w;

if (gameData.demo.nState == ND_STATE_PLAYBACK) {
   if (nDemoDoLeft) {
      if (nDemoDoLeft == 3)
			cockpit->RenderWindow (0, gameData.objs.consoleP, 1, WBU_REAR, "REAR");
      else
			cockpit->RenderWindow (0, &demoLeftExtra, bDemoRearCheck [nDemoDoLeft], nDemoWBUType [nDemoDoLeft], szDemoExtraMessage [nDemoDoLeft]);
		}
   else
		cockpit->RenderWindow (0, NULL, 0, WBU_WEAPON, NULL);
	if (nDemoDoRight) {
      if (nDemoDoRight == 3)
			cockpit->RenderWindow (1, gameData.objs.consoleP, 1, WBU_REAR, "REAR");
      else
			cockpit->RenderWindow (1, &demoRightExtra, bDemoRearCheck [nDemoDoRight], nDemoWBUType [nDemoDoRight], szDemoExtraMessage [nDemoDoRight]);
		}
   else
		cockpit->RenderWindow (1, NULL, 0, WBU_WEAPON, NULL);
   nDemoDoLeft = nDemoDoRight = 0;
	nDemoDoingLeft = nDemoDoingRight = 0;
   return;
   }
bDidMissileView = RenderMissileView ();
for (w = 0; w < 2 - bDidMissileView; w++) {
	//show special views if selected
	switch (gameStates.render.cockpit.n3DView [w]) {
		case CV_NONE:
			gameStates.render.nRenderingType = 255;
			cockpit->RenderWindow (w, NULL, 0, WBU_WEAPON, NULL);
			break;

		case CV_REAR:
			if (gameStates.render.bRearView) {		//if big window is rear view, show front here
				gameStates.render.nRenderingType = 3+ (w<<4);
				cockpit->RenderWindow (w, gameData.objs.consoleP, 0, WBU_REAR, "FRONT");
				}
			else {					//show Normal rear view
				gameStates.render.nRenderingType = 3+ (w<<4);
				cockpit->RenderWindow (w, gameData.objs.consoleP, 1, WBU_REAR, "REAR");
				}
			break;

		case CV_ESCORT: {
			CObject *buddy = FindEscort ();
			if (!buddy) {
				cockpit->RenderWindow (w, NULL, 0, WBU_WEAPON, NULL);
				gameStates.render.cockpit.n3DView [w] = CV_NONE;
				}
			else {
				gameStates.render.nRenderingType = 4+ (w<<4);
				cockpit->RenderWindow (w, buddy, 0, WBU_ESCORT, gameData.escort.szName);
				}
			break;
			}

		case CV_COOP: {
			int nPlayer = gameStates.render.cockpit.nCoopPlayerView [w];
	      gameStates.render.nRenderingType = 255; // don't handle coop stuff
			if ((nPlayer != -1) &&
				 gameData.multiplayer.players [nPlayer].connected &&
				 (IsCoopGame || (IsTeamGame && (GetTeam (nPlayer) == GetTeam (gameData.multiplayer.nLocalPlayer)))))
				cockpit->RenderWindow (w, &OBJECTS [gameData.multiplayer.players [gameStates.render.cockpit.nCoopPlayerView [w]].nObject], 0, WBU_COOP, gameData.multiplayer.players [gameStates.render.cockpit.nCoopPlayerView [w]].callsign);
			else {
				cockpit->RenderWindow (w, NULL, 0, WBU_WEAPON, NULL);
				gameStates.render.cockpit.n3DView [w] = CV_NONE;
				}
			break;
			}

		case CV_MARKER: {
			char label [10];
			short v = gameData.marker.viewers [w];
			gameStates.render.nRenderingType = 5+ (w<<4);
			if ((v == -1) || (gameData.marker.objects [v] == -1)) {
				gameStates.render.cockpit.n3DView [w] = CV_NONE;
				break;
				}
			sprintf (label, "Marker %d", gameData.marker.viewers [w]+1);
			cockpit->RenderWindow (w, OBJECTS + gameData.marker.objects [gameData.marker.viewers [w]], 0, WBU_MARKER, label);
			break;
			}

		case CV_RADAR_TOPDOWN:
		case CV_RADAR_HEADSUP:
			if (!(gameStates.app.bNostalgia || COMPETITION) && EGI_FLAG (bRadarEnabled, 0, 1, 0))
				cockpit->RenderWindow (w, gameData.objs.consoleP, 0,
					(gameStates.render.cockpit.n3DView [w] == CV_RADAR_TOPDOWN) ? WBU_RADAR_TOPDOWN : WBU_RADAR_HEADSUP, "MINI MAP");
			else
				gameStates.render.cockpit.n3DView [w] = CV_NONE;
			break;
		default:
			Int3 ();		//invalid window nType
		}
	}
gameStates.render.nRenderingType = 0;
gameData.demo.nState = saveNewDemoState;
}

//	-----------------------------------------------------------------------------
//draw all the things on the HUD

void CGenericCockpit::Render (int bExtraInfo, fix xStereoSeparation)
{
if (Hide ())
	return;
if (gameStates.render.bChaseCam || gameStates.render.bFreeCam)
	return;
#if 1
if (!cockpit->Setup (false))
	return;
#else
if (!cockpit->Setup (true))
	return;
#endif
ogl.SetDepthMode (GL_ALWAYS);
ogl.SetBlendMode (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
ogl.ColorMask (1,1,1,1,0);
CCanvas::SetCurrent (CurrentGameScreen ());
CCanvas::Current ()->SetColorRGBi (BLACK_RGBA);
fontManager.SetCurrent (GAME_FONT);
cockpit->SetColor (WHITE_RGBA);
cockpit->SetLineSpacing (m_info.nLineSpacing);
m_info.fontWidth = CCanvas::Current ()->Font ()->Width ();
m_info.fontHeight = CCanvas::Current ()->Font ()->Height ();
m_info.xStereoSeparation = xStereoSeparation;
m_info.xScale = screen.Scale (0);
m_info.yScale = screen.Scale (1);
fontManager.SetScale (floor (float (CCanvas::Current ()->Width ()) / 640.0f));
m_info.nLineSpacing = int (GAME_FONT->Height () + GAME_FONT->Height () * fontManager.Scale () / 4);
fontManager.SetScale (1.0f);
m_info.heightPad = (ScaleY (m_info.fontHeight) - m_info.fontHeight) / 2;
m_info.nEnergy = X2IR (LOCALPLAYER.energy);
m_info.nShields = X2IR (LOCALPLAYER.shields);
m_info.nDamage [0] = gameData.objs.consoleP->AimDamage ();
m_info.nDamage [1] = gameData.objs.consoleP->DriveDamage ();
m_info.nDamage [2] = gameData.objs.consoleP->GunDamage ();
m_info.bCloak = ((LOCALPLAYER.flags & PLAYER_FLAGS_CLOAKED) != 0);
m_info.nCockpit = (gameStates.video.nDisplayMode && !gameStates.app.bDemoData) ? gameData.models.nCockpits / 2 : 0;
m_info.nEnergy = X2IR (LOCALPLAYER.energy);
if (m_info.nEnergy < 0)
	m_info.nEnergy  = 0;
m_info.nShields = X2IR (LOCALPLAYER.shields);
if (m_info.nShields < 0)
	m_info.nShields  = 0;
m_info.bCloak = ((LOCALPLAYER.flags & PLAYER_FLAGS_CLOAKED) != 0);
m_info.tInvul = (LOCALPLAYER.invulnerableTime == 0x7fffffff) ? LOCALPLAYER.invulnerableTime : LOCALPLAYER.invulnerableTime + INVULNERABLE_TIME_MAX - gameData.time.xGame;
m_info.nColor = WHITE_RGBA;

if (gameOpts->render.cockpit.bScaleGauges) {
	m_info.xGaugeScale = float (CCanvas::Current ()->Height ()) / 480.0f;
	m_info.yGaugeScale = float (CCanvas::Current ()->Height ()) / 640.0f;
	}
else
	m_info.xGaugeScale =
	m_info.yGaugeScale = 1;


if ((gameData.demo.nState == ND_STATE_PLAYBACK))
	gameData.app.nGameMode = gameData.demo.nGameMode;

CCanvas::SetCurrent (&gameStates.render.vr.buffers.subRender [0]);
CCanvas::Current ()->SetColorRGBi (BLACK_RGBA);
fontManager.SetCurrent (GAME_FONT);

bool bLimited = (gameStates.render.bRearView || gameStates.render.bChaseCam || gameStates.render.bFreeCam);

//if (!(transformation.m_info.bUsePlayerHeadAngles || gameOpts->render.n3DGlasses))
	DrawReticle (0);

if (!bLimited) {
	DrawPlayerNames ();
	RenderWindows ();
	}
DrawCockpit (false);
#if 1
if (bExtraInfo) {
#if DBG
	DrawWindowLabel ();
#endif
	DrawMultiMessage ();
	DrawMarkerMessage ();
	DrawFrameRate ();
	DrawCruise ();
	}
DrawPacketLoss ();
DrawSlowMotion ();
DrawPlayerStats ();
DrawScore ();
if (m_info.scoreTime)
	DrawAddedScore ();
DrawEnergyBar ();
DrawAfterburnerBar ();
DrawShieldBar ();
DrawEnergy ();
DrawShield ();
DrawAfterburner ();
DrawDamage ();
DrawWeapons ();
DrawTimerCount ();
DrawCloak ();
DrawInvul ();
if (!bLimited) {
	DrawTime ();
	DrawKeys ();
	DrawFlag ();
	DrawOrbs ();
	DrawLives ();
	DrawBombCount ();
	DrawHomingWarning ();
	DrawKillList ();
	DrawPlayerShip ();
	}
hudIcons.Render ();
if (bExtraInfo) {
	DrawCountdown ();
	DrawRecording ();
	}

CCanvas::SetCurrent (CurrentGameScreen ());

if ((gameData.demo.nState == ND_STATE_PLAYBACK))
	gameData.app.nGameMode = GM_NORMAL;

if (gameStates.app.bPlayerIsDead)
	PlayerDeadMessage ();

if (!gameStates.render.bRearView)
	HUDRenderMessageFrame ();
else if (gameStates.render.cockpit.nType != CM_REAR_VIEW) {
	HUDRenderMessageFrame ();
	fontManager.SetColorRGBi (GREEN_RGBA, 1, 0, 0);
	GrPrintF (NULL, 0x8000, CCanvas::Current ()->Height () - ((gameData.demo.nState == ND_STATE_PLAYBACK) ? 14 : 10), TXT_REAR_VIEW);
	}
DemoRecording ();
#endif
m_history [gameStates.render.vr.nCurrentPage].bCloak = m_info.bCloak;
}

//	-----------------------------------------------------------------------------

void CGenericCockpit::DemoRecording (void)
{
if (gameData.demo.nState == ND_STATE_RECORDING) {
	if (LOCALPLAYER.homingObjectDist >= 0)
		NDRecordHomingDistance (LOCALPLAYER.homingObjectDist);

	if (m_info.nEnergy != m_history [gameStates.render.vr.nCurrentPage].energy) {
		NDRecordPlayerEnergy (m_history [gameStates.render.vr.nCurrentPage].energy, m_info.nEnergy);
		m_history [gameStates.render.vr.nCurrentPage].energy = m_info.nEnergy;
		}

	if (gameData.physics.xAfterburnerCharge != m_history [gameStates.render.vr.nCurrentPage].afterburner) {
		NDRecordPlayerAfterburner (m_history [gameStates.render.vr.nCurrentPage].afterburner, gameData.physics.xAfterburnerCharge);
		m_history [gameStates.render.vr.nCurrentPage].afterburner = gameData.physics.xAfterburnerCharge;
		}

	if (LOCALPLAYER.flags & PLAYER_FLAGS_INVULNERABLE)
		m_history [gameStates.render.vr.nCurrentPage].shields = m_info.nShields ^ 1;
	else {
		if (m_info.nShields != m_history [gameStates.render.vr.nCurrentPage].shields) {		// Draw the shield gauge
			NDRecordPlayerShields (m_history [gameStates.render.vr.nCurrentPage].shields, m_info.nShields);
			m_history [gameStates.render.vr.nCurrentPage].shields = m_info.nShields;
			}
		}
	if (LOCALPLAYER.flags != m_history [gameStates.render.vr.nCurrentPage].flags) {
		NDRecordPlayerFlags (m_history [gameStates.render.vr.nCurrentPage].flags, LOCALPLAYER.flags);
		m_history [gameStates.render.vr.nCurrentPage].flags = LOCALPLAYER.flags;
		}
	}
}

//	---------------------------------------------------------------------------------------------------------
//	Call when picked up a laser powerup.
//	If laser is active, set previous weapon [0] to -1 to force redraw.

void CGenericCockpit::UpdateLaserWeaponInfo (void)
{
#if 0
if (m_history [gameStates.render.vr.nCurrentPage].weapon [0] == 0)
	if (!(LOCALPLAYER.laserLevel > MAX_LASER_LEVEL && m_history [gameStates.render.vr.nCurrentPage].laserLevel <= MAX_LASER_LEVEL))
		m_history [gameStates.render.vr.nCurrentPage].weapon [0] = -1;
#endif
}

//	---------------------------------------------------------------------------------------------------------
//draws a 3d view into one of the cockpit windows.  win is 0 for left,
//1 for right.  viewerP is CObject.  NULL CObject means give up window
//nUser is one of the WBU_ constants.  If bRearView is set, show a
//rear view.  If label is non-NULL, print the label at the top of the
//window.

void CGenericCockpit::RenderWindow (int nWindow, CObject *viewerP, int bRearView, int nUser, const char *pszLabel)
{
if (Hide ())
	return;
if (gameStates.app.bPlayerIsDead)
	return;
if (gameStates.app.bEndLevelSequence >= EL_LOOKBACK)
	return;
if ((gameStates.render.cockpit.nType >= CM_FULL_SCREEN) && (gameStates.zoom.nFactor > gameStates.zoom.nMinFactor))
	return;

	CCanvas windowCanv, * cockpitCanv;
	static CCanvas overlapCanv;

	CObject*	viewerSave = gameData.objs.viewerP;
	int		bRearViewSave = gameStates.render.bRearView;
	//fix		xStereoSeparation = ogl.StereoSeparation ();
	float		nZoomSave;

	static int bOverlapDirty [2] = {0, 0};
	static int y, x;

if (!viewerP) {								//this nUser is done
	Assert (nUser == WBU_WEAPON || nUser == WBU_STATIC);
	if ((nUser == WBU_STATIC) && (m_info.weaponBoxUser [nWindow] != WBU_STATIC))
		staticTime [nWindow] = 0;
	if (m_info.weaponBoxUser [nWindow] == WBU_WEAPON || m_info.weaponBoxUser [nWindow] == WBU_STATIC)
		return;		//already set
	m_info.weaponBoxUser [nWindow] = nUser;
	if (bOverlapDirty [nWindow]) {
		//CCanvas::SetCurrent (&gameStates.render.vr.buffers.screenPages [gameStates.render.vr.nCurrentPage]);
		//FillBackground ();
		bOverlapDirty [nWindow] = 0;
		}
	return;
	}
UpdateRenderedData (nWindow + 1, viewerP, bRearView, nUser);
m_info.weaponBoxUser [nWindow] = nUser;						//say who's using window
gameData.objs.viewerP = viewerP;
gameStates.render.bRearView = -bRearView;
SetupWindow (nWindow, &windowCanv);
cockpitCanv = CCanvas::Current ();
CCanvas::Push ();
CCanvas::SetCurrent (&windowCanv);
fontManager.SetCurrent (GAME_FONT);
transformation.Push ();
nZoomSave = gameStates.zoom.nFactor;
gameStates.zoom.nFactor = float (I2X (gameOpts->render.cockpit.nWindowZoom + 1));					//the CPlayerData's zoom factor
if ((nUser == WBU_RADAR_TOPDOWN) || (nUser == WBU_RADAR_HEADSUP)) {
	if (!IsMultiGame || (netGame.m_info.gameFlags & NETGAME_FLAG_SHOW_MAP)) {
		automap.m_bDisplay = -1;
		automap.DoFrame (0, 1 + (nUser == WBU_RADAR_TOPDOWN));
		automap.m_bDisplay = 0;
		}
	else
		RenderFrame (0, nWindow + 1);
	}
else
	RenderFrame (0, nWindow + 1);
gameStates.zoom.nFactor = nZoomSave;
//ogl.SetStereoSeparation (xStereoSeparation);
transformation.Pop ();
//	HACK!If guided missile, wake up robots as necessary.
if (viewerP->info.nType == OBJ_WEAPON) {
	// -- Used to require to be GUIDED -- if (viewerP->id == GUIDEDMSL_ID)
	WakeupRenderedObjects (viewerP, nWindow + 1);
	}
if (pszLabel) {
	fontManager.SetColorRGBi (GREEN_RGBA, 1, 0, 0);
	GrPrintF (NULL, 0x8000, 2, pszLabel);
	}
if (nUser == WBU_GUIDED) {
	DrawGuidedCrosshair (m_info.xStereoSeparation);
	}
if (gameStates.render.cockpit.nType >= CM_FULL_SCREEN) {
	int smallWindowBottom, bigWindowBottom, extraPartHeight;

	if (gameStates.app.bNostalgia)
		CCanvas::Current ()->SetColorRGBi (RGB_PAL (0, 0, 32));
	else
		CCanvas::Current ()->SetColorRGBi (RGB_PAL (47, 31, 0));
	//glLineWidth (((cockpitCanv->Width () < 1200) ? 1.0f : 2.0f));
	glLineWidth (float (cockpitCanv->Width ()) / 640.0f);
	OglDrawEmptyRect (0, 0, CCanvas::Current ()->Width () - 1, CCanvas::Current ()->Height ());
	glLineWidth (1);

	//if the window only partially overlaps the big 3d window, copy
	//the extra part to the visible screen
	bigWindowBottom = gameData.render.window.y + gameData.render.window.h - 1;
	if (x > bigWindowBottom) {
		//the small window is completely outside the big 3d window, so
		//copy it to the visible screen
		windowCanv.BlitClipped (y, x);
		bOverlapDirty [nWindow] = 1;
		}
	else {
		smallWindowBottom = x + windowCanv.Height () - 1;
		if (0 < (extraPartHeight = smallWindowBottom - bigWindowBottom)) {
			windowCanv.SetupPane (&overlapCanv, 0, windowCanv.Height ()-extraPartHeight, windowCanv.Width (), extraPartHeight);
			overlapCanv.BlitClipped (y, bigWindowBottom + 1);
			bOverlapDirty [nWindow] = 1;
			}
		}
	}
else {
	//CCanvas::SetCurrent (CurrentGameScreen ());
	}
//force redraw when done
m_history [gameStates.render.vr.nCurrentPage].weapon [nWindow] = m_history [gameStates.render.vr.nCurrentPage].ammo [nWindow] = -1;

gameData.objs.viewerP = viewerSave;
CCanvas::Pop ();
#if 0
// draw a thicker frame with rounded edges around the cockpit displays
if (!gameStates.app.bNostalgia && (gameStates.render.cockpit.nType >= CM_FULL_SCREEN)) {
	int x0 = windowCanv.Left ();
	int y0 = windowCanv.Top () - CCanvas::Current ()->Top ();
	int x1 = windowCanv.Right () - 1;
	int y1 = windowCanv.Bottom () - CCanvas::Current ()->Top ();
	CCanvas::Current ()->SetColorRGBi (RGB_PAL (47, 31, 0));
	//CCanvas::Current ()->SetColorRGBi (RGB_PAL (0, 0, 32));
	//CCanvas::Current ()->SetColorRGBi (RGB_PAL (60, 60, 60));
	//CCanvas::Current ()->SetColorRGBi (RGB_PAL (31, 31, 31));
	OglDrawLine (x0 + 1, y0 - 1, x1 - 1, y0 - 1);
	OglDrawLine (x0 + 1, y1 + 1, x1 - 1, y1 + 1);
	OglDrawLine (x0 - 1, y0 + 1, x0 - 1, y1 - 1);
	OglDrawLine (x1 + 1, y0 + 1, x1 + 1, y1 - 1);
	//CCanvas::Current ()->SetColorRGBi (RGB_PAL (30, 30, 30));
	OglDrawLine (x0 + 3, y0 - 2, x1 - 3, y0 - 2);
	OglDrawLine (x0 + 3, y1 + 2, x1 - 3, y1 + 2);
	OglDrawLine (x0 - 2, y0 + 3, x0 - 2, y1 - 3);
	OglDrawLine (x1 + 2, y0 + 3, x1 + 2, y1 - 3);
	}
#endif
gameStates.render.bRearView = bRearViewSave;
}

//------------------------------------------------------------------------------
//initialize the various canvases on the game screen
//called every time the screen mode or cockpit changes
bool CGenericCockpit::Setup (bool bRebuild)
{
if (gameStates.video.nScreenMode != SCREEN_GAME)
	return false;
if (gameData.demo.nState == ND_STATE_RECORDING)
	NDRecordCockpitChange (gameStates.render.cockpit.nType);
if (gameStates.render.vr.nRenderMode != VR_NONE)
	gameStates.render.cockpit.nType = CM_FULL_SCREEN;
if (gameStates.video.nScreenMode == SCREEN_EDITOR)
	gameStates.render.cockpit.nType = CM_FULL_SCREEN;
CCanvas::SetCurrent (NULL);
fontManager.SetCurrent (GAME_FONT);
if (bRebuild)
	gameStates.render.cockpit.nShieldFlash = 0;
return true;
}

//------------------------------------------------------------------------------

void CGenericCockpit::Activate (int nType, bool bClearMessages)
{
if (nType == CM_FULL_COCKPIT)
	cockpit = &fullCockpit;
else if (nType == CM_STATUS_BAR)
	cockpit = &statusBarCockpit;
else if (nType == CM_FULL_SCREEN)
	cockpit = &hudCockpit;
else if (nType == CM_LETTERBOX)
	cockpit = &letterboxCockpit;
else if (nType == CM_REAR_VIEW) {
	cockpit = &rearViewCockpit;
	gameStates.render.bRearView = 1;
	}
else
	return;
gameStates.render.cockpit.nType = nType;
gameStates.zoom.nFactor = float (gameStates.zoom.nMinFactor);
m_info.nCockpit = (gameStates.video.nDisplayMode && !gameStates.app.bDemoData) ? gameData.models.nCockpits / 2 : 0;
gameStates.render.cockpit.nNextType = -1;
cockpit->Setup (false);
if (bClearMessages)
	HUDClearMessages ();
if (!gameStates.app.bPlayerIsDead)
	SavePlayerProfile ();
}

//------------------------------------------------------------------------------

int CGenericCockpit::WidthPad (char* pszText)
{
	int	w, h, aw;

fontManager.Current ()->StringSize (pszText, w, h, aw);
return (int (ScaleX (w) / fontManager.Scale () + 0.5f) - w) / 2;
}

//------------------------------------------------------------------------------

int CGenericCockpit::HeightPad (void)
{
return int (m_info.heightPad / fontManager.Scale () + 0.5f);
}

//------------------------------------------------------------------------------

int CGenericCockpit::WidthPad (int nValue)
{
	char szValue [20];

sprintf (szValue, "%d", nValue);
return WidthPad (szValue);
}

//	-----------------------------------------------------------------------------

bool CGenericCockpit::Save (bool bInitial)
{
if (bInitial && IsSaved ())
	return true;
return m_save.Push (gameStates.render.cockpit.nTypeSave = gameStates.render.cockpit.nType);
}

//	-----------------------------------------------------------------------------

bool CGenericCockpit::Restore (void)
{
if (!m_save.ToS ()) {
	gameStates.render.cockpit.nTypeSave = -1;
	return false;
	}
cockpit->Activate (m_save.Pop ());
gameStates.render.cockpit.nTypeSave = m_save.ToS () ? * m_save.Top () : -1;
return true;
}

//	-----------------------------------------------------------------------------

void CGenericCockpit::Rewind (bool bActivate)
{
if (bActivate)
	while (Restore ())
		;
else {
	m_save.Reset ();
	gameStates.render.cockpit.nTypeSave = -1;
	}
}

//	-----------------------------------------------------------------------------

bool CGenericCockpit::IsSaved (void)
{
return m_save.ToS () > 0;
}

//	-----------------------------------------------------------------------------
