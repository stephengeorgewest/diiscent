#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <ogcsys.h>
#include <gccore.h>

#include "glint.h"
#include "GL/gl.h"

void CopyMtxToMtx44(const MtxP src, Mtx44P dest)
{
	size_t i,j;
	for(i = 0; i < 3; ++i)
	{
		for(j = 0; j < 4; ++j)
		{
			dest[i][j] = src[i][j];
		}
	}
	
	for(j = 0; j < 4; ++j)
		dest[3][j] = 0.0f;
}

void CopyMtx44ToMtx(const Mtx44P src, MtxP dest)
{
	size_t i,j;
	for(i = 0; i < 3; ++i)
	{
		for(j = 0; j < 4; ++j)
		{
			dest[i][j] = src[i][j];
		}
	}
}

static void guessProjection(PrjMtx *m)
{
	if(m->mat[0][3] != 0 || m->mat[1][3] != 0)
	{
		// Problably Orthographic
		m->mode = GX_ORTHOGRAPHIC;
	}
	else
	{
		// Who knows, assume perspective
		m->mode = GX_PERSPECTIVE;
	}
}

static void computeZplanes(PrjMtx *m)
{
	f32 c = m->mat[2][2];
	f32 d = m->mat[2][3];
	
	guessProjection(m);
	
	// Reverse the equations in glFrustum and glOrtho to compute z-plane locations
	if(m->mode == GX_PERSPECTIVE)
	{
		m->farZclip = d/(c+1);
		m->nearZclip = d/(c-1);
	}
	else
	{
		m->nearZclip = (d+1)/c;
		m->farZclip = (d-1)/c;
	}
}

void glFrustum( GLdouble left,
			  GLdouble right,
			  GLdouble bottom,
			  GLdouble top,
			  GLdouble zNear,
			  GLdouble zFar	)
{
	NO_CALL_IN_BEGIN;
	
	if(cur_mode == GL_PROJECTION)
	{
		PrjMtx * p = TopPrjMtx(&projection_stack);
		Mtx44 *tmp = TopMtx44(&projection_stack);
		
		if(tmp == NULL)
		{
			return;
		}
		
		guFrustum(*tmp,top,bottom,left,right,zNear,zFar);
		p->mode = GX_PERSPECTIVE;
	
		p->nearZclip = zNear;
		p->farZclip = zFar;
	}
	else
	{
		Mtx44 tmp;
		Mtx *cur_mat = TopMtx(curmtx);
		
		guFrustum(tmp,top,bottom,left,right,zNear,zFar);
	
		CopyMtx44ToMtx(tmp, *cur_mat);
	}
}

void glOrtho(GLdouble left, 
 	GLdouble right, 
 	GLdouble bottom, 
 	GLdouble top, 
 	GLdouble nearVal, 
 	GLdouble farVal)
{
	NO_CALL_IN_BEGIN;
	
	if(left == right || bottom == top)
	{
		_glSetError(GL_INVALID_VALUE);
		return;		
	}
	
	if(cur_mode == GL_PROJECTION)
	{
		PrjMtx * p = TopPrjMtx(&projection_stack);
				
		if(p == NULL)
		{
			return;
		}
		
	
		guOrtho(p->mat,top,bottom,left,right,nearVal,farVal);
		p->mode = GX_ORTHOGRAPHIC;
	
		p->nearZclip = nearVal;
		p->farZclip = farVal;
	}
	else
	{
		Mtx44 tmp;
		Mtx *cur_mat = TopMtx(curmtx);
		
		if(cur_mat == NULL)
		{
			return;
		}
		
		guOrtho(tmp,top,bottom,left,right,nearVal,farVal);
	
		CopyMtx44ToMtx(tmp, *cur_mat);
	}
}

void glMatrixMode( GLenum mode )
{
	NO_CALL_IN_BEGIN;
	
	switch(mode)
	{
	case GL_MODELVIEW:
		curmtx = &model_stack;
		break;
	case GL_PROJECTION:
		curmtx = &projection_stack;
		break;
	case GL_TEXTURE:
		curmtx = texture_stack+cur_tex;
		break;
	default:
		_glSetError(GL_INVALID_ENUM);
		return;
		break;
		
	}
	cur_mode = mode;
}

void glLoadIdentity(void) {
	NO_CALL_IN_BEGIN;
	
	if(cur_mode == GL_PROJECTION)
	{
		PrjMtx * p = TopPrjMtx(&projection_stack);
				
		if(p == NULL)
		{
			return;
		}
		
		int i;
		
		Mtx44P cur_mat = p->mat;
		
		memset(cur_mat, 0, sizeof(Mtx44));
		
		for(i = 0; i < 4; ++i)
		{
			cur_mat[i][i] = 1.0;
		}
		
		p->mode = GX_ORTHOGRAPHIC;
		p->nearZclip = 1;
		p->farZclip = -1;
	}
	else
	{
		Mtx *cur_mat = TopMtx(curmtx);
		
		if(cur_mat == NULL)
		{
			return;
		}
		
		guMtxIdentity(*cur_mat);
	}
}

void glLoadMatrixf( const GLfloat *m )
{
	NO_CALL_IN_BEGIN;
	
	size_t i,j;
	
	if(cur_mode == GL_PROJECTION)
	{
		Mtx44 *cur_mat = TopMtx44(curmtx);
		
		if(cur_mat == NULL)
		{
			return;
		}
		
		for(j = 0; j < 4; ++j)
		{	
			for(i = 0; i < 4; ++i)
			{
				(*cur_mat)[i][j] = *m++;
			}
		}
		
		computeZplanes(TopPrjMtx(curmtx));
	}
	else
	{
		Mtx *cur_mat = TopMtx(curmtx);
		
		if(cur_mat == NULL)
		{
			return;
		}
				
		if(!cur_mat)
			return;
	
		for(j = 0; j < 4; ++j)
		{	
			for(i = 0; i < 3; ++i)
			{
				(*cur_mat)[i][j] = *m++;
			}
			m++;
		}
	}
}

void glLoadMatrixd( const GLdouble *m)
{
	NO_CALL_IN_BEGIN;
	
	size_t i,j;
	
	if(cur_mode == GL_PROJECTION)
	{
		Mtx44 *cur_mat = TopMtx44(curmtx);
		
		if(cur_mat == NULL)
		{
			return;
		}
		
		for(j = 0; j < 4; ++j)
		{	
			for(i = 0; i < 4; ++i)
			{
				(*cur_mat)[i][j] = *m++;
			}
		}
		
		computeZplanes(TopPrjMtx(curmtx));
	}
	else
	{
		Mtx *cur_mat = TopMtx(curmtx);
		
		if(cur_mat == NULL)
		{
			return;
		}
		
		for(j = 0; j < 4; ++j)
		{	
			for(i = 0; i < 3; ++i)
			{
				(*cur_mat)[i][j] = *m++;
			}
			m++;
		}
	}
}

void glTranslatef( GLfloat x, GLfloat y, GLfloat z ) {
	NO_CALL_IN_BEGIN;
	
	if(cur_mode == GL_PROJECTION)
	{
		// 4x4 Matrix translation not provided by gu, hack it
		GLfloat arr[16];
		
		arr[0] = 1; arr[4] = 0; arr[ 8] = 0; arr[12] = x;
		arr[1] = 0; arr[5] = 1; arr[ 9] = 0; arr[13] = y;
		arr[2] = 0; arr[6] = 0; arr[10] = 1; arr[14] = z;
		arr[3] = 0; arr[7] = 0; arr[11] = 0; arr[15] = 1;
		
		glMultMatrixf(arr); // gl is column major, so transpose to see the "matrix"
	}
	else
	{
		Mtx temp;
		
		Mtx *cur_mat = TopMtx(curmtx);
		
		if(cur_mat == NULL)
		{
			return;
		}

		guMtxIdentity(temp);
		guMtxTrans(temp, x, y, z);	
		guMtxConcat((*cur_mat),temp,(*cur_mat));
	}
}

void glTranslated( GLdouble x, GLdouble y, GLdouble z )
{
	NO_CALL_IN_BEGIN;
	
	glTranslatef( (GLfloat) x, (GLfloat) y, (GLfloat) z );
}

void  glRotatef (GLfloat angle, GLfloat x, GLfloat y, GLfloat z) {
	NO_CALL_IN_BEGIN;
	
	if(cur_mode == GL_PROJECTION)
	{
		// 4x4 Matrix rotation not provided by gu, hack it
		GLfloat s = sinf(DegToRad(angle));
		GLfloat c = cosf(DegToRad(angle));
				
		/*|	xx(1-c)+c	xy(1-c)-zs  xz(1-c)+ys	 0  |
		  | yx(1-c)+zs	yy(1-c)+c   yz(1-c)-xs	 0  |
		  | xz(1-c)-ys	yz(1-c)+xs  zz(1-c)+c	 0  |
		  |	 0	     	0		 	0	 	 	 1  |*/

		GLfloat arr[16];
		
		arr[0] = x*x*(1-c)+c;   arr[4] = y*x*(1-c)+z*s; arr[ 8] = x*z*(1-c)-y*s; arr[12] = 0;
		arr[1] = x*y*(1-c)-z*s; arr[5] = y*y*(1-c)+c  ; arr[ 9] = y*z*(1-c)+x*s; arr[13] = 0;
		arr[2] = x*z*(1-c)+y*s; arr[6] = y*z*(1-c)-x*s; arr[10] = z*z*(1-c)+c  ; arr[14] = 0;
		arr[3] = 0;             arr[7] = 0;             arr[11] = 0;             arr[15] = 0;
		glMultMatrixf(arr); // gl is column major, so transpose to see the "matrix"
		
	}
	else
	{
		Mtx temp;
		
		Mtx *cur_mat = TopMtx(curmtx);
		
		if(cur_mat == NULL)
		{
			return;
		}
		
		guVector axis;

		axis.x = x;
		axis.y = y;
		axis.z = z;
		guMtxIdentity(temp);
		guMtxRotAxisDeg(temp, &axis, angle);
		guMtxConcat((*cur_mat),temp,(*cur_mat));
	}
}

void  glScalef (GLfloat x, GLfloat y, GLfloat z){
	NO_CALL_IN_BEGIN;
	
	if(cur_mode == GL_PROJECTION)
	{
		// 4x4 Matrix scaling not provided by gu, hack it
		GLfloat arr[16];
		
		arr[0] = x; arr[4] = 0; arr[ 8] = 0; arr[12] = 0;
		arr[1] = 0; arr[5] = y; arr[ 9] = 0; arr[13] = 0;
		arr[2] = 0; arr[6] = 0; arr[10] = z; arr[14] = 0;
		arr[3] = 0; arr[7] = 0; arr[11] = 0; arr[15] = 1;
		
		glMultMatrixf(arr);
	}
	else
	{
		Mtx temp;
		
		Mtx *cur_mat = TopMtx(curmtx);
		
		if(cur_mat == NULL)
		{
			return;
		}

		guMtxIdentity(temp);
		guMtxScale(temp, x, y, z);
		guMtxConcat((*cur_mat),temp,(*cur_mat));
	}
}

void glMultMatrixf( const GLfloat *m )
{
	NO_CALL_IN_BEGIN;
	
	size_t i,j;
	if(cur_mode == GL_PROJECTION)
	{
		Mtx44 *p = TopMtx44(curmtx);
		
		if(p == NULL)
		{
			return;
		}
		
		Mtx44P cur_mat = *p;
		
		Mtx44 mat;
		Mtx44 tmp;
		
		CopyGLfloatToMtx44(m, mat);
		
		size_t k;
		
		for(i = 0; i < 4; ++i)
		{
			for(j = 0; j < 4; ++j)
			{	
				tmp[i][j] = 0;
				for(k = 0; k < 4; ++k)
				{
					tmp[i][j] += cur_mat[i][k]*mat[k][j];
				}
			}
		}

		for(i = 0; i < 4; ++i)
		{
			for(j = 0; j < 4; ++j)
			{	
				cur_mat[i][j] = tmp[i][j];
			}
		}
		
		computeZplanes(TopPrjMtx(curmtx));
	}
	else
	{
		Mtx *cur_mat = TopMtx(curmtx);
		Mtx mat;
		
		if(cur_mat == NULL)
		{
			return;
		}
				
		if(!cur_mat)
			return;
	
		CopyGLfloatToMtx(m, mat);
		
		guMtxConcat((*cur_mat),mat,(*cur_mat));
	}
}

void  glPopMatrix (void){
	NO_CALL_IN_BEGIN;
	
	Pop(curmtx);
}

void  glPushMatrix (void){

	NO_CALL_IN_BEGIN;
	
	Push(curmtx);
}

/* Get functions */

void _glGetMatrixf( GLenum pname, GLfloat *params)
{
	size_t i,j;
	
	if(!params)
		return;
		
	switch(pname)
	{
	case GL_MODELVIEW_MATRIX:
		{
			MtxP model = *TopMtx(&model_stack);
			for(j = 0; j < 4; ++j)
			{
				for(i = 0; i < 3; ++i)
				{
					*params++ = model[i][j];
				}
				*params++ = 0.0f;
			}
		}
		break;
	}	
}

void CopyGLfloatToMtx(const GLfloat *m, MtxP mat)
{
	size_t i,j;
	
	for(j = 0; j < 4; ++j)
	{	
		for(i = 0; i < 3; ++i)
		{
			mat[i][j] = *m++;
		}
		m++;
	}
}

void CopyGLfloatToMtx44(const GLfloat *m , Mtx44P mat)
{
	size_t i,j;
	
	for(j = 0; j < 4; ++j)
	{	
		for(i = 0; i < 4; ++i)
		{
			mat[i][j] = *m++;
		}
	}
}


/* glu */

void gluLookAt( GLdouble eyeX,
				GLdouble eyeY,
				GLdouble eyeZ,
				GLdouble centerX,
				GLdouble centerY,
				GLdouble centerZ,
				GLdouble upX,
				GLdouble upY,
				GLdouble upZ ) {

	NO_CALL_IN_BEGIN;
	
	guVector cam = {eyeX, eyeY, eyeZ},
			up = {upX, upY, upZ},
		  look = {centerX, centerY, centerZ};

	if(cur_mode == GL_PROJECTION)
	{
		Mtx44 *tmp = TopMtx44(&projection_stack);
		Mtx mat;
		
		if(tmp == NULL)
		{
			return;
		}
	
		guLookAt(mat, &cam, &up, &look);
		
		CopyMtxToMtx44(mat, *tmp);
		
		computeZplanes(TopPrjMtx(curmtx));
	}
	else
	{
		Mtx *cur_mat = TopMtx(curmtx);
		
		if(cur_mat == NULL)
		{
			return;
		}
		
		guLookAt(*cur_mat, &cam, &up, &look);
	}
}

void gluPerspective( GLdouble	fovy,
			       GLdouble	aspect,
			       GLdouble	zNear,
				   GLdouble	zFar ) {
	NO_CALL_IN_BEGIN;
	
	if(cur_mode == GL_PROJECTION)
	{
		PrjMtx * p = TopPrjMtx(&projection_stack);
		
		if(p == NULL)
		{
			return;
		}
		
		guPerspective(p->mat, fovy, aspect, zNear, zFar);
		p->mode = GX_PERSPECTIVE;
		
		p->nearZclip = zNear;
		p->farZclip = zFar;
	}
	else
	{
		Mtx44 tmp;
		Mtx *cur_mat = TopMtx(curmtx);
		
		guPerspective(tmp, fovy, aspect, zNear, zFar);
	
		CopyMtx44ToMtx(tmp, *cur_mat);
	}
}
