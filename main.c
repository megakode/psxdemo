#include <sys/types.h>
#include <libetc.h>
#include <libgte.h>
#include <libgpu.h>
#include <libmath.h>
#include <Gtemac.h>
#include <Inline_c.h>
#include <libgs.h>

#include <libapi.h>

#include "land.h" 		// Land fly-over intro scene
//#include "cubescroll.h"	// Cube scroller
#include "model.h"	// show picture
//#include "stars.h"

//#include "war320.h"		// GFX: war of the worlds picture data
#include "dsrpsx.h" 		// GFX: Desire PSX logo

#include "hitmod/hitmod.h"
#include "pal.h"

int main()
{
	//MOD_Init();
	//MOD_Load((unsigned char*)pal);
	
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
	
	// Sometimes these 3 lines makes the PSX crash. Reset the h/w, that sometimes helps (!?)
	
	
	//MOD_Start();
	
	//doPicture((u_long*)dsrpsx,256,200);
	
	//doStars();
	//doCubes();
	
	//doLandscape();
	doModel();
}