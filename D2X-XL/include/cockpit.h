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

#ifndef _COCKPIT_H
#define _COCKPIT_H

#include "fix.h"
#include "gr.h"
#include "piggy.h"
#include "object.h"
#include "hudmsgs.h"
#include "ogl_defs.h"
#include "ogl_bitmap.h"
#include "ogl_hudstuff.h"
#include "ogl_render.h"
#include "endlevel.h"
//from gauges.c

// Flags for gauges/hud stuff
extern ubyte Reticle_on;

// Call to flash a message on the HUD.  Returns true if message drawn.
// (message might not be drawn if previous message was same)
#define gauge_message HUDInitMessage

//valid modes for cockpit
#define CM_FULL_COCKPIT    0	// normal screen with cockpit
#define CM_REAR_VIEW       1	// looking back with bitmap
#define CM_STATUS_BAR      2	// small status bar, w/ reticle
#define CM_FULL_SCREEN     3	// full screen, no cockpit (w/ reticle)
#define CM_LETTERBOX       4	// half-height window (for cutscenes)

#define WBU_WEAPON			0	// the weapons display
#define WBUMSL					1	// the missile view
#define WBU_ESCORT			2	// the "buddy bot"
#define WBU_REAR				3	// the rear view
#define WBU_COOP				4	// coop or team member view
#define WBU_GUIDED			5	// the guided missile
#define WBU_MARKER			6	// a dropped marker
#define WBU_STATIC			7	// playing static after missile hits
#define WBU_RADAR_TOPDOWN	8
#define WBU_RADAR_HEADSUP	9

//	-----------------------------------------------------------------------------

#define HUD_SCALE(v, s)		(int (float (v) * (s) /*+ 0.5*/))
#define HUD_SCALE_X(v)		HUD_SCALE (v, m_info.xScale)
#define HUD_SCALE_Y(v)		HUD_SCALE (v, m_info.yScale)
#define HUD_LHX(x)			(gameStates.menus.bHires ? 2 * (x) : x)
#define HUD_LHY(y)			(gameStates.menus.bHires? (24 * (y)) / 10 : y)
#define HUD_ASPECT			(float (screen.Height ()) / float (screen.Width ()) / 0.75f)

//	-----------------------------------------------------------------------------

typedef struct tSpan {
	sbyte l, r;
} tSpan;

typedef struct tGaugeBox {
	int left, top;
	int right, bot;		//maximal box
	tSpan *spanlist;	//list of left, right spans for copy
} tGaugeBox;

//	-----------------------------------------------------------------------------

class CCockpitHistory {
	public:
		int		score;
		int		energy;
		int		shields;
		uint	flags;
		int		bCloak;
		int		lives;
		fix		afterburner;
		int		bombCount;
		int		laserLevel;
		int		weapon [2];
		int		ammo [2];
		fix		xOmegaCharge;

	public:
		void Init (void);
};

class CCockpitInfo {
	public:
		int	nCloakFadeState;
		int	bLastHomingWarningDrawn [2];
		int	addedScore [2];
		fix	scoreTime;
		fix	lastWarningBeepTime [2];
		int	bHaveGaugeCanvases;
		int	nInvulnerableFrame;
		int	weaponBoxStates [2];
		fix	weaponBoxFadeValues [2];
		int	weaponBoxUser [2];
		int	nLineSpacing;
		int	nType;
		int	nColor;
		float	xScale;
		float	yScale;
		float	xGaugeScale;
		float	yGaugeScale;
		int	fontWidth;
		int	fontHeight;
		int	heightPad;
		int	nCockpit;
		int	nShields;
		int	nEnergy;
		int	bCloak;
		int	nDamage [3];
		fix	tInvul;
		fix	xStereoSeparation;
		bool	bRebuild;

	public:
		void Init (void);
};

//	-----------------------------------------------------------------------------

class CGenericCockpit {
	protected:
		CCockpitHistory		m_history [2];
		CCockpitInfo			m_info;
		static CStack<int>	m_save;

	public:
		CGenericCockpit() { m_save.Create (10); }
		void Init (void);

		static bool Save (bool bInitial = false);
		static bool Restore (void);
		static bool IsSaved (void);
		static void Rewind (bool bActivate = true);

		inline float Aspect (void) { return float (screen.Height ()) / float (screen.Width ()) / 0.75f; }
		inline int ScaleX (int v) { return int (float (v) * m_info.xScale + 0.5f); }
		inline int ScaleY (int v) { return int (float (v) * m_info.yScale + 0.5f); }
		inline int LHX (int x) { return x << gameStates.render.fonts.bHires; }
		inline int LHY (int y) { return gameStates.render.fonts.bHires ? 24 * y / 10 : y; }
		inline void PageInGauge (int nGauge) { LoadBitmap (gameData.cockpit.gauges [!gameStates.render.fonts.bHires][nGauge].index, 0); }
		inline ushort GaugeIndex (int nGauge) { return gameData.cockpit.gauges [!gameStates.render.fonts.bHires][nGauge].index; }
		CBitmap* BitBlt (int nGauge, int x, int y, bool bScalePos = true, bool bScaleSize = true, int scale = I2X (1), int orient = 0, CBitmap* bmP = NULL, CBitmap* bmoP = NULL);
		int _CDECL_ PrintF (int *idP, int x, int y, const char *pszFmt, ...);
		void Rect (int left, int top, int width, int height) {
			OglDrawFilledRect (HUD_SCALE_X (left), HUD_SCALE_Y (top), HUD_SCALE_X (width), HUD_SCALE_Y (height));
			}

		char* ftoa (char *pszVal, fix f);
		char* Convert1s (char* s);
		void DemoRecording (void);

		void DrawMarkerMessage (void);
		void DrawMultiMessage (void);
		void DrawCountdown (int y);
		void DrawRecording (int y);
		void DrawPacketLoss (void);
		void DrawFrameRate (void);
		void DrawPlayerStats (void);
		void DrawSlowMotion (void);
		void DrawTime (void);
		void DrawTimerCount (void);
		void PlayHomingWarning (void);
		void DrawCruise (int x, int y);
		void DrawBombCount (int x, int y, int bgColor, int bShowAlways);
		void DrawAmmoInfo (int x, int y, int ammoCount, int bPrimary);
		void CheckForExtraLife (int nPrevScore);
		void AddPointsToScore (int points);
		void AddBonusPointsToScore (int points);
		void DrawPlayerShip (int nCloakState, int nOldCloakState, int x, int y);
		void DrawWeaponInfo (int nWeaponType, int nIndex, tGaugeBox *box, int xPic, int yPic, const char *pszName, int xText, int yText, int orient);
		int DrawWeaponDisplay (int nWeaponType, int nWeaponId);
		void DrawStatic (int nWindow, int nIndex);
		void DrawOrbs (int x, int y);
		void DrawFlag (int x, int y);
		void DrawKillList (int x, int y);
		void DrawDamage (void);
		void DrawCockpit (int nCockpit, int y, bool bAlphaTest = false);
		void UpdateLaserWeaponInfo (void);
		void DrawReticle (int bForceBig, fix xStereoSeparation = 0);
		int CanSeeObject (int nObject, int bCheckObjs);
		void DrawPlayerNames (void);
		void RenderWindows (void);
		void Render (int bExtraInfo, fix xStereoSeparation = 0);
		void RenderWindow (int nWindow, CObject *viewerP, int bRearView, int nUser, const char *pszLabel);
		void Activate (int nType, bool bClearMessages = false);

		virtual void GetHostageWindowCoords (int& x, int& y, int& w, int& h) = 0;
		virtual void DrawRecording (void) = 0;
		virtual void DrawCountdown (void) = 0;
		virtual void DrawCruise (void) = 0;
		virtual void DrawScore (void) = 0;
		virtual void DrawAddedScore (void) = 0;
		virtual void DrawHomingWarning (void) = 0;
		virtual void DrawKeys (void) = 0;
		virtual void DrawOrbs (void) = 0;
		virtual void DrawFlag (void) = 0;
		virtual void DrawShield (void) = 0;
		virtual void DrawShieldBar (void) = 0;
		virtual void DrawEnergy (void) = 0;
		virtual void DrawEnergyBar (void) = 0;
		virtual void DrawAfterburner (void) = 0;
		virtual void DrawAfterburnerBar (void) = 0;
		virtual void ClearBombCount (int bgColor) = 0;
		virtual void DrawBombCount (void) = 0;
		virtual int DrawBombCount (int& nIdBombCount, int y, int x, int nColor, char* pszBombCount) = 0;
		virtual void DrawPrimaryAmmoInfo (int ammoCount) = 0;
		virtual void DrawSecondaryAmmoInfo (int ammoCount) = 0;
		virtual void DrawCloak (void) = 0;
		virtual void DrawInvul (void) = 0;
		virtual void DrawLives (void) = 0;
		virtual void DrawPlayerShip (void) = 0;
		virtual void DrawKillList (void) = 0;
		virtual void DrawStatic (int nWindow) = 0;
		virtual void Toggle (void) = 0;
		virtual void DrawWeaponInfo (int nWeaponType, int nWeaponId, int laserLevel);
		virtual void DrawWeapons (void);
		virtual void DrawCockpit (bool bAlphaTest = false) = 0;
		virtual void SetupWindow (int nWindow, CCanvas* canvP) = 0;
		virtual bool Setup (bool bRebuild = false);

		inline CCockpitInfo& Info (void) { return m_info; }
		inline int Type (void) { return m_info.nType; }
		inline int LineSpacing (void) { return m_info.nLineSpacing = GAME_FONT->Height () + GAME_FONT->Height () / 4; }
		inline float XScale (void) { return m_info.xScale = screen.Scale (0); }
		inline float YScale (void) { return m_info.yScale = screen.Scale (1); }
		inline void SetScales (float xScale, float yScale) { m_info.xScale = xScale, m_info.yScale = yScale; }
		inline void Rebuild (void) { m_info.bRebuild = true; }

		int WidthPad (char* pszText);
		int WidthPad (int nValue);
		int HeightPad (void);

		inline bool ShowAlways (void) { 
			return (gameStates.render.cockpit.nType == CM_FULL_COCKPIT) || (gameStates.render.cockpit.nType == CM_STATUS_BAR); 
			}

		inline bool Show (void) { 
			return !gameStates.app.bEndLevelSequence && (!gameStates.app.bNostalgia || gameOpts->render.cockpit.bHUD || !ShowAlways ()); 
			}
		
		inline bool Hide (void) {
			return (gameStates.app.bEndLevelSequence >= EL_LOOKBACK) || 
					 (!(gameStates.app.bNostalgia || gameOpts->render.cockpit.bHUD) && (gameStates.render.cockpit.nType >= CM_FULL_SCREEN));
			}

		inline int ScoreTime (void) { return m_info.scoreTime; }
		inline int SetScoreTime (int nTime) { return m_info.scoreTime = nTime; }
		inline int AddedScore (int i = 0) { return m_info.addedScore [i]; }
		inline void AddScore (int i, int nScore) { m_info.addedScore [i] += nScore; }
		inline void SetAddedScore (int i, int nScore) { m_info.addedScore [i] = nScore; }
		inline void SetLineSpacing (int nLineSpacing) { m_info.nLineSpacing = nLineSpacing; }
		inline void SetColor (int nColor) { m_info.nColor = nColor; }
	};

//	-----------------------------------------------------------------------------

class CHUD : public CGenericCockpit {
	public:
		virtual void GetHostageWindowCoords (int& x, int& y, int& w, int& h);
		virtual void DrawRecording (void);
		virtual void DrawCountdown (void);
		virtual void DrawCruise (void);
		virtual void DrawScore (void);
		virtual void DrawAddedScore (void);
		virtual void DrawHomingWarning (void);
		virtual void DrawKeys (void);
		virtual void DrawOrbs (void);
		virtual void DrawFlag (void);
		virtual void DrawShield (void);
		virtual void DrawShieldBar (void);
		virtual void DrawEnergy (void);
		virtual void DrawEnergyBar (void);
		virtual void DrawAfterburner (void);
		virtual void DrawAfterburnerBar (void);
		virtual void ClearBombCount (int bgColor);
		virtual void DrawBombCount (void);
		virtual int DrawBombCount (int& nIdBombCount, int y, int x, int nColor, char* pszBombCount);
		virtual void DrawPrimaryAmmoInfo (int ammoCount);
		virtual void DrawSecondaryAmmoInfo (int ammoCount);
		virtual void DrawWeapons (void);
		virtual void DrawCloak (void);
		virtual void DrawInvul (void);
		virtual void DrawLives (void);
		virtual void DrawPlayerShip (void) {}
		virtual void DrawStatic (int nWindow);
		virtual void DrawKillList (void);
		virtual void DrawCockpit (bool bAlphaTest = false);
		virtual void Toggle (void);

		virtual void SetupWindow (int nWindow, CCanvas* canvP);
		virtual bool Setup (bool bRebuild);

	private:
		int FlashGauge (int h, int *bFlash, int tToggle);
		inline int LineSpacing (void) {
			return ((gameStates.render.cockpit.nType == CM_FULL_COCKPIT) || (gameStates.render.cockpit.nType == CM_STATUS_BAR))
					 ? m_info.nLineSpacing
					 : 5 * GAME_FONT->Height () / 4;
			}
	};

//	-----------------------------------------------------------------------------

class CWideHUD : public CHUD {
	public:
		virtual void DrawRecording (void);
		virtual void Toggle (void);
		virtual bool Setup (bool bRebuild);
		virtual void SetupWindow (int nWindow, CCanvas* canvP);
	};

//	-----------------------------------------------------------------------------

class CStatusBar : public CGenericCockpit {
	public:
		CBitmap* StretchBlt (int nGauge, int x, int y, double xScale, double yScale, int scale = I2X (1), int orient = 0);

		virtual void GetHostageWindowCoords (int& x, int& y, int& w, int& h);
		virtual void DrawRecording (void);
		virtual void DrawCountdown (void);
		virtual void DrawCruise (void);
		virtual void DrawScore (void);
		virtual void DrawAddedScore (void);
		virtual void DrawHomingWarning (void);
		virtual void DrawKeys (void);
		virtual void DrawOrbs (void);
		virtual void DrawFlag (void);
		virtual void DrawEnergy (void);
		virtual void DrawShield (void);
		virtual void DrawShieldBar (void);
		virtual void DrawEnergyBar (void);
		virtual void DrawAfterburner (void);
		virtual void DrawAfterburnerBar (void);
		virtual void DrawBombCount (void);
		virtual int DrawBombCount (int& nIdBombCount, int y, int x, int nColor, char* pszBombCount);
		virtual void DrawPrimaryAmmoInfo (int ammoCount);
		virtual void DrawSecondaryAmmoInfo (int ammoCount);
		virtual void DrawWeaponInfo (int nWeaponType, int nWeaponId, int laserLevel);
		virtual void DrawCloak (void) {}
		virtual void DrawInvul (void);
		virtual void DrawLives (void);
		virtual void DrawPlayerShip (void);
		virtual void DrawStatic (int nWindow);
		virtual void DrawKillList (void);
		virtual void ClearBombCount (int bgColor);
		virtual void DrawCockpit (bool bAlphaTest = false);
		virtual void Toggle (void);

		virtual void SetupWindow (int nWindow, CCanvas* canvP);
		virtual bool Setup (bool bRebuild);
	};

//	-----------------------------------------------------------------------------

class CCockpit : public CGenericCockpit {
	public:
		virtual void GetHostageWindowCoords (int& x, int& y, int& w, int& h);
		virtual void DrawRecording (void);
		virtual void DrawCountdown (void);
		virtual void DrawCruise (void);
		virtual void DrawScore (void);
		virtual void DrawAddedScore (void);
		virtual void DrawHomingWarning (void);
		virtual void DrawKeys (void);
		virtual void DrawOrbs (void);
		virtual void DrawFlag (void);
		virtual void DrawEnergy (void);
		virtual void DrawEnergyBar (void);
		virtual void DrawAfterburner (void) {}
		virtual void DrawAfterburnerBar (void);
		virtual void DrawBombCount (void);
		virtual int DrawBombCount (int& nIdBombCount, int y, int x, int nColor, char* pszBombCount);
		virtual void DrawPrimaryAmmoInfo (int ammoCount);
		virtual void DrawSecondaryAmmoInfo (int ammoCount);
		virtual void DrawWeaponInfo (int nWeaponType, int nWeaponId, int laserLevel);
		virtual void DrawCloak (void) {}
		virtual void DrawInvul (void);
		virtual void DrawShield (void);
		virtual void DrawShieldBar (void);
		virtual void DrawLives (void);
		virtual void DrawPlayerShip (void);
		virtual void DrawStatic (int nWindow);
		virtual void DrawKillList (void);
		virtual void ClearBombCount (int bgColor);
		virtual void DrawCockpit (bool bAlphaTest = false);
		virtual void Toggle (void);

		virtual void SetupWindow (int nWindow, CCanvas* canvP);
		virtual bool Setup (bool bRebuild);
	};

//	-----------------------------------------------------------------------------

class CRearView : public CGenericCockpit {
	public:
		virtual void GetHostageWindowCoords (int& x, int& y, int& w, int& h) { x = y = w = h = -1; }
		virtual void DrawRecording (void) {}
		virtual void DrawCountdown (void) {}
		virtual void DrawCruise (void) {}
		virtual void DrawScore (void) {}
		virtual void DrawAddedScore (void) {}
		virtual void DrawHomingWarning (void) {}
		virtual void DrawKeys (void) {}
		virtual void DrawOrbs (void) {}
		virtual void DrawFlag (void) {}
		virtual void DrawEnergy (void) {}
		virtual void DrawEnergyBar (void) {}
		virtual void DrawAfterburner (void) {}
		virtual void DrawAfterburnerBar (void) {}
		virtual void DrawBombCount (void) {}
		virtual int DrawBombCount (int& nIdBombCount, int y, int x, int nColor, char* pszBombCount) { return -1; }
		virtual void DrawPrimaryAmmoInfo (int ammoCount) {}
		virtual void DrawSecondaryAmmoInfo (int ammoCount) {}
		virtual void DrawWeaponInfo (int nWeaponType, int nWeaponId, int laserLevel) {}
		virtual void DrawWeapons (void) {}
		virtual void DrawCloak (void) {}
		virtual void DrawInvul (void) {}
		virtual void DrawShield (void) {}
		virtual void DrawShieldBar (void) {}
		virtual void DrawLives (void) {}
		virtual void DrawPlayerShip (void) {}
		virtual void DrawCockpit (void) {}
		virtual void DrawStatic (int nWindow) {}
		virtual void DrawKillList (void) {}
		virtual void ClearBombCount (int bgColor) {}
		virtual void DrawCockpit (bool bAlphaTest = false) { 
			CGenericCockpit::DrawCockpit (CM_REAR_VIEW + m_info.nCockpit, 0, bAlphaTest); 
			}
		virtual void Toggle (void) {};
		virtual void SetupWindow (int nWindow, CCanvas* canvP) {}
		virtual bool Setup (bool bRebuild);
	};

//	-----------------------------------------------------------------------------

extern tRgbColorb playerColors [];

extern CHUD			hudCockpit;
extern CWideHUD	letterboxCockpit;
extern CCockpit	fullCockpit;
extern CStatusBar	statusBarCockpit;
extern CRearView	rearViewCockpit;

extern CGenericCockpit* cockpit;

//	-----------------------------------------------------------------------------

#endif // _COCKPIT_H
