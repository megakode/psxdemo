#include <sys/types.h>
#include <libetc.h>
#include <libgte.h>
#include <libgpu.h>
#include <libmath.h>
#include <Gtemac.h>
#include <Inline_c.h>
#include <libgs.h>

#include "dsrlib.h" // Desire lib

#include "picture.h"

#define OTSIZE 20

const static SPRITE_WIDTH = 64;
const static SPRITE_HEIGHT = 256;

typedef struct {		
	DRAWENV		draw;			/* drawing environment */
	DISPENV		disp;			/* display environment */
	u_long		ot[OTSIZE];		/* ordering table */
	SPRT	*sprites;	/* mesh cell */
	DR_TPAGE *spriteTexturePages;
} DB;

DB	db[2];		/* double buffer */
DB	*cdb;		/* current double buffer */
u_long	*ot;		/* current OT */

// *************************************************************
// Setup primitives
// *************************************************************

static void setupPrimitives( DB *db, int cols, int rows, int spriteWidth, int spriteHeight, int pictureSrcX, int pictureSrcY, int clutSrcX,int clutSrcY, int textureMode)
{	
	int x,y;
	SPRT *sprite;
	DR_TPAGE *tpage;
	u_short tpageid;
	int texturePageWidth;
	
	// 0=4bit 1=8bit 2=16bit_direct
	
	db->sprites = (SPRT*)malloc(sizeof(SPRT)*cols*rows);
	db->spriteTexturePages = (DR_TPAGE*)malloc(sizeof(DR_TPAGE)*cols*rows);
	//db->ot = (u_long*)malloc(sizeof(u_long*)*(cols*rows + 1));
//	db->ot = (u_long*)malloc(OTSIZE);

	sprite = db->sprites;
	tpage = db->spriteTexturePages;
	
	for( y = 0 ; y < rows; y++ )
	for( x = 0 ; x < cols; x++ )
	{
		int texturePageX = (x*spriteWidth);
		int texturePageY = (y*spriteHeight);
		
		setSprt(sprite);
		setXY0(sprite,texturePageX,texturePageY);
		setWH(sprite,spriteWidth,spriteHeight);
		SetShadeTex(sprite, 1); // Set texture shading to off
        
		if (textureMode==0){ // 4 bit
			// texturePageWidth = 64;
			setUV0(sprite,0,0);
		} else if(textureMode==1){ // 8 bit
			// texturePageWidth = 128;
			setUV0(sprite,0 + (texturePageX & 64) ,0);
		} else if (textureMode==2){ // 16 bit direct
			// texturePageWidth = 256;
			setUV0(sprite,0,0);
		}


		setClut(sprite,clutSrcX,clutSrcY);

		tpageid = GetTPage(textureMode,0,pictureSrcX + (texturePageX>>1),pictureSrcY + texturePageY);
		setDrawTPage(tpage,0,0,tpageid);	   

		sprite+=1;
		tpage+=1;
	}
	

}

// *************************************************************
// doPicture
// *************************************************************

void doFadePicture( u_long *tim , int screenWidth, int xoffs, int yoffs, int showPictureTicks)
{
	const int screenHeight = 256;
	int i,timWidth,timHeight,cols,rows;
	int textureMode = 0; 	// 0=4bit 1=8bit 2=16bit_direct
	
	typedef enum {
		FadeInState,
		ShowPictureState,
		FadeOutState,
		DoneState
	} State;
	
	State state = FadeInState;
	int ticks = 0;
	int shrinkingLine = 0;
	int shrinkTrigger = 0;
	int shrinkAdvanceLineTrigger = 0;
	
	TIM_IMAGE	header;
	
	SPRT *sprite;
	DR_TPAGE *tpage;
/*
	InitGeom();
	
	ResetGraph(0);
	ResetCallback();
	PadInit(0);
	SetGraphDebug(0);
	
	InitGeom();
*/
	SetGeomOffset(0, 0);
	SetGeomScreen(1024);
	//SetVideoMode(MODE_PAL);
	
	/* initialize environment for double buffer */
	SetDefDrawEnv(&db[0].draw, 0,   0, screenWidth, screenHeight);
	SetDefDrawEnv(&db[1].draw, 0, screenHeight, screenWidth, screenHeight);
	SetDefDispEnv(&db[0].disp, 0, screenHeight, screenWidth, screenHeight);
	SetDefDispEnv(&db[1].disp, 0,   0, screenWidth, screenHeight);
	
	
	// PAL setup
	db[1].disp.screen.x = db[0].disp.screen.x = 1;
	db[1].disp.screen.y = db[0].disp.screen.y = 18;
	db[1].disp.screen.h = db[0].disp.screen.h = screenHeight;
	db[1].disp.screen.w = db[0].disp.screen.w = screenWidth;
	
	// Set background clear color
	db[0].draw.isbg = 1;
	db[1].draw.isbg = 1;
	setRGB0(&db[0].draw, 0, 0, 0);
	setRGB0(&db[1].draw, 0, 0, 0);
	
	SetDispMask(1);

	OpenTIM(tim);
	if(ReadTIM(&header))
	{
		if (header.caddr) {	/* load CLUT (if needed) */
//			setSTP(image.caddr, image.crect->w);
			LoadImage(header.crect, header.caddr);
		}
		if (header.paddr){ 	/* load texture pattern */
			timWidth = header.prect->w;
			printf("pixel mode=%d\n",header.mode);
			//header.mode = 1;
			if(header.mode==8){
				printf("4bpp TIM detected\n");
				timWidth*=4; // 4bpp (as the prect->width says how many 16bpp pixels in the VRAM that the tim file occupies
				textureMode = 0;
			}else if(header.mode==9){
				printf("8bpp TIM detected\n");
				timWidth*=2; // 8bpp
				textureMode = 1;
			} else {
				printf("16bpp TIM detected\n");
				textureMode = 2;
			}
			timHeight = header.prect->h;
			LoadImage(header.prect, header.paddr);
			dumpRECT(header.prect);
			//printf("header width=%d",header.prect->w);
		}

		printf("timWidth=%d timHeight=%d \n",timWidth,timHeight);
	} else {
		printf("error reading tim!\n");
	}
	
	
	
	
	/*
	TIM is located at:
	header.prect.x
	header.prect.y
	header.prect.width
	header.prect.height
	*/

	cols = screenWidth / SPRITE_WIDTH;
	rows = screenHeight / SPRITE_HEIGHT;
	
	setupPrimitives(&db[0],cols,rows,SPRITE_WIDTH,SPRITE_HEIGHT,header.prect->x,header.prect->y,header.crect->x,header.crect->y,textureMode);	
	setupPrimitives(&db[1],cols,rows,SPRITE_WIDTH,SPRITE_HEIGHT,header.prect->x,header.prect->y,header.crect->x,header.crect->y,textureMode);	
	clutFadeInit(header.crect->x,header.crect->y, FadeInState);
	
	while(state != DoneState)
	{	
		int x,y;
		u_long *ot;
	
		cdb = (cdb==db)? db+1: db;	/* swap double buffer ID */
		ot = cdb->ot;
		PutDrawEnv(&cdb->draw); /* update drawing environment */
		PutDispEnv(&cdb->disp); /* update display environment */

		ClearOTag(ot,OTSIZE);

		for( i = 0, sprite = cdb->sprites, tpage = cdb->spriteTexturePages ; i < rows*cols ; i++ )
		{
			AddPrim(ot,sprite);
			AddPrim(ot,tpage);

			sprite+=1;
			tpage+=1;
			ot+=1;
		}

		ticks++;
		
		if( state == FadeInState )
		{
			if(!clutFade()){
				state = ShowPictureState;
				// Restore the original palette, just to be sure we have the correct colors when showing he final image.
				clutFadeRestore();
				// Setup the fade routine for fading down
				
			}
		}
		else if( state == ShowPictureState )
		{	
			if( ticks >= showPictureTicks )
			{
				state = FadeOutState;
				clutFadeInit(header.crect->x,header.crect->y, FadeDown);
				clutFade();
			}
		} 
		else if( state == FadeOutState )
		{
			if(!clutFade()){
				state = DoneState;				
			}
		} 

		DrawSync(0);
		DrawOTag(cdb->ot);	  /* draw */
		VSync(0);		

	}

	free(db[0].sprites);
	free(db[0].spriteTexturePages);
	free(db[1].sprites);
	free(db[1].spriteTexturePages);

}