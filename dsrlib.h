#ifndef __dsr_h__
#define __dsr_h__

#define FONT_X_OFFSET 640
#define TEXT_SPRITES 100

#include <sys/types.h>
#include <libetc.h>
#include <libgte.h>
#include <libgpu.h>
#include <libmath.h>
#include <Gtemac.h>
#include <Inline_c.h>
#include <libgs.h>


// vertexData

// int16 x;
// int16 y;
// int16 z;

// faceData

// uint16 vi1
// uint16 vi2
// uint16 vi3
// uint16 ni1
// uint16 ni2
// uint16 ni3

typedef struct {

	short x;
	short y;
	short z;

} ModelVertex;

typedef struct {

	// Vertex indexes
	unsigned short vi1; 
	unsigned short vi2;
	unsigned short vi3;
	
	// Normal indexes
	unsigned short ni1; 
	unsigned short ni2; 
	unsigned short ni3; 

} ModelFace;

// ************************************************************
// Load TIM
// ************************************************************

void loadTIM(unsigned char *addr);

// ************************************************************
//
// Set a SPRT_8 sprites UV cords according to the 'letter' argument, 
// and position according to xpos,ypos
//
// It is assumed that the font sheet is at {0,0} in the sprites texture page
// And the letter size is 8x8.
//
// ************************************************************

void setLetter(SPRT_8 *sprite,char letter, int xpos, int ypos);

// ************************************************************
//
// - Loop through the 'str' string 
// - For each letter: configure a SPRT_8 primitive with correct UV and XY.
// - Add the SPRT_8 primitive to OT
// - Increase startingSprite pointer (no overflow check!)
//
// NOTE: OT is not increased! All sprite primitives are added to the same entry.
//
// ************************************************************

void drawString(char *str,int xpos,int ypos,SPRT_8 **startingSprite,u_long *ot);

void initFadeClut(int clutX, int clutY);

void doFadeClut(int clutX, int clutY);

#endif