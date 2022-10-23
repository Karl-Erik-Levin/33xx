/* micAppl.c - Dataton 3397 Transponder 
**
**	Microphone application
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
#include "driver\fmtDrv.h"
#include "driver\hpcDrv.h"

#include "platform\FreeRTOS\FreeRTOS.h"
#include "platform\FreeRTOS\task.h"
#include "platform\evtMng.h"
#include "platform\ledMng.h"

#include "3397\micAppl.h"

//----------------------------GLOBAL VARIABLES------------------------------------
static Word myFreq;

//--------------------------------FUNCTIONS---------------------------------------
static void
dispatchKBEvent(EMEvent *evt)
{
	TRACE_MIC(("KBEvent: ");)
	
	switch (evt->sw.key) {
	case ESWK_KeyLeft:
		TRACE_MIC(("Key Left  ");)
		break;
	case ESWK_KeyRight:
		TRACE_MIC(("Key Right ");)
		break;
	}
	
	switch (evt->sw.event) {
	case ESWE_KeyDown:
		TRACE_MIC(("Down  ");)
		break;
	case ESWE_KeyUp:
		TRACE_MIC(("Up ");)
		break;
	}
	
	TRACE_MIC(("\r\n");)
}

static void
dispatchIREvent(EMEvent *evt)
{
	switch (evt->ir.what) {
	
	case KEHSC_IDSet:
		if ((evt->ir.id>=880) && (evt->ir.id<=1080)) {
			myFreq = evt->ir.id;
			fmtSetFreq(myFreq);
			
			ledDisplay(kLedRing);
			TRACE_MIC(("IR Set     %04d\r\n", myFreq);)
			irAnswSetID(myFreq);
			}
		break;
	}
}

static void
micClose()
{
	ledDisplay(kLedEnd);
	irClose();
	fmtOff();
}

static void
micInit()
{
	TRACE_MIC(("Application Microphone\r\n");)

	hpcReleaseUSB();					// Select audio signal through headphone connector
	
	fmtOn(EFMT_TxAnalogInput);			// Enable FM transmitter
	if ((myFreq<880) || (myFreq>1080))
		myFreq = 1002;					// Default frequency = 100.2 MHz
	fmtSetFreq(myFreq);					// Set transmitter frequency
	
	irInit();
	ledDisplay(kLedInit);
}

void
micAppl(void)
{
	EMEvent evt;

	micInit();
	
	while (hpcSense(true)==EHPC_MIC) {
		cpuKickWatchDog();
		vTaskDelay(10);
		
		while (EM_GetNextEvent(&evt, 0)) {
			switch (evt.source) {
			case KEVS_Keyboard:	dispatchKBEvent(&evt);
				break;
			case KEVS_IRCom:	dispatchIREvent(&evt);
				break;
			}
		}
		
		ledDisplay(kLedMaint);
	}
	
	micClose();
}



