#include <sys/types.h>
#include <libetc.h>
#include <libgte.h>
#include <libgpu.h>
#include <libmath.h>
#include <Gtemac.h>
#include <Inline_c.h>
#include <libgs.h>

#include <libapi.h>

#include "font.h"
#include "dsrlib.h" 	// Desire lib
//#include "land.h" 		// Land fly-over intro scene
//#include "cubescroll.h"	// Cube scroller
//#include "picture.h"	// show picture

//#include "war320.h"		// GFX: war of the worlds picture data
#include "dsrlava.h"
//#include "dsrpsx.h" 		// GFX: Desire PSX logo

#define OTSIZE 10
typedef struct {		
	DRAWENV		draw;			/* drawing environment */
	DISPENV		disp;			/* display environment */
	u_long		ot[OTSIZE];		/* ordering table */
	TILE_1		*tiles;
	DR_TPAGE	logoTexturePage;
	SPRT		logoSpr;
	
	DR_TPAGE	textTPage;
	SPRT_8 		textSprites[TEXT_SPRITES];
	
} DB;

DB	db[2];		/* double buffer */
DB	*cdb;		/* current double buffer */
u_long	*ot;	/* current OT */

SVECTOR *stars;
#define NUM_STARS 400
#define LOGO_TEXTURE_X_POS 320
#define LOGO_TEXTURE_HEIGHT 256

int sinTable[256];
int sinTable2[256];
const static int screenWidth = 320;
const static int screenHeight = 256;

void setupBuffer( DB *buffer )
{
	int i;
	
	// PAL setup
	buffer->disp.screen.x = 1;
	buffer->disp.screen.y = 18;
	buffer->disp.screen.h = screenHeight;
	buffer->disp.screen.w = screenWidth;
	
	// Set background clear color
	buffer->draw.isbg = 1;
	setRGB0(&buffer->draw, 0, 0, 0);
	
	SetDrawTPage(&buffer->textTPage,0,0,GetTPage(0,0,FONT_X_OFFSET,0)); // 4Bit CLUT
	for(i=0;i<TEXT_SPRITES;i++)
	{
		SetSprt8(&buffer->textSprites[i]);
		//setClut(&buffer->textSprites[i],640,32);
		buffer->textSprites[i].clut = GetClut(640,32);
		SetShadeTex(&buffer->textSprites[i], 1); // Set texture shading to off
	}	
	
	
	// Setup DSR lava logo
	SetDrawTPage(&buffer->logoTexturePage,0,0,GetTPage(2,0,LOGO_TEXTURE_X_POS,0)); // 256=LOGO_TEXTURE_X_POS
	//buffer->draw.tpage = GetTPage(2,0,LOGO_TEXTURE_X_POS,0);
	SetSprt(&buffer->logoSpr);
	setUV0(&buffer->logoSpr, 0, 0);
	setWH(&buffer->logoSpr,64,LOGO_TEXTURE_HEIGHT);
	setXY0(&buffer->logoSpr,0,0);
	SetShadeTex(&buffer->logoSpr, 1); // Set texture shading to off
	//buffer->logoSpr.clut = GetClut(320,0);
}

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
	
	setupBuffer(&db[0]);
	setupBuffer(&db[1]);

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
	
	/*
	SetPolyFT4(&db[0].logoPoly);
	SetPolyFT4(&db[1].logoPoly);
	setUV0(&db[0].logoPoly,0,0);
	setUV0(&db[1].logoPoly,0,0);
	
	setXY4(&db[0].logoPoly,
			   0,0,
			   0+32,0,
			   0,0+255,
			   0+32,0+255
			   );
	setXY4(&db[1].logoPoly,
			   0,0,
			   0+32,0,
			   0,0+255,
			   0+32,0+255
			   );
	*/
	RotMatrix_gte(&rot, &m);
	
	SetTransMatrix(&m);
	
	PadInit(0);
	
	//dumpMatrix(&m);
	
	SetFogNearFar(near,far,512); // 512= Screen z
	
	//printf("loading tim...\r\n");
	// Setup logo
	loadTIM((unsigned char*)&dsrlava);
	loadTIM((unsigned char*)&font);
	
	//printf("done");
	
	while(1)
	{
		CVECTOR colorIn,colorOut;
		SPRT_8 *currentSpritePtr;
		
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
		
	
		AddPrim(cdb->ot,&cdb->logoSpr);
		AddPrim(cdb->ot,&cdb->logoTexturePage); // Add texture page change as last item in current OT entry (which is actually run first)
		
				//Draw text
		
		currentSpritePtr = cdb->textSprites;
		
		drawString("       DSR Rocking the PSX      ",64,104,&currentSpritePtr,cdb->ot);
		drawString("       Coding like its 1999     ",64,114,&currentSpritePtr,cdb->ot);
		drawString("                                ",64,124,&currentSpritePtr,cdb->ot);
		drawString("           Code: sBeam          ",64,134,&currentSpritePtr,cdb->ot);
		drawString("            GFX: ozan           ",64,144,&currentSpritePtr,cdb->ot);
		
		AddPrim(cdb->ot,&cdb->textTPage); // Add texture page change as last item in current OT
	
		
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
			
			if( (flag & 1<<17) ) // Bit that indicates whether in front of the near clipping plane
			{
				// Teleport the star back on the screen at some random x/y location 
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