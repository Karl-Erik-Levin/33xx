/* pcmAppl.c - Dataton 3397 Transponder 
**
**	PCM application. Just for test, send PCM audio to Pickup D/A converter
** 		
**
** Created 09-09-24	Kalle
**
** (C) Copyright Dataton AB 2009, All Rights Reserved
**
**/

//---------------------------------INCLUDES---------------------------------------
#include "MFLib\Dataton_Types.h"
#include "MFLib\TraceAssert.h"

#include "driver\audDrv.h"
#include "driver\cpuDrv.h"
#include "driver\hpcDrv.h"
#include "driver\ledDrv.h"

#include "platform\FreeRTOS\FreeRTOS.h"
#include "platform\FreeRTOS\task.h"

#include "3397\pcmMng.h"
#include "3397\micAppl.h"

//--------------------------------FUNCTIONS---------------------------------------
static void
micClose()
{
	Word idx;
	
	for (idx=0; idx < LEDD_NUM_RING_LEDS+1; idx++)
		ledSetLedLevel(idx, LEDD_LEVEL_MIN);

#if (HARDWARE == 3356)
	PB_Close();
	audClose();
#endif
}

static void
micInit()
{
	TRACE_MIC(("Application Microphone\r\n");)

	hpcReleaseUSB();			// Select audio signal through headphone connector

#if (HARDWARE == 3356)
	audInit();
	PB_Init();
	PB_SetVolume(50);
	audSpeakerAmp(false);		// Disable speaker
	
	PB_Play(44100);
#endif
}

void
micAppl(void)
{
	LongWord cnt1 = 0, cnt2 = 0;
	
	micInit();
	
	while (hpcSense(true)==EHPC_MIC) {
		cpuKickWatchDog();
		vTaskDelay(10);
		
		if (!(cnt1%10)) {
			if (cnt2%2)
				ledSetLedLevel(LEDI_Center, LEDD_LEVEL_MAX);
			else
				ledSetLedLevel(LEDI_Center, LEDD_LEVEL_MIN);
			
			cnt2++;
		}
		cnt1++;	
	}
	
	micClose();
}



