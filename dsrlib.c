#include "dsrlib.h"

#define FONT_X_OFFSET 640
#define TEXT_SPRITES 100

// ************************************************************
// Load TIM
// ************************************************************

void loadTIM(unsigned char *addr)
{
	TIM_IMAGE	image;		/* TIM header */
	
	OpenTIM((u_long*)addr);			/* open TIM */
	while (ReadTIM(&image)) {
		if (image.caddr) {	/* load CLUT (if needed) */
//			setSTP(image.caddr, image.crect->w);
			LoadImage(image.crect, image.caddr);
		}
		if (image.paddr) 	/* load texture pattern */
			LoadImage(image.prect, image.paddr);
	}
}

// ************************************************************
//
// Set a SPRT_8 sprites UV cords according to the 'letter' argument, 
// and position according to xpos,ypos
//
// It is assumed that the font sheet is at {0,0} in the sprites texture page
// And the letter size is 8x8.
//
// ************************************************************

void setLetter(SPRT_8 *sprite,char letter, int xpos, int ypos)
{
	int charindex;
	int xoffs;
	int yoffs;
	int row;
	
	charindex = letter - 33;
	row = charindex >> 5; 	// divide by 32 (letters pr row)
	yoffs = row << 3; 		// * 8 (row height)
	xoffs = (charindex & 31) << 3;

	setXY0(sprite,xpos,ypos);
	setUV0(sprite,xoffs,yoffs);

}

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

void drawString(char *str,int xpos,int ypos,SPRT_8 **startingSprite,u_long *ot)
{
	while(*str != 0)
	{
		if( *str != ' '){
			// Adjust UV cords in sprite to show the given letter
			setLetter(*startingSprite,*str,xpos,ypos);
			// Add to OT to be drawn
			AddPrim(ot,*startingSprite);
			(*startingSprite)++;
		}
		
		xpos += 8;
		str++;
	}
}


static u_short origClut[256]; // The original CLUT located in 
static u_short currentClut[256];
static u_short storedClutX;
static u_short storedClutY;
static int fadeStepsLeft;
static int fadeDelay;
static ClutFadeDirection fadeDirection;

void clutFadeInit(int clutX, int clutY, ClutFadeDirection direction )
{
	int i;
	RECT rect = {clutX,clutY,256,1};

	StoreImage2(&rect,(u_long*)&origClut);
	fadeStepsLeft = 32;
	fadeDelay = 2;
	storedClutX = clutX;
	storedClutY = clutY;
	fadeDirection = direction;
	
	if(direction == FadeDown ) {
		for(i=0;i<256;i++){
			// If fading down:
			currentClut[i] = origClut[i];
		}
	} else {
		// If fading up: preset all colors to black
		for(i=0;i<256;i++){
			
			currentClut[i] = 0;
		}
		// And store that black CLUT into VRAM
		LoadClut((u_long*)currentClut,storedClutX,storedClutY);
	}
}

// Restores the original CLUT to VRAM that was saved before fading
void clutFadeRestore()
{
	LoadClut((u_long*)origClut,storedClutX,storedClutY);
}

int clutFade()
{	
	u_short colorIndex = 0;
	
	if(fadeDelay>0){
		fadeDelay--;
		return 1;
	} else {
		fadeDelay=2;
	}
	
	if(fadeStepsLeft==0){

		return 0;
	}
	
	// Clut 16 bit pixel format:
	// |x|B|B|B|B|B|G|G|G|G|G|R|R|R|R|R| 

	// Fade clut
	if(fadeDirection==FadeDown)
	{
		printf("fadedirection=%d down",fadeDirection);
	}
	else if(fadeDirection==FadeUp)
	{
		printf("fadedirection=%d up",fadeDirection);
	} else
	{
		printf("fade dir unknown");
	}
	
	
	
	for( colorIndex = 0 ; colorIndex < 256 ; colorIndex++ ){
	
		
		
		if(fadeDirection==FadeDown){
			u_short color = currentClut[colorIndex];
			u_short r =  color & 31;
			u_short g = (color >> 5) & 31;
			u_short b = (color >>10) & 31;
			if(r>0) r-= 1;
			if(g>0) g-= 1;
			if(b>0) b-= 1;

			currentClut[colorIndex] = (r) | ((g << 5)) | (b << 10);
			//printf("r=%d",r);
		} else {

			u_short origColor = origClut[colorIndex];
			u_short origR =  origColor & 31;
			u_short origG = (origColor >> 5) & 31;
			u_short origB = (origColor >>10) & 31;
			u_short currentColor = currentClut[colorIndex];
			u_short r =  currentColor & 31;
			u_short g = (currentColor >> 5) & 31;
			u_short b = (currentColor >>10) & 31;
			
			if(r<origR) r+=1;
			if(g<origG) g+=1;
			if(b<origB) b+=1;

			currentClut[colorIndex] = (r) | ((g << 5)) | (b << 10);
		}
		
	}
	
	fadeStepsLeft--;
	
	// Load a stored clut into VRAM
	LoadClut((u_long*)currentClut,storedClutX,storedClutY);

	return 1;

}

// **************************************************************
// Camera animation 
// *************************************************************

const int fixedPointDecimals = 0;

// **************************************************************
//
// Setup a CameraAnimation based on a preset and a given number of target steps.
//
// **************************************************************
void playCameraAnimation( const CameraAnimationPreset *preset , CameraAnimation *outputAnimation, int numberOfSteps)
{
	int deltaRotX,deltaRotY,deltaRotZ;

	outputAnimation->numTotalSteps = numberOfSteps;
	outputAnimation->currentStep = 0;

	// Store from/to rotation and position, and convert them to fixed point, so we can make a smoother LERP with them

	outputAnimation->fromRot.vx = preset->from->rotation.vx << fixedPointDecimals;
	outputAnimation->fromRot.vy = preset->from->rotation.vy << fixedPointDecimals;
	outputAnimation->fromRot.vz = preset->from->rotation.vz << fixedPointDecimals;
	outputAnimation->toRot.vx = preset->to->rotation.vx << fixedPointDecimals;
	outputAnimation->toRot.vy = preset->to->rotation.vy << fixedPointDecimals;
	outputAnimation->toRot.vz = preset->to->rotation.vz << fixedPointDecimals;

	outputAnimation->fromPos.vx = preset->from->position.vx << fixedPointDecimals;
	outputAnimation->fromPos.vy = preset->from->position.vy << fixedPointDecimals;
	outputAnimation->fromPos.vz = preset->from->position.vz << fixedPointDecimals;
	outputAnimation->toPos.vx = preset->to->position.vx << fixedPointDecimals;
	outputAnimation->toPos.vy = preset->to->position.vy << fixedPointDecimals;
	outputAnimation->toPos.vz = preset->to->position.vz << fixedPointDecimals;

	// Calculate position deltas

	outputAnimation->deltaPos.vx = (outputAnimation->toPos.vx - outputAnimation->fromPos.vx) / numberOfSteps;
	outputAnimation->deltaPos.vy = (outputAnimation->toPos.vy - outputAnimation->fromPos.vy) / numberOfSteps;
	outputAnimation->deltaPos.vz = (outputAnimation->toPos.vz - outputAnimation->fromPos.vz) / numberOfSteps;

	// Calculate rotation deltas

	deltaRotX = (preset->to->rotation.vx - preset->from->rotation.vx);
	deltaRotY = (preset->to->rotation.vy - preset->from->rotation.vy);
	deltaRotZ = (preset->to->rotation.vz - preset->from->rotation.vz);

	// If delta rotation is more than a half circle, take the other way around
	if(deltaRotX>2048){
		outputAnimation->deltaRot.vx =( ((deltaRotX-2048)*-1)<<fixedPointDecimals ) / numberOfSteps;
	} else {
		outputAnimation->deltaRot.vx = (deltaRotX<<fixedPointDecimals) / numberOfSteps;
	}

	if(deltaRotY>2048){
		outputAnimation->deltaRot.vy = (((deltaRotY-2048)*-1)<<fixedPointDecimals) / numberOfSteps;
	} else {
		outputAnimation->deltaRot.vy = (deltaRotY<<fixedPointDecimals) / numberOfSteps;
	}

	if(deltaRotZ>2048){
		outputAnimation->deltaRot.vz = (((deltaRotZ-2048)*-1)<<fixedPointDecimals) / numberOfSteps;
	} else {
		outputAnimation->deltaRot.vz = (deltaRotZ<<fixedPointDecimals) / numberOfSteps;
	}

	//printf("playCameraAnimation delta pos = %d,%d,%d rot = %d,%d,%d \n", outputAnimation->deltaPos.vx, outputAnimation->deltaPos.vy, outputAnimation->deltaPos.vz, outputAnimation->deltaRot.vx, outputAnimation->deltaRot.vy, outputAnimation->deltaRot.vz);

	outputAnimation->currentPos = outputAnimation->fromPos;
	outputAnimation->currentRot = outputAnimation->fromRot;

}

// **************************************************************
//
// Update a CameraAnimation and output a world matrix based on it.
// 
// animation: the CameraAnimation to update and use for calculating the world matrix
// outputMatrix: the world camera output matrix ready to use for SetRotMatrix and SetTransMatrix.
// 
// **************************************************************

void updateCameraAnimation( CameraAnimation *animation, MATRIX *outputMatrix )
{
	if(animation->currentStep >= animation->numTotalSteps){
		return;
	}

	animation->currentStep++;

	// X pos

	if( animation->deltaPos.vx < 0 )
	{
		if(animation->currentPos.vx > animation->toPos.vx )
		{
			animation->currentPos.vx += animation->deltaPos.vx;
		}
	} 
	else 
	{
		if(animation->currentPos.vx < animation->toPos.vx )
		{
			animation->currentPos.vx += animation->deltaPos.vx;
		}
	}

	// Y pos	
	
	if( animation->deltaPos.vy < 0 )
	{
		if(animation->currentPos.vy > animation->toPos.vy )
		{
			animation->currentPos.vy += animation->deltaPos.vy;
		}
	} 
	else 
	{
		if(animation->currentPos.vy < animation->toPos.vy )
		{
			animation->currentPos.vy += animation->deltaPos.vy;
		}
	}

	
	// Z pos
	
	if( animation->deltaPos.vz < 0 )
	{
		if(animation->currentPos.vz > animation->toPos.vz )
		{
			animation->currentPos.vz += animation->deltaPos.vz;
		}
	} 
	else 
	{
		if(animation->currentPos.vz < animation->toPos.vz )
		{
			animation->currentPos.vz += animation->deltaPos.vz;
		}
	}

	// X rotation

	if( animation->deltaRot.vx < 0)
	{
		if(animation->currentRot.vx > animation->toRot.vx)
		{
			animation->currentRot.vx += animation->deltaRot.vx;
		}
	} else 
	{
		if(animation->currentRot.vx < animation->toRot.vx)
		{
			animation->currentRot.vx += animation->deltaRot.vx;
		}
	}

	// Y rotation

	if( animation->deltaRot.vy < 0)
	{
		if(animation->currentRot.vy > animation->toRot.vy)
		{
			animation->currentRot.vy += animation->deltaRot.vy;
		}
	} else 
	{
		if(animation->currentRot.vy < animation->toRot.vy)
		{
			animation->currentRot.vy += animation->deltaRot.vy;
		}
	}

	// Z rotation

	if( animation->deltaRot.vz < 0)
	{
		if(animation->currentRot.vz > animation->toRot.vz)
		{
			animation->currentRot.vz += animation->deltaRot.vz;
		}
	} else 
	{
		if(animation->currentRot.vz < animation->toRot.vz)
		{
			animation->currentRot.vz += animation->deltaRot.vz;
		}
	}

	{
		SVECTOR roundedRotation;
		VECTOR roundedPosition;

		roundedRotation.vx = animation->currentRot.vx >> fixedPointDecimals;
		roundedRotation.vy = animation->currentRot.vy >> fixedPointDecimals;
		roundedRotation.vz = animation->currentRot.vz >> fixedPointDecimals;

		roundedPosition.vx = animation->currentPos.vx >> fixedPointDecimals;
		roundedPosition.vy = animation->currentPos.vy >> fixedPointDecimals;
		roundedPosition.vz = animation->currentPos.vz >> fixedPointDecimals;
		
		RotMatrix_gte(&roundedRotation, outputMatrix); // Calculate rotation matrix from vector
		TransMatrix(outputMatrix,&roundedPosition);
	}
}