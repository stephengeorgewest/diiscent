#ifdef HAVE_CONFIG_H
#	include <conf.h>
#endif

#include <stdio.h>

#include "descent.h"
#include "u_mem.h"
#include "error.h"
#include "key.h"
#include "ogl_bitmap.h"
#include "menu.h"
#include "gamefont.h"
#include "gamecntl.h"
#include "text.h"
#include "textdata.h"
#include "menubackground.h"

//------------------------------------------------------------------------------

void FreeTextData (CTextData *msgP)
{
delete[] msgP->textBuffer;
msgP->textBuffer = NULL;
delete[] msgP->index;
msgP->index = NULL;
if (msgP->bmP) {
	delete msgP->bmP;
	msgP->bmP = NULL;
	}
msgP->nMessages = 0;
}

//------------------------------------------------------------------------------

void QSortTextData (tTextIndex *indexP, int left, int right)
{
	int	l = left,
			r = right,
			m = indexP [(left + right) / 2].nId;

do {
	while (indexP [l].nId < m)
		l++;
	while (indexP [r].nId > m)
		r--;
	if (l <= r) {
		if (l < r) {
			tTextIndex h = indexP [l];
			indexP [l] = indexP [r];
			indexP [r] = h;
			}
		l++;
		r--;
		}
	} while (l <= r);
if (l < right)
	QSortTextData (indexP, l, right);
if (left < r)
	QSortTextData (indexP, left, r);
}

//------------------------------------------------------------------------------

void LoadTextData (const char *pszLevelName, const char *pszExt, CTextData *msgP)
{
	char			szFilename [SHORT_FILENAME_LEN];
	CFile			cf;
	int			bufSize, nLines;
	char			*p, *q;
	tTextIndex	*pi;

//first, free up data allocated for old bitmaps
PrintLog ("   loading mission messages\n");
FreeTextData (msgP);
CFile::ChangeFilenameExtension (szFilename, pszLevelName, pszExt);
bufSize = cf.Size (szFilename, gameFolders.szDataDir, 0);
if (bufSize <= 0)
	return;
if (!(msgP->textBuffer = new char [bufSize + 2]))
	return;
if (!cf.Open (szFilename, gameFolders.szDataDir, "rb", 0)) {
	FreeTextData (msgP);
	return;
	}
cf.Read (msgP->textBuffer + 1, 1, bufSize);
cf.Close ();
msgP->textBuffer [0] = '\n';
msgP->textBuffer [bufSize + 1] = '\0';
for (p = msgP->textBuffer + 1, nLines = 1; *p; p++) {
	if (*p == '\n')
		nLines++;
	}
if (!(msgP->index = new tTextIndex [nLines])) {
	FreeTextData (msgP);
	return;
	}
msgP->nMessages = nLines;
nLines = 0;
for (p = q = msgP->textBuffer, pi = msgP->index; ; p++) {
	if (!*p) {
		*q++ = *p;
		(pi - 1)->nLines = nLines;
		break;
		}
	else if ((*p == '\r') || (*p == '\n')) {
		if (nLines)
			(pi - 1)->nLines = nLines;
		*q++ = '\0';
		if ((*p == '\r') && (*(p + 1) == '\n'))
			p += 2;
		else
			p++;
		if (!*p) {
			*q++ = '\0';
			break;
			}
		pi->nId = atoi (p);
		if (!pi->nId)
			continue;
		while (::isdigit (*p))
			p++;
		pi->pszText = q;
		pi++;
		nLines = 1;
		}
	else if ((*p == '\\') && (*(p + 1) == 'n')) {
		*q++ = '\n';
		nLines++;
		p++;
		}
	else
		*q++ = *p;
	}
msgP->nMessages = (int) (pi - msgP->index);
QSortTextData (msgP->index, 0, msgP->nMessages - 1);
}

//------------------------------------------------------------------------------

tTextIndex *FindTextData (CTextData *msgP, int nId)
{
	int	h, m, l = 0, r = msgP->nMessages - 1;

do {
	m = (l + r) / 2;
	h = msgP->index [m].nId;
	if (nId > h)
		l = m + 1;
	else if (nId < h)
		r = m - 1;
	else
		return msgP->index + m;
	} while (l <= r);
return NULL;
}

//------------------------------------------------------------------------------

int ShowGameMessage (CTextData *msgP, int nId, int nDuration)
{
	tTextIndex	*indexP;
	short			w, h, x, y;
	float			fAlpha;

if (nId < 0) {
	if (!(indexP = msgP->currentMsg))
		return 0;
	if ((msgP->nEndTime > 0) && (msgP->nEndTime <= gameStates.app.nSDLTicks)) {
		msgP->currentMsg = NULL;
		return 0;
		}
	}
else {
	if (!(indexP = FindTextData (msgP, nId)))
		return 0;
	msgP->currentMsg = indexP;
	msgP->nStartTime = gameStates.app.nSDLTicks;
	msgP->nEndTime = (nDuration < 0) ? -1 : gameStates.app.nSDLTicks + 1000 * nDuration;
	if (msgP->bmP) {
		delete msgP->bmP;
		msgP->bmP = NULL;
		}
	}
if (msgP->nEndTime < 0) {
	if (nId < 0)
		return 0;
	PauseGame ();
	MsgBox (NULL, NULL, 1, TXT_CLOSE, indexP->pszText);
	msgP->currentMsg = NULL;
	ResumeGame ();
	}
else if (!gameStates.render.nWindow) {
	if (!msgP->bmP) {
		fontManager.SetCurrent (NORMAL_FONT);
		fontManager.SetColorRGBi (GOLD_RGBA, 1, 0, 0);
		}
	if (msgP->bmP || (msgP->bmP = CreateStringBitmap (indexP->pszText, 0, 0, NULL, 0, 0, -1))) {
		w = msgP->bmP->Width ();
		h = msgP->bmP->Height ();
		x = (CCanvas::Current ()->Width () - w) / 2;
		y = (CCanvas::Current ()->Height () - h) * 2 / 5;
		if (msgP->nEndTime < 0)
			fAlpha = 1.0f;
		else if (gameStates.app.nSDLTicks - msgP->nStartTime < 250)
			fAlpha = (float) (gameStates.app.nSDLTicks - msgP->nStartTime) / 250.0f;
		else if (msgP->nEndTime - gameStates.app.nSDLTicks < 500)
			fAlpha = (float) (msgP->nEndTime - gameStates.app.nSDLTicks) / 500.0f;
		else
			fAlpha = 1.0f;
		backgroundManager.DrawBox (x - 8, y - 8, x + w + 4, y + h + 4, 3, fAlpha, 1);
		msgP->bmP->Render (CCanvas::Current (), x, y, w, h, 0, 0, w, h, 1, 0, 0, fAlpha);
		}
	}
return 1;
}

//------------------------------------------------------------------------------
//eof
