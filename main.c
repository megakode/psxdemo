#include <sys/types.h>
#include <libetc.h>
#include <libgte.h>
#include <libgpu.h>
#include <libmath.h>
#include <Gtemac.h>
#include <Inline_c.h>
#include <libgs.h>

#include "dsrlib.h" // Desire lib

#include "Sinsweep.h"

#include "land.h"

#include "cubes.h"

#include "war320.h"


#define OTSIZE_WAR 100
#define CELL_ROWS 12
#define CELL_COLS 20

#define PT_UX	16			/* cell size (in UV space) */
#define PT_UY	16

#define MAX_SHRINK_AMOUNT 8

typedef struct {		
	DRAWENV		draw;			/* drawing environment */
	DISPENV		disp;			/* display environment */
	u_long		ot[OTSIZE_WAR];		/* ordering table */
	POLY_FT4	cell[CELL_ROWS*CELL_COLS];	/* mesh cell */
} DB;

DB	db[2];		/* double buffer */
DB	*cdb;		/* current double buffer */
u_long	*ot;		/* current OT */

int cellShrinkAmount[CELL_ROWS*CELL_COLS];

// *************************************************************
// Setup primitives
// *************************************************************

static void setupPrimitives( DB *db )
{	
	int x,y,ux,uy;
	POLY_FT4 *poly;
	u_short	tpage,tpageLeft, tpageRight;	
	
	poly = db->cell;
	tpageLeft = GetTPage(2, 0, 320,     0);	// 2 = 16bpp mode
	tpageRight = GetTPage(2, 0, 320+256, 0); 
	tpage = tpageLeft;
	
	ux = 0; uy = 0;
	
	for( y = 0 ; y < CELL_ROWS; y++, uy += PT_UY )
	for( x = 0 , ux = 0; x < CELL_COLS; x++, ux += PT_UX )
	{
		int px = x<<4;
		int py = y<<4;
		

		tpage = ux > 255 ? tpageRight : tpageLeft;
		
		
		SetPolyFT4(poly);		/* FlatTexture Quadrangle */
		SetShadeTex(poly, 1);	/* no shade-texture */
			
		//setRGB0(poly,128,128,128);
		setXY4(poly,
			   px,py,
			   px+16,py,
			   px,py+16,
			   px+16,py+16
			   );
				   
		setUV4(poly, ux, uy, 
					 ux+PT_UX-1, uy,
					 ux, uy+PT_UY-1, 
					 ux+PT_UX-1, uy+PT_UY-1);
					 
		
			
		poly->tpage = tpage;
	
		poly++;
	}
	

}

// *************************************************************
//   Main
// *************************************************************

int main(){

	int i;
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
	
	POLY_FT4 *poly;

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
	SetDefDrawEnv(&db[0].draw, 0,   0, 320, 256);
	SetDefDrawEnv(&db[1].draw, 0, 256, 320, 256);
	SetDefDispEnv(&db[0].disp, 0, 256, 320, 256);
	SetDefDispEnv(&db[1].disp, 0,   0, 320, 256);
	
	
	// PAL setup
	db[1].disp.screen.x = db[0].disp.screen.x = 1;
	db[1].disp.screen.y = db[0].disp.screen.y = 18;
	db[1].disp.screen.h = db[0].disp.screen.h = 256;
	db[1].disp.screen.w = db[0].disp.screen.w = 320;
	
	// Set background clear color
	db[0].draw.isbg = 1;
	db[1].draw.isbg = 1;
	setRGB0(&db[0].draw, 0, 0, 0);
	setRGB0(&db[1].draw, 0, 0, 0);
	
	SetDispMask(1);
	
	setupPrimitives(&db[0]);	/* initialize primitive buffers #0 */
	setupPrimitives(&db[1]);	/* initialize primitive buffers #1 */
	

	loadTIM(&war320);
	
	// *****
	//
	// Kig sample/graphics/morph.h for inspiration
	//
	// ******
	
	for(i = 0 ; i < CELL_ROWS*CELL_COLS ; i++)
	{
		cellShrinkAmount[i] = MAX_SHRINK_AMOUNT;
	}
	
	
	while(1)
	{
		int x,y;
		SVECTOR debugVector;
	
		cdb = (cdb==db)? db+1: db;	/* swap double buffer ID */
		
		PutDrawEnv(&cdb->draw); /* update drawing environment */
		PutDispEnv(&cdb->disp); /* update display environment */
		
		
		ClearOTag(cdb->ot, OTSIZE_WAR);	/* clear ordering table */
		
		ticks++;
		
		if( state == FadeIn )
		{
			if(cellShrinkAmount[CELL_ROWS-1] > 0)
			{	
				shrinkTrigger++;
				if(shrinkTrigger == 3)
				{
					shrinkTrigger = 0;
					
					// Start shrinking of next line?
					shrinkAdvanceLineTrigger++;
					if(shrinkAdvanceLineTrigger == 3 && shrinkingLine < CELL_COLS-2)
					{
						shrinkAdvanceLineTrigger = 0;
						shrinkingLine++;
					}
				
					// Shrink all the lines that are above 'shrinkingLine'
					for(i = 0; i < shrinkingLine ; i++)
					{
						if(cellShrinkAmount[i] > 0)
						{
							cellShrinkAmount[i]--;
						}
					}
					
				}
			
			} else 
			{
				shrinkingLine = 0;
				state = ShowPicture;	
			}
		
		} 
		else if( state == ShowPicture )
		{
			// Show picture
			
			if( ticks == 500 )
			{
				state = FadeOut;
			}
		
		} else if( state == FadeOut )
		{
			// Fade out (line style)
			
			if(cellShrinkAmount[CELL_ROWS-1] < MAX_SHRINK_AMOUNT)
			{	
				shrinkTrigger++;
				if(shrinkTrigger == 3)
				{
					shrinkTrigger = 0;
					
					// Start shrinking of next line?
					shrinkAdvanceLineTrigger++;
					if(shrinkAdvanceLineTrigger == 3 && shrinkingLine < CELL_COLS-2)
					{
						shrinkAdvanceLineTrigger = 0;
						shrinkingLine++;
					}
				
					// Shrink all the lines that are above 'shrinkingLine'
					for(i = 0; i < shrinkingLine ; i++)
					{
						if(cellShrinkAmount[i] < MAX_SHRINK_AMOUNT)
						{
							cellShrinkAmount[i]++;
						}
					}
					
				}
			
			} else 
			{
				state = Done;	
			}
		
		}
		
		
		
		/*
		debugVector.vx = shrinkingLine;
		debugVector.vy = cellShrinkAmount[CELL_ROWS-1];
		debugVector.vz = 1;
		dumpVector("debugVector=",&debugVector);
		*/
		
		/* draw */
		poly = cdb->cell;
		
		for( y = 0 ; y < CELL_ROWS; y++ )
		for( x = 0 ; x < CELL_COLS; x++ , poly++)
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

	//doCubes();
	//doLandscape();
}