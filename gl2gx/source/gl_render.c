#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <ogcsys.h>
#include <gccore.h>

#include "glint.h"
#include "GL/gl.h"

static __inline__ void multColorff2f(const GXColorf *a, const GXColorf *b, GXColorf *res)
{
	res->r = a->r*b->r;
	res->g = a->r*b->g;
	res->b = a->r*b->b;
	res->a = a->r*b->a;
}

static __inline__ void scaleColorf2b(const GXColorf *a, GXColor *res)
{
	res->r *= a->r;
	res->g *= a->g;
	res->b *= a->b;
	res->a *= a->a;
}

static __inline__ void multColorff2b(const GXColorf *a, const GXColorf *b, GXColor *res)
{
	res->r = a->r*b->r * 0xFF;
	res->g = a->r*b->g * 0xFF;
	res->b = a->r*b->b * 0xFF;
	res->a = a->r*b->a * 0xFF;
}

static __inline__ void Color_f2b(const GXColorf *in, GXColor *out)
{
	out->r = in->r * 0xFF;
	out->g = in->g * 0xFF;
	out->b = in->b * 0xFF;
	out->a = in->a * 0xFF;
}


typedef enum _VertexMode
{
	INTERNAL,
	ARRAY,
	ELEMENTS
} VertexMode;

static VertexMode render_mode = INTERNAL;

/* functions */
void glBegin(GLenum type) {

	NO_CALL_IN_BEGIN;

	// Set cur_state bit to GL_STATE_BEGIN
	cur_state |= GL_STATE_BEGIN;
	
	_GLtype = GL_POINTS;
	render_mode = INTERNAL;
	
	vert_i = 0;
	
	//store the type
	switch(type)
	{
		case GL_POINTS: _type = GX_POINTS; break;
		case GL_LINES: _type = GX_LINES; break;
		case GL_LINE_STRIP: _type = GX_LINESTRIP; break;      
		case GL_TRIANGLES: _type = GX_TRIANGLES; break;
        case GL_TRIANGLE_STRIP: _type = GX_TRIANGLESTRIP; break;
		case GL_TRIANGLE_FAN: _type = GX_TRIANGLEFAN; break;
		case GL_QUADS: _type = GX_QUADS; break;
		case GL_POLYGON: _type = GX_TRIANGLEFAN; break;
		case GL_QUAD_STRIP:
		case GL_LINE_LOOP:
				 _type = GX_NONE;
				 _GLtype = type;
				 break;
		default:
			_GLtype = GL_INVALID_ENUM;
			_glSetError(GL_INVALID_ENUM);
			cur_state &= ~GL_STATE_BEGIN;
			return;
	};
}

static __inline__ void *OffsetByte(const void *p, int n)
{
	u8 * ptr = (u8 *) p;
	return ptr+n;
}

static guVector tmpVert;
static guVector tmpNorm;
static GXColorf tmpColor;
static TexCoordElement tmpTexCoord;

static __inline__ size_t getElemSize(GLenum type)
{
	switch(type)
	{
	default:
	case GL_UNSIGNED_BYTE:
	case GL_BYTE: return sizeof(GLbyte);
	case GL_UNSIGNED_SHORT:
	case GL_SHORT: return sizeof(GLshort);
	case GL_UNSIGNED_INT:
	case GL_INT: return sizeof(GLint);
	case GL_FLOAT: return sizeof(GLfloat);
	case GL_DOUBLE: return sizeof(GLdouble);
	}
}

typedef const void * voidP;

static __inline__ void getFloat(voidP *p, GLenum type, GLfloat *f)
{
	switch(type)
	{
	case GL_UNSIGNED_BYTE:
	{
		GLubyte ** ptr = (GLubyte **) p;
		
		*f = *(*ptr)++;
		break;
	}
	case GL_BYTE:
	{
		GLbyte ** ptr = (GLbyte **) p;
		
		*f = *(*ptr)++;
		break;
	}
	case GL_UNSIGNED_SHORT:
	{
		GLushort ** ptr = (GLushort **) p;
		
		*f = *(*ptr)++;
		break;
	}
	case GL_SHORT:
	{
		GLshort ** ptr = (GLshort **) p;
		
		*f = *(*ptr)++;
		break;
	}
	case GL_UNSIGNED_INT:
	{
		GLuint ** ptr = (GLuint **) p;
		
		*f = *(*ptr)++;
		break;
	}
	case GL_INT:
	{
		GLint ** ptr = (GLint **) p;
		
		*f = *(*ptr)++;
		break;
	}
	case GL_FLOAT:
	{
		GLfloat ** ptr = (GLfloat **) p;
		
		*f = *(*ptr)++;
		break;
	}
	case GL_DOUBLE:
	{
		GLdouble ** ptr = (GLdouble **) p;
		
		*f = *(*ptr)++;
		break;
	}
	}
}

static __inline__ const void * getElem(VertexArray *p, int index)
{
	return (guVector*)OffsetByte(p->p, p->stride*index);
}

static __inline__ void getVector(VertexArray *ptr, int index, guVector * v)
{
	const void * p = getElem(ptr, index);
	
	getFloat(&p, ptr->type, &v->x);
	getFloat(&p, ptr->type, &v->y);
	getFloat(&p, ptr->type, &v->z);	
}


static __inline__ const guVector * getVertex(int index)
{
	if(vert.enable)
	{
		if(vert.type == GL_FLOAT && vert.size == 3)
		{
			return getElem(&vert, index);
		}
		else
		{
			getVector(&vert, index, &tmpVert);
			return &tmpVert;
		}
	}
	else
	{
		return NULL;
	}
}

static __inline__ const guVector *getNormal(int index)
{
	if(norm.enable)
	{
		if(norm.type == GL_FLOAT)
		{
			return getElem(&norm, index);
		}
		else
		{
			getVector(&norm, index, &tmpNorm);
			return &tmpNorm;
		}
	}
	else
	{
		return NULL;
	}
}

static __inline__ GLfloat getCorrection(GLenum type)
{
	switch(type)
	{
	default:
	case GL_UNSIGNED_BYTE: return 255.0f;
	case GL_BYTE: return 127.0f;
	case GL_UNSIGNED_SHORT: return 65535.f;
	case GL_SHORT: return 32767.f;
	case GL_UNSIGNED_INT: return 4294967295.f;
	case GL_INT: return 2147483647.f;
	case GL_FLOAT: return 1;
	case GL_DOUBLE: return 1;
	}
}

static __inline__ const GXColorf *getColor(int index)
{
	if(color.enable)
	{
		if(color.type == GL_FLOAT && color.size == 4)
		{
			return getElem(&color, index);			
		}
		else
		{
			GLfloat c = getCorrection(color.type);
			
			const void * p = getElem(&color, index);
			getFloat(&p, color.type, &tmpColor.r);
			getFloat(&p, color.type, &tmpColor.g);
			getFloat(&p, color.type, &tmpColor.b);
			if(color.size == 4)
			{
				getFloat(&p, color.type, &tmpColor.a);
			}
			else
			{
				tmpColor.a = 1.0f;
			}
			
			tmpColor.r /= c;
			tmpColor.g /= c;
			tmpColor.b /= c;
			tmpColor.a /= c;
			
			return &tmpColor;
		}
	}
	else
	{
		return &_tempcolorelement;
	}
}

static __inline__ const TexCoordElement *getTexCoord(int index, u8 i)
{
	if(tex[i].enable)
	{
		if(tex[i].type == GL_FLOAT && tex[i].size == 2)
		{
			return getElem(tex+i, index);			
		}
		else
		{
			const void * p = getElem(tex+i, index);
			
			getFloat(&p, tex[i].type, &tmpTexCoord.s);
			if(tex[i].size > 2)
			{
				getFloat(&p, tex[i].type, &tmpTexCoord.t);
			}
			
			return &tmpTexCoord;
		}
	}
	else
	{
		return NULL;
	}
}

static GLint arrayOffset = 0;

static u8 vert_mode = GX_DIRECT;
static u8 norm_mode = GX_DIRECT;
static u8 color_mode = GX_DIRECT;
static u8 tex_mode[MAX_NO_TEXTURES];

static GLenum elemType;
static const GLvoid *elemIndex = NULL;

static __inline__ GLuint getElement(GLenum type, const GLvoid *ptr, GLuint index)
{
	switch(type)
	{
	case GL_UNSIGNED_BYTE:
	{
		GLubyte * p = (GLubyte *)ptr;
		return p[index];
	}
	case GL_UNSIGNED_SHORT:
	{
		GLushort * p = (GLushort *)ptr;
		return p[index];
	}
	case GL_UNSIGNED_INT:
	{
		GLuint * p = (GLuint *)ptr;
		return p[index];
	}
	default:
		return 0;
	}
}

static __inline__ GLint getIndex(int index)
{
	switch(render_mode)
	{
	default:
	case INTERNAL:
		return index;
	case ARRAY:
		return index-arrayOffset;
	case ELEMENTS:
		return getElement(elemType, elemIndex, index)-arrayOffset;
	}
}


typedef const TexCoordElement * TexCoordP;

// Render a vertex to Gecko (used by glEnd)
static __inline__ void UploadVertex(int index)
{	
	int i;
	if(vert_mode == GX_INDEX16)
	{
		if(vert.enable)
			GX_Position1x16(getIndex(index)+arrayOffset);
	}
	else
	{
		const guVector * v = NULL;
		switch(render_mode)
		{
		case INTERNAL:
			v = _vertexelements+index;
			break;
		case ARRAY:
			v = getVertex(index+arrayOffset);
			break;
		case ELEMENTS:
			v = getVertex(getElement(elemType, elemIndex, index));
			break;
		}
		if(v)
			GX_Position3f32( v->x, v->y, v->z);
	}
	
	if(norm_mode == GX_INDEX16)
	{
		if(norm.enable)
		{
			GX_Normal1x16(getIndex(index)+arrayOffset);
		}
	}
	else
	{
		const guVector * n = NULL;
		switch(render_mode)
		{
		case INTERNAL:
			n = _normalelements+index;
			break;
		case ARRAY:
			n = getNormal(index+arrayOffset);
			break;
		case ELEMENTS:
			n = getNormal(getElement(elemType, elemIndex, index));
			break;
		}
		if(n)
			GX_Normal3f32(n->x, n->y, n->z);
	}
	
	if(color_mode == GX_INDEX16)
	{
		if(color.enable)
		{
			GX_Color1x16(getIndex(index)+arrayOffset);
		}
	}
	else
	{
		const GXColorf *c = NULL;
		switch(render_mode)
		{
		case INTERNAL:
			c = _colorelements+index;
			break;
		case ARRAY:
			c = getColor(index+arrayOffset);
			break;
		case ELEMENTS:
			c = getColor(getElement(elemType, elemIndex, index));
			break;
		}
		
		if(c)
			GX_Color4u8(255* c->r, 255*c->g, 255*c->b, 255*c->a); //glmaterialfv call instead when glcolormaterial call is used
	}

	//when using GL_FLAT only one color is allowed!!! //GL_SMOOTH allows for an color to be specified at each vertex
	
	for(i = 0; i < MAX_NO_TEXTURES; ++i)
	{
		if(!_glGetEnableTex(i))
			continue;
			
		if(tex_mode[i] == GX_INDEX16)
		{				
			if(tex[i].enable)
			{
				GX_TexCoord1x16(getIndex(index)+arrayOffset);
			}
		}
		else
		{
			TexCoordP t = NULL;
			switch(render_mode)
			{
			case INTERNAL:
				t = _texcoordelements[i]+index;
				break;
			case ARRAY:
				t = getTexCoord(index+arrayOffset, i);
				break;
			case ELEMENTS:
				t = getTexCoord(getElement(elemType, elemIndex, index), i);
				break;
			}
			if(t)
				GX_TexCoord2f32(t->s,t->t);
		}
	}
};

#include <boost/preprocessor/config/limits.hpp>
#include <boost/preprocessor/repetition/repeat.hpp>
#include <boost/preprocessor/array/elem.hpp>

#define first(arr) BOOST_PP_ARRAY_ELEM(0,arr)
#define second(arr) BOOST_PP_ARRAY_ELEM(1,arr)
#define third(arr) BOOST_PP_ARRAY_ELEM(2,arr)
#define fourth(arr) BOOST_PP_ARRAY_ELEM(3,arr)

#if (BOOST_PP_LIMIT_REPEAT < MAX_NO_TEXTURES)
#error BOOST_PP_LIMIT_REPEAT < MAX_NO_TEXTURES
#endif

#define MAKE_NAME(opts, n, type) \
BOOST_PP_CAT(UploadVertex, BOOST_PP_CAT(opts, BOOST_PP_CAT(n, BOOST_PP_CAT(_, type))))


#define TEXTURE_INDEX(z, n, method) GX_TexCoord1x16(method);

#define GEN_TEXTURE(method, n) \
BOOST_PP_REPEAT(n, TEXTURE_INDEX, method)


#define GEN_UPLOAD_VERTEX_PT(z, n, arr) \
static __inline__ void MAKE_NAME(PT, n, first(arr)) (int index) \
{ \
	GLint cache = (second(arr)); \
	GX_Position1x16(cache); \
	\
	GXColorf *c = &_tempcolorelement; \
	GX_Color4u8(255* c->r, 255*c->g, 255*c->b, 255*c->a); \
	GEN_TEXTURE(cache, n) \
};

#define GEN_UPLOAD_VERTEX_PNT(z, n, arr) \
static __inline__ void MAKE_NAME(PNT, n, first(arr)) (int index) \
{ \
	GLint cache = (second(arr)); \
	GX_Position1x16(cache); \
	\
	GX_Normal1x16(cache); \
	\
	GXColorf *c = &_tempcolorelement; \
	GX_Color4u8(255* c->r, 255*c->g, 255*c->b, 255*c->a); \
	GEN_TEXTURE(cache, n) \
};

#define GEN_UPLOAD_VERTEX_PCT(z, n, arr) \
static __inline__ void MAKE_NAME(PCT, n, first(arr)) (int index) \
{ \
	GLint cache = (second(arr)); \
	GX_Position1x16(cache); \
	\
	GX_Color1x16(cache); \
	\
	GEN_TEXTURE(cache, n) \
};

#define GEN_UPLOAD_VERTEX_PNCT(z, n, arr) \
static __inline__ void MAKE_NAME(PNCT, n, first(arr)) (int index) \
{ \
	GLint cache = (second(arr)); \
	GX_Position1x16(cache); \
	\
	GX_Normal1x16(cache); \
	\
	GX_Color1x16(cache); \
	\
	GEN_TEXTURE(cache, n) \
};

#define FUNC_SET (4, (PT,        PNT, 	    PCT,       PNCT) )
#define FUNC_MAP (4, ((2,(0,0)), (2,(1,0)), (2,(0,1)), (2,(1,1))))

#define GEN_UPLOAD_VERTEX_REC(z, i, data) \
BOOST_PP_REPEAT(MAX_NO_TEXTURES, BOOST_PP_CAT(GEN_UPLOAD_VERTEX_, BOOST_PP_ARRAY_ELEM(i, FUNC_SET)), data)

#define GEN_FULL_SET(name, method) \
BOOST_PP_REPEAT(BOOST_PP_ARRAY_SIZE(FUNC_SET), GEN_UPLOAD_VERTEX_REC, (2, (name, method) ))

GEN_FULL_SET(Array, index)

#define GEN_ELEMENT_SET(type) \
GEN_FULL_SET(BOOST_PP_CAT(Elem, type), *(((type *)elemIndex)+index))

GEN_ELEMENT_SET(GLubyte)
GEN_ELEMENT_SET(GLushort)
GEN_ELEMENT_SET(GLuint)

#define TEXT_QUEST(z, n, func) \
if(tex_count == n) { \
	sent_verts = 1; \
	first(func) \
		(MAKE_NAME(first(second(func)), n, second(second(func)))) \
} else

#define GEN_TEX_QUEST(macro, func) \
BOOST_PP_REPEAT(MAX_NO_TEXTURES, TEXT_QUEST, (2, (macro, func)))

#define GEN_FORMAT_QUEST(z, n, data) \
if(first(first( data)) == first( BOOST_PP_ARRAY_ELEM(n, third( data))) && \
	second(first( data)) == second( BOOST_PP_ARRAY_ELEM(n, third( data)))) { \
		GEN_TEX_QUEST(BOOST_PP_ARRAY_ELEM(4, data),\
				(2, (BOOST_PP_ARRAY_ELEM(n, second( data)), fourth(data) )) \
			)\
	{}\
} else

#define GEN_FULL_QUEST(macro, func, args) \
BOOST_PP_REPEAT(BOOST_PP_ARRAY_SIZE(FUNC_SET), GEN_FORMAT_QUEST, (5, (args, FUNC_SET, FUNC_MAP, func, macro))) \
{}

#define GEN_FULL_QUEST_ARRAY(macro, normal, color) \
GEN_FULL_QUEST(macro, Array, (3, (normal, color, macro)))

#define GEN_FULL_QUEST_ELEM_TYPE(macro, type, normal, color) \
GEN_FULL_QUEST(macro, BOOST_PP_CAT(Elem, type), (3, (normal, color, macro)))

void ResetArrays()
{
	int i = 0;
	int j = 0;
	
	for( i=0; i<vert_i; i++)
	{
		_vertexelements[i].x = 0.0F;
		_vertexelements[i].y = 0.0F;
		_vertexelements[i].z = 0.0F;

		_normalelements[i].x = 0.0F;
		_normalelements[i].y = 0.0F;
		_normalelements[i].z = 1.0F;

		_colorelements[i].r = 1.0F;
		_colorelements[i].g = 1.0F;
		_colorelements[i].b = 1.0F;
		_colorelements[i].a = 1.0F;

		for(j = 0; j < MAX_NO_TEXTURES; ++j)
		{
			_texcoordelements[j][i].s = 0.0F;
			_texcoordelements[j][i].t = 0.0F;
		}

	}
	vert_i =0;
}

static void GX_TestInitSpecularDir(GXLightObj *lit_obj,f32 nx,f32 ny,f32 nz) { 
     //f32 px, py, pz;
      f32 hx, hy, hz, mag;  
     // Compute half-angle vector 
     hx = -nx;
     hy = -ny; 
     hz = (-nz + 1.0f); 
     mag = 1.0f / sqrtf((hx * hx) + (hy * hy) + (hz * hz)); 
     hx *= mag; 
     hy *= mag; 
     hz *= mag;  
     
     //px = -nx * BIG_NUMBER; 
     //py = -ny * BIG_NUMBER; 
     //pz = -nz * BIG_NUMBER;  
     //((f32*)lit_obj)[10] = px; 
     //((f32*)lit_obj)[11] = py; 
     //((f32*)lit_obj)[12] = pz; 
     ((f32*)lit_obj)[13] = hx; 
     ((f32*)lit_obj)[14] = hy; 
     ((f32*)lit_obj)[15] = hz;
} 

static void GX_TestInitLightShininess(GXLightObj *lobj, f32 shininess) {

   shininess = (shininess / 128) * 256; //convert opengl range to gx
   GX_InitLightAttn(lobj, 1.0, 0.0, 2.0, (shininess)*0.5, 0.0, 1.0F-(shininess)*0.5 );      

//Redshade
//math behind a and k is [a0 + a1^2 + a2^3], brightness as you go away from the center, [or brightness as you go away from source in k
//if you want to see the GL equivalent, look up GL_(CONSTANT|LINEAR|QUADRATIC)_ATTENUATION, since those are k0,k1,k2 respectively
//your end equation value should be between 0.0 and 1.0
}

static void SetTev()
{
	u8 texstage0 = GX_TEVSTAGE0;

	GX_SetBlendMode(blend_type, blend_src, blend_dst, blend_op);	
	
	if(lighting)
	{
		u8 gxlightmask = 0x00000000;
		u8 gxlightmaskspec = 0x00000000;
		int lightcounter = 0;
		//int countlights =0;
		//getting opengl lights to work on gx
		//
		//xg has only one light color (a diffuse one)
		//opengl lights have the following colors: diffuse, ambient and specular
		//also opengl has a global ambient color independend of a individual light
		//gx has 2 material colors: diffuse (mat) and ambient (can also be considered part of light?) 
		//opengl material have the followning colors: diffuse, ambient, specular and emission
		//so we (may) have problem (or call it a challenge)

		//now how does opengl calculate light with all these colors
		//vertex color	= material emission 
		//				+ global ambient scaled by material ambient
		//				+ ambient, diffuse, specular contributions from light(s), properly attinuated
		//
		//let us take these apart.
		//
		//material emission is like a constant color. So we can just add this in a tev stage. The only problem is how to add a color to a specific stage?)
		//
		//global ambient scaled by material ambient
		//this is global ambient * material ambient so we can feed that result to an tev stage.
		//
		//Now comes the hard part as each color is used in the light calulation. And we only have once color in gx.
		//Maybe we are lucky as each colors term is added to each other and only then used in light calculation
		//So we might get away with just adding the 3 colors upfront and feed that as color to the light source
		//But first let see how these terms are calculated.
		//
		//Ambient Term = light ambient * material ambient					= GXChanAmbColor ?							
		//Diffuse Term = surface * (light diffuse * material diffuse)		light diffues = light color	| material diffuse = GXChanMatColor	(let gx handle this)	
		//Specular Term = normal.shininess * (light specular * material specular)	(let gx handle this, but add lspec*mspec to GXChanMatColor)
		//
		//now we could use 3 light to emulate 1 opengl light but that would not be helpfull
		//so maybe there is an other way also gx material misses color components
		//
		//gx has max to channels
		//each can be setup differently so we can have on chanel for normal diffuse
		//and the other for specular. But we only have on light color so unless the specular color equals light color this it not usefull)
		//maybe some experiments with GXChanMatColor help with that? So light color to none and all color CHANMatColor?
		//
		//also we have multiple tev stages.
		//as we have used 2 channels we have to use 3 stages
		//stage 1 = emissive + global ambient scaled by material as constant color (maybe 2 stages?)
		//stage 2 = ambient + diffuse
		//stage 3 = specular
		//
		//So this might do the trick in theory. Now on to practice...

		//<h0lyRS> did you try setting specular to channel GX_COLOR1A1, and diffuse to GX_COLOR0A0, so you can blend them anyway you want?
		//this could add an extra color?
		//one way is adding the specular light, other way is multiplying


		//Setup lightmask with enabled lights (thanks h0lyRS)
		for (lightcounter =0; lightcounter < 4; lightcounter++){
			if(lights[lightcounter].enabled){ //when light is enabled
				gxlightmask |= (GX_LIGHT0 << lightcounter);
				gxlightmaskspec |= (GX_LIGHT0 << (lightcounter+4) );
				//countlights++;
			}
		};
		
		//Setup light system/channels
		GX_SetNumChans(2); //dependend on if there is a specular color/effect needed

		//channel 1 (ambient + diffuse)                          
		GX_SetChanCtrl(GX_COLOR0A0,GX_TRUE,GX_SRC_REG,GX_SRC_REG,gxlightmask,    GX_DF_CLAMP,GX_AF_SPOT);
		
		//channel 2 (specular)
		GX_SetChanCtrl(GX_COLOR1A1,GX_TRUE,GX_SRC_REG,GX_SRC_REG,gxlightmaskspec,GX_DF_CLAMP,GX_AF_SPEC);
		
		//stage 1 (global ambient light)
		
		//color
		GX_SetTevColorIn(GX_TEVSTAGE0, GX_CC_C1, GX_CC_ZERO, GX_CC_ZERO, GX_CC_C0); //shagkur method
		GX_SetTevColorOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_ENABLE, GX_TEVPREV);
		
		//alpha
		GX_SetTevAlphaIn(GX_TEVSTAGE0, GX_CA_A1, GX_CA_ZERO, GX_CA_ZERO, GX_CA_A0); //shagkur method
		GX_SetTevAlphaOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_ENABLE, GX_TEVPREV);
		
		//tevorder
		GX_SetTevOrder(GX_TEVSTAGE0, GX_TEXCOORDNULL, GX_TEXMAP_NULL, GX_COLOR0A0); //only use color

		//end stage 1
		
		
		//stage 2 (ambient and diffuse light from material and lights)

		//color
		GX_SetTevColorIn(GX_TEVSTAGE1, GX_CC_CPREV, GX_CC_ZERO, GX_CC_ZERO, GX_CC_RASC);
		GX_SetTevColorOp(GX_TEVSTAGE1, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_ENABLE, GX_TEVPREV);
		
		//alpha
		GX_SetTevAlphaIn(GX_TEVSTAGE1, GX_CA_APREV, GX_CA_ZERO, GX_CA_ZERO, GX_CC_RASA); 
		GX_SetTevAlphaOp(GX_TEVSTAGE1, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_ENABLE, GX_TEVPREV);
		
		//tevorder
		GX_SetTevOrder(GX_TEVSTAGE1, GX_TEXCOORDNULL, GX_TEXMAP_NULL, GX_COLOR0A0); //only use color
		
		// end stage 2
		
		
		//stage 3 (specular light)	

		// color - blend
		GX_SetTevColorIn(GX_TEVSTAGE2, GX_CC_ZERO,  GX_CC_RASC, GX_CC_ONE,  GX_CC_CPREV);
		GX_SetTevColorOp(GX_TEVSTAGE2, GX_TEV_ADD,  GX_TB_ZERO, GX_CS_SCALE_1, GX_ENABLE, GX_TEVPREV);

		
		// alpha - nop
		GX_SetTevAlphaIn(GX_TEVSTAGE2, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO, GX_CA_APREV); //shagkur method
		GX_SetTevAlphaOp(GX_TEVSTAGE2, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_ENABLE, GX_TEVPREV);

		GX_SetTevOrder(GX_TEVSTAGE2, GX_TEXCOORDNULL, GX_TEXMAP_NULL, GX_COLOR1A1);
		
		texstage0 = GX_TEVSTAGE3;
	}
	else
	{
		GX_SetChanCtrl(GX_COLOR0A0,GX_DISABLE,GX_SRC_REG,GX_SRC_VTX,GX_LIGHTNULL,GX_DF_NONE,GX_AF_NONE);
		texstage0 = GX_TEVSTAGE0;
		GX_SetNumChans(1); //keep this at one all time?
	}
	
	u8 nextstage = GX_SetTextures(texstage0);

	if(nextstage == GX_TEVSTAGE0)
	{
		GX_SetTevOp(GX_TEVSTAGE0, GX_PASSCLR);
		nextstage++;
	}
	
	int nstages = nextstage-GX_TEVSTAGE0;
		
	GX_SetNumTevStages(nstages);
	
	// Flush GX before continuing
	GX_DrawDone();
}

static void setup_render()
{
	GX_SetCurrentGXThread();
	
	if(point_size < 127)
	{
		GX_SetPointSize (point_size*2, GX_TO_HALF);
	}
	else
	{
		GX_SetPointSize (point_size, GX_TO_ONE);
	}
	
	if(line_width < 127)
	{
		GX_SetLineWidth(line_width*2, GX_TO_HALF);
	}
	else
	{
		GX_SetLineWidth(line_width, GX_TO_ONE);
	}
	
	GX_SetZMode(depthtestenabled, depthfunction, (depthtestenabled && depthupdate) ? GX_TRUE : GX_FALSE);
	
	// Check if all color write masks are false
	if(!color_write_mask[0] && !color_write_mask[1] && !color_write_mask[2])
	{
		GX_SetColorUpdate(GX_DISABLE);
	}
	else
	{
		GX_SetColorUpdate(GX_ENABLE);
	}
	
	if(color_write_mask[3])
	{
		GX_SetAlphaUpdate(GX_ENABLE);
	}
	else
	{
		GX_SetAlphaUpdate(GX_DISABLE);
	}
	
	GX_SetModViewport(viewPort.x, viewPort.y, viewPort.width, viewPort.height, normNear, normFar);

	if(_GLtype == GL_INVALID_ENUM)
	{
		return;		
	}
	
	if(scissor_test)
	{
		GX_SetModScissor(scissorInfo.x, scissorInfo.y, scissorInfo.width, scissorInfo.height); 
	}
	else
	{
		GX_SetMaxScissor();
	}
	
	updateFog();
	
	PrjMtx * p = TopPrjMtx(&projection_stack);
	
	if(p)
		GX_LoadProjectionMtx(p->mat, p->mode);

	MtxP modelview = *TopMtx(&model_stack);
	
	// load the modelview matrix into matrix memory
	GX_LoadPosMtxImm(modelview, GX_PNMTX0);
    GX_LoadNrmMtxImm(modelview, GX_PNMTX0);
	
	GX_SetCurrentMtx(GX_PNMTX0);
	
	SetTev();

	//use global ambient light together with current material ambient and add emissive material color
	GXColor constcolor;
	multColorff2b(&curmat.ambient, &globAmbient, &constcolor);
	GX_SetTevColor(GX_TEVREG0, constcolor);
	
	GXColor emiscolor;
	Color_f2b(&curmat.emissive, &emiscolor);
	GX_SetTevColor(GX_TEVREG1, emiscolor);

	//first check if a lightdirtyflag is set (thanks ector) so we do not have to set up light every run
	//also usefull on matrices etc.

	//now set each light
	GXColor gxchanambient;
	Color_f2b(&curmat.ambient, &gxchanambient);
	
	GXColor gxchanspecular;
	Color_f2b(&curmat.specular, &gxchanspecular);
	
	int li = 0;
	for (li = 0; li < 4; li++){

		if(lights[li].enabled) { //when light is enabled

            //somewhere here an error happens?

            //Setup mat/light ambient color 
			scaleColorf2b(&lights[li].ambient, &gxchanambient);
			GX_SetChanAmbColor(GX_COLOR0A0, gxchanambient ); 
			
			//Setup diffuse material color
			GXColor mdc;
			Color_f2b(&curmat.diffuse, &mdc);
			GX_SetChanMatColor(GX_COLOR0A0, mdc ); 
			
			//Setup specular material color
//			gxcurrentmaterialshininess *
			scaleColorf2b(&lights[li].specular, &gxchanspecular);
			GX_SetChanMatColor(GX_COLOR1A1, gxchanspecular); // use red as test color

            //Setup light diffuse color
            GXColor ldc;
			Color_f2b(&lights[li].diffuse, &ldc);
			GX_InitLightColor(&lights[li].obj, ldc ); //move call to glend or init?;
            GX_InitLightColor(&lights[li+4].obj, ldc ); //move call to glend or init?;

			//Setup light postion
			
			//check on w component when 1. light is positional
			//                     when 0. light is directional at infinite pos
			
			guVector lpos = lights[li].pos.v;
			guVector wpos;
			
            if (lights[li].pos.w == 0) {
                guVecNormalize(&lpos);
                lpos.x *= BIG_NUMBER;
                lpos.y *= BIG_NUMBER;
                lpos.z *= BIG_NUMBER;
            }
			
			GX_InitLightPosv(&lights[li].obj, &wpos);   //feed corrected coord to light pos
			GX_InitLightPosv(&lights[li+4].obj, &wpos); //feed corrected coord to light pos

            //Setup light direction (when w is 1 dan dir = 0,0,0
            guVector ldir;
            if (lights[li].pos.w == 0){
				ldir = lights[li].pos.v;
            }
            else
            {
                if (lights[li].spotCutoff != 180){
					// if we have a spot light direction is needed
					ldir = lights[li].spotDir;
                }
                else { 
					ldir.x = 0;
					ldir.y = 0;
					ldir.z = -1;
				}
            }
            
            guVecNormalize(&ldir);
            
            GX_InitLightDir(&lights[li].obj, ldir.x, ldir.y, ldir.z); //feed corrected coord to light dir
            GX_InitLightDir(&lights[li+4].obj, ldir.x, ldir.y, ldir.z); //feed corrected coord to light dir
           
            
			if (lights[li].spotCutoff != 180){
               //Setup specular light (only for spotlight when GL_SPOT_CUTOFF <> 180)
			   //make this line optional? If on it disturbs diffuse light?
               guVector sdir;
               sdir = lights[li].spotDir;
               //guVecNormalize(&sdir);
                     
               //sdir.x *= BIG_NUMBER;
               //sdir.y *= BIG_NUMBER;
               //sdir.z *= BIG_NUMBER;
			   
			   guVector light_dir;
			   guVecSub(&sdir, &lpos, &light_dir);
			   
			   GX_TestInitSpecularDir(&lights[li].obj, light_dir.x, light_dir.y, light_dir.z); //needed to enable specular light
               GX_TestInitSpecularDir(&lights[li+4].obj, light_dir.x, light_dir.y, light_dir.z); //needed to enable specular light               
            };
			
			//Attenuation factor = 1 / (kc + kl*d + kq*d2) 
            //kc = constant attenuation factor (default = 1.0) 
            //kl = linear attenuation factor (default = 0.0) 
            //kq = quadratic attenuation factor (default = 0.0) 
         
            float distance = BIG_NUMBER; //either distance of light or falloff factor
            float factor = 1 / (lights[li].constant + lights[li].linear*distance + lights[li].quad*distance*distance);
             
            GX_InitLightDistAttn(&lights[li].obj, factor ,1.0, GX_DA_STEEP); //gxspotexponent[lightcounter] GX_DA_GENTLE
            GX_InitLightDistAttn(&lights[li+4].obj, factor ,1.0, GX_DA_STEEP); //gxspotexponent[lightcounter] GX_DA_GENTLE
			
			
            //setup normal spotlight
            GX_InitLightSpot(&lights[li].obj, lights[li].spotCutoff, GX_SP_RING1); //not this is not a spot light ()

            if ( curmat.shininess != 0 ) {
                 //if (gxspotcutoff[lightcounter] != 180) {
                    GX_TestInitLightShininess(&lights[li+4].obj, curmat.shininess);
                 //}
            };

			//Load the light up
			switch (li){
				case 0: 
                     GX_LoadLightObj(&lights[li].obj, GX_LIGHT0);
                     GX_LoadLightObj(&lights[li+4].obj, GX_LIGHT4);  
                     break;
				case 1: 
                     GX_LoadLightObj(&lights[li].obj, GX_LIGHT1);
                     GX_LoadLightObj(&lights[li+4].obj, GX_LIGHT5); 
                     break;
				case 2: 
                     GX_LoadLightObj(&lights[li].obj, GX_LIGHT2); 
                     GX_LoadLightObj(&lights[li+4].obj, GX_LIGHT6);
                     break;
				case 3: 
                     GX_LoadLightObj(&lights[li].obj, GX_LIGHT3); 
                     GX_LoadLightObj(&lights[li+4].obj, GX_LIGHT7);
                     break;
			}
		}
	}
}

static __attribute__((__noinline__)) void glRender(int count)  {
	
	// Unset GL_STATE_BEGIN
	cur_state &= ~GL_STATE_BEGIN;
	
	setup_render();
	
	int special = 0;
	int lineloop = 0;
	
	if(_type == GX_NONE)
	{
		switch(_GLtype)
		{
		case GL_LINE_LOOP: _type = GX_LINESTRIP; lineloop = 1; break;
		case GL_QUAD_STRIP: _type = GX_QUADS; special = 1; break;
		default: _type = GX_LINES; break;
		}
	}
		
	bool cw = true;
	bool ccw = false;

	if(gxcullfaceanabled==true){
		cw = false;
		ccw = false;                            
		switch(gxwinding){
			case GL_CW: cw = true; break;
			case GL_CCW: ccw = true; break;
		}

		GX_SetCullMode(cull_mode);
	}
	else
	{
		GX_SetCullMode(GX_CULL_NONE);
	}
	
	int countelements = 0;
	
	// Compute number of elements to pass to GX
	// Must be correct or it stalls
	// Any modifications the number of points sent in actual render portion should be reflected here
	if(!special)
	{
		if (cw==true){ 
	       //CW
		   
		   countelements += count;
		   
		   if(lineloop)
		   {
				++countelements;
		   }
	    }
	    
	    if (ccw==true){
				
	       //CCW
		   countelements += count;
		   
		   if(lineloop)
		   {
				++countelements;
		   }
	    }
	}
	else
	{
		if(_GLtype == GL_QUAD_STRIP)
		{
			//    3-------4
			//    |       |
			//	  |       |
			//	  |       |
			//	  1-------2
			
			if(ccw==true)
			{
				countelements += 4*(count/2-1);
			}
			if(cw==true)
			{
				countelements += 4*(count/2-1);
			}
		}
	
	}
	
	GX_DrawDone();
	
	GX_Begin(_type, GX_VTXFMT0, countelements);
	
	int i =0;
	
	// The renderer is commented in bottom else statement
	#define DO_RENDER(upload_method) \
	if(!special) \
	{\
		if (ccw==true){ \
		   for( i=count-1; i>=0; i--) \
		   {\
				upload_method(i);\
		   }\
		   \
		   if(lineloop)\
		   {\
				upload_method(count-1);\
		   }\
		}\
		\
		if (cw==true){\
				\
		   for( i=0; i<count; i++)\
		   {\
				upload_method(i);\
		   }\
		   \
		   if(lineloop)\
		   {\
				upload_method(0);\
		   }\
		}\
	}\
	else\
	{\
		if(_GLtype == GL_QUAD_STRIP)\
		{\
			if(cw==true)\
			{\
				for(i = 1; i <= count/2-1; ++i)\
				{\
					upload_method(2*i-1 -1);\
					upload_method(2*i   -1);\
					upload_method(2*i+2 -1);\
					upload_method(2*i+1 -1);\
				}\
			}\
			\
			if(ccw==true)\
			{\
				for(i = 1; i <= count/2-1; ++i)\
				{\
					upload_method(2*i-1 -1);\
					upload_method(2*i+1 -1);\
					upload_method(2*i+2 -1);\
					upload_method(2*i   -1);\
				}\
			}\
		}\
	}
	
	int tex_count = 0;
	int all_tex_index = 1;
	int sent_verts = 0;
	for(i = 0; i < MAX_NO_TEXTURES; ++i)
	{
		if(tex[i].enable)
		{
			++tex_count;
			if(tex_mode[i] != GX_INDEX16)
			{
				all_tex_index = 0;
			}
		}
	}
	
	if(vert_mode == GX_INDEX16 && all_tex_index &&
		(norm_mode == GX_INDEX16 || !norm.enable) &&
		(color_mode == GX_INDEX16 || !color.enable))
	{
		if(render_mode == ARRAY)
		{
			GEN_FULL_QUEST_ARRAY(DO_RENDER, norm.enable, color.enable)
		}
		else if(render_mode == ELEMENTS)
		{
			switch(elemType)
			{
			case GL_UNSIGNED_BYTE:
				GEN_FULL_QUEST_ELEM_TYPE(DO_RENDER, GLubyte, norm.enable, color.enable)
				break;
			case GL_UNSIGNED_SHORT:
				GEN_FULL_QUEST_ELEM_TYPE(DO_RENDER, GLushort, norm.enable, color.enable)
				break;
			case GL_UNSIGNED_INT:
				GEN_FULL_QUEST_ELEM_TYPE(DO_RENDER, GLuint, norm.enable, color.enable)
				break;
			}
		}			
	}
	
	if(!sent_verts)
	{
		// No fast render option, just use old method
		if(!special)
		{
			if (ccw==true){ 
		       //CCW     
		       for( i=count-1; i>=0; i--)
		       {
		            UploadVertex(i);
		       }
			   
			   if(lineloop)
			   {
					UploadVertex(count-1);
			   }
		    }
		    
		    if (cw==true){
					
		       //CW
		       for( i=0; i<count; i++)
		       {
		            UploadVertex(i);    	
		       }
			   
			   if(lineloop)
			   {
					UploadVertex(0);
			   }
		    }
		}
		else
		{
			if(_GLtype == GL_QUAD_STRIP)
			{
				//    3-------4
				//    |       |
				//	  |       |
				//	  |       |
				//	  1-------2
				if(cw==true)
				{
					for(i = 1; i <= count/2-1; ++i)
					{
						UploadVertex(2*i-1 -1); // 1
						UploadVertex(2*i   -1); // 2
						UploadVertex(2*i+2 -1); // 4
						UploadVertex(2*i+1 -1); // 3
					}
				}
					
				if(ccw==true)
				{
					for(i = 1; i <= count/2-1; ++i)
					{
						UploadVertex(2*i-1 -1); // 1
						UploadVertex(2*i+1 -1); // 3
						UploadVertex(2*i+2 -1); // 4
						UploadVertex(2*i   -1); // 2
					}
				}
			}
		}
	}
	
	//clean up just to be sure
	ResetArrays();
	
	GX_End();
	
	GX_DrawDone();
}

void glEnd()
{
	if(!(cur_state & GL_STATE_BEGIN)) { _glSetError(GL_INVALID_OPERATION); return; }
	
	// Reset Vertex Description		
	GX_ClearVtxDesc();
	vert_mode = GX_DIRECT; GX_SetVtxDesc(GX_VA_POS, GX_DIRECT);
	norm_mode = GX_DIRECT; GX_SetVtxDesc(GX_VA_NRM, GX_DIRECT);
	color_mode = GX_DIRECT; GX_SetVtxDesc(GX_VA_CLR0, GX_DIRECT);
	
	int i;
	for(i = 0; i < MAX_NO_TEXTURES; ++i)
	{
		if(!_glGetEnableTex(i))
		{
			continue;
		}
		
		tex_mode[i] = GX_DIRECT; GX_SetVtxDesc(incTexDesc(i), GX_DIRECT);
	}
	
	render_mode = INTERNAL;
	
	glRender(vert_i);
}

static __inline__ void _glSetClientState( GLenum cap, bool val )
{
	NO_CALL_IN_BEGIN;
switch(cap)
{
case GL_COLOR_ARRAY: color.enable = val; break;
case GL_NORMAL_ARRAY: norm.enable = val; break;
case GL_TEXTURE_COORD_ARRAY: tex[cur_tex_client].enable = val; break;
case GL_VERTEX_ARRAY: vert.enable = val; break;
case GL_EDGE_FLAG_ARRAY:
case GL_INDEX_ARRAY:
default: _glSetError(GL_INVALID_ENUM); break;
}
}

void glEnableClientState( GLenum cap )
{
	NO_CALL_IN_BEGIN;
	
	_glSetClientState(cap, true);
}

void glDisableClientState( GLenum cap )
{
	NO_CALL_IN_BEGIN;
	
	_glSetClientState(cap, false);
}

static __inline__ GLsizei computeStride(GLint size, GLenum type)
{
	return size*getElemSize(type);
}

void glTexCoordPointer( GLint size, GLenum type,
                                         GLsizei stride, const GLvoid *ptr )
{
	NO_CALL_IN_BEGIN;
	
	if(size < 1 || size > 4)
	{
		_glSetError(GL_INVALID_VALUE);
		return;
	}
	
	switch(type)
	{
	case GL_SHORT:
	case GL_INT:
	case GL_FLOAT:
	case GL_DOUBLE: break;
	default: _glSetError(GL_INVALID_ENUM); break;
	}
	
	if(stride < 0)
	{
		_glSetError(GL_INVALID_VALUE);
		return;
	}
	
	if(stride == 0)
	{
		stride = computeStride(size, type);
	}
	
	tex[cur_tex_client].size = size;
	tex[cur_tex_client].type = type;
	tex[cur_tex_client].stride = stride;
	tex[cur_tex_client].p = ptr;
	
};


GLboolean _glIsArrayEnabled(GLenum cap)
{
	switch(cap)
	{
	case GL_TEXTURE_COORD_ARRAY: return GLbool(tex[cur_tex_client].enable); break;
	case GL_VERTEX_ARRAY: return GLbool(vert.enable); break;
	case GL_COLOR_ARRAY: return GLbool(color.enable); break;
	case GL_NORMAL_ARRAY: return GLbool(norm.enable); break;
	case GL_INDEX_ARRAY:
	case GL_EDGE_FLAG_ARRAY:
	default:
		return GL_FALSE;
	}
}

void glVertexPointer( GLint size, GLenum type,
                                       GLsizei stride, const GLvoid *ptr )
{
	NO_CALL_IN_BEGIN;
	
	if(size < 2 || size > 4)
	{
		_glSetError(GL_INVALID_VALUE);
		return;
	}
		
	switch(type)
	{
	case GL_SHORT:
	case GL_INT:
	case GL_FLOAT:
	case GL_DOUBLE: break;
	default: _glSetError(GL_INVALID_ENUM); break;
	}
	
	if(stride < 0)
	{
		_glSetError(GL_INVALID_VALUE);
		return;
	}
	
	if(stride == 0)
	{
		stride = computeStride(size, type);
	}
	
	vert.size = size;
	vert.type = type;
	vert.stride = stride;
	vert.p = ptr;
};

void glNormalPointer( GLenum type, GLsizei stride,
                                       const GLvoid *ptr )
{	
	NO_CALL_IN_BEGIN;
	
	switch(type)
	{ 
	case GL_BYTE:
	case GL_SHORT:
	case GL_INT:
	case GL_FLOAT:
	case GL_DOUBLE: break;
	default: _glSetError(GL_INVALID_ENUM); break;
	}
	
	if(stride < 0)
	{
		_glSetError(GL_INVALID_VALUE);
		return;
	}
	
	if(stride == 0)
	{
		stride = computeStride(3, type);
	}
	
	norm.size = 3;
	norm.type = type;
	norm.stride = stride;
	norm.p = ptr;
};

void glColorPointer( GLint size,
			       GLenum type,
			       GLsizei stride,
			       const GLvoid *ptr )
{
	NO_CALL_IN_BEGIN;
	
	if(size < 3 || size > 4)
	{
		_glSetError(GL_INVALID_VALUE);
		return;
	}
	
	switch(type)
	{
	case GL_UNSIGNED_BYTE:
	case GL_BYTE:
	case GL_UNSIGNED_SHORT:
	case GL_SHORT:
	case GL_UNSIGNED_INT:
	case GL_INT:
	case GL_FLOAT:
	case GL_DOUBLE: break;
	default: _glSetError(GL_INVALID_ENUM); break;
	}
	
	if(stride < 0)
	{
		_glSetError(GL_INVALID_VALUE);
		return;
	}
	
	if(stride == 0)
	{
		stride = computeStride(size, type);
	}
	
	color.size = size;
	color.type = type;
	color.stride = stride;
	color.p = ptr;
};

u8 incTexDesc(u8 i)
{
	switch(i)
	{
	default:
	case 0: return GX_VA_TEX0;
	case 1: return GX_VA_TEX1;
	case 2: return GX_VA_TEX2;
	case 3: return GX_VA_TEX3;
	case 4: return GX_VA_TEX4;
	case 5: return GX_VA_TEX5;
	case 6: return GX_VA_TEX6;
	case 7: return GX_VA_TEX7;
	}
}

static bool native = 1;

void set_native(bool n)
{
	native = n;
}

#define SYSMEM2_SIZE 0x04000000
#define SYSMEM2_START 0x90000000

static __inline__ void InstallArray(u32 attr, u8 *type, VertexArray *ptr, size_t native_size, size_t native_type, int count)
{
	if(ptr->enable)
	{
		if(ptr->size == native_size && ptr->type == native_type && native)
		{
			*type = GX_INDEX16;
			const void * p = getElem(ptr, arrayOffset);
			GX_SetArray (attr, p, ptr->stride);
			DCFlushRange(p, count*ptr->stride);
		}
		else
		{
			*type = GX_DIRECT;
		}
	}
	else
	{
		*type = GX_DIRECT;
	}
}

static void confVtx(int count)
{
	GX_ClearVtxDesc();
	
	if(native)
	{
		GX_InvVtxCache();
	}
	
	InstallArray(GX_VA_POS, &vert_mode, &vert, 3, GL_FLOAT, count);
	GX_SetVtxDesc(GX_VA_POS, vert_mode);
	
	InstallArray(GX_VA_NRM, &norm_mode, &norm, 3, GL_FLOAT, count);
	if(norm.enable)
	{
		GX_SetVtxDesc(GX_VA_NRM, norm_mode);
	}
	
	InstallArray(GX_VA_CLR0, &color_mode, &color, 4, GL_UNSIGNED_BYTE, count);
	GX_SetVtxDesc(GX_VA_CLR0, color_mode);
		
	int i;
	for(i = 0; i < MAX_NO_TEXTURES; ++i)
	{
		if(!_glGetEnableTex(i))
		{
			continue;
		}
		
		InstallArray(incTexDesc(i), tex_mode+i, tex+i, 2, GL_FLOAT, count);
		if(tex[i].enable)
		{
			GX_SetVtxDesc(incTexDesc(i), tex_mode[i]);
		}
	}
}

void glDrawArrays( GLenum mode, GLint first, GLsizei count )
{
	NO_CALL_IN_BEGIN;
	
	if(count > MAX_VERTEX)
	{
		int elem_size = 0;
		int step = 0;
		switch(mode)
		{
		case GL_POINTS:
			elem_size = 1;
			step = 1;
			break;
		case GL_LINES:
			elem_size = 2;
			step = 1;
			break;
		case GL_LINE_STRIP:
			elem_size = 1;
			step = 0;
			break;
		case GL_TRIANGLES: 
			elem_size = 3;
			step = 1;
			break;
        case GL_TRIANGLE_STRIP:
			elem_size = 1;
			step = -1;
			break;
		case GL_QUADS:
			elem_size = 4;
			step = 0;
			break;
		case GL_QUAD_STRIP:
			elem_size = 2;
			step = -1;
		case GL_POLYGON:
		case GL_LINE_LOOP:
		case GL_TRIANGLE_FAN:
		default:
			// Add emulation mode
			_glSetError(GL_INVALID_ENUM);
			return;
		}
		
		int cur_first = first;
		int total_count = count+first;
		while(cur_first < total_count)
		{
			if(total_count - cur_first < MAX_VERTEX)
			{
				glDrawArrays(mode, cur_first, total_count - cur_first );
				cur_first = total_count;
			}
			else
			{
				int next_count = MAX_VERTEX - (MAX_VERTEX % elem_size);
				glDrawArrays(mode, cur_first, next_count);
				cur_first += next_count + step - 1;
			}			
		}
		return;
	}
	
	if(!vert.enable)
	{
		return;
	}
	
	glBegin(mode);
	
	if(_GLtype == GL_INVALID_ENUM)
	{
		return;
	}
	
	render_mode = ARRAY;
		
	arrayOffset = first;
	
	confVtx(count);
	
	glRender(count);
};

static __inline__ GLuint getMaxIndex(GLsizei count, GLenum type, const GLvoid *indices )
{
	int i,t;
	int max_index = 0;
	for(i = 0; i < count; ++i)
	{
		t = getElement(type, indices, i);
		if(t > max_index)
		{
			max_index = t;
		}
	}
	return max_index;
}

void glDrawElements(GLenum mode,
			       GLsizei count,
			       GLenum type,
			       const GLvoid *indices )
{
	glDrawRangeElementsEXT(mode, 0, getMaxIndex(count, type, indices), count, type, indices);
}


void glDrawRangeElements(GLenum mode,
        GLuint start,
        GLuint end,
        GLsizei count,
        GLenum type,
        const GLvoid *indices)
{
	glDrawRangeElementsEXT(mode,start,end,count,type,indices);
}

void glDrawRangeElementsEXT(GLenum mode,
        GLuint start,
        GLuint end,
        GLsizei count,
        GLenum type,
        const GLvoid *indices)
{
	NO_CALL_IN_BEGIN;
	
	if(count > MAX_VERTEX)
	{
		// Add emulation mode
		_glSetError(GL_INVALID_ENUM);
		return;
	}
	
	if(end > ~((u16)0))
	{
		// Indices are sent are as u16, so max end is u16 max
		_glSetError(GL_INVALID_VALUE);
		return;		
	}
	
	switch(type)
	{
	case GL_UNSIGNED_BYTE:
	case GL_UNSIGNED_SHORT:
	case GL_UNSIGNED_INT: break;
	default: _glSetError(GL_INVALID_ENUM); break;
	}
	
	if(!vert.enable)
	{
		return;
	}
	
	glBegin(mode);	
	
	if(_GLtype == GL_INVALID_ENUM)
	{
		return;
	}
		
	render_mode = ELEMENTS;
		
	arrayOffset = start;
	
	confVtx(end-start);	
	
	elemType = type;
	
	elemIndex = indices;
	
	glRender(count);
}
