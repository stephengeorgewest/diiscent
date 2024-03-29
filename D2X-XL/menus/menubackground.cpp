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

#ifdef _WIN32
#	include <windows.h>
#	include <stddef.h>
#endif

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>
#ifndef _WIN32
#	include <unistd.h>
#endif

#include "error.h"
#include "descent.h"
#include "key.h"
#include "text.h"
#include "findfile.h"
#include "gamefont.h"
#include "pcx.h"
#include "u_mem.h"
#include "strutil.h"
#include "ogl_lib.h"
#include "ogl_bitmap.h"
#include "ogl_render.h"
#include "rendermine.h"
#include "menu.h"
#include "input.h"
#include "menubackground.h"

#if defined (TACTILE)
 #include "tactile.height"
#endif

#define LHX(x)      (gameStates.menus.bHires? 2 * (x) : x)
#define LHY(y)      (gameStates.menus.bHires? (24 * (y)) / 10 : y)

CPalette* menuPalette;

CBackgroundManager backgroundManager;

#define MENU_PCX_FULL		menuBgNames [0][!gameStates.app.bDemoData && gameStates.menus.bHires]
#define MENU_PCX_OEM			menuBgNames [1][!gameStates.app.bDemoData && gameStates.menus.bHires]
#define MENU_PCX_SHAREWARE	menuBgNames [2][!gameStates.app.bDemoData && gameStates.menus.bHires]
#define MENU_PCX_MAC_SHARE	menuBgNames [3][!gameStates.app.bDemoData && gameStates.menus.bHires]

int bHiresBackground;

//------------------------------------------------------------------------------

#if DBG

const char* menuBgNames [4][2] = {
	{"menu.pcx", "menub.pcx"},
	{"menuo.pcx", "menuob.pcx"},
	{"menud.pcx", "menud.pcx"},
	{"menub.pcx", "menub.pcx"}
	};

#else

const char* menuBgNames [4][2] = {
	{"\x01menu.pcx", "\x01menub.pcx"},
	{"\x01menuo.pcx", "\x01menuob.pcx"},
	{"\x01menud.pcx", "\x01menud.pcx"},
	{"\x01menub.pcx", "\x01menub.pcx"}
	};
#endif

	static char szBackgrounds [3][2][12] = {
		{"stars.pcx", "starsb.pcx"},
		{"scores.pcx", "scoresb.pcx"},
		{"\x01map.pcx", "\x01mapb.pcx"}
		};

//------------------------------------------------------------------------------

char* MenuPCXName (void)
{
if (CFile::Exist (MENU_PCX_FULL, gameFolders.szDataDir, 0))
	return const_cast<char*> (MENU_PCX_FULL);
if (CFile::Exist (MENU_PCX_OEM, gameFolders.szDataDir, 0))
	return const_cast<char*> (MENU_PCX_OEM);
if (CFile::Exist (MENU_PCX_SHAREWARE, gameFolders.szDataDir, 0))
	return const_cast<char*> (MENU_PCX_SHAREWARE);
return const_cast<char*> (MENU_PCX_MAC_SHARE);
}

//------------------------------------------------------------------------------

char *BackgroundName (int nType, int bHires)
{
return nType ? szBackgrounds [nType - 1][gameStates.app.bDemoData ? 0 : ((bHires < 0) ? gameStates.menus.bHires : bHires)] : MenuPCXName ();
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

void CBackground::Init (void)
{
m_canvas [0] = NULL;
m_canvas [1] = NULL;
m_saved [0] = NULL;
m_saved [1] = NULL;
m_background = NULL;
m_filename = NULL;
m_bIgnoreCanv = false;
m_bIgnoreBg = false;
m_bSetup = false;
gameStates.app.bClearMessage = 0;
}

//------------------------------------------------------------------------------

void CBackground::Destroy (void)
{
if (m_background &&
	 (m_background != backgroundManager.Background (0)) &&
	 (m_background != backgroundManager.Background (1)))
	delete m_background;
if (m_saved [0]) {
	delete m_saved [0];
	m_saved [0] = NULL;
	}
if (m_saved [1]) {
	delete m_saved [1];
	m_saved [1] = NULL;
	}
if (m_canvas [0]) {
	m_canvas [0]->Destroy ();
	m_canvas [0] = NULL;
	}
if (m_canvas [1]) {
	m_canvas [1]->Destroy ();
	m_canvas [1] = NULL;
	}
Init ();
}

//------------------------------------------------------------------------------

CBitmap* CBackground::Load (char* filename, int width, int height)
{
m_filename = filename;
if (!m_filename)
	return (gameOpts->menus.nStyle && !backgroundManager.IsDefault (backgroundManager.Filename ()))
			 ? backgroundManager.Background (0)
			 : backgroundManager.Background (1); //->CreateChild (0, 0, width, height);
else if (backgroundManager.IsDefault (filename) || !(m_background = backgroundManager.LoadBackground (filename)))
	return backgroundManager.Background (0);
return m_background;
}

//------------------------------------------------------------------------------

void CBackground::Setup (int x, int y, int width, int height)
{
if (m_canvas [1])
	m_canvas [1]->Destroy ();
m_canvas [1] = screen.Canvas ()->CreatePane (x, y, width, height);
m_canvas [0] = screen.Canvas ()->CreatePane (0, 0, screen.Width (), screen.Height ());
}

//------------------------------------------------------------------------------

void CBackground::Save (int i, int width, int height)
{
}

//------------------------------------------------------------------------------

bool CBackground::Create (char* filename, int x, int y, int width, int height, bool bTop)
{
Destroy ();
m_bTopMenu = (backgroundManager.Depth () == 0) || bTop;
m_bMenuBox = !gameStates.app.bNostalgia; // && (gameOpts->menus.altBg.bHave > 0);
if (!(m_background = Load (filename, width, height)))
	return false;
Setup (x, y, width, height);
Draw (false);
return true;
}

//------------------------------------------------------------------------------

void CBackground::Draw (bool bDrawBox, bool bUpdate)
{
//paletteManager.SetEffect (0, 0, 0);
if (!(gameStates.menus.bNoBackground || (gameStates.app.bGameRunning && !gameStates.app.bNostalgia))) {
	if (m_filename) {
		CCanvas::Push ();
		CCanvas::SetCurrent (m_canvas [0]);
		m_background->RenderStretched ();
		PrintVersionInfo ();
		CCanvas::Pop ();
		}
	}
if (bDrawBox && !((gameStates.app.bNostalgia && m_bTopMenu) || backgroundManager.IsDefault (m_filename))) {
	CCanvas::Push ();
	CCanvas::SetCurrent (m_canvas [1]);
	if (m_bMenuBox)
		backgroundManager.DrawBox (0, 0, m_canvas [1]->Width (), m_canvas [1]->Height (), gameData.menu.nLineWidth, 1.0f, 0);
	else
		DrawArea (0, 0, m_canvas [1]->Width (), m_canvas [1]->Height ());
		CCanvas::Pop ();
					//0, 0, CCanvas::Current ()->Right (), CCanvas::Current ()->Bottom ());
		//CCanvas::Current ()->Left (), CCanvas::Current ()->Top (),
		//			 CCanvas::Current ()->Right (), CCanvas::Current ()->Bottom ());
	}
//paletteManager.ResumeEffect ();
if (bUpdate && !gameStates.app.bGameRunning)
	GrUpdate (0);
}

//------------------------------------------------------------------------------
// Redraw a part of the menu area's background

void CBackground::DrawArea (int left, int top, int right, int bottom)
{
if (left < 0)
	left = 0;
if (top < 0)
	top = 0;
int width = right - left + 1;
int height = bottom - top + 1;
//if (width > nmBackground.Width ()) width = nmBackground.Width ();
//if (height > nmBackground.Height ()) height = nmBackground.Height ();
right = left + width - 1;
bottom = top + height - 1;
ogl.SetBlending (false);
if (!backgroundManager.Shadow ()) {
	CCanvas::Current ()->SetLeft (CCanvas::Current ()->Left () + LHX (10));
	CCanvas::Current ()->SetTop (CCanvas::Current ()->Top () + LHX (10));
	m_background->RenderFixed (NULL, left, top, width, height); //, LHX (10), LHY (10));
	CCanvas::Current ()->SetLeft (CCanvas::Current ()->Left () - LHX (10));
	CCanvas::Current ()->SetTop (CCanvas::Current ()->Top () - LHX (10));
	}
else {
	m_background->RenderFixed (NULL, left, top, width, height); //, 0, 0);
	gameStates.render.grAlpha = GrAlpha (2 * 7);
	ogl.SetBlending (true);
	ogl.SetBlendMode (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	CCanvas::Current ()->SetColorRGB (0, 0, 0, 200);
	OglDrawFilledRect (right - 5, top + 5, right - 6, bottom - 5);
	OglDrawFilledRect (right - 4, top + 4, right - 5, bottom - 5);
	OglDrawFilledRect (right - 3, top + 3, right - 4, bottom - 5);
	OglDrawFilledRect (right - 2, top + 2, right - 3, bottom - 5);
	OglDrawFilledRect (right - 1, top + 1, right - 2, bottom - 5);
	OglDrawFilledRect (right + 0, top + 0, right - 1, bottom - 5);
	OglDrawFilledRect (left + 5, bottom - 5, right, bottom - 6);
	OglDrawFilledRect (left + 4, bottom - 4, right, bottom - 5);
	OglDrawFilledRect (left + 3, bottom - 3, right, bottom - 4);
	OglDrawFilledRect (left + 2, bottom - 2, right, bottom - 3);
	OglDrawFilledRect (left + 1, bottom - 1, right, bottom - 2);
	OglDrawFilledRect (left + 0, bottom - 0, right, bottom - 1);

	CCanvas::Current ()->SetColorRGB (255, 255, 255, 50);
	OglDrawFilledRect (left, top + 0, right - 1, top + 1);
	OglDrawFilledRect (left, top + 1, right - 2, top + 2);
	OglDrawFilledRect (left, top + 2, right - 3, top + 3);
	OglDrawFilledRect (left, top + 3, right - 4, top + 4);
	OglDrawFilledRect (left, top + 4, right - 5, top + 5);
	OglDrawFilledRect (left, top + 5, right - 6, top + 6);
	OglDrawFilledRect (left + 0, top + 6, left + 1, bottom - 1);
	OglDrawFilledRect (left + 1, top + 6, left + 2, bottom - 2);
	OglDrawFilledRect (left + 2, top + 6, left + 3, bottom - 3);
	OglDrawFilledRect (left + 3, top + 6, left + 4, bottom - 4);
	OglDrawFilledRect (left + 4, top + 6, left + 5, bottom - 5);
	OglDrawFilledRect (left + 5, top + 6, left + 6, bottom - 6);
	ogl.SetBlending (false);
	}
gameStates.render.grAlpha = 1.0f;
}

//------------------------------------------------------------------------------

void CBackground::Restore (void)
{
if (!gameStates.app.bGameRunning) {
	CCanvas::SetCurrent (m_canvas [0]);
	m_background->RenderStretched ();
	}
}

//------------------------------------------------------------------------------

void CBackground::Restore (int dx, int dy, int w, int h, int sx, int sy)
{
int x1 = sx;
int x2 = sx+w-1;
int y1 = sy;
int y2 = sy+h-1;

if (x1 < 0)
	x1 = 0;
if (y1 < 0)
	y1 = 0;

if (x2 >= m_background->Width ())
	x2 = m_background->Width () - 1;
if (y2 >= m_background->Height ())
	y2 = m_background->Height () - 1;

w = x2 - x1 + 1;
h = y2 - y1 + 1;
m_background->Render (CCanvas::Current (), dx, dy, w, h, x1, y1, w, h);
}

//------------------------------------------------------------------------------

CBackgroundManager::CBackgroundManager ()
{
Init ();
}

//------------------------------------------------------------------------------

void CBackgroundManager::Remove (void)
{
if (m_nDepth >= 0) {
	m_bg [m_nDepth--].Destroy ();
	Redraw (true);
	//paletteManager.ResumeEffect (gameStates.app.bGameRunning);
	}
}

//------------------------------------------------------------------------------

void CBackgroundManager::Init (void)
{
m_background [0] = NULL;
m_background [1] = NULL;
m_filename [0] = BackgroundName (BG_MENU);
m_filename [1] = BackgroundName (BG_SCORES);
m_nDepth = -1;
m_bShadow = true;
m_bValid = false;
}

//------------------------------------------------------------------------------

void CBackgroundManager::Destroy (void)
{
while (m_nDepth >= 0)
	m_bg [m_nDepth--].Destroy ();
if (m_background [1] && (m_background [1] != m_background [0]))
	m_background [1]->Destroy ();
if (m_background [0])
	m_background [0]->Destroy ();
Init ();
}

//------------------------------------------------------------------------------

void CBackgroundManager::DrawBox (int left, int top, int right, int bottom, int nLineWidth, float fAlpha, int bForce)
{
gameStates.render.nFlashScale = 0;
if (left <= 0)
	left = 1;
if (top <= 0)
	top = 1;
if (right >= screen.Width ())
	right = screen.Width () - 1;
if (bottom >= screen.Height ())
	bottom = screen.Height () - 1;
CCanvas::Current ()->SetColorRGB (PAL2RGBA (22), PAL2RGBA (22), PAL2RGBA (38), (ubyte) (gameData.menu.alpha * fAlpha));
ogl.SetTexturing (false);
OglDrawFilledRect (left, top, right, bottom);
CCanvas::Current ()->SetColorRGB (PAL2RGBA (22), PAL2RGBA (22), PAL2RGBA (38), 255);
glLineWidth (GLfloat (nLineWidth) * sqrt (GLfloat (screen.Width ()) / 640.0f));
OglDrawEmptyRect (left, top, right, bottom);
glLineWidth (1);
}

//------------------------------------------------------------------------------

void CBackgroundManager::Redraw (bool bUpdate)
{
if (m_nDepth < 0)
	return;
if (gameStates.app.bGameRunning)
	Draw (true);
else {
	for (int i = 0; i <= m_nDepth; i++)
		m_bg [i].Draw (i == m_nDepth);
	if (bUpdate)
		GrUpdate (1);
	}
}

//------------------------------------------------------------------------------

bool CBackgroundManager::IsDefault (char* filename)
{
return filename && !strcmp (filename, m_filename [0]);
}

//------------------------------------------------------------------------------

CBitmap* CBackgroundManager::LoadCustomBackground (void)
{
if (m_background [0] || gameStates.app.bNostalgia)
	return m_background [0];

CBitmap* bmP;

gameOpts->menus.altBg.bHave = 0;
if (!(bmP = CBitmap::Create (0, 0, 0, 1)))
	return NULL;
if (!ReadTGA (gameOpts->menus.altBg.szName,
				  gameFolders.szWallpaperDir,
				  bmP,
				 (gameOpts->menus.altBg.alpha < 0) ? -1 : (int) (gameOpts->menus.altBg.alpha * 255),
				 gameOpts->menus.altBg.brightness, gameOpts->menus.altBg.grayscale, 0)) {
	delete bmP;
	gameOpts->menus.altBg.bHave = -1;
	return NULL;
	}
gameOpts->menus.altBg.bHave = 1;
return bmP;
}

//------------------------------------------------------------------------------

CBitmap* CBackgroundManager::LoadBackground (char* filename)
{
	int width, height;

if (PCXGetDimensions (filename, &width, &height) != PCX_ERROR_NONE) {
	Error ("Could not open menu background file <%s>\n", filename);
	return NULL;
	}
CBitmap* bmP;
if (!(bmP = CBitmap::Create (0, width, height, 1))) {
	Error ("Not enough memory for menu backgroun\n");
	return NULL;
	}
if (PCXReadBitmap (filename, bmP, bmP->Mode (), 0) != PCX_ERROR_NONE) {
	Error ("Could not read menu background file <%s>\n", filename);
	return NULL;
	}
bmP->SetName (filename);
bmP->SetTranspType (3);
bmP->Bind (0);
return bmP;
}

//------------------------------------------------------------------------------

void CBackgroundManager::Create (void)
{
if (!m_bValid) {
	if ((m_background [0] = LoadCustomBackground ()))
		m_background [1] = m_background [0];
	else {
		m_background [0] = LoadBackground (m_filename [0]);
		m_background [1] = LoadBackground (m_filename [1]);
		}
	m_bValid = true;
	}
}

//------------------------------------------------------------------------------

bool CBackgroundManager::Setup (char *filename, int x, int y, int width, int height, bool bTop)
{
Create ();
if (m_nDepth >= 2)
	return false;
if (!m_bg [++m_nDepth].Create (filename, x, y, width, height, bTop))
	return false;
Redraw ();
return true;
}

//------------------------------------------------------------------------------

void CBackgroundManager::Rebuild (void)
{
Destroy ();
Setup (BackgroundName (BG_MENU), 0, 0, screen.Width (), screen.Height ());
GrUpdate (0);
}

//------------------------------------------------------------------------------

void CBackgroundManager::LoadStars (bool bTop)
{
Setup (BackgroundName (BG_STARS), 0, 0, CCanvas::Current ()->Width (), CCanvas::Current ()->Height (), bTop);
}

//------------------------------------------------------------------------------
