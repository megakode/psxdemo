/*
 Tip on how to do dynamic light using precalculated dot products:

sbeam: what do you do with normals in your vertex animation system?

XL2: vertex normal, which is in fact a precomputed dotproduct table, giving me realtime lighting for nearly free
The normals are in fact only an index (uint8) to a table of 32x168 dot product results. 
The 32 being 32 possible rotations and 168 simply being the number of normals I have (just use a sphere in Blender and have fewer than 256 vertices. It should give you 168 vertices)
So when I have a realtime light, I just need to update the distance and direction from the light to the model, 
add it to my model's rotation and shift it right to limit my amount of possible results to 32.  
Then it's just a matter of setting your index pointer (the 32x168 array) to the right place. And voilà ! Nearly free real time light for every single entity, even the flying body parts.

sbeam: aaah... and by rotations, you mean only around one axis? the y axis i.e.

XL2: Yeah, since enemy models only rotate on the y axis. You could support more axis with a more complex lookup table.

sbeam: do you have a fast way of looking up the distance/direction then?
XL2: You only do it for the whole model instead of per vertex, so only once per model. If your model has say 200 vertices, instead of rotating 200 normals and doing 200 dotproducts, you just use a quick look up table
XL2: And if the dynamic light is too far, you can ignore it and just use the static light value, which you could simply get from the floor under you and add your rotation value to add some variety when you move around


sbeam: so something like dotLookup[ normalindex ][ ratan2(lightx - objx, lighty-objy) >> 5 ]
*/

#include <sys/types.h>
#include <libetc.h>
#include <libgte.h>
#include <libgpu.h>
#include <libmath.h>
#include <Gtemac.h>
#include <Inline_c.h>
#include <libgs.h>
#include <libapi.h>

#include "dsrlib.h"	// Desire lib
#include "crates.h" // crates.tim texture file
#include "crash.h"	// 3D model: Crash bandicoot
//#include "crash_jump.h"	// 3D model: Crash bandicoot
#include "ball.h"	// 3D model: Amiga ball

#define OTSIZE 4096

typedef struct {		
	DRAWENV		draw;			/* drawing environment */
	DISPENV		disp;			/* display environment */
	u_long		ot[OTSIZE];		/* ordering table */
	POLY_G3	*poly3s;	/* mesh cell */
} DB;

typedef struct {
	VECTOR *position;
	SVECTOR *rotation;
	CVECTOR *materials;
	SVECTOR *vertices;
	SVECTOR *normals;
	short *polys; // format: vi(0), vi(1), vi(2), materialIndex (vi = Vertex Index = normal index)
	int polyCount; // Number of polygons
} Model;

typedef struct {
	Model *model;
	SVECTOR *vertices;
	int numberOfVertices;
	int numberOfFrames;
	int currentFrame;
} Animation;

typedef struct {
	VECTOR position;
	SVECTOR rotation;
} CameraPreset;

typedef struct {
	const CameraPreset *from;
	const CameraPreset *to;
} CameraAnimationPreset;

typedef struct {
	VECTOR fromRot;	// From rotation vector
	VECTOR fromPos;	// From position vector
	VECTOR toPos;	// To position vector
	VECTOR toRot;	// To rotation vector
	VECTOR currentPos;	// Current position in the animation
	VECTOR currentRot;	// Current rotation in the animation
	VECTOR deltaPos; 	// The delta amount to add to currentPos for every animation step
	VECTOR deltaRot;	// The delta amount to add to currentRot for every animation step
	int currentStep;
	int numTotalSteps;
} CameraAnimation;

DB	db[2];		/* double buffer */
DB	*cdb;		/* current double buffer */
u_long	*ot;	/* current OT */

/*************************************************************/
// Light stuff
/*************************************************************/
// Scale vector: 4096 = 1.0
VECTOR Scale = { 4096,4096,4096, 0 };

static const SVECTOR lightDirVec = {0,0,4000,0};
//static SVECTOR lightDirVec = {0,0,-4096}; // Directly into screen

/* Local Light Matrix */
static MATRIX	llm = {0,0,0,
				   0,0,0,
				   0,0,0,
				   0,0,0};

	/* Local Color Matrix */
static MATRIX	lcm = {4096,0,0, 
				   4096,0,0, 
				   4096,0,0, 
				   0,0,0};

void drawPolys( Model *model , POLY_G3 *dstPrimitive, MATRIX *worldMatrix)
{
	int i;
	SVECTOR pos;
	MATRIX m = {ONE,0,0,
				0,ONE,0,
				0,0,ONE,
				0,0,0};
	VECTOR worldPos = { 0,0,0,0 };
	VECTOR worldRot = { 0,0,0,0 };
	MATRIX modelmatrix;
	MATRIX inverseLightMatrix;
	MATRIX ls;
	//PushMatrix();

	RotMatrixZYX(model->rotation, &modelmatrix); // Calculate rotation matrix from vector
	TransMatrix(&modelmatrix,model->position);

	// Create a rotation matrix for the light
	TransposeMatrix(&modelmatrix, &inverseLightMatrix);
	ApplyMatrixSV(&inverseLightMatrix,(SVECTOR*)&lightDirVec,(SVECTOR*)&llm);

	/* make local-screen coordinate */
	CompMatrix(worldMatrix, &modelmatrix, &ls);

	/* set matrix*/
	SetRotMatrix(&ls);		
	SetTransMatrix(&ls);
	
	SetColorMatrix(&lcm);
	SetLightMatrix(&llm);

	//printf("model->rot = %d,%d,%d modelmatrix=%d,%d,%d mout=%d,%d,%d \n", model->rotation[0], model->rotation[1], model->rotation[2],
	//printf("m = %d,%d,%d worldMatrix=%d,%d,%d mout=%d,%d,%d \n", m.t[0],m.t[1],m.t[2] ,worldMatrix->t[0],worldMatrix->t[1],worldMatrix->t[2],mout.t[0],mout.t[1],mout.t[2]);

	for( i = 0 ; i < model->polyCount ; i++ )
	{
			short *vi;
			POLY_G3 *poly;
			int isomote = 0;
			long	p, otz, opz, flg;

			poly = dstPrimitive+i;
			vi = &model->polys[i*4]; // model Input. *4 because there are 4 shorts for each poly: vi1,vi2,vi3,material
			
			isomote = RotAverageNclip3(&model->vertices[*(vi)], &model->vertices[*(vi+1)], &model->vertices[*(vi+2)],
			(long *)&poly->x0, (long *)&poly->x1,(long *)&poly->x2,
			&p,&otz,&flg);

			if (isomote <= 0) continue;
			if (otz >= 0 && otz < OTSIZE) 
			{
				CVECTOR color1,color2,color3;
				CVECTOR *colorIn = &model->materials[*(vi+3)];
				
				// NormalColorCol destroys the .code on poly, so set it manually afterwards.
				// The code for POLY_G3 is 0x30. 
				// From libgpu.h:
				// setPolyG3(p)	setlen(p, 6),  setcode(p, 0x30)
	
				NormalColorCol(&model->normals[*(vi)], colorIn, (CVECTOR*)&poly->r0);
				NormalColorCol(&model->normals[*(vi+1)], colorIn, (CVECTOR*)&poly->r1);
				NormalColorCol(&model->normals[*(vi+2)], colorIn, (CVECTOR*)&poly->r2);
				poly->code = 0x30;
	
				addPrim(cdb->ot+otz, poly);
	
			} else {
				printf("otz=%d",otz);
			
			}
			
		}

		//PopMatrix();
	
}

void updateAnimation(Animation *animation)
{

	if(animation->currentFrame >= animation->numberOfFrames )
	{
		animation->currentFrame = 0;
	}

	animation->model->vertices = animation->vertices + animation->currentFrame * animation->numberOfVertices;
	animation->currentFrame+=1;
}

const int fixedPointDecimals = 0;

/*
 Create and set a world matrix based on a CameraAnimation
*/
void playCameraAnimation( const CameraAnimationPreset *preset , CameraAnimation *outputAnimation, int numberOfSteps)
{
	int deltaRotX,deltaRotY,deltaRotZ;

	outputAnimation->numTotalSteps = numberOfSteps;
	outputAnimation->currentStep = 0;

	// Store from/to rotation and position, and convert them to fixed point, so we can make a smoother LERP with them

	outputAnimation->fromRot.vx = preset->from->rotation.vx << fixedPointDecimals;
	outputAnimation->fromRot.vy = preset->from->rotation.vy << fixedPointDecimals;
	outputAnimation->fromRot.vz = preset->from->rotation.vz << fixedPointDecimals;
	outputAnimation->toRot.vx = preset->to->rotation.vx << fixedPointDecimals;
	outputAnimation->toRot.vy = preset->to->rotation.vy << fixedPointDecimals;
	outputAnimation->toRot.vz = preset->to->rotation.vz << fixedPointDecimals;

	outputAnimation->fromPos.vx = preset->from->position.vx << fixedPointDecimals;
	outputAnimation->fromPos.vy = preset->from->position.vy << fixedPointDecimals;
	outputAnimation->fromPos.vz = preset->from->position.vz << fixedPointDecimals;
	outputAnimation->toPos.vx = preset->to->position.vx << fixedPointDecimals;
	outputAnimation->toPos.vy = preset->to->position.vy << fixedPointDecimals;
	outputAnimation->toPos.vz = preset->to->position.vz << fixedPointDecimals;

	// Calculate position deltas

	outputAnimation->deltaPos.vx = (outputAnimation->toPos.vx - outputAnimation->fromPos.vx) / numberOfSteps;
	outputAnimation->deltaPos.vy = (outputAnimation->toPos.vy - outputAnimation->fromPos.vy) / numberOfSteps;
	outputAnimation->deltaPos.vz = (outputAnimation->toPos.vz - outputAnimation->fromPos.vz) / numberOfSteps;

	// Calculate rotation deltas

	deltaRotX = (preset->to->rotation.vx - preset->from->rotation.vx);
	deltaRotY = (preset->to->rotation.vy - preset->from->rotation.vy);
	deltaRotZ = (preset->to->rotation.vz - preset->from->rotation.vz);

	// If delta rotation is more than a half circle, take the other way around
	if(deltaRotX>2048){
		outputAnimation->deltaRot.vx =( ((deltaRotX-2048)*-1)<<fixedPointDecimals ) / numberOfSteps;
	} else {
		outputAnimation->deltaRot.vx = (deltaRotX<<fixedPointDecimals) / numberOfSteps;
	}

	if(deltaRotY>2048){
		outputAnimation->deltaRot.vy = (((deltaRotY-2048)*-1)<<fixedPointDecimals) / numberOfSteps;
	} else {
		outputAnimation->deltaRot.vy = (deltaRotY<<fixedPointDecimals) / numberOfSteps;
	}

	if(deltaRotZ>2048){
		outputAnimation->deltaRot.vz = (((deltaRotZ-2048)*-1)<<fixedPointDecimals) / numberOfSteps;
	} else {
		outputAnimation->deltaRot.vz = (deltaRotZ<<fixedPointDecimals) / numberOfSteps;
	}

	printf("playCameraAnimation delta pos = %d,%d,%d rot = %d,%d,%d \n", outputAnimation->deltaPos.vx, outputAnimation->deltaPos.vy, outputAnimation->deltaPos.vz, outputAnimation->deltaRot.vx, outputAnimation->deltaRot.vy, outputAnimation->deltaRot.vz);

	outputAnimation->currentPos = outputAnimation->fromPos;
	outputAnimation->currentRot = outputAnimation->fromRot;

}

void updateCameraAnimation( CameraAnimation *animation, MATRIX *outputMatrix )
{
	if(animation->currentStep >= animation->numTotalSteps){
		return;
	}

	animation->currentStep++;

	// X pos

	if( animation->deltaPos.vx < 0 )
	{
		if(animation->currentPos.vx > animation->toPos.vx )
		{
			animation->currentPos.vx += animation->deltaPos.vx;
		}
	} 
	else 
	{
		if(animation->currentPos.vx < animation->toPos.vx )
		{
			animation->currentPos.vx += animation->deltaPos.vx;
		}
	}

	// Y pos	
	
	if( animation->deltaPos.vy < 0 )
	{
		if(animation->currentPos.vy > animation->toPos.vy )
		{
			animation->currentPos.vy += animation->deltaPos.vy;
		}
	} 
	else 
	{
		if(animation->currentPos.vy < animation->toPos.vy )
		{
			animation->currentPos.vy += animation->deltaPos.vy;
		}
	}

	
	// Z pos
	
	if( animation->deltaPos.vz < 0 )
	{
		if(animation->currentPos.vz > animation->toPos.vz )
		{
			animation->currentPos.vz += animation->deltaPos.vz;
		}
	} 
	else 
	{
		if(animation->currentPos.vz < animation->toPos.vz )
		{
			animation->currentPos.vz += animation->deltaPos.vz;
		}
	}

	// X rotation

	if( animation->deltaRot.vx < 0)
	{
		if(animation->currentRot.vx > animation->toRot.vx)
		{
			animation->currentRot.vx += animation->deltaRot.vx;
		}
	} else 
	{
		if(animation->currentRot.vx < animation->toRot.vx)
		{
			animation->currentRot.vx += animation->deltaRot.vx;
		}
	}

	// Y rotation

	if( animation->deltaRot.vy < 0)
	{
		if(animation->currentRot.vy > animation->toRot.vy)
		{
			animation->currentRot.vy += animation->deltaRot.vy;
		}
	} else 
	{
		if(animation->currentRot.vy < animation->toRot.vy)
		{
			animation->currentRot.vy += animation->deltaRot.vy;
		}
	}

	// Z rotation

	if( animation->deltaRot.vz < 0)
	{
		if(animation->currentRot.vz > animation->toRot.vz)
		{
			animation->currentRot.vz += animation->deltaRot.vz;
		}
	} else 
	{
		if(animation->currentRot.vz < animation->toRot.vz)
		{
			animation->currentRot.vz += animation->deltaRot.vz;
		}
	}

	{
		SVECTOR roundedRotation;
		VECTOR roundedPosition;

		roundedRotation.vx = animation->currentRot.vx >> fixedPointDecimals;
		roundedRotation.vy = animation->currentRot.vy >> fixedPointDecimals;
		roundedRotation.vz = animation->currentRot.vz >> fixedPointDecimals;

		roundedPosition.vx = animation->currentPos.vx >> fixedPointDecimals;
		roundedPosition.vy = animation->currentPos.vy >> fixedPointDecimals;
		roundedPosition.vz = animation->currentPos.vz >> fixedPointDecimals;
		
		RotMatrix_gte(&roundedRotation, outputMatrix); // Calculate rotation matrix from vector
		TransMatrix(outputMatrix,&roundedPosition);
	}
}

int doModel()
{
	const static int screenWidth = 320;
	const static int screenHeight = 256;
	const static int BALL_RADIUS = 184;

	const CameraPreset cameraRunRightBegin 	= { {-400,170,1100,0} , {0,-590,0,0} };
	const CameraPreset cameraRunRightEnd 	= { {1000,170,730,0} , {0,-890,0,0} };

	const CameraPreset cameraRunLeftBegin = {{400,220,1100,0},{0,860,0,0}};
	const CameraPreset cameraRunLeftEnd = {{-1000,220,1100,0},{0,860,0,0}};

	const CameraPreset cameraFacingCamera 	= { { 0,110,200,0} , {0,20,0,0} };
	const CameraPreset cameraFacingCameraZoomedALittleMore 	= { { 0,280,310,0} , {-600,20,0,0} };
	const CameraPreset cameraDefault 			= { { 0,100,1000,0 } , { 0,0,0,0 } };
	const CameraPreset cameraFacingRight 		= { {-10,110,150,0} , {0,-970,0,0} };

	const CameraAnimationPreset cameraAnimationRunRight = { &cameraRunRightBegin , &cameraRunRightEnd };
	const CameraAnimationPreset cameraAnimationRunLeft = { &cameraRunLeftBegin , &cameraRunLeftEnd };

	const CameraAnimationPreset cameraAnimationZoomFace = { &cameraFacingCamera, &cameraFacingCameraZoomedALittleMore };
	
	// current camera animation
	CameraAnimation cameraAnimation =  {0,0,0,0,0,0,0,0,0,0}; 

	// Current camera
	CameraPreset camera = cameraDefault;


	int numModels = 2;
	int ballvelocity = -10;
	VECTOR ballposition = {0,-BALL_RADIUS,300,0};
	SVECTOR ballRotation = {0,0,0,0};
	VECTOR bodyposition = {0,0,0,0};
	SVECTOR bodyrotation = {0,0,0,0};
	Model modelBody = { &bodyposition, &bodyrotation, crashbandicoot_materials ,crashbandicoot_vertices , crashbandicoot_normals, crashbandicoot_polys, crashbandicoot_poly_count};
	Model modelBall = { &ballposition, &ballRotation, ball_materials ,ball_vertices , ball_normals, ball_polys, ball_poly_count};
	Model* models[] = {  &modelBall, &modelBody };
	Animation animation = { &modelBody, crashbandicoot_vertices, crashbandicoot_vertex_count, crashbandicoot_frame_count, 0 };

	int frames = 0;
	int numPoly3s = 0;

	{
		int i;
		int totalPolyCount = 0;
		for( i = 0 ; i < numModels ; i++)
		{
			totalPolyCount += models[i]->polyCount;
		}
		numPoly3s = totalPolyCount;
	}

	
	/*
	InitGeom();
	
	SetDispMask(0);
	
	ResetGraph(0);
	ResetCallback();
	PadInit(0);
	SetGraphDebug(0);
	
	InitGeom();
	
	SetGeomScreen(512);
	SetVideoMode(MODE_PAL);

	SetDispMask(1);
	*/

	SetGeomOffset(160, 128);
	SetGeomScreen(512);
	
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
	
	//SetBackColor(100,100,100);
	
	// Allocate polygon structs and initialize them
	
	{
		int i = 0;
		
		db[0].poly3s = (POLY_G3*)malloc(sizeof(POLY_G3)*numPoly3s);
		db[1].poly3s = (POLY_G3*)malloc(sizeof(POLY_G3)*numPoly3s);

		for(i=0;i<numPoly3s;i++)
		{
			setPolyG3( (db[0].poly3s+i) );
			setPolyG3( (db[1].poly3s+i) );
			SetSemiTrans( (db[0].poly3s+i) , 0);
			SetSemiTrans( (db[1].poly3s+i) , 0);
		}
		
	}
			
	while(1)
	{
		static int animationTrigger = 0;
		static int steps = 0;
		
		MATRIX m;
		POLY_G3 *currentDstPoly;
		u_long pad;
		
		cdb = (cdb==db)? db+1: db;	/* swap double buffer ID */	
		ClearOTagR(cdb->ot, OTSIZE);	/* clear ordering table */
		
		pad = PadRead(0);
		
		if (pad & PADLup) camera = cameraDefault;
		if (pad & PADLleft)	camera = cameraFacingRight;
		if (pad & PADLdown)	camera = cameraFacingCamera;

		if (pad & PADRleft) playCameraAnimation(&cameraAnimationRunLeft,&cameraAnimation,200);
		if (pad & PADRright) playCameraAnimation(&cameraAnimationRunRight,&cameraAnimation,200);

		// Scripted steps

		if(steps==0){
			//playCameraAnimation(&cameraAnimationZoomFace,&cameraAnimation,200);
			playCameraAnimation(&cameraAnimationRunLeft,&cameraAnimation,200);
		}

		if( steps == 200 ){
			playCameraAnimation(&cameraAnimationRunRight,&cameraAnimation,200);
		}

		if( steps >= 400 )
		{
			camera = cameraFacingCamera;
			RotMatrix_gte(&camera.rotation, &m);
			TransMatrix(&m,&camera.position);
		}

		// Set camera

		if( cameraAnimation.currentStep < cameraAnimation.numTotalSteps ){
				updateCameraAnimation(&cameraAnimation,&m);
		} else {
			RotMatrix_gte(&camera.rotation, &m); // Calculate rotation matrix from vector
			TransMatrix(&m,&camera.position);
		}

		steps++;

		// update animation

		modelBall.rotation->vx += 10;
		modelBall.position->vy += ballvelocity;
		
		if(modelBall.position->vy >= -BALL_RADIUS){
			ballvelocity = -17;
		}

		ballvelocity+=1;

		animationTrigger += 1;
		if(animationTrigger == 2)
		{
			updateAnimation(&animation);
			animationTrigger = 0;
		}
		
		// add Poly 3s

		currentDstPoly = cdb->poly3s;

		{
			int mi = 0;
			for(mi = 0 ; mi < numModels ; mi++ ){
				drawPolys(models[mi], currentDstPoly , &m );
				currentDstPoly += models[mi]->polyCount;
			}
		}

		DrawSync(0);
		VSync(0);

		PutDrawEnv(&cdb->draw); /* update drawing environment */
		PutDispEnv(&cdb->disp); /* update display environment */
		DrawOTag(cdb->ot+OTSIZE-1);	  /* draw */
	//return;
	}
	
	free(db[0].poly3s);
	free(db[1].poly3s);

}