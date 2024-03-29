#ifndef _GLINT_H_
#define _GLINT_H_

#include <assert.h>
#include <memtracer.h>
#include <GL/gl.h>


#include <ogcsys.h>
#include <gccore.h>

//TODO: rewrite to support dynamic arrays
#define MAX_ARRAY 1000

#define LARGE_NUMBER 1048576
#define BIG_NUMBER		(1024*1024)
//1048576
//1.0e-18F
//9.9999998e+017
//typedef struct{float x;float y; float z;}Vector;
//#define Vector guVector;
typedef struct 
{
	guVector v;
	float w;
} VertexElement;

typedef struct 
{
	float s;
	float t;
} TexCoordElement;

typedef struct 
{
	float r;
	float g;
	float b;
	float a;
} GXColorf;

void CopyGLfloatToMtx(const GLfloat *, MtxP);
void CopyGLfloatToMtx44(const GLfloat *, Mtx44P);
void CopyMtxToMtx44(const MtxP src, Mtx44P dest);
void CopyMtx44ToMtx(const Mtx44P src, MtxP dest);


void _glGetMatrixf( GLenum pname, GLfloat *params);

// Controls the number of glTexture units allowed.  Minimum is 8
#define MAX_NO_TEXTURES 8
// Defines the initial number of glTex slots.  If more than this are needed the memory will be realloc-ed
#define NUM_INIT_TEXT 4000
// Controls texture memory cache.  If enabled texture memory spaces will not be freed when glDeleteTextures is called, insteaded cache.  When
// glTexImage2D or equiv is called, this memory can be re-used.  Good if your texture sizes are fairly constant and the amount of textures you
// need are constant.  See gl_texture_slot.c
#define MEM_TEX_CACHE 0
// Defines the number of texture memory cache slots.  Texture memory above this is free-ed when glDeleteTexture is called
#define NUM_MEM_TEX_CACHE 4000

GXColorf _tempcolorelement;
guVector _tempnormalelement;
TexCoordElement _temptexcoordelement[MAX_NO_TEXTURES];


typedef struct _array
{
	bool enable;
	const void *p;
	u8 stride;
	GLint size;
	GLenum type;
} VertexArray;

VertexArray norm;
VertexArray vert;
VertexArray tex[MAX_NO_TEXTURES];
VertexArray color;

guVector _normalelements[MAX_ARRAY] ATTRIBUTE_ALIGN (32);
guVector _vertexelements[MAX_ARRAY] ATTRIBUTE_ALIGN (32);
TexCoordElement _texcoordelements[MAX_NO_TEXTURES][MAX_ARRAY] ATTRIBUTE_ALIGN (32);
GXColorf _colorelements[MAX_ARRAY] ATTRIBUTE_ALIGN (32);

typedef struct _VertexData
{
	VertexElement vert;
	VertexElement norm;
	GXColorf color;
	TexCoordElement TexCoord;
} VertexData;

int vert_i;
int _type;
GLenum _GLtype;

int cur_tex;
int cur_tex_client;

/* Depth Buffer */

u8 depthtestenabled;
u8 depthupdate;
GLenum depthfunction;
float _cleardepth;

typedef struct _prj_mat
{
	Mtx44 mat;
	u8 mode;
	f32 nearZclip;
	f32 farZclip;
} PrjMtx;

#include <mat_stack.h>

Stack model_stack;
Stack projection_stack;
Stack texture_stack[MAX_NO_TEXTURES];
Stack *curmtx;
GLenum cur_mode;

u8 incTexDesc(u8 i);

//light specs
typedef struct _light
{
	bool enabled;
	
	GXLightObj obj;

	VertexElement pos; //light position for each light
	
	guVector spotDir; //light direction for each light
	
	GXColorf ambient;
	GXColorf diffuse;
	GXColorf specular;
	
	float spotExponent;
	float spotCutoff;
	float constant;
	float linear;
	float quad;
} LightObj;

#define MAX_LIGHT 8
LightObj lights[MAX_LIGHT];

//lightmodel specs
GXColorf globAmbient;


//material specs
typedef struct _mat
{
	GXColorf emissive;
	GXColorf ambient;
	GXColorf diffuse;
	GXColorf specular;
	float shininess;
} Material;

Material curmat;

/* culling */
bool gxcullfaceanabled;
GLenum gxwinding;

/* textures */

bool lighting;

void initTextures();
void initCalllist();
void _glInitStacks();


u8 blend_type;
u8 blend_src;
u8 blend_dst;
u8 blend_op;

u8 cull_mode;

typedef enum _glState
{
	GL_STATE_NONE = 0,
	GL_STATE_BEGIN = 1 << 0,
	GL_STATE_NEWLIST = 1 << 1
} glState;

glState cur_state;

#define _glSetError(a) _glSetErrorEx(a, __FILE__, __LINE__)
#define NO_CALL_IN_BEGIN if(cur_state & GL_STATE_BEGIN) { _glSetError(GL_INVALID_OPERATION); return; }
#define NO_CALL_IN_BEGIN_A(a) if(cur_state & GL_STATE_BEGIN) { _glSetError(GL_INVALID_OPERATION); return a; }


void _glSetErrorEx(GLenum, const char *, int line);


void _glSetEnableTex(bool val);
bool _glGetEnableTex(int tex);

void _glGetPixelTransferf(GLenum pname, GLfloat *param);

GLboolean _glIsArrayEnabled(GLenum	cap);

u8 GX_SetTextures(u8 stage);

void *_glGetProcAddress(const char* proc);

void * getFrontFramebuffer();
void * getBackFramebuffer();

void getVImode(u32 *w, u32 *h);

void glDrawRangeElementsEXT(GLenum mode,
        GLuint start,
        GLuint end,
        GLsizei count,
        GLenum type,
        const GLvoid *indices);
		
		
typedef struct boxInfo
{
	GLint x;
	GLint y;
	GLsizei width;
	GLsizei height;
} boxInfo;

int fb_max_height;
int fb_max_width;
GLboolean scissor_test;
boxInfo scissorInfo;
boxInfo viewPort;
f32 normNear;
f32 normFar;

f32 line_width;
f32 point_size;

typedef struct _pixelStore
{
	bool swap;
	bool lsb_first;
	int row_length;
	int skip_rows;
	int skip_pixels;
	int alignment;
} pixelStore;

pixelStore pack;
pixelStore unpack;
bool color_write_mask[4];

void GX_SetModViewport(f32 xOrig,f32 yOrig,f32 wd,f32 ht,f32 nearZ,f32 farZ);
void GX_SetModScissor(u32 xOrigin,u32 yOrigin,u32 wd,u32 ht);
void GX_SetMaxScissor();

bool copy_mat_enabled;
GLenum copy_material;


GXColor _clearcolor;

void _glGetPixelStore(GLenum pname, GLint * params);
const GLvoid * pixel_offset(GLenum glFormat, const GLvoid * pixels, GLenum type, size_t width, size_t height, size_t col, size_t row, pixelStore *store);
static __inline__ void TransformRGBA(GLfloat *r, GLfloat *g, GLfloat *b, GLfloat *a);

typedef struct _trans
{
	f32 scale;
	f32 bias;
} ColorTrans;

enum transfer_enum
{
	TRAN_R,
	TRAN_G,
	TRAN_B,
	TRAN_A
};

ColorTrans Trans[4];

typedef struct _tex_env
{
	bool enabled;
	
	u8 min_filter;
	u8 max_filter;
	u8 wrap_s;
	u8 wrap_t;
	f32 minlod;
	f32 maxlod;
	f32 lodbias;
	u8 biasclamp;
	u8 edgelod;
	u8 maxaniso;

	GLenum mode;
	
	GLenum comRGB;
	GLenum comAlpha;
	
	GLenum Carg[3];
	GLenum CargOp[3];
	GLenum Aarg[3];
	GLenum AargOp[3];
		
	GLint tex;
	
	GXColorf color;
	
	u8 rgb_scale;
	u8 alpha_scale;
} glTexEnvSet;

void updateFog();

bool fog_enable;
u8 fog_mode;
f32 fog_startz;
f32 fog_endz;
f32 fog_density;
GXColor fog_color;

glTexEnvSet glTexEnvs[MAX_NO_TEXTURES];

#define MAX_ATTRIB_STACK_DEPTH 16


GLenum read_mode;


GLuint call_offset;

static __inline__ void OffsetN(void **dest, size_t n)
{
	u8 **p = (u8 **)dest;
	(*p) += n;
	
}

void TransferPixels(GLenum glFormat, void *dest, u32 format, const GLvoid *pixels, GLenum type, size_t width, size_t height, size_t xoffset, size_t yoffset, size_t transwidth, size_t transheight);

void initTextureSlots();
void * getTextureSlot(size_t size);
void releaseTextureSlot(void * slot, size_t size);


void ResetArrays();

static __inline__ void TransformC(GLfloat *c, ColorTrans *p)
{	
	*c = *c*p->scale + p->bias;
	
	if(*c > 1)
		*c = 1;
	if(*c < 0)
		*c = 0;
}


static __inline__ void TransformRGBA(GLfloat *r, GLfloat *g, GLfloat *b, GLfloat *a)
{
	TransformC(r,Trans+0);
	TransformC(g,Trans+1);
	TransformC(b,Trans+2);
	TransformC(a,Trans+3);
}

#define MAX_VERTEX 32000

#define GLbool(x) ((x) ? GL_TRUE : GL_FALSE)

#ifndef NDEBUG
#define dprintf printf
#else
#define dprintf(...) {}
#endif

#endif
