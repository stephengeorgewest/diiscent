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

/*
 *
 * Header for morph.c
 *
 *
 */


#ifndef _H
#define _H

#include "object.h"

#define MAX_VECS 10000

typedef struct tMorphInfo {
	CObject			*objP;                                // CObject which is morphing
	CFixVector		vecs [MAX_VECS];
	CFixVector		deltas [MAX_VECS];
	fix				times [MAX_VECS];
	int				submodelActive [MAX_SUBMODELS];         // which submodels are active
	int				nMorphingPoints [MAX_SUBMODELS];       // how many active points in each part
	int				submodelStartPoints [MAX_SUBMODELS];    // first point for each submodel
	int				nSubmodelsActive;
	ubyte				saveControlType;
	ubyte				saveMovementType;
	tPhysicsInfo	savePhysInfo;
	int				nSignature;
} __pack__ tMorphInfo;

#define MAX_MORPH_OBJECTS 250

#define MORPH_RATE (I2X (1)*3)

//process the morphing CObject for one frame
void DoMorphFrame(CObject *obj);

//called at the start of a level
void MorphInit();

extern tMorphInfo *MorphFindData (CObject *obj);

#endif /* _H */
