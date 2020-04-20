// *************************************************************
// Cube mosaic 
// *************************************************************
#include <sys/types.h>
#include <libetc.h>
#include <libgte.h>
#include <libgpu.h>
#include <libmath.h>
#include <Gtemac.h>
#include <Inline_c.h>
#include <libgs.h>

#include "biosfont.h"
#include "Sinsweep.h"

//#define ENDLESS_LOOP true

#define CUBESCENE_X_RES 320
#define CUBESCENE_Y_RES 256

#define CUBE_OTSIZE 1024
#define CUBE_ROWS 8
#define CUBE_COLS 15 //13
#define NUMBER_OF_CUBES CUBE_ROWS*CUBE_COLS
#define POLYS_PER_CUBE 12

#define CUBE_ROT_PR_FRAME 32
#define CUBE_ROT_OFFSET_PR_COL 200

#define CUBE_MOVE_PR_FRAME 4
#define NUM_FRAMES_BETWEEN_ADVANCE_SCROLL (HORIZ_DISTANCE_BETWEEN_CUBES / CUBE_MOVE_PR_FRAME)
#define HORIZ_DISTANCE_BETWEEN_CUBES 12
#define VERT_DISTANCE_BETWEEN_CUBES 12
#define CS (8 / 2) // half because all points are offset from the cube center by this value

const char *scrollText = "    KEEPING THE SCENE ALIVE ";
//const char *scrollText = "    TEST! ";
int scrollTextLetterIndex = 0;
int scrollTextPixelIndex = 0;
int sinIndex = 0;

#define SCR_Z (210)	/* screen depth */

typedef struct {

	DRAWENV	draw;
	DISPENV	disp;
	u_long	ot[CUBE_OTSIZE];	/* Ordering Table */
	
	POLY_F3 cubePolys[NUMBER_OF_CUBES * POLYS_PER_CUBE];
	
} CubeMosaicScene;

typedef struct {

	int xpos;
	int ypos;
	int zpos;
	
	int xrot;
	int yrot;
	int zrot;

} Cube;


typedef struct {

	CVECTOR color;
	CVECTOR colorOut;
	int cubeVertIndex;
	int polyIndex;
			
	MATRIX m;
	SVECTOR	rotVec;
	VECTOR	posVec;
			
	MATRIX	inverseLightMatrix; // Matrix with the inverse rotation of the object, used for counter-rotating the light.
	SVECTOR lightDirVec; // Light direction vector
	MATRIX	llm; // Local Light Matrix
	
	int nclip;
	int avgz;
	
	SVECTOR	cubeNormals[6];

} CubeScratchPad;



CubeMosaicScene *cubeSceneBuffers[2];
CubeMosaicScene* currentCubeSceneBuffer;
Cube *cubes;

char cubeColors[NUMBER_OF_CUBES];

static const int colorSine1[256] = {50,48,47,46,45,43,42,41,40,39,37,36,35,34,33,32,30,29,
28,27,26,25,24,23,22,21,20,19,18,17,16,15,14,13,12,12,11,10,9,9,8,7,7,6,5,5,4,4,
3,3,2,2,2,1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,2,2,2,3,3,4,4,5,5,6,7,7,
8,9,9,10,11,12,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,33,34
,35,36,37,39,40,41,42,43,45,46,47,48,49,51,52,53,54,56,57,58,59,60,62,63,64,65,66,
67,69,70,71,72,73,74,75,76,77,78,79,80,81,82,83,84,85,86,87,87,88,89,90,90,91,
92,92,93,94,94,95,95,96,96,97,97,97,98,98,98,99,99,99,99,99,99,99,99,99,99,99,99
,99,99,99,99,99,98,98,98,97,97,97,96,96,95,95,94,94,93,92,92,91,90,90,89,88,87,87,
86,85,84,83,82,81,80,79,78,77,76,75,74,73,72,71,70,69,68,66,65,64,63,62,60,59,58,57,56,54,53,52};

static const int colorSine2[256] = {20,20,20,20,20,21,21,21,21,22,22,22,22,23,23,23,23,24,24,24,24,25,25,25,25,26,26
,26,26,26,27,27,27,27,28,28,28,28,28,29,29,29,29,30,30,30,30,30,31,31,31,31,31,
32,32,32,32,32,33,33,33,33,33,33,34,34,34,34,34,34,35,35,35,35,35,35,36,36,36,36,
36,36,36,37,37,37,37,37,37,37,37,37,38,38,38,38,38,38,38,38,38,38,38,39,39,39,39,
39,39,39,39,39,39,39,39,39,39,39,39,39,39,39,39,39,39,39,39,39,39,39,39,39,39,39,
39,39,39,39,39,39,39,39,39,39,39,39,39,39,39,39,39,39,39,39,38,38,38,38,38,38,
38,38,38,38,38,37,37,37,37,37,37,37,37,37,36,36,36,36,36,36,36,35,35,35,35,35,
35,34,34,34,34,34,34,33,33,33,33,33,33,32,32,32,32,32,31,31,31,31,31,30,30,30,
30,30,29,29,29,29,28,28,28,28,28,27,27,27,27,26,26,26,26,26,25,25,25,25,24,24,
24,24,23,23,23,23,22,22,22,22,21,21,21,21,20,20,20};

static const int colorSine3[256] = {79,79,79,79,79,79,79,78,78,77,77,76,76,75,74,74,73,72,71,70,69,68,67,66,65,64,63
,61,60,59,57,56,55,53,52,51,49,48,46,45,43,42,40,39,38,36,35,33,32,30,29,27,26,25,
23,22,21,19,18,17,16,15,13,12,11,10,9,8,7,7,6,5,4,4,3,2,2,1,1,1,0,0,0,0,0,0,0,0,0,
0,0,0,1,1,2,2,3,3,4,4,5,6,7,8,9,10,11,12,13,14,15,16,17,19,20,21,22,24,25,26,
28,29,31,32,34,35,37,38,39,41,42,44,45,47,48,50,51,53,54,55,57,58,59,60,62,63,64,
65,66,67,68,69,70,71,72,73,74,75,75,76,76,77,77,78,78,79,79,79,79,79,79,79,79,
79,79,79,79,78,78,78,77,77,76,75,75,74,73,72,72,71,70,69,68,67,66,65,63,62,61,60
,58,57,56,54,53,52,50,49,47,46,44,43,41,40,39,37,36,34,33,31,30,28,27,26,24,23,22,
20,19,18,16,15,14,13,12,11,10,9,8,7,6,5,5,4,3,3,2,2,1,1,0,0,0,0,0};

unsigned char colorSineIndex1 = 0;
unsigned char colorSineIndex2 = 64;
unsigned char colorSineIndex3 = 128;

// Cube vertices (cube with center origin)

static  SVECTOR cubeV1 = {-CS,-CS,-CS,0};
static  SVECTOR cubeV2 = {CS,-CS,-CS,0};
static  SVECTOR cubeV3 = {-CS,CS,-CS,0};
static  SVECTOR cubeV4 = {CS,CS,-CS,0};
static  SVECTOR cubeV5 = {-CS,-CS,CS,0};
static  SVECTOR cubeV6 = {CS,-CS,CS,0};
static  SVECTOR cubeV7 = {-CS,CS,CS,0};
static  SVECTOR cubeV8 = {CS,CS,CS,0};


// Cube vertices (cube with origin in top,left, outermost corner
/*
#define CUBE_SIZE 3
static  SVECTOR cubeV1 = {0,0,0,0};
static  SVECTOR cubeV2 = {CUBE_SIZE,0,0,0};
static  SVECTOR cubeV3 = {0,CUBE_SIZE,0,0};
static  SVECTOR cubeV4 = {CUBE_SIZE,CUBE_SIZE,0,0};
static  SVECTOR cubeV5 = {0,0,CUBE_SIZE,0};
static  SVECTOR cubeV6 = {CUBE_SIZE,0,CUBE_SIZE,0};
static  SVECTOR cubeV7 = {0,CUBE_SIZE,CUBE_SIZE,0};
static  SVECTOR cubeV8 = {CUBE_SIZE,CUBE_SIZE,CUBE_SIZE,0};
*/
// Cube polys
static const SVECTOR *cubePolys[12*3] = {
	// Front
	&cubeV1,&cubeV2,&cubeV3,
	&cubeV3,&cubeV2,&cubeV4,
	// Left
	&cubeV5,&cubeV1,&cubeV3,
	&cubeV7,&cubeV5,&cubeV3,
	// Back
	&cubeV8,&cubeV6,&cubeV7,
	&cubeV6,&cubeV5,&cubeV7,
	// Right
	&cubeV2,&cubeV6,&cubeV4,
	&cubeV4,&cubeV6,&cubeV8,
	// Bottom
	&cubeV7,&cubeV3,&cubeV4,
	&cubeV7,&cubeV4,&cubeV8,
	// Top
	&cubeV1,&cubeV5,&cubeV6,
	&cubeV1,&cubeV6,&cubeV2
};

// Hardcoded cube normals
static SVECTOR nFront = {0,      0, -ONE, 0,};
static SVECTOR nLeft = {-ONE,   0,    0, 0,};
static SVECTOR nBack = {0,      0,  ONE, 0,};
static SVECTOR nRight = { ONE,   0,    0, 0,};
static SVECTOR nBottom = {0,    ONE,    0, 0,};
static SVECTOR nTop = {0,   -ONE,    0, 0,};
static SVECTOR	*cubeNormals[6] = {	&nBack, &nRight, &nFront, &nLeft, &nTop, &nBottom};

// *************************************************************
//
// initCubePrimitives
//
// Initialize all primitive structs in Scene buffer
//
// *************************************************************				  

void initCubePrimitives( CubeMosaicScene* buffer )
{
//	int i = 0;
//	for( i = 0 ; i < NUMBER_OF_CUBES * POLYS_PER_CUBE ; i++ )
//	{
//		SetPolyF3(&buffer->cubePolys[i]);
//	}
}

// *************************************************************
//
// Translate and add poly to primitive drawing list
//
// Take v0,v1,v2 3d points, translate them to 2d primitive and add it to drawing list and ordering table
// and calculate light using lcm and llm
//
// *************************************************************

inline void translateAndAddPoly(u_long *ot, POLY_F3 *poly,SVECTOR *v0,SVECTOR *v1,SVECTOR *v2,SVECTOR *n,CVECTOR *colorIn)
{
	CVECTOR colorOut;
	int	isomote;
	long	p, otz, opz, flg;
		
	gte_RotAverageNclip3(v0, v1, v2, 
		(long *)&poly->x0, (long *)&poly->x1,(long *)&poly->x2, 
		&p,
		&otz,
		&flg,&isomote);
		
	if (isomote <= 0) return;

	//if (otz > 0 && otz < CUBE_OTSIZE) {	

		// Get local light value..
		
		gte_NormalColorCol(n, colorIn, &colorOut);
		
		// and apply it to surface
		setRGB0(poly,colorOut.r,colorOut.g,colorOut.b);
		
		addPrim(ot+otz, poly);
	//}

}

// *************************************************************	
//
// advanceScrollText
//
// returns: 1 if scroll has restarted, 0 if not.
//
// *************************************************************	

int advanceScrollText()
{
	int x,y;
	char currentLetter = scrollText[scrollTextLetterIndex];
	int pixelsToMoveCubesBack = NUM_FRAMES_BETWEEN_ADVANCE_SCROLL * CUBE_MOVE_PR_FRAME;
	
	// Move all data left
	
	for( y = 0 ; y < CUBE_ROWS ; y++ )
	{
		for( x = 0 ; x < CUBE_COLS ; x++ )
		{
			int cubeIndex = y*CUBE_COLS + x;
		
			cubeColors[ cubeIndex ] = cubeColors[ cubeIndex+1 ];
			cubes[cubeIndex].yrot = cubes[cubeIndex+1].yrot;
			cubes[cubeIndex].xpos += pixelsToMoveCubesBack;
		}
	}
	
	// Generate new data at rightmost end
	
	for( y = 0 ; y < CUBE_ROWS ; y++ )
	{
		int fontRowData = biosfont[currentLetter*8+y];
		
		char color = ( (128 >> scrollTextPixelIndex) & fontRowData) ? 1 : 0;
		int cubeIndex = (y+1)*CUBE_COLS-1;
		cubeColors[cubeIndex] = color;
		cubes[cubeIndex].yrot = cubes[cubeIndex-1].yrot+CUBE_ROT_OFFSET_PR_COL;
		  
	}
	
	// Advance scroll text to next pixel
	scrollTextPixelIndex +=1;
	
	// When advanced 8 pixels, advance to next letter
	if(scrollTextPixelIndex == 8)
	{
		scrollTextPixelIndex = 0 ;
		scrollTextLetterIndex += 1;
		
		// When string ends, restart
		if(scrollText[scrollTextLetterIndex] == 0){
			scrollTextLetterIndex=0;
			return 1;
		}
	}
	
	return 0;

}

// *************************************************************	
//
// initSineTables
//
// *************************************************************	

void initSineTables(){

// Commented out as the tables are now precalculated for speed (float math is SLOW!).
// Left in here so we can adjust them and precalculate them again if needed.
/*

	int i;
	float sindex1 = 3.1415;
	float sindex2 = 0;
	float sindex3 = 3.1415/2;
	
	printf("\n");

	for( i = 0 ; i < 255 ; i ++ ){
	
		//colorSine1[i] = 50 + sin(sindex1)*50;
		//colorSine2[i] = 20 + sin(sindex2)*20;
		colorSine3[i] = 40 + sin(sindex3)*40;
		
		sindex1 += 2*3.1415 / 256;
		sindex2 += 1*3.1415 / 256;
		sindex3 += 3*3.1415 / 256;
		
		printf("%d,",colorSine3[i]);
	
	}
	
	printf("\n");
	*/
}

void *CubeAsm(void *pPrims, SVECTOR *verts, SVECTOR *cubeNormals, u_long* ot);


/*
static void DrawCube(int cubeIndex, CubeScratchPad *cubeScratchPad, SVECTOR *cubeDefs)
{

	POLY_F3 *poly = &currentCubeSceneBuffer->cubePolys[cubeIndex*POLYS_PER_CUBE];

	int polyIndex;
	int cubeVertIndex = 0;
	for( polyIndex = 0; polyIndex < POLYS_PER_CUBE ; polyIndex++ )
	{	
		long	p, flg;
		gte_RotAverageNclip3(&cubeDefs[cubeVertIndex], &cubeDefs[cubeVertIndex+1], &cubeDefs[cubeVertIndex+2], 
		(long *)&poly->x0, (long *)&poly->x1,(long *)&poly->x2, 
		&p,
		&cubeScratchPad->avgz,
		&flg,&cubeScratchPad->nclip);


		if (cubeScratchPad->nclip > 0) // If poly is facing the camera...
		{
			int otz = cubeScratchPad->avgz;

			// Get local light value..
			gte_NormalColorCol(&cubeScratchPad->cubeNormals[polyIndex>>1], &cubeScratchPad->color, (unsigned int*)&poly->r0);
		
			addPrim(currentCubeSceneBuffer->ot+otz, poly);
		}
							
		cubeVertIndex += 3;
		poly++;
	}
}
*/

// *************************************************************
// Do cubes
// *************************************************************

void doCubes()
{
	int rasterTime = 0;

	SVECTOR lightDirVec = {1000,0,4000};
	MATRIX	lcm = { 4095,0,0, 
							4095,0,0, 
							4095,0,0, 
							0,0,0};
							
	int cpuCounter = 0;
	int scrollTrigger = 0;
	
	int numFadeColsLeft = CUBE_COLS; // Number of columns to set to black, starting from left side of screen
	int numColoredColumns = CUBE_COLS; // Number of columns allowed to have color. Used when fading out.
	int startFadingOut = 0; // flag that indicates whether to start fading out
	int isDone = 0;
	
	SVECTOR *cubeDefs = (SVECTOR *) getScratchAddr(0);
	CubeScratchPad *cubeScratchPad = (CubeScratchPad *) getScratchAddr( (sizeof(SVECTOR)*12*3)>>2 );
	
	initSineTables();
	
	SetDispMask(0);
	/*
	ResetGraph(0);
	ResetCallback();
	PadInit(0);
	SetGraphDebug(0);
	
	InitGeom();
*/
	SetGeomOffset(0, 0);	
	SetGeomScreen(SCR_Z);	
	//SetVideoMode(MODE_PAL);
	

	// Init cube scene
	
	cubeSceneBuffers[0] = (CubeMosaicScene*)malloc(sizeof(CubeMosaicScene));
	cubeSceneBuffers[1] = (CubeMosaicScene*)malloc(sizeof(CubeMosaicScene));
	cubes = (Cube*)malloc(sizeof(Cube)*NUMBER_OF_CUBES);
	
	SetDefDrawEnv(&cubeSceneBuffers[0]->draw, 0,   0, CUBESCENE_X_RES, CUBESCENE_Y_RES);
	SetDefDrawEnv(&cubeSceneBuffers[1]->draw, 0, CUBESCENE_Y_RES, CUBESCENE_X_RES, CUBESCENE_Y_RES);
	SetDefDispEnv(&cubeSceneBuffers[0]->disp, 0, CUBESCENE_Y_RES, CUBESCENE_X_RES, CUBESCENE_Y_RES);
	SetDefDispEnv(&cubeSceneBuffers[1]->disp, 0,   0, CUBESCENE_X_RES, CUBESCENE_Y_RES);
	
	// PAL setup
	cubeSceneBuffers[1]->disp.screen.x = cubeSceneBuffers[0]->disp.screen.x = 1;
	cubeSceneBuffers[1]->disp.screen.y = cubeSceneBuffers[0]->disp.screen.y = 18;
	cubeSceneBuffers[1]->disp.screen.h = cubeSceneBuffers[0]->disp.screen.h = 256;
	cubeSceneBuffers[1]->disp.screen.w = cubeSceneBuffers[0]->disp.screen.w = 320;
	
	cubeSceneBuffers[0]->draw.isbg = 1;
	cubeSceneBuffers[1]->draw.isbg = 1;
	setRGB0(&cubeSceneBuffers[0]->draw, 0, 0, 0);
	setRGB0(&cubeSceneBuffers[1]->draw, 0, 0, 0);
	/*
	SetFarColor(0,0,0);
	SetBackColor(100,100,100);
	SetFogNearFar(600,2000,SCR_Z); // 0% fog at , 100% fog at, "distance between visual point and screen"
	*/
	initCubePrimitives( cubeSceneBuffers[0] );
	initCubePrimitives( cubeSceneBuffers[1] );
	
	// Copy cube vertice definitions to scratch pad
	{
		int i = 0;
		
		for(i=0;i<12*3;i++){
			cubeDefs[i] = *cubePolys[i];
		}

	
		cubeScratchPad->cubeNormals[0] = nBack;
		cubeScratchPad->cubeNormals[1] = nRight;
		cubeScratchPad->cubeNormals[2] = nFront;
		cubeScratchPad->cubeNormals[3] = nLeft;
		cubeScratchPad->cubeNormals[4] = nTop;
		cubeScratchPad->cubeNormals[5] = nBottom;
		
	}
	
	
	// setup initial cube positions
	
	{
		int i = 0;
		int y,x = 0;
		for( i = 0 ; i < NUMBER_OF_CUBES ; i++ )
		{
			cubes[i].xpos = x*HORIZ_DISTANCE_BETWEEN_CUBES;
			cubes[i].ypos = y*VERT_DISTANCE_BETWEEN_CUBES;
			cubes[i].zpos = 100;
	

			cubes[i].xrot = 256;
			cubes[i].yrot = (x+y)*CUBE_ROT_OFFSET_PR_COL;
			cubes[i].zrot = 0;
			x++;
			
			if(x==CUBE_COLS){
				y++;
				x=0;
			}
		}
	}
		
	FntLoad(960,256);
	SetDumpFnt(FntOpen(64, 64, 200, 20, 1, 512));
	
	// Do cube scene
	
	gte_SetColorMatrix(&lcm);

	SetDispMask(1);
	
	while(!isDone)
	{
		void *pPrims;
		int cubeIndex = 0, column = 0;
		
		
		currentCubeSceneBuffer  = (currentCubeSceneBuffer == cubeSceneBuffers[0]) ? cubeSceneBuffers[1] : cubeSceneBuffers[0];
		
		ClearOTagR(currentCubeSceneBuffer->ot, CUBE_OTSIZE);	
	
		cpuCounter=-VSync(1);
	
		sinIndex++;
		
		if( sinIndex >= SINSWEEP_LEN-CUBE_COLS ){
			sinIndex = 0;
		}
		
		pPrims = currentCubeSceneBuffer->cubePolys;
		
		for( cubeIndex = 0 ; cubeIndex < NUMBER_OF_CUBES ; cubeIndex++)
		{
					
			int polyIndex = 0;
			int ysin = (sinsweep[sinIndex+column]>>2) - 20;
			
			SVECTOR	rotVec  = {cubes[cubeIndex].xrot,cubes[cubeIndex].yrot,cubes[cubeIndex].zrot};
			VECTOR	posVec  = {cubes[cubeIndex].xpos+(ysin>>1),cubes[cubeIndex].ypos + ysin,cubes[cubeIndex].zpos,0};

			if(++column == CUBE_COLS)
				column = 0; 

			// Set position and rotation matrix for current cube, before translating polygons
			RotMatrix_gte(&rotVec, &cubeScratchPad->m);
			TransMatrix(&cubeScratchPad->m,&posVec);
			
			// Create a rotation matrix for the light, with the negated rotation from the cube.
			TransposeMatrix(&cubeScratchPad->m, &cubeScratchPad->inverseLightMatrix);
	
			gte_ApplyMatrixSV(&cubeScratchPad->inverseLightMatrix,(SVECTOR*)&lightDirVec,(SVECTOR*)&cubeScratchPad->llm);

			gte_SetLightMatrix(&cubeScratchPad->llm);
			gte_SetRotMatrix(&cubeScratchPad->m);	
			gte_SetTransMatrix(&cubeScratchPad->m);

			// Calculate color, light and 2D translation for each polygon on current cube
			
			
			if(numFadeColsLeft>0 && column <= numFadeColsLeft-1){
			
				cubeScratchPad->color.r = 0;
				cubeScratchPad->color.g = 0;
				cubeScratchPad->color.b = 0;
			
			} else if( startFadingOut > 0 && column >= numColoredColumns-1 ) {
			
				cubeScratchPad->color.r = 0;
				cubeScratchPad->color.g = 0;
				cubeScratchPad->color.b = 0;
			
			} else if(cubeColors[cubeIndex] > 0){ // && polyIndex < 2
				cubeScratchPad->color.r = 255;
				cubeScratchPad->color.g = 210;
				cubeScratchPad->color.b = 255;
			} else {
				// With sine sweep colors
				/*
				cubeScratchPad->color.r = sinsweep[sinIndex+column];
				cubeScratchPad->color.g = 32;
				cubeScratchPad->color.b = column * 20;
				*/
				// With sine colors:
				cubeScratchPad->color.r = colorSine1[(unsigned char)(colorSineIndex1+cubeIndex)]; 
				cubeScratchPad->color.g = colorSine2[(unsigned char)(colorSineIndex2)] ;
				cubeScratchPad->color.b = column + colorSine3[colorSineIndex3];
			}

			cubeScratchPad->color.cd = 0x20;  // primitive code for polyf3
			
			gte_ldrgb(&cubeScratchPad->color);
			pPrims = CubeAsm(pPrims, cubeDefs, cubeScratchPad->cubeNormals, currentCubeSceneBuffer->ot);
			
		}
		
		
		colorSineIndex1++;
		colorSineIndex2++;
		colorSineIndex3++;
		
		cpuCounter+=VSync(1);
	
		DrawSync(0);
		rasterTime = VSync(0);
		
		PutDispEnv(&currentCubeSceneBuffer->disp);
		PutDrawEnv(&currentCubeSceneBuffer->draw);	

		DrawOTag(currentCubeSceneBuffer->ot+CUBE_OTSIZE-1);
		
		//FntPrint("CPU: %d%% %d\n",  (cpuCounter * 100) / 312, rasterTime);
		//FntFlush(-1);

		// 1024 / CUBE_ROT_PR_FRAME
		// 4096 = full rotation. 1024 = one quarter (which we need), as we do xrot += 32 pr step it is 32 steps to reach 1024
		
		for( cubeIndex = 0 ; cubeIndex < NUMBER_OF_CUBES ; cubeIndex++ )
		{
			cubes[cubeIndex].yrot += CUBE_ROT_PR_FRAME;
			cubes[cubeIndex].xpos -= CUBE_MOVE_PR_FRAME;
		}
		
		
		scrollTrigger++;
		
		if(scrollTrigger == NUM_FRAMES_BETWEEN_ADVANCE_SCROLL){ 
		
			if( advanceScrollText() ){
				#ifndef ENDLESS_LOOP
				startFadingOut = 1; // Set begin fadeout flag
				#endif
			}
			
			scrollTrigger = 0;
			
			// If fading out, decrease number of colored cubes
			if(numColoredColumns>0 && startFadingOut>0){
				numColoredColumns--;
			}
			
			// If fading in, decrease number of black cubes
			if(numFadeColsLeft>0){
				numFadeColsLeft--;
			}
			
			// If done fading out, set done flag
			#ifndef ENDLESS_LOOP
			if( numColoredColumns==0 && startFadingOut>0){
				isDone = 1;
			}
			#endif
		
		}	
		
	}
	
	// Clean up
	
	free(cubeSceneBuffers[0]);
	free(cubeSceneBuffers[1]);
	free(cubes);
	
}
