//prototypes opengl functions - Added 9/15/99 Matthew Mueller
#ifndef _OGL_TMU_H
#define _OGL_TMU_H

#ifdef _WIN32
#include <windows.h>
#include <stddef.h>
#endif

#define	USE_DISPLAY_LISTS	0

//------------------------------------------------------------------------------

typedef void tInitTMU (int);
typedef tInitTMU *pInitTMU;

//------------------------------------------------------------------------------

static inline void InitTMU (int i, int bVertexArrays)
{
	static GLuint tmuIds [] = {GL_TEXTURE0, GL_TEXTURE1, GL_TEXTURE2_ARB};

#if !USE_DISPLAY_LISTS
	ogl.SelectTMU (tmuIds [i], bVertexArrays != 0);
	ogl.SetTexturing (true);
	if (i == 0)
		glTexEnvf (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	else if (i == 1)
		glTexEnvi (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL);
	else
		glTexEnvi (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
#else
if (glIsList (g3InitTMU [i][bVertexArrays]))
	glCallList (g3InitTMU [i][bVertexArrays]);
else {
	g3InitTMU [i][bVertexArrays] = glGenLists (1);
	if (g3InitTMU [i][bVertexArrays])
		glNewList (g3InitTMU [i][bVertexArrays], GL_COMPILE);
	ogl.SelectTMU (tmuIds [i], bVertexArrays != 0);
	ogl.SetTexturing (true);
	if (i == 0)
		glTexEnvf (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	else if (i == 1)
		glTexEnvi (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL);
	else
		glTexEnvi (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	if (g3InitTMU [i][bVertexArrays]) {
		glEndList ();
		InitTMU (i, bVertexArrays);
		}
	}
#endif
}

//------------------------------------------------------------------------------

static inline void InitTMU0 (int bVertexArrays, int bLightmaps = 0)
{
#if !USE_DISPLAY_LISTS
	ogl.SelectTMU (GL_TEXTURE0, bVertexArrays != 0);
	ogl.SetTexturing (true);
	glTexEnvi (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
#else
if (glIsList (g3InitTMU [0][bVertexArrays]))
	glCallList (g3InitTMU [0][bVertexArrays]);
else {
	g3InitTMU [0][bVertexArrays] = glGenLists (1);
	if (g3InitTMU [0][bVertexArrays])
		glNewList (g3InitTMU [0][bVertexArrays], GL_COMPILE);
	ogl.SelectTMU (GL_TEXTURE0, bVertexArrays != 0);
	ogl.SetTexturing (true);
	glTexEnvi (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	if (g3InitTMU [0][bVertexArrays]) {
		glEndList ();
		InitTMU0 (bVertexArrays);
		}
	}
#endif
}

//------------------------------------------------------------------------------

static inline void InitTMU1 (int bVertexArrays, int bLightmaps = 0)
{
#if !USE_DISPLAY_LISTS
	ogl.SelectTMU (GL_TEXTURE1, bVertexArrays != 0);
	ogl.SetTexturing (true);
	glTexEnvi (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, bLightmaps ? GL_MODULATE : GL_DECAL);
#else
if (glIsList (g3InitTMU [1][bVertexArrays]))
	glCallList (g3InitTMU [1][bVertexArrays]);
else 
	{
	g3InitTMU [1][bVertexArrays] = glGenLists (1);
	if (g3InitTMU [1][bVertexArrays])
		glNewList (g3InitTMU [1][bVertexArrays], GL_COMPILE);
	ogl.SelectTMU (GL_TEXTURE1, bVertexArrays != 0);
	ogl.SetTexturing (true);
	glTexEnvi (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, bLightmaps ? GL_MODULATE : GL_DECAL);
	if (g3InitTMU [1][bVertexArrays]) {
		glEndList ();
		InitTMU1 (bVertexArrays);
		}
	}
#endif
}

//------------------------------------------------------------------------------

static inline void InitTMU2 (int bVertexArrays, int bLightmaps = 0)
{
#if !USE_DISPLAY_LISTS
	ogl.SelectTMU (GL_TEXTURE2, bVertexArrays != 0);
	ogl.SetTexturing (true);
	glTexEnvi (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, bLightmaps ? GL_DECAL : GL_MODULATE);
#else
if (glIsList (g3InitTMU [2][bVertexArrays]))
	glCallList (g3InitTMU [2][bVertexArrays]);
else {
	g3InitTMU [2][bVertexArrays] = glGenLists (1);
	if (g3InitTMU [2][bVertexArrays])
		glNewList (g3InitTMU [2][bVertexArrays], GL_COMPILE);
	ogl.SelectTMU (GL_TEXTURE2, bVertexArrays != 0);
	ogl.SetTexturing (true);
	glTexEnvi (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, bLightmaps ? GL_DECAL : GL_MODULATE);
	if (g3InitTMU [2][bVertexArrays]) {
		glEndList ();
		InitTMU2 (bVertexArrays);
		}
	}
#endif
}

//------------------------------------------------------------------------------

static inline void InitTMU3 (int bVertexArrays, int bLightmaps = 0)
{
#if !USE_DISPLAY_LISTS
	ogl.SelectTMU (GL_TEXTURE3, bVertexArrays != 0);
	ogl.SetTexturing (true);
	glTexEnvi (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
#else
if (glIsList (g3InitTMU [3][bVertexArrays]))
	glCallList (g3InitTMU [3][bVertexArrays]);
else 
 {
	g3InitTMU [3][bVertexArrays] = glGenLists (1);
	if (g3InitTMU [3][bVertexArrays])
		glNewList (g3InitTMU [3][bVertexArrays], GL_COMPILE);
	ogl.SelectTMU (GL_TEXTURE3, bVertexArrays != 0);
	ogl.SetTexturing (true);
	glTexEnvi (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	if (g3InitTMU [3][bVertexArrays]) {
		glEndList ();
		InitTMU3 (bVertexArrays);
		}
	}
#endif
}

//------------------------------------------------------------------------------

static inline void ExitTMU (int bVertexArrays)
{
#if !USE_DISPLAY_LISTS
	ogl.SelectTMU (GL_TEXTURE3, bVertexArrays != 0);
	ogl.BindTexture (0);
	ogl.SetTexturing (false);
	ogl.SelectTMU (GL_TEXTURE2, bVertexArrays != 0);
	ogl.BindTexture (0);
	ogl.SetTexturing (false);
	ogl.SelectTMU (GL_TEXTURE1, bVertexArrays != 0);
	ogl.BindTexture (0);
	ogl.SetTexturing (false);
	ogl.SelectTMU (GL_TEXTURE0, bVertexArrays != 0);
	ogl.BindTexture (0);
	ogl.SetTexturing (false);
#else
if (glIsList (g3ExitTMU [bVertexArrays]))
	glCallList (g3ExitTMU [bVertexArrays]);
else {
	g3ExitTMU [bVertexArrays] = glGenLists (1);
	if (g3ExitTMU [bVertexArrays])
		glNewList (g3ExitTMU [bVertexArrays], GL_COMPILE);
	ogl.SelectTMU (GL_TEXTURE3, bVertexArrays != 0);
	ogl.BindTexture (0);
	ogl.SetTexturing (false);
	ogl.SelectTMU (GL_TEXTURE2, bVertexArrays != 0);
	ogl.BindTexture (0);
	ogl.SetTexturing (false);
	ogl.SelectTMU (GL_TEXTURE1, bVertexArrays != 0);
	ogl.BindTexture (0);
	ogl.SetTexturing (false);
	ogl.SelectTMU (GL_TEXTURE0, bVertexArrays != 0);
	ogl.BindTexture (0);
	ogl.SetTexturing (false);
	if (g3ExitTMU [bVertexArrays]) {
		glEndList ();
		ExitTMU (bVertexArrays);
		}
	}
#endif
}

//------------------------------------------------------------------------------

#define	G3_BIND(_tmu,_bmP,_lmP,_bClient) \
			ogl.SelectTMU (_tmu, _bClient != 0); \
			if (_bmP) {\
				if ((_bmP)->Bind (1)) \
					return 1; \
				(_bmP) = (_bmP)->CurFrame (-1); \
				(_bmP)->Texture ()->Wrap (GL_REPEAT); \
				} \
			else { \
				ogl.BindTexture ((_lmP)->handle); \
				CTexture::Wrap (GL_CLAMP); \
				}

#define	INIT_TMU(_initTMU,_tmu,_bmP,_lmP,_bClient,bLightmaps) \
			_initTMU (_bClient, bLightmaps); \
			G3_BIND (_tmu,_bmP,_lmP,_bClient)


//------------------------------------------------------------------------------

#endif
