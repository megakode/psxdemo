

#include "dsrlava.h" // Desire Lava logo
#include "font.h"
#include "dsrlib.h" 	// Desire lib

#define OTSIZE 10
#define NUM_STARS 400
#define LOGO_TEXTURE_X_POS 320
#define LOGO_TEXTURE_HEIGHT 256
#define TEXT_LINES_PR_PAGE 5
#define TEXT_LINES 35
#define TEXT_SPRITES 32*5

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

int textLineIndex = 0;

const char *textLines[TEXT_LINES] = {

"          Desire presents       ",
"       a small Playstation 1    ",
"              intro...          ",
"                                ",
"                                ",

"                                ",
"       Coding like its 1999     ",
"                                ",
"                                ",
"                                ",

"         Developed using:       ",
"         Win98 PC @ 133MHz      ",
"          SONY Psy-Q SDK        ",
"        X-Plorer cartridge      ",
"             Notepad++          ",

"              Code:             ",
"                                ",
"              sBeam             ",
"                                ",
"                                ",

"               GFX:             ",
"                                ",
"               Ozan             ",
"                                ",
"           VisionVortex         ",

"              Music:            ",
"                                ",
"             ........           ",
"                                ",
"                                ",

"          We <3 you all         ",
"                                ",
"       Keep the scene alive     ",
"                                ",
"                                "
};


const static int screenWidth = 320;
const static int screenHeight = 256;

const static int sinTable[] = {0,12,24,36,49,61,73,85,97,109,121,133,145,156,168,179,191,202,213,224,235,246,257,267,277,287,297,307,317,326,335,344,353,362,370,378,386,394,401,408,415,422,428,435,440,446,451,457,461,466,470,474,478,481,485,487,490,492,494,496,497,498,499,499,499,499,499,498,497,496,494,492,490,487,485,481,478,474,470,466,461,457,452,446,440,435,428,422,415,408,401,394,386,378,370,362,353,344,335,326,317,307,297,287,277,267,257,246,235,224,213,202,191,179,168,156,145,133,121,109,97,85,73,61,49,36,24,12,0,-12,-24,-36,-48,-61,-73,-85,-97,-109,-121,-133,-145,-156,-168,-179,-191,-202,-213,-224,-235,-246,-257,-267,-277,-287,-297,-307,-317,-326,-335,-344,-353,-362,-370,-378,-386,-394,-401,-408,-415,-422,-428,-435,-440,-446,-451,-457,-461,-466,-470,-474,-478,-481,-484,-487,-490,-492,-494,-496,-497,-498,-499,-499,-499,-499,-499,-498,-497,-496,-494,-492,-490,-487,-485,-481,-478,-474,-470,-466,-461,-457,-452,-446,-440,-435,-428,-422,-415,-408,-401,-394,-386,-378,-370,-362,-353,-344,-335,-326,-317,-307,-297,-287,-277,-267,-257,-246,-235,-224,-213,-202,-191,-180,-168,-156,-145,-133,-121,-109,-97,-85,-73,-61,-49,-36,-24,-24};

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
	unsigned char sinIndex;
	unsigned char sinIndex2 = 50; // random offset
	int textTicks = 0;
	SVECTOR *star;
	SVECTOR rot = {0,0,0};
	MATRIX	m = {4096,0,0,
				 0,4096,0,
				 0,0,4096,
				 0,0,0};
				
		/*
	for(i=0;i<255;i++){
		sinTable[i] = sin((2*3.1415 / 256) * i)*500;
		sinTable2[i] = sin((2*3.1415 / 256) * i)*200;
		//printf("%d,",sinTable2[i]);
	}
	*/
	
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
	
	//PadInit(0);
	
	//dumpMatrix(&m);
	
	SetGeomOffset(160, 128);
	SetGeomScreen(100);
	
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
		
		rot.vx = sinTable[sinIndex2]>>1;
		rot.vz = sinTable[sinIndex];
		//printf("sintable[%d] = %d \r\n",sinIndex+128,rot.vx);
		
		sinIndex++;
		sinIndex2++;
	
		
		RotMatrix_gte(&rot, &m);
		SetRotMatrix(&m);
		
		//pad = PadRead(0);
		
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
		
		textTicks += 1;
		
		if(textTicks<200){
		
			// Show current text page
			currentSpritePtr = cdb->textSprites;
		
			drawString((char*)textLines[textLineIndex],64,104,&currentSpritePtr,cdb->ot);
			drawString((char*)textLines[textLineIndex+1],64,114,&currentSpritePtr,cdb->ot);
			drawString((char*)textLines[textLineIndex+2],64,124,&currentSpritePtr,cdb->ot);
			drawString((char*)textLines[textLineIndex+3],64,134,&currentSpritePtr,cdb->ot);
			drawString((char*)textLines[textLineIndex+4],64,144,&currentSpritePtr,cdb->ot);
		
			AddPrim(cdb->ot,&cdb->textTPage); // Add texture page change as last item in current OT
		
		
		} else if(textTicks<230){
		
			// do nothing (show no text for a short period)
		
		} else {
		
			// Next text page
			textTicks = 0;
			textLineIndex += 5;
			if(textLineIndex>=TEXT_LINES){
				textLineIndex = 0;
			}
		}
		
		
	
		
		for( star = stars, i = 0; i < NUM_STARS ; i++ ,star++)
		{
			long p,flag,sz;
			u_long pad;
			
			star->vz -= 1;
			
			sz = RotTransPers(star,(long *)&cdb->tiles[i].x0,&p,&flag);

			colorIn.r = 255;
			colorIn.g = 255;
			colorIn.b = 255;
			DpqColor(&colorIn,p,&colorOut);
			cdb->tiles[i].r0 = colorOut.r;
			cdb->tiles[i].g0 = colorOut.g;
			cdb->tiles[i].b0 = colorOut.b;
			
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
	
	free(stars);
	free(db[0].tiles);
	free(db[1].tiles);
	

} 