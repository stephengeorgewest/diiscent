#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#include <string.h>	// for memset
#include <stdio.h>
#include <time.h>
#include <math.h>

#include "descent.h"
#include "newdemo.h"
#include "network.h"
#include "interp.h"
#include "ogl_lib.h"
#include "rendermine.h"
#include "transprender.h"
#include "glare.h"
#include "sphere.h"
#include "marker.h"
#include "fireball.h"
#include "objsmoke.h"
#include "objrender.h"
#include "objeffects.h"
#include "hiresmodels.h"
#include "hitbox.h"

#ifndef fabsf
#	define fabsf(_f)	(float) fabs (_f)
#endif

//------------------------------------------------------------------------------

void TransformHitboxf (CObject *objP, CFloatVector *vertList, int iSubObj)
{

	CFloatVector		hv;
	tHitbox*				phb = gameData.models.hitboxes [objP->rType.polyObjInfo.nModel].hitboxes + iSubObj;
	CFixVector			vMin = phb->vMin;
	CFixVector			vMax = phb->vMax;
	int					i;

for (i = 0; i < 8; i++) {
	hv [X] = X2F (hitBoxOffsets [i][X] ? vMin [X] : vMax [X]);
	hv [Y] = X2F (hitBoxOffsets [i][Y] ? vMin [Y] : vMax [Y]);
	hv [Z] = X2F (hitBoxOffsets [i][Z] ? vMin [Z] : vMax [Z]);
	transformation.Transform (vertList [i], hv, 0);
	}
}

//------------------------------------------------------------------------------

#if RENDER_HITBOX

void RenderHitbox (CObject *objP, float red, float green, float blue, float alpha)
{
if (objP->rType.polyObjInfo.nModel < 0)
	return;

	CFloatVector	v;
	tHitbox*			pmhb = gameData.models.hitboxes [objP->rType.polyObjInfo.nModel].hitboxes.Buffer ();
	tCloakInfo		ci = {0, FADE_LEVELS, 0, 0, 0, 0, 0};
	int				i, j, iBox, nBoxes, bHit = 0;
	float				fFade;

if (!SHOW_OBJ_FX)
	return;
if (objP->info.nType == OBJ_PLAYER) {
	if (!EGI_FLAG (bPlayerShield, 0, 1, 0))
		return;
	if (gameData.multiplayer.players [objP->info.nId].flags & PLAYER_FLAGS_CLOAKED) {
		if (!GetCloakInfo (objP, 0, 0, &ci))
			return;
		fFade = (float) ci.nFadeValue / (float) FADE_LEVELS;
		red *= fFade;
		green *= fFade;
		blue *= fFade;
		}

	}
else if (objP->info.nType == OBJ_ROBOT) {
	if (!(gameOpts->render.effects.bEnabled && gameOpts->render.effects.bRobotShields))
		return;
	if (objP->cType.aiInfo.CLOAKED) {
		if (!GetCloakInfo (objP, 0, 0, &ci))
			return;
		fFade = (float) ci.nFadeValue / (float) FADE_LEVELS;
		red *= fFade;
		green *= fFade;
		blue *= fFade;
		}
	}
if (!EGI_FLAG (nHitboxes, 0, 0, 0)) {
	DrawShieldSphere (objP, red, green, blue, alpha, 1);
	return;
	}
else if (extraGameInfo [IsMultiGame].nHitboxes == 1) {
	iBox =
	nBoxes = 0;
	}
else {
	iBox = 1;
	nBoxes = gameData.models.hitboxes [objP->rType.polyObjInfo.nModel].nHitboxes;
	}
ogl.SetDepthMode (GL_LEQUAL);
ogl.SetBlending (true);
ogl.SetBlendMode (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
ogl.SetTexturing (false);
ogl.SetDepthWrite (false);

tBox hb [MAX_HITBOXES + 1];
TransformHitboxes (objP, &objP->info.position.vPos, hb);

//transformation.Begin (objP->info.position.vPos, objP->info.position.mOrient);
for (; iBox <= nBoxes; iBox++) {
#if 0
	if (iBox)
		transformation.Begin (pmhb [iBox].vOffset, CAngleVector::ZERO);
	TransformHitboxf (objP, vertList, iBox);
#endif
	glColor4f (red, green, blue, alpha / 2);
	glBegin (GL_QUADS);
	for (i = 0; i < 6; i++) {
		for (j = 0; j < 4; j++) {
			v.Assign (hb [iBox].faces [i].v [j]);
			transformation.Transform (v, v, 0);
			glVertex3fv (reinterpret_cast<GLfloat*> (&v));
		//	glVertex3fv (reinterpret_cast<GLfloat*> (vertList + hitboxFaceVerts [i][j]));
			}
		}
	glEnd ();
	glLineWidth (3);
	glColor4f (red, green, blue, alpha);
	for (i = 0; i < 6; i++) {
		CFixVector c;
		c.SetZero ();
		glColor4f (red, green, blue, alpha / 2);
		glBegin (GL_LINES);
		for (j = 0; j < 4; j++) {
			c += hb [iBox].faces [i].v [j];
			v.Assign (hb [iBox].faces [i].v [j]);
			transformation.Transform (v, v, 0);
			glVertex3fv (reinterpret_cast<GLfloat*> (&v));
			//glVertex3fv (reinterpret_cast<GLfloat*> (vertList + hitboxFaceVerts [i][j]));
			}
		c /= I2X (4);
		v.Assign (c);
		glColor4f (1.0f, 0.5f, 0.0f, alpha);
		transformation.Transform (v, v, 0);
		glVertex3fv (reinterpret_cast<GLfloat*> (&v));
		v.Assign (c + hb [iBox].faces [i].n [1] * I2X (5));
		transformation.Transform (v, v, 0);
		glVertex3fv (reinterpret_cast<GLfloat*> (&v));
		glEnd ();
		}
	glLineWidth (1);
//	if (iBox)
//		transformation.End ();
	}
//transformation.End ();
float r = X2F (CFixVector::Dist (pmhb->vMin, pmhb->vMax) / 2);
#if 0 //DBG //display collision point
if (gameStates.app.nSDLTicks - gameData.models.hitboxes [objP->rType.polyObjInfo.nModel].tHit < 500) {
	CObject	o;

	o.info.position.vPos = gameData.models.hitboxes [objP->rType.polyObjInfo.nModel].vHit;
	o.info.position.mOrient = objP->info.position.mOrient;
	o.info.xSize = I2X (2);
	objP->rType.polyObjInfo.nModel = -1;
	//SetRenderView (0, NULL);
	DrawShieldSphere (&o, 1, 0, 0, 0.33f, 1);
	}
#endif
ogl.SetDepthWrite (true);
ogl.SetDepthMode (GL_LESS);
}

#endif

// -----------------------------------------------------------------------------

void RenderPlayerShield (CObject *objP)
{
	int			bStencil, dt = 0, i = objP->info.nId, nColor = 0;
	float			alpha, scale = 1;
	tCloakInfo	ci;

	static tRgbaColorf shieldColors [3] = {{0, 0.5f, 1, 1}, {1, 0.5f, 0, 1}, {1, 0.8f, 0.6f, 1}};

if (!SHOW_OBJ_FX)
	return;
if (SHOW_SHADOWS &&
	 (FAST_SHADOWS ? (gameStates.render.nShadowPass != 1) : (gameStates.render.nShadowPass != 3)))
	return;
if (EGI_FLAG (bPlayerShield, 0, 1, 0)) {
	if (gameData.multiplayer.players [i].flags & PLAYER_FLAGS_CLOAKED) {
		if (!GetCloakInfo (objP, 0, 0, &ci))
			return;
		scale = (float) ci.nFadeValue / (float) FADE_LEVELS;
		scale *= scale;
		}
	bStencil = ogl.StencilOff ();
	gameData.render.shield.SetPulse (gameData.multiplayer.spherePulse + i);
	if (gameData.multiplayer.bWasHit [i]) {
		if (gameData.multiplayer.bWasHit [i] < 0) {
			gameData.multiplayer.bWasHit [i] = 1;
			gameData.multiplayer.nLastHitTime [i] = gameStates.app.nSDLTicks;
			SetupSpherePulse (gameData.multiplayer.spherePulse + i, 0.1f, 0.5f);
			dt = 0;
			}
		else if ((dt = gameStates.app.nSDLTicks - gameData.multiplayer.nLastHitTime [i]) >= 300) {
			gameData.multiplayer.bWasHit [i] = 0;
			SetupSpherePulse (gameData.multiplayer.spherePulse + i, 0.02f, 0.4f);
			}
		}
#if !RENDER_HITBOX
	if (gameOpts->render.effects.bOnlyShieldHits && !gameData.multiplayer.bWasHit [i])
		return;
#endif
	if (gameData.multiplayer.players [i].flags & PLAYER_FLAGS_INVULNERABLE)
		nColor = 2;
	else if (gameData.multiplayer.bWasHit [i])
		nColor = 1;
	else
		nColor = 0;
	if (gameData.multiplayer.bWasHit [i]) {
		alpha = (gameOpts->render.effects.bOnlyShieldHits ? (float) cos (sqrt ((double) dt / float (SHIELD_EFFECT_TIME)) * Pi / 2) : 1);
		scale *= alpha;
		}
	else if (gameData.multiplayer.players [i].flags & PLAYER_FLAGS_INVULNERABLE)
		alpha = 1;
	else {
		alpha = X2F (gameData.multiplayer.players [i].shields) / 100.0f;
		scale *= alpha;
		if (gameData.multiplayer.spherePulse [i].fSpeed == 0.0f)
			SetupSpherePulse (gameData.multiplayer.spherePulse + i, 0.02f, 0.5f);
		}
#if RENDER_HITBOX
	RenderHitbox (objP, shieldColors [nColor].red * scale, shieldColors [nColor].green * scale, shieldColors [nColor].blue * scale, alpha);
#else
	DrawShieldSphere (objP, shieldColors [nColor].red * scale, shieldColors [nColor].green * scale, shieldColors [nColor].blue * scale, alpha, 1);
#endif
	ogl.StencilOn (bStencil);
	}
}

// -----------------------------------------------------------------------------

void RenderRobotShield (CObject *objP)
{
	static tRgbaColorf shieldColors [3] = {{0.75f, 0, 0.75f, 1}, {0, 0.5f, 1},{1, 0.1f, 0.25f, 1}};

#if RENDER_HITBOX
RenderHitbox (objP, 0.5f, 0.0f, 0.6f, 0.4f);
#else
#endif
#if 1
	float			scale = 1;
	tCloakInfo	ci;
	fix			dt;

if (!(gameOpts->render.effects.bEnabled && gameOpts->render.effects.bRobotShields))
	return;
if ((objP->info.nType == OBJ_ROBOT) && objP->cType.aiInfo.CLOAKED) {
	if (!GetCloakInfo (objP, 0, 0, &ci))
		return;
	scale = (float) ci.nFadeValue / (float) FADE_LEVELS;
	scale *= scale;
	}
dt = gameStates.app.nSDLTicks - objP->TimeLastHit ();
if (dt < SHIELD_EFFECT_TIME) {
	scale *= gameOpts->render.effects.bOnlyShieldHits ? float (cos (sqrt (float (dt) / float (SHIELD_EFFECT_TIME)) * Pi / 2)) : 1;
	DrawShieldSphere (objP, shieldColors [2].red * scale, shieldColors [2].green * scale, shieldColors [2].blue * scale, 0.5f * scale, 1);
	}
else if (!gameOpts->render.effects.bOnlyShieldHits) {
	if ((objP->info.nType != OBJ_ROBOT) || ROBOTINFO (objP->info.nId).companion)
		DrawShieldSphere (objP, 0.0f, 0.5f * scale, 1.0f * scale, objP->Damage () / 2 * scale, 1);
	else
		DrawShieldSphere (objP, 0.75f * scale, 0.0f, 0.75f * scale, objP->Damage () / 2 * scale, 1);
	}
#endif
}

//------------------------------------------------------------------------------
//eof
