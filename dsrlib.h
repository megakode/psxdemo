#ifndef __dsr_h__
#define __dsr_h__

#define FONT_X_OFFSET 640

#include <sys/types.h>
#include <libetc.h>
#include <libgte.h>
#include <libgpu.h>
#include <libmath.h>
#include <Gtemac.h>
#include <Inline_c.h>
#include <libgs.h>


typedef enum { FadeUp , FadeDown  } ClutFadeDirection;

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
	VECTOR position;
	SVECTOR rotation;
} CameraPreset;

typedef struct {
	const CameraPreset *from;
	const CameraPreset *to;
} CameraAnimationPreset;

typedef struct {
	VECTOR fromRot;	// From rotation vector
	VECTOR fromPos;	// From position vector
	VECTOR toPos;	// To position vector
	VECTOR toRot;	// To rotation vector
	VECTOR currentPos;	// Current position in the animation
	VECTOR currentRot;	// Current rotation in the animation
	VECTOR deltaPos; 	// The delta amount to add to currentPos for every animation step
	VECTOR deltaRot;	// The delta amount to add to currentRot for every animation step
	int currentStep;
	int numTotalSteps;
} CameraAnimation;

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

// ************************************************************
//  Store the clut located at video memory position clutX,clutY
//  into memory.
// ************************************************************

void clutFadeInit(int clutX, int clutY, ClutFadeDirection direction );

// ************************************************************
// Restores the stored Clut back into VRAM
// ************************************************************
void clutFadeRestore();

// ************************************************************
// Fade out the clut cached in memory, and copy it back to the
// location in video memory where we initially got it from in initFadeClut.
//
// return 1 if still fading, 0 if done
//
// ************************************************************
int clutFadeDown();

#endif