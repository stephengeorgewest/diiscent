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
COPYRIGHT 1993-1998 PARALLAX SOFTWARE CORPORATION.  ALL RIGHTS RESERVED.
*/

#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#include "descent.h"
#include "3d.h"
#include "globvars.h"
#include "ogl_defs.h"
#include "ogl_lib.h"
#include "oof.h"

void ScaleMatrix (int bOglScale);

//------------------------------------------------------------------------------
//set view from x,y,z & p,b,h, xZoom.  Must call one of g3_setView_*()
void G3SetViewAngles (CFixVector *vPos, CAngleVector *mOrient, fix xZoom)
{
transformation.m_info.zoom = xZoom;
transformation.m_info.pos = *vPos;
transformation.m_info.view [0] = CFixMatrix::Create (*mOrient);
transformation.m_info.posf [0].Assign (transformation.m_info.pos);
transformation.m_info.viewf [0].Assign (transformation.m_info.view [0]);
ScaleMatrix (1);
}

//------------------------------------------------------------------------------
//set view from x,y,z, viewer matrix, and xZoom.  Must call one of g3_setView_*()

#define ZSCREEN 10.0

void G3SetViewMatrix (const CFixVector& vPos, const CFixMatrix& mOrient, fix xZoom, int bOglScale, fix xStereoSeparation)
{
transformation.m_info.zoom = xZoom;
transformation.m_info.zoomf = (float) xZoom / 65536.0f;
transformation.m_info.pos = vPos;
transformation.m_info.posf [0].Assign (transformation.m_info.pos);
transformation.m_info.posf [1].Assign (transformation.m_info.pos);
if (xStereoSeparation && (gameOpts->render.n3DMethod == STEREO_TOE_IN)) {
	fix zScreen = F2X (ogl.ZScreen () * 10.0);
	CFixVector o = CFixVector::Create (fix (xStereoSeparation / 2), 0, 0);
	CFixVector h = CFixVector::Create (fix (xStereoSeparation / 2), 0, zScreen);
	CFixVector::Normalize (o);
	CFixVector::Normalize (h);
	fix d = CFixVector::Dot (h, o);
	CAngleVector a = CAngleVector::Create (0, 0, (xStereoSeparation < 0) ? d : -d);
	CFixMatrix r = CFixMatrix::Create (a);
	transformation.m_info.view [0] = const_cast<CFixMatrix&> (mOrient) * r;
	}
else
	transformation.m_info.view [0] = mOrient;
transformation.m_info.viewf [0].Assign (transformation.m_info.view [0]);
ScaleMatrix (bOglScale);
CFixMatrix::Transpose (transformation.m_info.viewf [2], transformation.m_info.view [0]);
ogl.SetupProjection ();
}

//------------------------------------------------------------------------------
//performs aspect scaling on global view matrix
void ScaleMatrix (int bOglScale)
{
	transformation.m_info.view [1] = transformation.m_info.view [0];		//so we can use unscaled if we want
	transformation.m_info.viewf [1] = transformation.m_info.viewf [0];		//so we can use unscaled if we want

transformation.m_info.scale = transformation.m_info.aspect;
if (transformation.m_info.zoom <= I2X (1)) 		//xZoom in by scaling z
	transformation.m_info.scale [Z] = FixMul (transformation.m_info.scale [Z], transformation.m_info.zoom);
else {			//xZoom out by scaling x&y
	fix s = FixDiv (I2X (1), transformation.m_info.zoom);

	transformation.m_info.scale [X] = FixMul (transformation.m_info.scale [X], s);
	transformation.m_info.scale [Y] = FixMul (transformation.m_info.scale [Y], s);
	}
transformation.m_info.scalef.Assign (transformation.m_info.scale);
//transformation.m_info.scale [X] = transformation.m_info.scale [Y] = transformation.m_info.scale [Z] = I2X (1);
//now scale matrix elements
if (bOglScale) {
#if 0
	glScalef (transformation.m_info.scalef [X], transformation.m_info.scalef [Y], -transformation.m_info.scalef [Z]);
#else
	glScalef (1, 1, -1);
#endif
	}
else {
	transformation.m_info.view [0].RVec () *= (transformation.m_info.scale [X]);
	transformation.m_info.view [0].UVec () *= (transformation.m_info.scale [Y]);
	transformation.m_info.view [0].FVec () *= (transformation.m_info.scale [Z]);
	transformation.m_info.viewf [0].Assign (transformation.m_info.view [0]);
	glScalef (1, 1, -1);
	}
}

//------------------------------------------------------------------------------
//eof
