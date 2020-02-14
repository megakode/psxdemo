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


static u_short storedClut[256];
static u_short storedClutX;
static u_short storedClutY;
static int fadeStepsLeft;
static int fadeDelay;

void initFadeClut(int clutX, int clutY)
{
	RECT rect = {clutX,clutY,256,1};
	StoreImage2(&rect,(u_long*)&storedClut);
	fadeStepsLeft = 32;
	fadeDelay = 2;
	storedClutX = clutX;
	storedClutY = clutY;
}

void doFadeClut()
{	
	u_short colorIndex = 0;
	u_short color;
	u_short r;
	u_short g;
	u_short b;
	
	
	if(fadeDelay>0){
		fadeDelay--;
		return;
	} else {
		fadeDelay=2;
	}
	
	if(fadeStepsLeft==0){
		return;
	}
	
	// Clut 16 bit pixel format:
	// |x|B|B|B|B|B|G|G|G|G|G|R|R|R|R|R| 

	// Fade clut
	
	for( colorIndex = 0 ; colorIndex < 256 ; colorIndex++ ){
	
		color = storedClut[colorIndex];
	
		r =  color & 31;
		g = (color >> 5) & 31;
		b = (color >>10) & 31;
		
		if(r>0) r-= r/fadeStepsLeft;
		if(g>0) g-= g/fadeStepsLeft;
		if(b>0) b-= b/fadeStepsLeft;

		storedClut[colorIndex] = (r) | ((g << 5)) | (b << 10);
		
	}
	
	fadeStepsLeft--;
	
	LoadClut((u_long*)storedClut,storedClutX,storedClutY);

}