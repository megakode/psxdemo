#include <sys/types.h>
#include <libetc.h>
#include <libgte.h>
#include <libgpu.h>
#include <libmath.h>
#include <Gtemac.h>
#include <Inline_c.h>
#include <libgs.h>
#include <LIBCD.H>

#include <libapi.h>

#include "land.h" 		// Land fly-over intro scene
#include "cubescroll.h"	// Cube scroller
#include "model.h"	// show picture
#include "stars.h"
#include "binaryst.h"
#include "citadel.h"
#include "PICTURE.H"
//#include "war320.h"		// GFX: war of the worlds picture data
#include "dsrpsx.h" 		// GFX: Desire PSX logo

//#include "hitmod/hitmod.h"
//#include "pal.h"

int main()
{
	int tracks[] = { 2 };
	CdlLOC loc;
/*
	EnterCriticalSection();
	InitHeap3(0x80060A58,100000);
	ExitCriticalSection();
*/
	loc.minute = 0;
	loc.track = 2;
	
	SetDispMask(0);
	
	ResetGraph(0);
	CdInit ();			/* reset CD env. */
    //SpuInit ();			/* reset SPU env. */
	ResetCallback();
	PadInit(0);
	SetGraphDebug(0);
	
	InitGeom();
	SetGeomOffset(160, 128);
	SetGeomScreen(100);
	SetVideoMode(MODE_PAL);

	SetDispMask(1);
	
	SetFarColor(10,10,10);
	
	/*
	MOD_Init();
	
	// Returns 1 for success, 0 for failure 
	if(!MOD_Load((unsigned char*)pal)){
		printf("ERROR loading module!!!");
	};

	MOD_Start();
	*/
	CdPlay(2,tracks,0);
	//CdControl(CdlPlay, (u_char*)&loc, 0);


	
		
	doPicture((u_long*)dsrpsx,256,32,16,200);
/*
	doLandscape();

	{
		int delay = 60;
		while(delay--){
			VSync(0);		// wait for the next V-BLNK 
		}
	}

	doPicture((u_long*)binaryst,320,0,0,500);
*/	
	doStars();

	doPicture((u_long*)cita,320,0,0,500);
	
	doCubes();

	doModel();

}