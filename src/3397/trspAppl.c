/* trspAppl.c - Dataton 3397 Transponder 
**
**	Transponder application
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

#include "platform\FreeRTOS\FreeRTOS.h"
#include "platform\FreeRTOS\task.h"
#include "platform\evtMng.h"
#include "platform\irMng.h"
#include "platform\ledMng.h"
#include "platform\setMng.h"

#include "3397\trspAppl.h"

//---------------------------------TYPEDEFS---------------------------------------

//----------------------------GLOBAL VARIABLES------------------------------------
static Word myID;

//--------------------------------FUNCTIONS---------------------------------------
static void
dispatchKBEvent(EMEvent *evt)
{
	TRACE_TRS(("KBEvent: ");)
	
	switch (evt->sw.key) {
	case ESWK_KeyLeft:
		TRACE_TRS(("Key Left  ");)
		break;
	case ESWK_KeyRight:
		TRACE_TRS(("Key Right ");)
		break;
	}
	
	switch (evt->sw.event) {
	case ESWE_KeyDown:
		TRACE_TRS(("Down  ");)
		break;
	case ESWE_KeyUp:
		TRACE_TRS(("Up ");)
		break;
	}
	
	TRACE_TRS(("\r\n");)
}

static void
dispatchIREvent(EMEvent *evt)
{
	switch (evt->ir.what) {
	
	case KEHSC_IDRequest:
		TRACE_TRS(("IR Request %04d\r\n", myID);)
		irAnswReqID(myID);
		break;
		
	case KEHSC_IDAck:
		ledDisplay(kLedCenter);
		TRACE_TRS(("IR Ack\r\n");)
		break;
		
	case KEHSC_IDSet: {
		prdInfo *ai = prdInfoGet();
		
		ledDisplay(kLedRing);
		ai->irID = myID = evt->ir.id;
		prdInfoStore();
		
		TRACE_TRS(("IR Set     %04d\r\n", myID);)
		irAnswSetID(myID);
		break;
		}
	}
}

static void
trspClose()
{
	irClose();
	ledDisplay(kLedEnd);
}

static void
trspInit()
{
	prdInfo *ai = prdInfoGet();
	
	TRACE_TRS(("Application Transponder\r\n");)

	hpcReleaseUSB();		// Select audio signal through headphone connector

	myID = ai->irID;		// Get transponder ID
	irInit();
	ledDisplay(kLedInit);
}

void
trspAppl(void)
{
	EMEvent evt;
	Word pwrDwn = 0;
	
	trspInit();
	
	while (hpcSense(true)==EHPC_VOID) {
		cpuKickWatchDog();
		vTaskDelay(10);
		
		while (EM_GetNextEvent(&evt, 0)) {
			pwrDwn = 0;
			
			switch (evt.source) {
			case KEVS_Keyboard:	dispatchKBEvent(&evt);
				break;
			case KEVS_IRCom:	dispatchIREvent(&evt);
				break;
			}
		}
		
		ledDisplay(kLedMaint);
		
		if (++pwrDwn > 500)				// Powerdown 5 seconds after last event
			pwrDownRequest();
	}
	
	trspClose();
}



