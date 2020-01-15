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
#include "ship.h"

#define OTSIZE 4096

typedef struct {		
	DRAWENV		draw;			/* drawing environment */
	DISPENV		disp;			/* display environment */
	u_long		ot[OTSIZE];		/* ordering table */
	POLY_G3	*poly3s;	/* mesh cell */
	POLY_G4	*poly4s;	/* mesh cell */
} DB;

DB	db[2];		/* double buffer */
DB	*cdb;		/* current double buffer */
u_long	*ot;	/* current OT */

/*************************************************************/
// Light stuff
/*************************************************************/
// Scale vector: 4096 = 1.0
VECTOR Scale = { 4096,4096,4096, 0 };

static const SVECTOR lightDirVec = {0,3000,0};
//static SVECTOR lightDirVec = {0,0,-4096}; // Directly into screen

/* Local Light Matrix */
	MATRIX	llm = {0,0,0,
				   0,0,0,
				   0,0,0,
				   0,0,0};

	/* Local Color Matrix */
	MATRIX	lcm = {4096,0,0, 
				   4096,0,0, 
				   4096,0,0, 
				   0,0,0};

int doModel()
{
	const static int screenWidth = 320;
	const static int screenHeight = 256;
	
	VECTOR posVec = { 0,0,1000,0 };
	SVECTOR rotVec = { 1,0,0,0 };
	int frames = 0;
	int numPoly4s = 0;
	int numPoly3s = body_poly_count + arm_left_poly_count + arm_right_poly_count;
	
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
	// Enable/disable dithering
	db[0].draw.dtd = 1;
	db[1].draw.dtd = 1;
	setRGB0(&db[0].draw, 0, 0, 100);
	setRGB0(&db[1].draw, 0, 0, 100);
	
	SetBackColor(100,100,100);
	
	// Allocate cdb->poly4s and initialize them to POLY_G4 structs
	
	{
		int i = 0;
		
		db[0].poly4s = (POLY_G4*)malloc(sizeof(POLY_G4)*numPoly4s);
		db[1].poly4s = (POLY_G4*)malloc(sizeof(POLY_G4)*numPoly4s);
		
		db[0].poly3s = (POLY_G3*)malloc(sizeof(POLY_G3)*numPoly3s);
		db[1].poly3s = (POLY_G3*)malloc(sizeof(POLY_G3)*numPoly3s);

		for(i=0;i<numPoly3s;i++)
		{
			setPolyG3( (db[0].poly3s+i) );
			setPolyG3( (db[1].poly3s+i) );
			SetSemiTrans( (db[0].poly3s+i) , 0);
			SetSemiTrans( (db[1].poly3s+i) , 0);
		}
		
		for(i=0;i<numPoly4s;i++)
		{
			setPolyG4( (db[0].poly4s+i) );
			setPolyG4( (db[1].poly4s+i) );
			
			SetSemiTrans( (db[0].poly4s+i) , 0);
			SetSemiTrans( (db[1].poly4s+i) , 0);
		}
		
	}
	

			
	while(1)
	{
		int i = 0;
		
		MATRIX m;
		MATRIX inverseLightMatrix;
		POLY_G4 *poly;
		u_long pad;
		
		cdb = (cdb==db)? db+1: db;	/* swap double buffer ID */	
		ClearOTagR(cdb->ot, OTSIZE);	/* clear ordering table */
		
		// TODO: Clean this shit up. Just experimenting with rotation and scaling.
		//rotVec.vx += 10;
		//rotVec.vy += 10;
		//rotVec.vz += 10;
		
		pad = PadRead(0);
		
		if (pad & PADLup)	rotVec.vz += 10;
		if (pad & PADLdown)	rotVec.vz -= 10;
		if (pad & PADLleft)	rotVec.vy += 10;
		if (pad & PADLright)rotVec.vy -= 10;
		if (pad & PADRup)	rotVec.vz += 10;
		if (pad & PADRdown)	rotVec.vz -= 10;
		
		RotMatrix_gte(&rotVec, &m); // Calculate rotation matrix from vector
		
		TransposeMatrix(&m, &inverseLightMatrix);
		
		TransMatrix(&m,&posVec);
		
		// Create a rotation matrix for the light, with the negated rotation from the cube.
		TransposeMatrix(&m, &inverseLightMatrix);
		ApplyMatrixSV(&inverseLightMatrix,(SVECTOR*)&lightDirVec,(SVECTOR*)&llm);
		
		ScaleMatrix(&m, &Scale);
		
		SetTransMatrix(&m);
		SetRotMatrix(&m);	

		SetColorMatrix(&lcm);
		SetLightMatrix(&llm);
		

		// add primitives here
		/*
		for( i = 0 ; i < numPoly4s ; i++ )
		{
			short *vi;
			
			int isomote = 0;
			long	p, otz, opz, flg;

			poly = cdb->poly4s+i;
			vi = &poly4arr[i*9]; // model Input. *8 because there are 8 shorts for each poly4: vi1,vi2,vi3,vi4,ni1,ni2,ni3,ni4

			isomote = RotAverageNclip4(&vertices[*(vi)], &vertices[*(vi+1)], &vertices[*(vi+3)],&vertices[*(vi+2)],
			(long *)&poly->x0, (long *)&poly->x1,(long *)&poly->x2,(long *)&poly->x3, 
			&p,&otz,&flg);

			if (isomote <= 0) continue;
			if (otz >= 0 && otz < OTSIZE) 
			{
				CVECTOR color1,color2,color3,color4;
				CVECTOR *colorIn = &materials[*(vi+8)];
				
				// Make colors for each of the 4 vertices
				
				// NormalColorCol destroys the .code on poly, so set it manually afterwards.
				// The code for POLY_G4 is 0x38. 
				// From libgpu.h:
				// #define setPolyG4(p)	setlen(p, 8),  setcode(p, 0x38)
				NormalColorCol(&normals[*(vi+4)], colorIn, (CVECTOR*)&poly->r0);
				NormalColorCol(&normals[*(vi+5)], colorIn, (CVECTOR*)&poly->r1);
				NormalColorCol(&normals[*(vi+7)], colorIn, (CVECTOR*)&poly->r2);
				NormalColorCol(&normals[*(vi+6)], colorIn, (CVECTOR*)&poly->r3);
				poly->code = 0x38;
				
				addPrim(cdb->ot+otz, poly);
				//printf("%d,",otz);
			}else {
				printf("otz=%d",otz);
			
			}
			
			
		}
*/
		
		// add Poly 3s
		
		for( i = 0 ; i < numPoly3s ; i++ )
		{
			short *vi;
			POLY_G3 *poly;
			int isomote = 0;
			long	p, otz, opz, flg;

			poly = cdb->poly3s+i;
			vi = &body_polys[i*4]; // model Input. *4 because there are 4 shorts for each poly: vi1,vi2,vi3,material
			
			isomote = RotAverageNclip3(&body_vertices[*(vi)], &body_vertices[*(vi+1)], &body_vertices[*(vi+2)],
			(long *)&poly->x0, (long *)&poly->x1,(long *)&poly->x2,
			&p,&otz,&flg);

			if (isomote <= 0) continue;
			if (otz >= 0 && otz < OTSIZE) 
			{
				CVECTOR color1,color2,color3;
				CVECTOR *colorIn = &materials[*(vi+3)];
				
				// NormalColorCol destroys the .code on poly, so set it manually afterwards.
				// The code for POLY_G3 is 0x30. 
				// From libgpu.h:
				// setPolyG3(p)	setlen(p, 6),  setcode(p, 0x30)
	
				NormalColorCol(&body_normals[*(vi)], colorIn, (CVECTOR*)&poly->r0);
				NormalColorCol(&body_normals[*(vi+1)], colorIn, (CVECTOR*)&poly->r1);
				NormalColorCol(&body_normals[*(vi+2)], colorIn, (CVECTOR*)&poly->r2);
				poly->code = 0x30;
	
				addPrim(cdb->ot+otz, poly);
	
			} else {
				printf("otz=%d",otz);
			
			}
			
		}
	

		DrawSync(0);
		VSync(0);

		PutDrawEnv(&cdb->draw); /* update drawing environment */
		PutDispEnv(&cdb->disp); /* update display environment */
		DrawOTag(cdb->ot+OTSIZE-1);	  /* draw */
	//return;
	}
	
	free(db[0].poly4s);
	free(db[1].poly4s);
	
	free(db[0].poly3s);
	free(db[1].poly3s);

}