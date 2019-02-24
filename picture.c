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


typedef struct {		
	DRAWENV		draw;			/* drawing environment */
	DISPENV		disp;			/* display environment */
	u_long		ot[OTSIZE_WAR];		/* ordering table */
	POLY_F4	*cells;	/* mesh cell */
} DB;

DB	db[2];		/* double buffer */
DB	*cdb;		/* current double buffer */
u_long	*ot;		/* current OT */

int *cellShrinkAmount;


// *************************************************************
// Setup primitives
// *************************************************************

static void setupPrimitives( DB *db, int cols, int rows )
{	
	int x,y;
	POLY_F4 *poly;
	
	db->cells = (POLY_F4*)malloc(sizeof(POLY_F4)*cols*rows);
	
	poly = db->cells;
	
	for( y = 0 ; y < rows; y++ )
	for( x = 0 ; x < cols; x++)
	{
		int px = x<<4;
		int py = y<<4;
		
		SetPolyF4(poly);		/* FlatTexture Quadrangle */
			
		setRGB0(poly,0,0,0);
		setXY4(poly,
			   px,py,
			   px+16,py,
			   px,py+16,
			   px+16,py+16
			   );
				   
		poly++;
	}
	

}

// *************************************************************
// doPicture
// *************************************************************

void doPicture( u_long *tim , int screenWidth, int showPictureTicks)
{
	const int screenHeight = 256;
	int i,timWidth,timHeight,cols,rows;
	int clutId = 0;
	
	typedef enum {
		FadeIn,
		ShowPicture,
		FadeOut,
		Done
	} State;
	
	State state = FadeIn;
	int ticks = 0;
	int shrinkingLine = 0;
	int shrinkTrigger = 0;
	int shrinkAdvanceLineTrigger = 0;
	
	TIM_IMAGE	header;
	
	POLY_F4 *poly;

	InitGeom();
	
	ResetGraph(0);
	ResetCallback();
	PadInit(0);
	SetGraphDebug(0);
	
	InitGeom();
	//SetGeomOffset(160, 128);
	SetGeomOffset(0, 0);
	SetGeomScreen(1024);
	SetVideoMode(MODE_PAL);
	
	

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
	while (ReadTIM(&header)) 
	{
		if (header.caddr) {	/* load CLUT (if needed) */
//			setSTP(image.caddr, image.crect->w);
			LoadImage(header.crect, header.caddr);
			//clutId = GetClut(header.crect->x,header.crect->y);
			clutId = GetClut(header.crect->x,header.crect->y);
		}
		if (header.paddr){ 	/* load texture pattern */
			timWidth = header.prect->w;
			printf("pixel mode=%d\n",header.mode);
			//header.mode = 1;
			if(header.mode==8){
				printf("4bpp TIM detected\n");
				timWidth*=4; // 4bpp (as the prect->width says how many 16bpp pixels in the VRAM that the tim file occupies
			}else if(header.mode==9){
				printf("8bpp TIM detected\n");
				timWidth*=2; // 8bpp
			} else {
				printf("16bpp TIM detected\n");
			}
			timHeight = header.prect->h;
			LoadImage(header.prect, header.paddr);
			dumpRECT(header.prect);
			//printf("header width=%d",header.prect->w);
		}
	}
	
	
	printf("timWidth=%d timHeight=%d",timWidth,timHeight);
	
	/*
	TIM is located at:
	header.prect.x
	header.prect.y
	header.prect.width
	header.prect.height
	*/

	cols = screenWidth / 16;
	rows = screenHeight / 16;
	
	setupPrimitives(&db[0],cols,rows);	/* initialize primitive buffers #0 */
	setupPrimitives(&db[1],cols,rows);	/* initialize primitive buffers #1 */
	
	
	cellShrinkAmount = (int*)malloc(sizeof(int)*rows);
	
	for(i = 0 ; i < rows ; i++)
	{
		cellShrinkAmount[i] = 0;
	}
	
	
	while(state != Done)
	{	
		int x,y;
	
		cdb = (cdb==db)? db+1: db;	/* swap double buffer ID */
		
		PutDrawEnv(&cdb->draw); /* update drawing environment */
		PutDispEnv(&cdb->disp); /* update display environment */
			
		ClearOTag(cdb->ot, OTSIZE_WAR);	/* clear ordering table */
		
		MoveImage(header.prect,32,cdb->draw.clip.y+16);
		ticks++;
		
		if( state == FadeIn )
		{

			// Fade in (line style)
			
			if(cellShrinkAmount[rows-1] < MAX_SHRINK_AMOUNT)
			{	
				shrinkTrigger++;
				if(shrinkTrigger >= 3)
				{
					shrinkTrigger = 0;
					
					// Start shrinking of next line?
					shrinkAdvanceLineTrigger++;
					if(shrinkAdvanceLineTrigger == 3 && shrinkingLine < rows-1)
					{
						shrinkAdvanceLineTrigger = 0;
						shrinkingLine++;
					}
				
					// Shrink all the lines that are above 'shrinkingLine'
					for(i = 0; i <= shrinkingLine ; i++)
					{
						if(cellShrinkAmount[i] < MAX_SHRINK_AMOUNT)
						{
							cellShrinkAmount[i]++;
						}
					}
					
				}
			
			} else 
			{
				shrinkingLine = 0;
				state = ShowPicture;	
			}
		
		} else if( state == ShowPicture )
		{
			
			// Show picture
			
			if( ticks >= showPictureTicks )
			{
				shrinkingLine = 0;
				state = FadeOut;
			}
		
		} else if( state == FadeOut )
		{
			if(cellShrinkAmount[rows-1] > 0)
			{	
				shrinkTrigger++;
				if(shrinkTrigger >= 3)
				{
					shrinkTrigger = 0;
					
					// Start shrinking of next line?
					shrinkAdvanceLineTrigger++;
					if(shrinkAdvanceLineTrigger >= 3 && shrinkingLine < rows-1)
					{
						shrinkAdvanceLineTrigger = 0;
						shrinkingLine++;
					}
				
					// Shrink all the lines that are above 'shrinkingLine'
					for(i = 0; i <= shrinkingLine ; i++)
					{
						if(cellShrinkAmount[i] > 0)
						{
							cellShrinkAmount[i]--;
						}
					}
					
				}
			
			} else 
			{
				
				state = Done;	
			}
		
		} 
		else if (state == Done )
		{
			//printf("state=Done");
		}
		
		poly = cdb->cells;
		
		for( y = 0 ; y < rows; y++ )
		for( x = 0 ; x < cols; x++ , poly++)
		{
			int px = x<<4;
			int py = y<<4;

			setXY4(poly,
				   px+cellShrinkAmount[y],py+cellShrinkAmount[y],
				   px+16-cellShrinkAmount[y],py+cellShrinkAmount[y],
				   px+cellShrinkAmount[y],py+16-cellShrinkAmount[y],
				   px+16-cellShrinkAmount[y],py+16-cellShrinkAmount[y]
				   );
				   
			AddPrim(cdb->ot+OTSIZE_WAR-1,poly);
					   
		}
		
		
		DrawSync(0);		/* wait for end of drawing */
		
		DrawOTag(cdb->ot);	  /* draw */
		
		VSync(0);		/* wait for the next V-BLNK */


	}

	free(db[0].cells);
	free(db[1].cells);
	free(cellShrinkAmount);

}