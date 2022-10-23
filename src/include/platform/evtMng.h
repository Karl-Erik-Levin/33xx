/* evtMng.h
**
**	Event manager
**	New manager built on FreeRTOS queue object
**
**
** Created 09-10-28	Kalle
**
** (C) Copyright Dataton AB 2009, All Rights Reserved
*/

#ifndef _evtMng_
#define _evtMng_

//---------------------------------INCLUDES---------------------------------------
#include "3356\usbVenReqHdlr.h"

#include "platform\irMng.h"

#include "driver\pwrDrv.h"

//---------------------------------TYPEDEFS---------------------------------------
// Sources that may generate events.
typedef enum
{
	KEVS_Keyboard,
	KEVS_IRCom,
	KEVS_PowerManager,
	KEVS_PotControl,
	KEVS_VendorRequest,
	_KEVS_NumEventSources
} EventSource;

typedef struct _EMEvent
{
	EventSource source;
	union {
		IRCEvent ir;
		SWEvent  sw;
		VREvent  vr;
	};
} EMEvent;

//-------------------------------PROTOTYPES--------------------------------------
void EM_Init();
Boolean EM_PostEvent(EMEvent *event, LongWord ticksToWait);
Boolean	EM_GetNextEvent(EMEvent *event, LongWord ticksToWait);

#endif	// _evtMng_
