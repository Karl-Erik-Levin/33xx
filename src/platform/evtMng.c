/* evtMng.c
**
**	Event manager
**	New manager built on FreeRTOS queue object
**
**
** Created 09-10-28	Kalle
**
** (C) Copyright Dataton AB 2009, All Rights Reserved
*/

//---------------------------------INCLUDES---------------------------------------
#include "MFLib\Dataton_Types.h"

#include "platform\evtMng.h"

#include "platform\FreeRTOS\FreeRTOS.h"
#include "platform\FreeRTOS\queue.h"

//---------------------------------DEFINES----------------------------------------
#define EVT_QUEUE_LENGTH 8

//---------------------------------TYPEDEFS---------------------------------------

//----------------------------GLOBAL VARIABLES------------------------------------
static xQueueHandle evtQueue;

//-------------------------------FUNCTIONS--------------------------------------
void
EM_Init()
{
	evtQueue = xQueueCreate(EVT_QUEUE_LENGTH, sizeof(EMEvent));
}

Boolean
EM_PostEvent(EMEvent *event, LongWord ticksToWait)
{
	return (xQueueSendToBack(evtQueue, event, ticksToWait) == pdPASS);
}

Boolean
EM_GetNextEvent(EMEvent *event, LongWord ticksToWait)
{
	return (xQueueReceive(evtQueue, event, ticksToWait) == pdPASS);
}


