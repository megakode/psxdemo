#include <sys/types.h>
#include <libetc.h>
#include <libgte.h>
#include <libgpu.h>
#include <libmath.h>
#include <Gtemac.h>
#include <Inline_c.h>
#include <libgs.h>

#include "dsr128.h" // TIM dsrlogo.png
#include "font.h" // TIM font.png

#include "biosfont.h"

#include "dsrlib.h" // Desire lib

#include "Sinsweep.h"

#include "land.h"


// *************************************************************
// Cube mosaic 
// *************************************************************

#define CUBESCENE_X_RES 320
#define CUBESCENE_Y_RES 256

#define CUBE_OTSIZE 1024
#define CUBE_ROWS 8
#define CUBE_COLS 13
#define NUMBER_OF_CUBES CUBE_ROWS*CUBE_COLS
#define POLYS_PER_CUBE 12

#define CUBE_ROT_PR_FRAME 32
#define CUBE_ROT_OFFSET_PR_COL 100

#define CUBE_MOVE_PR_FRAME 4
#define NUM_FRAMES_BETWEEN_ADVANCE_SCROLL (HORIZ_DISTANCE_BETWEEN_CUBES / CUBE_MOVE_PR_FRAME)
#define HORIZ_DISTANCE_BETWEEN_CUBES 16
#define VERT_DISTANCE_BETWEEN_CUBES 14
#define CS (10 / 2) // half because all points are offset from the cube center by this value

const char *scrollText = "REVISION 2019   ";
int scrollTextLetterIndex = 0;
int scrollTextPixelIndex = 0;
int sinIndex = 0;

#define SCR_Z (512)	/* screen depth */

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
			
	MATRIX	inverseLightMatrix;
	SVECTOR inverseLightVector;
	SVECTOR lightDirVec;
	/* Local Light Matrix */
	MATRIX	llm;

} CubeScratchPad;


typedef struct {

	int textOffset; // pixel offset into the scroll text that the cube column is currently showing

} CubeColumn;



CubeMosaicScene *cubeSceneBuffers[2];
CubeMosaicScene* currentCubeSceneBuffer;
Cube *cubes;

char cubeColors[NUMBER_OF_CUBES];

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
	int i = 0;
	
	for( i = 0 ; i < NUMBER_OF_CUBES * POLYS_PER_CUBE ; i++ )
	{
		SetPolyF3(&buffer->cubePolys[i]);
	}
}

// *************************************************************
//
// Translate and add poly to primitive drawing list
//
// Take v0,v1,v2 3d points, translate them to 2d primitive and add it to drawing list and ordering table
// and calculate light using lcm and llm
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

void advanceScrollText()
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
		}
	}

}

// *************************************************************
// Do cubes
// *************************************************************

void doCubes()
{
	SVECTOR lightDirVec = {1000,0,4000};
	MATRIX	lcm = { 4095,0,0, 
							4095,0,0, 
							4095,0,0, 
							0,0,0};
							
	int cpuCounter = 0;
	int scrollTrigger = 0;
	SVECTOR *cubeDefs = (SVECTOR *) getScratchAddr(0);
	CubeScratchPad *cubeScratchPad = (CubeScratchPad *) getScratchAddr( (sizeof(SVECTOR)*12*3)>>2 );
	
	
	
	ResetGraph(0);
	ResetCallback();
	PadInit(0);
	SetGraphDebug(0);
	
	InitGeom();
	//SetGeomOffset(160, 128);	
	gte_SetGeomOffset(0, 0);	
	gte_SetGeomScreen(SCR_Z);	
	SetVideoMode(MODE_PAL);
	SetDispMask(1);

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
		
	//FntLoad(960,256);
	//SetDumpFnt(FntOpen(64, 64, 200, 20, 1, 512));
	
	// Do cube scene
	
	gte_SetColorMatrix(&lcm);
	
	while(1)
	{
		
		int cubeIndex = 0;
		
		currentCubeSceneBuffer  = (currentCubeSceneBuffer == cubeSceneBuffers[0]) ? cubeSceneBuffers[1] : cubeSceneBuffers[0];
		
		
		ClearOTagR(currentCubeSceneBuffer->ot, CUBE_OTSIZE);	
		DrawSync(0);

		cpuCounter=-VSync(1);
		
		
		sinIndex++;
		
		if( sinIndex >= SINSWEEP_LEN-CUBE_COLS ){
			sinIndex = 0;
		}
		
		for( cubeIndex = 0 ; cubeIndex < NUMBER_OF_CUBES ; cubeIndex++ )
		{
			int polyIndex = 0;
			int cubeVertIndex = 0;
			int column = (cubeIndex % CUBE_COLS);
			int ysin = (sinsweep[sinIndex+column]>>2) - 20;
			
			SVECTOR	rotVec  = {cubes[cubeIndex].xrot,cubes[cubeIndex].yrot,cubes[cubeIndex].zrot};
			VECTOR	posVec  = {cubes[cubeIndex].xpos+(ysin>>1),cubes[cubeIndex].ypos + ysin,cubes[cubeIndex].zpos,0};
			
			
			
	
			// Create a rotation matrix for the light, with the negated rotation from the cube.
			
			cubeScratchPad->inverseLightVector.vx = -rotVec.vx;
			cubeScratchPad->inverseLightVector.vy = -rotVec.vy;
			cubeScratchPad->inverseLightVector.vz = -rotVec.vz;
			
			
			// The reverse axis sequence of the normal RotMatrix() function, to counter the rotation
			RotMatrixZYX_gte(&cubeScratchPad->inverseLightVector,&cubeScratchPad->inverseLightMatrix); 
			gte_ApplyMatrixSV(&cubeScratchPad->inverseLightMatrix,(SVECTOR*)&lightDirVec,(SVECTOR*)&cubeScratchPad->llm);
			gte_SetLightMatrix(&cubeScratchPad->llm);

			// Set position and rotation matrix for current cube, before translating polygon
			RotMatrix_gte(&rotVec, &cubeScratchPad->m);
			TransMatrix(&cubeScratchPad->m,&posVec);
			
			gte_SetRotMatrix(&cubeScratchPad->m);	
			gte_SetTransMatrix(&cubeScratchPad->m);
			

			// Calculate color, light and 2D translation for each polygon on current cube
			
			for( polyIndex = 0; polyIndex < POLYS_PER_CUBE ; polyIndex++ )
			{			
				POLY_F3 *poly = &currentCubeSceneBuffer->cubePolys[cubeIndex*POLYS_PER_CUBE + polyIndex];
				int	isomote;
				long	p, otz, opz, flg;
				
				if(cubeColors[cubeIndex] > 0){ // && polyIndex < 2
					cubeScratchPad->color.r = 255;
					cubeScratchPad->color.g = 210;
					cubeScratchPad->color.b = 255;
				} else {
					cubeScratchPad->color.r = sinsweep[sinIndex+column];
					cubeScratchPad->color.g = 32;
					cubeScratchPad->color.b = column * 20;
				}
				

				gte_RotAverageNclip3(&cubeDefs[cubeVertIndex], &cubeDefs[cubeVertIndex+1], &cubeDefs[cubeVertIndex+2], 
				(long *)&poly->x0, (long *)&poly->x1,(long *)&poly->x2, 
				&p,
				&otz,
				&flg,&isomote);


				if (isomote > 0) // If poly is facing the camera...
				{
					// Get local light value..
					gte_NormalColorCol(cubeNormals[polyIndex>>1], &cubeScratchPad->color, &cubeScratchPad->colorOut);
					// and apply it to surface
					setRGB0(poly,cubeScratchPad->colorOut.r,cubeScratchPad->colorOut.g,cubeScratchPad->colorOut.b);
					addPrim(currentCubeSceneBuffer->ot+otz, poly);
				}
			
			// ***********************************
									
				cubeVertIndex += 3;
			}
			

		}
		cpuCounter+=VSync(1);
		
		VSync(0);
		
		PutDispEnv(&currentCubeSceneBuffer->disp);
		PutDrawEnv(&currentCubeSceneBuffer->draw);	
	
		DrawOTag(currentCubeSceneBuffer->ot+CUBE_OTSIZE-1);
		
		
		//FntPrint("CPU: %d%% \n sinsweep:%d index=%d",(int)((float)cpuCounter/(float)256*100) , sinsweep[sinIndex]>>1,sinIndex);

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
		
			scrollTrigger = 0;
			
			advanceScrollText();
			
		
		}
		

				
		
	
	}
	
	// Clean up
	
	free(cubeSceneBuffers[0]);
	free(cubeSceneBuffers[1]);
	free(cubes);
	
	
}

// *************************************************************
//   Main
// *************************************************************

int main(){
	doCubes();
	//doLandscape();
}