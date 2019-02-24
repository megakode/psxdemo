#include <sys/types.h>
#include <libetc.h>
#include <libgte.h>
#include <libgpu.h>
#include <libmath.h>
#include <Gtemac.h>
#include <Inline_c.h>
#include <libgs.h>
#include <libapi.h>
#include "dsrlib.h" 	// Desire lib

typedef struct {		
	DRAWENV		draw;			/* drawing environment */
	DISPENV		disp;			/* display environment */
	u_long		ot[OTSIZE_WAR];		/* ordering table */
	POLY_F4	*cells;	/* mesh cell */
} DB;

DB	db[2];		/* double buffer */
DB	*cdb;		/* current double buffer */
u_long	*ot;	/* current OT */


int doScene()
{
	InitGeom();
	
	SetDispMask(0);
	
	ResetGraph(0);
	ResetCallback();
	PadInit(0);
	SetGraphDebug(0);
	
	InitGeom();
	SetGeomOffset(160, 128);
	SetGeomScreen(512);
	SetVideoMode(MODE_PAL);

	SetDispMask(1);
	
	const static int screenWidth = 320;
	const static int screenHeight = 256;
	
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
	
	while(1)
	{
		cdb = (cdb==db)? db+1: db;	/* swap double buffer ID */	
		ClearOTag(cdb->ot, OTSIZE_WAR);	/* clear ordering table */
		
		// add primitives here
		
		DrawSync(0);
		PutDrawEnv(&cdb->draw); /* update drawing environment */
		PutDispEnv(&cdb->disp); /* update display environment */
		DrawOTag(cdb->ot);	  /* draw */
	
	}
	

}