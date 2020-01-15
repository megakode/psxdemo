// quick example of how to use the player in your own stuff

#include <sys/types.h>
#include <libetc.h>
#include <libgte.h>
#include <libgpu.h>
#include <libgs.h>
#include <kernel.h>

#include "hitmod.h"

#define PACKETMAX 10000
#define PACKETMAX2 (PACKETMAX*24)

#define OT_LENGTH 14	  

GsOT WorldOrderingTable[2];
GsOT_TAG zSortTable[2][1<<OT_LENGTH];   
PACKET GpuOutputPacket[2][PACKETMAX2];

int main(void)
{
unsigned int outputBufferIndex;

MOD_Init();
MOD_Load((unsigned char*)0x80100000);

SetVideoMode(MODE_PAL);
ResetGraph(0);
GsInitGraph(320, 256, GsOFSGPU|GsNONINTER, 0, 0);
GsDefDispBuff(0, 0, 0, 256);

WorldOrderingTable[0].length = OT_LENGTH;
WorldOrderingTable[1].length = OT_LENGTH;
WorldOrderingTable[0].org = zSortTable[0];
WorldOrderingTable[1].org = zSortTable[1];
	
FntLoad(320, 0);	
FntOpen(8, 0, 312, 248, 0, 512);	

MOD_Start();

while(1)
  {
  outputBufferIndex = GsGetActiveBuff();
  GsSetWorkBase((PACKET*)GpuOutputPacket[outputBufferIndex]);
  GsClearOt(0, 0, &WorldOrderingTable[outputBufferIndex]);
				
  FntPrint("        HITMOD PSX MOD PLAYER\n");
  FntPrint("          BY SILPHEED/HITMEN\n\n");
  FntPrint("             VERSION 1.5\n\n\n");
  FntPrint("ORDER: %3d    PATTERN: %3d    ROW: %2d\n", *MOD_CurrentOrder, *MOD_CurrentPattern, *MOD_CurrentRow);
  FntFlush(-1);
        				
  DrawSync(0);
  VSync(0);
  GsSwapDispBuff();
		
  GsSortClear(0, 0, 0, &WorldOrderingTable[outputBufferIndex]); 
						
  GsDrawOt(&WorldOrderingTable[outputBufferIndex]);
  }
	
return 0;
}



