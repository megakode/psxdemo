#include <sys/types.h>
#include <libetc.h>
#include <libgte.h>
#include <libgpu.h>
#include <libmath.h>
#include <Gtemac.h>
#include <Inline_c.h>
#include <libgs.h>
#include <libapi.h>

#define OTSIZE 10
#define numBars 6
#define screenWidth 320
#define screenHeight 256

static const int barHeight = screenHeight / numBars;


typedef struct {		
	DRAWENV		draw;			/* drawing environment */
	DISPENV		disp;			/* display environment */
	u_long		ot[OTSIZE];		/* ordering table */
	POLY_F4	*polys;	/* mesh cell */
} DB;

typedef struct
{
    int x;
    int xspeed;
} Bar;

Bar bars[numBars];


DB	db[2];		/* double buffer */
DB	*cdb;		/* current double buffer */
u_long	*ot;	/* current OT */

void setupDBs(DB *db)
{
    int i = 0;
		
	db->polys = (POLY_F4*)malloc(sizeof(POLY_F4)*numBars);

    for(i=0;i<numBars;i++)
    {
        setPolyF4( (db[0].polys+i) );
        setRGB0(db[0].polys+i,0,0,100);
    }

    // Set background clear color
	db->draw.isbg = 1;
    setRGB0(&db->draw, 0, 0,0);	
}

void doGeomFade()
{

    int done = 0;
    POLY_F4 *poly;
    int i;

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
	
	// Allocate polygon structs and initialize them
    
    setupDBs(&db[0]);
    setupDBs(&db[1]);

    for(i = 0 ; i < numBars ; i++ )
    {
        bars[i].x = 320 + i*80;
        bars[i].xspeed = 1;
        
    }

    while(!done)
    {
        cdb = (cdb==db)? db+1: db;	/* swap double buffer ID */
		ot = cdb->ot;
		PutDrawEnv(&cdb->draw); /* update drawing environment */
		PutDispEnv(&cdb->disp); /* update display environment */

		ClearOTag(ot,OTSIZE);

		for( i = 0, poly = cdb->polys ; i < numBars ; i++ )
		{
            Bar *bar = &bars[i];

            done = 1;

            if(bar->x > 0){ bar->x -= bar->xspeed; done = 0; } else { bar->x = 0; }
            if(bar->xspeed < 13)bar->xspeed++;
            setXYWH(poly,bar->x,i*barHeight,320,barHeight);
            setRGB0(poly,0,0,100);
			AddPrim(ot,poly);
			poly+=1;
			ot+=1;
		}

        DrawSync(0);
		DrawOTag(cdb->ot);	  /* draw */
		VSync(0);		
    }

}