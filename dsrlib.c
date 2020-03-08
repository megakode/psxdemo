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