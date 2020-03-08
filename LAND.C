#include <sys/types.h>
#include <libetc.h>
#include <libgte.h>
#include <libgpu.h>
#include "dsrlib.h"
#include "landlogo.h" // TIM dsrlogo.png
#include "font.h" // TIM font.png

#define LAND_NUM_HORIZ_CELLS 25
#define LAND_NUM_VERT_CELLS 25

#define xoffset 0
#define yoffset 0 
#define LAND_CELL_WIDTH 50
#define LAND_CELL_DEPTH 100
#define MAX_LAND_HEIGHT 50

#define PERLIN_MULT -200
#define PERLIN_FREQ 0.2
#define PERLIN_DEPTH 6

#define	LAND_OTSIZE	(4096)
#define PROJ_Z (512)	/* screen depth */
#define TEXT_SPRITES 300

#include "perlin.h"

SVECTOR cellNormals[LAND_NUM_VERT_CELLS][LAND_NUM_HORIZ_CELLS*2];
SVECTOR cells[LAND_NUM_VERT_CELLS][LAND_NUM_HORIZ_CELLS];

SVECTOR	landRotVec  = {0,0,0};
VECTOR	landPosVec  = {-(LAND_CELL_WIDTH*LAND_NUM_HORIZ_CELLS/2),250,0,0};

typedef struct {
	DRAWENV	draw;		/* drawing environment */
	DISPENV	disp;		/* display environment */
	u_long	ot[LAND_OTSIZE];	/* Ordering Table */
	
	SPRT logoLeftSprite;
	SPRT logoRightSprite;
	
	DR_TPAGE logoLeftTPage; // drawing mode primitive (used only for setting texture page)
	DR_TPAGE logoRightTPage;
	DR_TPAGE textTPage;
	SPRT_8 textSprites[TEXT_SPRITES];
	
	POLY_F3 landPolys[LAND_NUM_HORIZ_CELLS*LAND_NUM_VERT_CELLS*2]; // *2 because each "cell" is two POLY_F3 triangles
	
} LandscapeScene;


// ***************************************************
//	initLandscapePrimitives
//
//	Initializes all the primitive structs
// ***************************************************

#define LAND_LOGO_TEXTURE_X_POS 320
#define LAND_LOGO_TEXTURE_HEIGHT 128

void initLandscapePrimitives(LandscapeScene *buffer)
{
		
	int i = 0;
	
	buffer->draw.isbg = 1;
	setRGB0(&buffer->draw, 0, 0, 0); // 60, 120, 120
	
	// Init text sprites
	
	SetDrawTPage(&buffer->textTPage,0,0,GetTPage(0,0,FONT_X_OFFSET,0)); // 4Bit CLUT
	
	for(i=0;i<TEXT_SPRITES;i++)
	{
		SetSprt8(&buffer->textSprites[i]);
		//setClut(&buffer->textSprites[i],640,32);
		buffer->textSprites[i].clut = GetClut(640,32);
		SetShadeTex(&buffer->textSprites[i], 1); // Set texture shading to off
	}	
	
	// Init logo sprite
	
	SetDrawTPage(&buffer->logoLeftTPage,0,0,GetTPage(1,0,LAND_LOGO_TEXTURE_X_POS,0));
	SetSprt(&buffer->logoLeftSprite);
	setUV0(&buffer->logoLeftSprite, 0, 0);
	setWH(&buffer->logoLeftSprite,256,LAND_LOGO_TEXTURE_HEIGHT);
	setXY0(&buffer->logoLeftSprite,0,0);
	SetShadeTex(&buffer->logoLeftSprite, 1); // Set texture shading to off
	buffer->logoLeftSprite.clut = GetClut(LAND_LOGO_TEXTURE_X_POS,LAND_LOGO_TEXTURE_HEIGHT);
	
	SetDrawTPage(&buffer->logoRightTPage,0,0,GetTPage(1,0,LAND_LOGO_TEXTURE_X_POS+160,0));
	SetSprt(&buffer->logoRightSprite);
	setUV0(&buffer->logoRightSprite, 0, 0);
	setWH(&buffer->logoRightSprite,64,LAND_LOGO_TEXTURE_HEIGHT);
	setXY0(&buffer->logoRightSprite,256,0);
	SetShadeTex(&buffer->logoRightSprite, 1); // Set texture shading to off
	buffer->logoRightSprite.clut = GetClut(LAND_LOGO_TEXTURE_X_POS,LAND_LOGO_TEXTURE_HEIGHT);
	
	for( i = 0 ; i < LAND_NUM_HORIZ_CELLS*LAND_NUM_VERT_CELLS*2 ; i++)
	{
		SetPolyF3(&buffer->landPolys[i]);
		
	}
	
}


// ************************************************
// ************************************************

void moveLand(int x,int y,int z)
{
	landPosVec.vx += x;
	landPosVec.vy += y;
	landPosVec.vz += z;
}

// ************************************************
//
// Swing land
//
// ************************************************

#define PI 2*3.1415f
void swing()
{
	static double angZ = 0;
	static double angY = 0;
	
	// Swing light left and right
		
	//landRotVec.vz = (int) (cos(angZ)*100);
	//landPosVec.vy = 250 + (int)(cos(angY)*10);
	angZ+=0.02;
	angY+=0.025;
	
	if(angY >= PI){
		angY = 0;
	}
	if(angZ >= PI){
		angZ = 0;
	}

}

// ************************************************
//
// Move land rows
//
// move all land data and normals one row down in the multidimensional 'cells' array
// and insert a newly generated row using perlin noise.
//
// ************************************************

void moveLandRows()
{
	int y,x;
	static int rowoffs = 0;
	
	rowoffs++;
	
	
	// Move rows down
	
	for( y = 0; y < LAND_NUM_VERT_CELLS-1; y++)
	{
		for( x = 0 ; x < LAND_NUM_HORIZ_CELLS; x++ )
		{
			cells[y][x].vy = cells[y+1][x].vy;
		}
		
		// Move normals (there are twice as many normals, as each cell is made of two triangles, which have each their normal
		for( x = 0 ; x < LAND_NUM_HORIZ_CELLS<<1; x++ )
		{
			cellNormals[y][x] = cellNormals[y+1][x];
		}
	}
	
	// Generate new row
	for( x = 0 ; x < LAND_NUM_HORIZ_CELLS; x++ )
	{
		y = LAND_NUM_VERT_CELLS-1;
		cells[y][x].vy = (int)(PERLIN_MULT*perlin2d(x, y+rowoffs, PERLIN_FREQ, PERLIN_DEPTH));
		
	}
	
	for( x = 0 ; x < LAND_NUM_HORIZ_CELLS*2; x++){
		// TODO: convert to memset
		cellNormals[y][x].vy = 0; // Indicate that we did not calculate this normal yet
	}
}

// ************************************************
//
// Calculate normal vector based on triangle v0,v1,v2
//
// ************************************************

inline SVECTOR calcNormal(SVECTOR *v0,SVECTOR *v1,SVECTOR *v2)
{
	SVECTOR a,b,n;
	SVECTOR normalized;
	// Calc normal vector ************

	a.vx = v1->vx - v0->vx;
	a.vy = v1->vy - v0->vy;
	a.vz = v1->vz - v0->vz;

	b.vx = v2->vx - v0->vx;
	b.vy = v2->vy - v0->vy;
	b.vz = v2->vz - v0->vz;

	// Cross product

	n.vx = (a.vy * b.vz - a.vz*b.vy);
	n.vy = (a.vz * b.vx - a.vx*b.vz);
	n.vz = (a.vx * b.vy - a.vy*b.vx);
	
	
	
	VectorNormalSS(&n,&normalized);

	return normalized;
}

// ************************************************
//
// addLandCell
//
// ************************************************

void addLandCell(u_long *ot, POLY_F3 *poly,SVECTOR *v0,SVECTOR *v1,SVECTOR *v2,SVECTOR *n)
{
	CVECTOR colorIn,colorOut;
	int	isomote;
	long	p, otz, opz, flg;

	isomote = RotAverageNclip3(v0, v1, v2, 
		(long *)&poly->x0, (long *)&poly->x1,(long *)&poly->x2, 
		&p, &otz, &flg);

	if (isomote <= 0) return;

	/* Put into OT:
	 * The length of the OT is assumed to 4096. */
	if (otz > 0 && otz < 4096) {	
		// just use a hardcoded unlit color for the surface
		colorIn.r = 128;
		colorIn.g = 32;
		colorIn.b = 0;
		
		// Get local light value..
		NormalColorDpq(n, &colorIn,p,&colorOut);
		
		// Lets try some experimental fast fake light
		/*
		if(v1->vy > v0->vy){
			short forskel = v1->vy - v0->vy;
			poly->r0 = forskel * 10;
			poly->g0 = colorIn.g >> 2;
			poly->b0 = colorIn.b >> 2; 
		
		} else {
			// Dark
			poly->r0 = colorIn.r >> 2;
			poly->g0 = colorIn.g >> 2;
			poly->b0 = colorIn.b >> 2; 
		}
		*/
		// and apply it to surface
		
		poly->r0 = colorOut.r;
		poly->g0 = colorOut.g;
		poly->b0 = colorOut.b;
		
		AddPrim(ot+otz, poly);
	}

}


// ************************************************
//
//	addLandPolys
//
// ************************************************

void addLandPolys(u_long *ot,POLY_F3 *polyList)
{
	int x,y;
	
	MATRIX rottrans;
		
	PushMatrix();
	
	RotMatrix_gte(&landRotVec, &rottrans); // Calculate rotation matrix from vector
	TransMatrix(&rottrans,&landPosVec);
	
	SetRotMatrix(&rottrans);	
	SetTransMatrix(&rottrans);

	for( y=LAND_NUM_VERT_CELLS-2; y>0; y--)
	{
		for( x=0; x<LAND_NUM_HORIZ_CELLS-1; x++)
		{
			// Clockwise
		
			SVECTOR *v0 = &cells[y][x];
			SVECTOR *v1 = &cells[y][x+1];
			SVECTOR *v2 = &cells[y-1][x];

			SVECTOR *v3 = &cells[y-1][x];
			SVECTOR *v4 = &cells[y][x+1];
			SVECTOR *v5 = &cells[y-1][x+1];
						
			SVECTOR n1; 
			SVECTOR n2;
			
			n1 = cellNormals[y][x<<1];
			
			if(n1.vy==0){
				n1 = calcNormal(v0,v1,v2);
				cellNormals[y][x<<1] = n1;
			}
			
			n2 = cellNormals[y][(x<<1) + 1];
			
			if(n2.vy==0){
				n2 = calcNormal(v3,v4,v5);
				cellNormals[y][(x<<1) + 1] = n2;
			}
		
			addLandCell(ot,polyList,v0,v1,v2,&n1);
			polyList++;
			
			
			addLandCell(ot,polyList,v3,v4,v5,&n2);
			polyList++;
			
		}
		
	}
	
	PopMatrix();

}

// ************************************************
// initLandscapeCells
// ************************************************

void initLandscapeCells()
{
	int x,y;
	
	for( y=0; y<LAND_NUM_VERT_CELLS; y++)
	{
		for( x=0; x<LAND_NUM_HORIZ_CELLS; x++)
		{
			cells[y][x].vx = x*LAND_CELL_WIDTH;
			cells[y][x].vz = y*LAND_CELL_DEPTH;
			cells[y][x].vy = (int)(PERLIN_MULT*perlin2d(x, y, PERLIN_FREQ, PERLIN_DEPTH));
		}
	}
	

}

// ************************************************
// Do landscape scene
// ************************************************

void doLandscape()
{

	VECTOR debugVector;
	u_long pad;
	
	LandscapeScene* landscapeSceneBuffers[2]; // Double buffers
	LandscapeScene	*cdb; // Current double buffer

	/* Local Light Matrix */
	MATRIX	llm = {3000,1000,0,
					   0,0,3000,
					   0,0,0,
					   0,0,0};

	/* Local Color Matrix */
	MATRIX	lcm = {4095,2000,0, 
					   4095,2000,0, 
					   4095,4000,0, 
					      0,0,0};

	
	int near = 600;
	int far = 4000;
	int landMoveCounter = 0;
	int ticks = 0;
	SPRT_8 *currentSpritePtr;
/*
	ResetGraph(0);
	ResetCallback();
	PadInit(0);
	
	SetGraphDebug(0);
	
	InitGeom();
	*/
	SetGeomOffset(160, 128);	
	SetGeomScreen(PROJ_Z);	
	/*
	SetVideoMode(MODE_PAL);
	SetDispMask(1);
	*/
	
	landscapeSceneBuffers[0] = (LandscapeScene*)malloc(sizeof(LandscapeScene));
	landscapeSceneBuffers[1] = (LandscapeScene*)malloc(sizeof(LandscapeScene));

	/* Render upper part of framebuffer, while drawing to bottom, and vice versa */

	SetDefDrawEnv(&landscapeSceneBuffers[0]->draw, 0,   0, 320, 256);
	SetDefDrawEnv(&landscapeSceneBuffers[1]->draw, 0, 256, 320, 256);
	SetDefDispEnv(&landscapeSceneBuffers[0]->disp, 0, 256, 320, 256);
	SetDefDispEnv(&landscapeSceneBuffers[1]->disp, 0,   0, 320, 256);
	
	// PAL setup
	landscapeSceneBuffers[1]->disp.screen.x = landscapeSceneBuffers[0]->disp.screen.x = 1;
	landscapeSceneBuffers[1]->disp.screen.y = landscapeSceneBuffers[0]->disp.screen.y = 18;
	landscapeSceneBuffers[1]->disp.screen.h = landscapeSceneBuffers[0]->disp.screen.h = 256;
	landscapeSceneBuffers[1]->disp.screen.w = landscapeSceneBuffers[0]->disp.screen.w = 320;
	
	
	SetFarColor(0,0,0);
	SetBackColor(100,100,100);
	SetFogNearFar(near,far,PROJ_Z); // 0% fog at , 100% fog at, "distance between visual point and screen"
	
	initLandscapeCells();
	
	initLandscapePrimitives(landscapeSceneBuffers[0]);
	initLandscapePrimitives(landscapeSceneBuffers[1]);
	
	loadTIM((unsigned char*)&landlogo);
	loadTIM((unsigned char*)&font);
	
	clutFadeInit(320,128,FadeDown);

	near = -600;

	while (ticks < 500) 
	{
		ticks++;
		printf("ticks %d \n",ticks);
		cdb  = (cdb == landscapeSceneBuffers[0]) ? landscapeSceneBuffers[1] : landscapeSceneBuffers[0];
		
		PutDispEnv(&cdb->disp);
		PutDrawEnv(&cdb->draw);

		pad = PadRead(0);
		
		if (ticks<200)near+=10;
		if(ticks>300)near-=10;
		if (pad & PADLup)	near += 100;
		if (pad & PADLdown)	near -= 100;
		if (pad & PADRup)	far += 100;
		if (pad & PADRdown)	far -= 100;

		SetFogNearFar(near,far,PROJ_Z); // 0% fog at , 100% fog at, "distance between visual point and screen"
		
		ClearOTagR(cdb->ot, LAND_OTSIZE);	
		
		SetColorMatrix(&lcm);
		SetLightMatrix(&llm);
		
		landMoveCounter+=5;
		if(landMoveCounter==LAND_CELL_DEPTH)
		{
			landMoveCounter = 0;
			moveLand(0,0,LAND_CELL_DEPTH);
			moveLandRows();
		}
		moveLand(0,0,-5);
		
		// Add Logo (lowest in OT)
		
		AddPrim(cdb->ot+LAND_OTSIZE-1,&cdb->logoLeftSprite);
		AddPrim(cdb->ot+LAND_OTSIZE-1,&cdb->logoLeftTPage);
		AddPrim(cdb->ot+LAND_OTSIZE-1,&cdb->logoRightSprite);
		AddPrim(cdb->ot+LAND_OTSIZE-1,&cdb->logoRightTPage);
		
		// transpose + light + perspective + add to OT
		
		currentSpritePtr = cdb->textSprites;
		
		drawString("               Presents                 ",0,135,&currentSpritePtr,cdb->ot);
		drawString("         A PlayStation intro            ",0,155,&currentSpritePtr,cdb->ot);
		drawString("        in state of the art 3D          ",0,165,&currentSpritePtr,cdb->ot);
		
		AddPrim(cdb->ot,&cdb->textTPage); // Add texture page change as last item in current OT entry (which is actually run first)
		
		addLandPolys(cdb->ot,cdb->landPolys);

		DrawSync(0);		
		
		DrawOTag(cdb->ot+LAND_OTSIZE-1);	
		
		VSync(0);
	
		/*
		debugVector.vx = 0;
		debugVector.vy = 0;
		debugVector.vz = 0;
		dumpVector("debugVector=",&debugVector);
		*/
		
	}
	
	free(landscapeSceneBuffers[0]);
	free(landscapeSceneBuffers[1]);
	

}