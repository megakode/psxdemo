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
#include "land.h" 		// Land fly-over intro scene
#include "cubescroll.h"	// Cube scroller
#include "picture.h"	// show picture

// #include "war320.h"		// GFX: war of the worlds picture data
#include "dsrpsx.h" 		// GFX: Desire PSX logo

#define OTSIZE 10
typedef struct {		
	DRAWENV		draw;			/* drawing environment */
	DISPENV		disp;			/* display environment */
	u_long		ot[OTSIZE];		/* ordering table */
	TILE_1		*tiles;
} DB;

DB	db[2];		/* double buffer */
DB	*cdb;		/* current double buffer */
u_long	*ot;	/* current OT */

SVECTOR *stars;
#define NUM_STARS 400

int sinTable[256];
int sinTable2[256];

void doStars()
{
	int starsNearClip = 50;
	int near = 250;
	int far = 500;
	u_long pad;
	int max = 0;
	int i;
	int sinIndex;
	SVECTOR *star;
	SVECTOR rot = {0,0,0};
	const static int screenWidth = 320;
	const static int screenHeight = 256;
	MATRIX	m = {4096,0,0,
				 0,4096,0,
				 0,0,4096,
				 0,0,0};
				 
	for(i=0;i<255;i++){
		sinTable[i] = sin((2*3.1415 / 256) * i)*500;
		sinTable2[i] = sin((2*3.1415 / 256) * i)*200;
		//printf("%d,",sinTable[i]);
	}
	
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

	stars = (SVECTOR*) malloc(sizeof(SVECTOR)*NUM_STARS);
	db[0].tiles = (TILE_1*) malloc(sizeof(TILE_1)*NUM_STARS);
	db[1].tiles = (TILE_1*) malloc(sizeof(TILE_1)*NUM_STARS);
	star = stars;
	
	for( i = 0; i < NUM_STARS ; i++ , star++ )
	{
		star->vx = (rand() % 256) - 128;
		star->vy = (rand() % 256) - 128;
		star->vz = (rand() % (128-starsNearClip)) + starsNearClip;
		
		setTile1(&db[0].tiles[i]);
		setTile1(&db[1].tiles[i]);
		db[0].tiles[i].r0 = 250;
		db[0].tiles[i].g0 = 250;
		db[0].tiles[i].b0 = 250;
		db[1].tiles[i].r0 = 250;
		db[1].tiles[i].g0 = 250;
		db[1].tiles[i].b0 = 250;
		
	}
	
	
	RotMatrix_gte(&rot, &m);
	
	SetTransMatrix(&m);
	
	PadInit(0);
	
	//dumpMatrix(&m);
	
	SetFogNearFar(near,far,PROJ_Z);
	
	while(1)
	{
		CVECTOR colorIn,colorOut;
		
		cdb = (cdb==db)? db+1: db;	/* swap double buffer ID */	
		ClearOTag(cdb->ot, OTSIZE);	/* clear ordering table */
		
		//rot.vx+=10;
		//rot.vy+=3;
		rot.vz = sinTable[sinIndex];
		//rot.vy = sinTable2[sinIndex];
		rot.vx = sinTable2[sinIndex]>>1;
		
		sinIndex++;
		if(sinIndex>=256){
			sinIndex=0;
		}
		
		RotMatrix_gte(&rot, &m);
		SetRotMatrix(&m);
		
		pad = PadRead(0);
		
		//if( pad & PADLleft) doFadeClut(320,128);
		/*
		if (pad & PADLup){	near += 10;SetFogNearFar(near,far,PROJ_Z); ; printf("near=%u\n",near); };
		if (pad & PADLdown){ near -= 10; SetFogNearFar(near,far,PROJ_Z);printf("near=%u\n",near); };
		if (pad & PADRup){	far += 10; SetFogNearFar(near,far,PROJ_Z);printf("far=%u\n",far); }
		if (pad & PADRdown){	far -=10; SetFogNearFar(near,far,PROJ_Z);printf("far=%u\n",far); }
		
		if (pad & PADLup){	starsNearClip += 1; printf("starsNearClip=%u\n",starsNearClip); };
		if (pad & PADLdown){ starsNearClip -= 1; printf("starsNearClip=%u\n",starsNearClip); };
		*/
		for( star = stars, i = 0; i < NUM_STARS ; i++ ,star++)
		{
			long p,flag,sz;
			u_long pad;
			
			star->vz -= 1;
			
			//if(star->vz < 60 )
			{/*
				if( star->vx <= 0 ){
					star->vx--;
				} else {
					star->vx++;
				}
				*/
				/*
				if( star->vy <= 0 ){
					star->vy--;
				} else {
					star->vy++;
				}
				*/
			}
			/*
			if( star->vz <= starsNearClip ){
				star->vz = 128;
				star->vx = (rand() % 256) - 128;
				star->vy = (rand() % 256) - 128;
			}
			*/
			
			
			
			sz = RotTransPers(star,(long *)&cdb->tiles[i].x0,&p,&flag);
			/*
			if(sz>max){
				max = sz;
				printf("sz=%d\n",sz);
			}
			
			color = (75-sz)*5;
			*/
			colorIn.r = 255;
			colorIn.g = 255;
			colorIn.b = 255;
			DpqColor(&colorIn,p,&colorOut);
			cdb->tiles[i].r0 = colorOut.r;
			cdb->tiles[i].g0 = colorOut.g;
			cdb->tiles[i].b0 = colorOut.b;
			
			//cdb->tiles[i].r0 = cdb->tiles[i].g0 = cdb->tiles[i].b0 = (color & 0xff);
			
			//printf("star.z=%d sz=%d\n",star->vz, sz);
			
			if( (flag & 1<<17) )
			{
				star->vz = 128;
				star->vx = (rand() % 256) - 128;
				star->vy = (rand() % 256) - 128;	
			} else {
				AddPrim(cdb->ot, &cdb->tiles[i]);
			}
		}
	
		
		DrawSync(0);
		VSync(0);
		PutDrawEnv(&cdb->draw); /* update drawing environment */
		PutDispEnv(&cdb->disp); /* update display environment */
		DrawOTag(cdb->ot);	  /* draw */
	
	}
	

}

int main()
{
InitGeom();
	
	SetDispMask(0);
	
	ResetGraph(0);
	ResetCallback();
	PadInit(0);
	SetGraphDebug(0);
	
	InitGeom();
	SetGeomOffset(160, 128);
	SetGeomScreen(100);
	SetVideoMode(MODE_PAL);

	SetDispMask(1);
	
	SetFarColor(10,10,10);

	doStars();
	//doPicture((u_long*)dsrpsx,256,200);
	//doCubes();
	//doLandscape();
}