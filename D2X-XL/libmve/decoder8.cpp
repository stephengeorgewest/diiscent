/* 8 bit decoding routines */

#include <stdio.h>
#include <string.h>

#include "decoders.h"

static void dispatchDecoder (ubyte **frameP, ubyte codeType, ubyte **dataP, int *remainDataP, int *curXb, int *curYb);

void decodeFrame8 (ubyte *frameP, ubyte *mapP, int mapRemain, ubyte *dataP, int dataRemain)
{
	int i, j;
	int xb, yb;

	xb = g_width >> 3;
	yb = g_height >> 3;
	for (j=0; j<yb; j++)
	{
		for (i=0; i<xb/2; i++)
		{
			dispatchDecoder (&frameP, (ubyte) (*mapP & 0xf), &dataP, &dataRemain, &i, &j);
#ifndef _WIN32
			if (frameP < reinterpret_cast<ubyte*> (g_vBackBuf1))
				fprintf (stderr, "danger!  pointing out of bounds below after dispatch decoder: %d, %d (1) [%x]\n", i, j, (*mapP) & 0xf);
			else if (frameP >= reinterpret_cast<ubyte*> (g_vBackBuf1) + g_width*g_height)
				fprintf (stderr, "danger!  pointing out of bounds above after dispatch decoder: %d, %d (1) [%x]\n", i, j, (*mapP) & 0xf);
#endif
			dispatchDecoder (&frameP, (ubyte) (*mapP >> 4), &dataP, &dataRemain, &i, &j);
#ifndef _WIN32
			if (frameP < reinterpret_cast<ubyte*> (g_vBackBuf1))
				fprintf (stderr, "danger!  pointing out of bounds below after dispatch decoder: %d, %d (2) [%x]\n", i, j, (*mapP) >> 4);
			else if (frameP >= reinterpret_cast<ubyte*> (g_vBackBuf1) + g_width*g_height)
				fprintf (stderr, "danger!  pointing out of bounds above after dispatch decoder: %d, %d (2) [%x]\n", i, j, (*mapP) >> 4);
#endif
			++mapP;
			--mapRemain;
		}

		frameP += 7*g_width;
	}
}

static void relClose (int i, int *x, int *y)
{
*x = (i & 0xf) - 8;
*y = (i >> 4) - 8;
}

static void relFar (int i, int sign, int *x, int *y)
{
if (i < 56) {
	*x = sign * (8 + (i % 7));
	*y = sign *      (i / 7);
	}
else {
	*x = sign * (-14 + (i - 56) % 29);
	*y = sign *   (8 + (i - 56) / 29);
	}
}

/* copies an 8x8 block from pSrc to pDest.
   pDest and pSrc are both g_width bytes wide */
static void copyFrame (ubyte *pDest, ubyte *pSrc)
{
for (int i = 0; i < 8; i++) {
	memcpy (pDest, pSrc, 8);
	pDest += g_width;
	pSrc += g_width;
	}
}

// Fill in the next eight bytes with p [0], p [1], p [2], or p [3],
// depending on the corresponding two-bit value in pat0 and pat1
static void patternRow4Pixels (ubyte *frameP, ubyte pat0, ubyte pat1, ubyte *p)
{
	ushort mask = 0x0003;
	ushort shift = 0;
	ushort pattern = (pat1 << 8) | pat0;

while (mask != 0) {
	*frameP++ = p [ (mask & pattern) >> shift];
	mask <<= 2;
	shift += 2;
	}
}

// Fill in the next four 2x2 pixel blocks with p [0], p [1], p [2], or p [3],
// depending on the corresponding two-bit value in pat0.
static void patternRow4Pixels2 (ubyte *frameP, ubyte pat0, ubyte *p)
{
	ubyte mask = 0x03;
	ubyte shift = 0;
	ubyte pel;

while (mask != 0) {
	pel = p [ (mask & pat0) >> shift];
	frameP [0] = pel;
	frameP [1] = pel;
	frameP [g_width + 0] = pel;
	frameP [g_width + 1] = pel;
	frameP += 2;
	mask <<= 2;
	shift += 2;
	}
}

// Fill in the next four 2x1 pixel blocks with p [0], p [1], p [2], or p [3],
// depending on the corresponding two-bit value in pat.
static void patternRow4Pixels2x1 (ubyte *frameP, ubyte pat, ubyte *p)
{
	ubyte mask = 0x03;
	ubyte shift = 0;
	ubyte pel;

while (mask != 0)	{
	pel = p [ (mask & pat) >> shift];
	frameP [0] = pel;
	frameP [1] = pel;
	frameP += 2;
	mask <<= 2;
	shift += 2;
	}
}

// Fill in the next 4x4 pixel block with p [0], p [1], p [2], or p [3],
// depending on the corresponding two-bit value in pat0, pat1, pat2, and pat3.
static void patternQuadrant4Pixels (ubyte *frameP, ubyte pat0, ubyte pat1, ubyte pat2, ubyte pat3, ubyte *p)
{
	uint mask = 3;
	int shift = 0;
	int i;
	uint pat = (pat3 << 24) | (pat2 << 16) | (pat1 << 8) | pat0;

for (i=0; i<16; i++) {
	frameP [i&3] = p [ (pat & mask) >> shift];
	if ((i&3) == 3)
		frameP += g_width;
	mask <<= 2;
	shift += 2;
	}
}

// fills the next 8 pixels with either p [0] or p [1], depending on pattern
static void patternRow2Pixels (ubyte *frameP, ubyte pat, ubyte *p)
{
	ubyte mask = 0x01;

while (mask != 0)	{
	*frameP++ = p [ (mask & pat) ? 1 : 0];
	mask <<= 1;
	}
}

// fills the next four 2 x 2 pixel boxes with either p [0] or p [1], depending on pattern
static void patternRow2Pixels2 (ubyte *frameP, ubyte pat, ubyte *p)
{
	ubyte pel;
	ubyte mask = 0x1;

while (mask != 0x10) {
	pel = p [ (mask & pat) ? 1 : 0];
	frameP [0] = pel;              // upper-left
	frameP [1] = pel;              // upper-right
	frameP [g_width + 0] = pel;    // lower-left
	frameP [g_width + 1] = pel;    // lower-right
	frameP += 2;
	mask <<= 1;
	}
}

// fills pixels in the next 4 x 4 pixel boxes with either p [0] or p [1], depending on pat0 and pat1.
static void patternQuadrant2Pixels (ubyte *frameP, ubyte pat0, ubyte pat1, ubyte *p)
{
	ubyte pel;
	ushort mask = 0x0001;
	int i, j;
	ushort pat = (pat1 << 8) | pat0;

for (i=0; i<4; i++) {
	for (j=0; j<4; j++) {
		pel = p [ (pat & mask) ? 1 : 0];
		frameP [j + i * g_width] = pel;
		mask <<= 1;
		}
	}
}
static void dispatchDecoder (ubyte **framePP, ubyte codeType, ubyte **dataPP, int *remainDataP, int *curXb, int *curYb)
{
	ubyte* frameP = *framePP;
	ubyte* dataP = *dataPP;
	ubyte p [4];
	ubyte pat [16];
	int i, j, k;
	int x, y;

	/* Data is processed in 8x8 pixel blocks.
	   There are 16 ways to encode each block.
	*/

switch (codeType) {
	case 0x0:
		/* block is copied from block in current frame */
		copyFrame (frameP, frameP + (reinterpret_cast<ubyte*> (g_vBackBuf2) - reinterpret_cast<ubyte*> (g_vBackBuf1)));

	case 0x1:
		/* block is unchanged from two frames ago */
		frameP += 8;
		break;

	case 0x2:
		/* Block is copied from nearby (below and/or to the right) within the
		   new frame.  The offset within the buffer from which to grab the
		   patch of 8 pixels is given by grabbing a byte B from the data
		   stream, which is broken into a positive x and y offset according
		   to the following mapping:

		   if B < 56:
		   x = 8 + (B % 7)
		   y = B / 7
		   else
		   x = -14 + ((B - 56) % 29)
		   y =   8 + ((B - 56) / 29)
		*/
		relFar (*dataP++, 1, &x, &y);
		copyFrame ( frameP, frameP + x + y*g_width);
		frameP += 8;
		--*remainDataP;
		break;

	case 0x3:
		/* Block is copied from nearby (above and/or to the left) within the
		   new frame.

		   if B < 56:
		   x = - (8 + (B % 7))
		   y = - (B / 7)
		   else
		   x = - (-14 + ((B - 56) % 29))
		   y = - (  8 + ((B - 56) / 29))
		*/
		relFar (*dataP++, -1, &x, &y);
		copyFrame (frameP, frameP + x + y*g_width);
		frameP += 8;
		--*remainDataP;
		break;

	case 0x4:
		/* Similar to 0x2 and 0x3, except this method copies from the
		   "current" frame, rather than the "new" frame, and instead of the
		   lopsided mapping they use, this one uses one which is symmetric
		   and centered around the top-left corner of the block.  This uses
		   only 1 byte still, though, so the range is decreased, since we
		   have to encode all directions in a single byte.  The byte we pull
		   from the data stream, I'll call B.  Call the highest 4 bits of B
		   BH and the lowest 4 bytes BL.  Then the offset from which to copy
		   the data is:

		   x = -8 + BL
		   y = -8 + BH
		*/
		relClose (*dataP++, &x, &y);
		copyFrame (frameP, frameP + (reinterpret_cast<ubyte*> (g_vBackBuf2) - reinterpret_cast<ubyte*> (g_vBackBuf1)) + x + y*g_width);
		frameP += 8;
		--*remainDataP;
		break;

	case 0x5:
		/* Similar to 0x4, but instead of one byte for the offset, this uses
		   two bytes to encode a larger range, the first being the x offset
		   as a signed 8-bit value, and the second being the y offset as a
		   signed 8-bit value.
		*/
		x = (signed char)*dataP++;
		y = (signed char)*dataP++;
		copyFrame (frameP, frameP + (reinterpret_cast<ubyte*> (g_vBackBuf2) - reinterpret_cast<ubyte*> (g_vBackBuf1)) + x + y*g_width);
		frameP += 8;
		*remainDataP -= 2;
		break;

	case 0x6:
		/* I can't figure out how any file containing a block of this nType
		   could still be playable, since it appears that it would leave the
		   internal bookkeeping in an inconsistent state in the BG tPlayer
		   code.  Ahh, well.  Perhaps it was a bug in the BG tPlayer code that
		   just didn't happen to be exposed by any of the included movies.
		   Anyway, this skips the next two blocks, doing nothing to them.
		   Note that if you've reached the end of a row, this means going on
		   to the next row.
		*/
		for (i=0; i<2; i++)
		{
			frameP += 16;
			if (++*curXb == (g_width >> 3))
			{
				frameP += 7*g_width;
				*curXb = 0;
				if (++*curYb == (g_height >> 3)) {
					*framePP = frameP;
					*dataPP = dataP;
					return;
					}
			}
		}
		break;

	case 0x7:
		/* Ok, here's where it starts to get really...interesting.  This is,
		   incidentally, the part where they started using self-modifying
		   code.  So, most of the following encodings are "patterned" blocks,
		   where we are given a number of pixel values and then bitmapped
		   values to specify which pixel values belong to which squares.  For
		   this encoding, we are given the following in the data stream:

		   P0 P1

		   These are pixel values (i.e. 8-bit indices into the palette).  If
		   P0 <= P1, we then get 8 more bytes from the data stream, one for
		   each row in the block:

		   B0 B1 B2 B3 B4 B5 B6 B7

		   For each row, the leftmost pixel is represented by the low-order
		   bit, and the rightmost by the high-order bit.  Use your imagination
		   in between.  If a bit is set, the pixel value is P1 and if it is
		   unset, the pixel value is P0.

		   So, for example, if we had:

		   11 22 fe 83 83 83 83 83 83 fe

		   This would represent the following layout:

		   11 22 22 22 22 22 22 22     ; fe == 11111110
		   22 22 11 11 11 11 11 22     ; 83 == 10000011
		   22 22 11 11 11 11 11 22     ; 83 == 10000011
		   22 22 11 11 11 11 11 22     ; 83 == 10000011
		   22 22 11 11 11 11 11 22     ; 83 == 10000011
		   22 22 11 11 11 11 11 22     ; 83 == 10000011
		   22 22 11 11 11 11 11 22     ; 83 == 10000011
		   11 22 22 22 22 22 22 22     ; fe == 11111110

		   If, on the other hand, P0 > P1, we get two more bytes from the
		   data stream:

		   B0 B1

		   Each of these bytes contains two 4-bit patterns. These patterns
		   work like the patterns above with 8 bytes, except each bit
		   represents a 2x2 pixel region.

		   B0 contains the pattern for the top two rows and B1 contains
		   the pattern for the bottom two rows.  Note that the low-order
		   nibble of each byte contains the pattern for the upper of the
		   two rows that that byte controls.

		   So if we had:

		   22 11 7e 83

		   The output would be:

		   11 11 22 22 22 22 22 22     ; e == 1 1 1 0
		   11 11 22 22 22 22 22 22     ;
		   22 22 22 22 22 22 11 11     ; 7 == 0 1 1 1
		   22 22 22 22 22 22 11 11     ;
		   11 11 11 11 11 11 22 22     ; 3 == 1 0 0 0
		   11 11 11 11 11 11 22 22     ;
		   22 22 22 22 11 11 11 11     ; 8 == 0 0 1 1
		   22 22 22 22 11 11 11 11     ;
		*/
		p [0] = *dataP++;
		p [1] = *dataP++;
		if (p [0] <= p [1])
		{
			for (i=0; i<8; i++)
			{
				patternRow2Pixels (frameP, *dataP++, p);
				frameP += g_width;
			}
		}
		else
		{
			for (i=0; i<2; i++)
			{
				patternRow2Pixels2 (frameP, (ubyte) (*dataP & 0xf), p);
				frameP += 2*g_width;
				patternRow2Pixels2 (frameP, (ubyte) (*dataP++ >> 4), p);
				frameP += 2*g_width;
			}
		}
		frameP -= (8*g_width - 8);
		break;

	case 0x8:
		/* Ok, this one is basically like encoding 0x7, only more
		   complicated.  Again, we start out by getting two bytes on the data
		   stream:

		   P0 P1

		   if P0 <= P1 then we get the following from the data stream:

		   B0 B1
		   P2 P3 B2 B3
		   P4 P5 B4 B5
		   P6 P7 B6 B7

		   P0 P1 and B0 B1 are used for the top-left corner, P2 P3 B2 B3 for
		   the bottom-left corner, P4 P5 B4 B5 for the top-right, P6 P7 B6 B7
		   for the bottom-right.  (So, each codes for a 4x4 pixel array.)
		   Since we have 16 bits in B0 B1, there is one bit for each pixel in
		   the array.  The convention for the bit-mapping is, again, left to
		   right and top to bottom.

		   So, basically, the top-left quarter of the block is an arbitrary
		   pattern with 2 pixels, the bottom-left a different arbitrary
		   pattern with 2 different pixels, and so on.

		   For example if the next 16 bytes were:

		   00 22 f9 9f  44 55 aa 55  11 33 cc 33  66 77 01 ef

		   We'd draw:

		   22 22 22 22 | 11 11 33 33     ; f = 1111, c = 1100
		   22 00 00 22 | 11 11 33 33     ; 9 = 1001, c = 1100
		   22 00 00 22 | 33 33 11 11     ; 9 = 1001, 3 = 0011
		   22 22 22 22 | 33 33 11 11     ; f = 1111, 3 = 0011
		   ------------+------------
		   44 55 44 55 | 66 66 66 66     ; a = 1010, 0 = 0000
		   44 55 44 55 | 77 66 66 66     ; a = 1010, 1 = 0001
		   55 44 55 44 | 66 77 77 77     ; 5 = 0101, e = 1110
		   55 44 55 44 | 77 77 77 77     ; 5 = 0101, f = 1111

		   I've added a dividing line in the above to clearly delineate the
		   quadrants.


		   Now, if P0 > P1 then we get 10 more bytes from the data stream:

		   B0 B1 B2 B3 P2 P3 B4 B5 B6 B7

		   Now, if P2 <= P3, then the first six bytes [P0 P1 B0 B1 B2 B3]
		   represent the left half of the block and the latter six bytes
		   [P2 P3 B4 B5 B6 B7] represent the right half.

		   For example:

		   22 00 01 37 f7 31   11 66 8c e6 73 31

		   yeilds:

		   22 22 22 22 | 11 11 11 66     ; 0: 0000 | 8: 1000
		   00 22 22 22 | 11 11 66 66     ; 1: 0001 | C: 1100
		   00 00 22 22 | 11 66 66 66     ; 3: 0011 | e: 1110
		   00 00 00 22 | 11 66 11 66     ; 7: 0111 | 6: 0101
		   00 00 00 00 | 66 66 66 11     ; f: 1111 | 7: 0111
		   00 00 00 22 | 66 66 11 11     ; 7: 0111 | 3: 0011
		   00 00 22 22 | 66 66 11 11     ; 3: 0011 | 3: 0011
		   00 22 22 22 | 66 11 11 11     ; 1: 0001 | 1: 0001


		   On the other hand, if P0 > P1 and P2 > P3, then
		   [P0 P1 B0 B1 B2 B3] represent the top half of the
		   block and [P2 P3 B4 B5 B6 B7] represent the bottom half.

		   For example:

		   22 00 cc 66 33 19   66 11 18 24 
 81

		   yeilds:

		   22 22 00 00 22 22 00 00     ; cc: 11001100
		   22 00 00 22 22 00 00 22     ; 66: 01100110
		   00 00 22 22 00 00 22 22     ; 33: 00110011
		   00 22 22 00 00 22 22 22     ; 19: 00011001
		   -----------------------
		   66 66 66 11 11 66 66 66     ; 18: 00011000
		   66 66 11 66 66 11 66 66     ; 24: 00100100
		   66 11 66 66 66 66 11 66     ; 
: 01000010
		   11 66 66 66 66 66 66 11     ; 81: 10000001
		*/
		if ( dataP [0] <= dataP [1])
		{
			// four quadrant case
			for (i=0; i<4; i++)
			{
				p [0] = *dataP++;
				p [1] = *dataP++;
				pat [0] = *dataP++;
				pat [1] = *dataP++;
				patternQuadrant2Pixels (frameP, pat [0], pat [1], p);

				// alternate between moving down and moving up and right
				if (i & 1)
					frameP += 4 - 4*g_width; // up and right
				else
					frameP += 4*g_width;     // down
			}
		}
		else if ( dataP [6] <= dataP [7])
		{
			// split horizontal
			for (i=0; i<4; i++)
			{
				if ((i & 1) == 0)
				{
					p [0] = *dataP++;
					p [1] = *dataP++;
				}
				pat [0] = *dataP++;
				pat [1] = *dataP++;
				patternQuadrant2Pixels (frameP, pat [0], pat [1], p);

				if (i & 1)
					frameP -= (4*g_width - 4);
				else
					frameP += 4*g_width;
			}
		}
		else
		{
			// split vertical
			for (i=0; i<8; i++)
			{
				if ((i & 3) == 0)
				{
					p [0] = *dataP++;
					p [1] = *dataP++;
				}
				patternRow2Pixels (frameP, *dataP++, p);
				frameP += g_width;
			}
			frameP -= (8*g_width - 8);
		}
		break;

	case 0x9:
		/* Similar to the previous 2 encodings, only more complicated.  And
		   it will get worse before it gets better.  No longer are we dealing
		   with patterns over two pixel values.  Now we are dealing with
		   patterns over 4 pixel values with 2 bits assigned to each pixel
		   (or block of pixels).

		   So, first on the data stream are our 4 pixel values:

		   P0 P1 P2 P3

		   Now, if P0 <= P1  AND  P2 <= P3, we get 16 bytes of pattern, each
		   2 bits representing a 1x1 pixel (00=P0, 01=P1, 10=P2, 11=P3).  The
		   ordering is again left to right and top to bottom.  The most
		   significant bits represent the left tSide at the top, and so on.

		   If P0 <= P1  AND  P2 > P3, we get 4 bytes of pattern, each 2 bits
		   representing a 2x2 pixel.  Ordering is left to right and top to
		   bottom.

		   if P0 > P1  AND  P2 <= P3, we get 8 bytes of pattern, each 2 bits
		   representing a 2x1 pixel (i.e. 2 pixels wide, and 1 high).

		   if P0 > P1  AND  P2 > P3, we get 8 bytes of pattern, each 2 bits
		   representing a 1x2 pixel (i.e. 1 pixel wide, and 2 high).
		*/
		if ( dataP [0] <= dataP [1])
		{
			if ( dataP [2] <= dataP [3])
			{
				p [0] = *dataP++;
				p [1] = *dataP++;
				p [2] = *dataP++;
				p [3] = *dataP++;

				for (i=0; i<8; i++)
				{
					pat [0] = *dataP++;
					pat [1] = *dataP++;
					patternRow4Pixels (frameP, pat [0], pat [1], p);
					frameP += g_width;
				}

				frameP -= (8*g_width - 8);
			}
			else
			{
				p [0] = *dataP++;
				p [1] = *dataP++;
				p [2] = *dataP++;
				p [3] = *dataP++;

				patternRow4Pixels2 (frameP, *dataP++, p);
				frameP += 2*g_width;
				patternRow4Pixels2 (frameP, *dataP++, p);
				frameP += 2*g_width;
				patternRow4Pixels2 (frameP, *dataP++, p);
				frameP += 2*g_width;
				patternRow4Pixels2 (frameP, *dataP++, p);
				frameP -= (6*g_width - 8);
			}
		}
		else
		{
			if ( dataP [2] <= dataP [3])
			{
				// draw 2x1 strips
				p [0] = *dataP++;
				p [1] = *dataP++;
				p [2] = *dataP++;
				p [3] = *dataP++;

				for (i=0; i<8; i++)
				{
					pat [0] = *dataP++;
					patternRow4Pixels2x1 (frameP, pat [0], p);
					frameP += g_width;
				}

				frameP -= (8*g_width - 8);
			}
			else
			{
				// draw 1x2 strips
				p [0] = *dataP++;
				p [1] = *dataP++;
				p [2] = *dataP++;
				p [3] = *dataP++;

				for (i=0; i<4; i++)
				{
					pat [0] = *dataP++;
					pat [1] = *dataP++;
					patternRow4Pixels (frameP, pat [0], pat [1], p);
					frameP += g_width;
					patternRow4Pixels (frameP, pat [0], pat [1], p);
					frameP += g_width;
				}

				frameP -= (8*g_width - 8);
			}
		}
		break;

	case 0xa:
		/* Similar to the previous, only a little more complicated.

		We are still dealing with patterns over 4 pixel values with 2 bits
		assigned to each pixel (or block of pixels).

		So, first on the data stream are our 4 pixel values:

		P0 P1 P2 P3

		Now, if P0 <= P1, the block is divided into 4 quadrants, ordered
		 (as with opcode 0x8) TL, BL, TR, BR.  In this case the next data
		in the data stream should be:

		B0  B1  B2  B3
		P4  P5  P6  P7  B4  B5  B6  B7
		P8  P9  P10 P11 B8  B9  B10 B11
		P12 P13 P14 P15 B12 B13 B14 B15

		Each 2 bits represent a 1x1 pixel (00=P0, 01=P1, 10=P2, 11=P3).
		The ordering is again left to right and top to bottom.  The most
		significant bits represent the right tSide at the top, and so on.

		If P0 > P1 then the next data on the data stream is:

		B0 B1 B2  B3  B4  B5  B6  B7
		P4 P5 P6 P7 B8 B9 B10 B11 B12 B13 B14 B15

		Now, in this case, if P4 <= P5,
		 [P0 P1 P2 P3 B0 B1 B2 B3 B4 B5 B6 B7] represent the left half of
		the block and the other bytes represent the right half.  If P4 >
		P5, then [P0 P1 P2 P3 B0 B1 B2 B3 B4 B5 B6 B7] represent the top
		half of the block and the other bytes represent the bottom half.
		*/
		if ( dataP [0] <= dataP [1])
		{
			for (i=0; i<4; i++)
			{
				p [0] = *dataP++;
				p [1] = *dataP++;
				p [2] = *dataP++;
				p [3] = *dataP++;
				pat [0] = *dataP++;
				pat [1] = *dataP++;
				pat [2] = *dataP++;
				pat [3] = *dataP++;

				patternQuadrant4Pixels (frameP, pat [0], pat [1], pat [2], pat [3], p);

				if (i & 1)
					frameP -= (4*g_width - 4);
				else
					frameP += 4*g_width;
			}
		}
		else
		{
			if ( dataP [12] <= dataP [13])
			{
				// split vertical
				for (i=0; i<4; i++)
				{
					if ((i&1) == 0)
					{
						p [0] = *dataP++;
						p [1] = *dataP++;
						p [2] = *dataP++;
						p [3] = *dataP++;
					}

					pat [0] = *dataP++;
					pat [1] = *dataP++;
					pat [2] = *dataP++;
					pat [3] = *dataP++;

					patternQuadrant4Pixels (frameP, pat [0], pat [1], pat [2], pat [3], p);

					if (i & 1)
						frameP -= (4*g_width - 4);
					else
						frameP += 4*g_width;
				}
			}
			else
			{
				// split horizontal
				for (i=0; i<8; i++)
				{
					if ((i&3) == 0)
					{
						p [0] = *dataP++;
						p [1] = *dataP++;
						p [2] = *dataP++;
						p [3] = *dataP++;
					}

					pat [0] = *dataP++;
					pat [1] = *dataP++;
					patternRow4Pixels (frameP, pat [0], pat [1], p);
					frameP += g_width;
				}

				frameP -= (8*g_width - 8);
			}
		}
		break;

	case 0xb:
		/* In this encoding we get raw pixel data in the data stream -- 64
		   bytes of pixel data.  1 byte for each pixel, and in the standard
		   order (l->r, t->b).
		*/
		for (i=0; i<8; i++)
		{
			memcpy (frameP, dataP, 8);
			frameP += g_width;
			dataP += 8;
			*remainDataP -= 8;
		}
		frameP -= (8*g_width - 8);
		break;

	case 0xc:
		/* In this encoding we get raw pixel data in the data stream -- 16
		   bytes of pixel data.  1 byte for each block of 2x2 pixels, and in
		   the standard order (l->r, t->b).
		*/
		for (i=0; i<4; i++)
		{
			for (j=0; j<2; j++)
			{
				for (k=0; k<4; k++)
				{
					 (frameP) [2*k]   = dataP [k];
					 (frameP) [2*k+1] = dataP [k];
				}
				frameP += g_width;
			}
			dataP += 4;
			*remainDataP -= 4;
		}
		frameP -= (8*g_width - 8);
		break;

	case 0xd:
		/* In this encoding we get raw pixel data in the data stream -- 4
		   bytes of pixel data.  1 byte for each block of 4x4 pixels, and in
		   the standard order (l->r, t->b).
		*/
		for (i=0; i<2; i++)
		{
			for (j=0; j<4; j++)
			{
				for (k=0; k<4; k++)
				{
					 (frameP) [k*g_width+j] = dataP [0];
					 (frameP) [k*g_width+j+4] = dataP [1];
				}
			}
			frameP += 4*g_width;
			dataP += 2;
			*remainDataP -= 2;
		}
		frameP -= (8*g_width - 8);
		break;

	case 0xe:
		/* This encoding represents a solid 8x8 frame.  We get 1 byte of pixel
		   data from the data stream.
		*/
		for (i=0; i<8; i++)
		{
			memset (frameP, *dataP, 8);
			frameP += g_width;
		}
		++dataP;
		--*remainDataP;
		frameP -= (8*g_width - 8);
		break;

	case 0xf:
		/* This encoding represents a "dithered" frame, which is
		   checkerboarded with alternate pixels of two colors.  We get 2
		   bytes of pixel data from the data stream, and these bytes are
		   alternated:

		   P0 P1 P0 P1 P0 P1 P0 P1
		   P1 P0 P1 P0 P1 P0 P1 P0
		   ...
		   P0 P1 P0 P1 P0 P1 P0 P1
		   P1 P0 P1 P0 P1 P0 P1 P0
		*/
		for (i=0; i<8; i++)
		{
			for (j=0; j<8; j++)
			{
				 (frameP) [j] = dataP [ (i+j)&1];
			}
			frameP += g_width;
		}
		dataP += 2;
		*remainDataP -= 2;
		frameP -= (8*g_width - 8);
		break;

	default:
		break;
	}
*framePP = frameP;
*dataPP = dataP;
}
