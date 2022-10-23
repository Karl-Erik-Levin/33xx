/* usbAppl.c - Dataton 3397 Transponder 
**
**	USB application
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

#include "driver\cpuDrv.h"
#include "driver\hpcDrv.h"
#include "driver\ledDrv.h"
#include "driver\rtcDrv.h"
#include "driver\tmrDrv.h"
#include "driver/fmtDrv.h"

#include "platform\FreeRTOS\FreeRTOS.h"
#include "platform\FreeRTOS\task.h"
#include "platform\evtMng.h"
#include "platform\ledMng.h"

#include "3397\usbAppl.h"

//--------------------------------FUNCTIONS---------------------------------------
static void
displayTime()
{
	LongWord rt = rtcGetRtValue();
	Byte sec, min, hour;
	
	sec = (rt/4)    % 60;
	min = (rt/240)  % 60;
	hour= (rt/14400)% 24;
	
	TRACE_USB(("%02d:%02d:%02d\r", hour, min, sec);)
}

static void
dispatchKBEvent(EMEvent *evt)
{
	TRACE_USB(("KBEvent: ");)
	
	switch (evt->sw.key) {
	case ESWK_KeyLeft:
		TRACE_USB(("Key Left  ");)
		break;
	case ESWK_KeyRight:
		TRACE_USB(("Key Right ");)
		break;
	}
	
	switch (evt->sw.event) {
	case ESWE_KeyDown:
		TRACE_USB(("Down  ");)
		break;
	case ESWE_KeyUp:
		TRACE_USB(("Up ");)
		break;
	}
	
	TRACE_USB(("\r\n");)
}

static void
usbClose()
{
	ledDisplay(kLedEnd);
}

static void
usbInit()
{
	TRACE_USB(("Application USB\r\n");)
	
	hpcGoUSB();
	rtcSetRtValue(14*14400 + 52*240 + 26*4);
	
	ledDisplay(kLedInit);
}

void
usbAppl(void)
{
	LongWord cnt1 = 0;
	EMEvent evt;

	usbInit();
	
	// Initialize radio
	fmtInit();
	fmtOn( EFMT_TxDigitalInput );
	fmtSetFreq( 9200 );
	
	while (hpcSense(true)==EHPC_USB) {
		cpuKickWatchDog();
		vTaskDelay(10);

		if (EM_GetNextEvent(&evt, 0)) {
			switch (evt.source) {
			case KEVS_Keyboard:	  dispatchKBEvent(&evt);
				break;
			}
		}
		
		ledDisplay(kLedMaint);
		
		if (!(cnt1++%25)) {
			displayTime();
		}
	}

	// Turn off radio
	fmtOff();
	usbClose();
}


