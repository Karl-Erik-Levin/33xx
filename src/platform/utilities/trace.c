/* trace.c - Dataton 3397 Transponder 
**   
** Trace and Assert
**
** Dependencies:
** Serial driver  (srlDrv)
**
** Created 09-09-24	Kalle
**
** (C) Copyright Dataton AB 2009, All Rights Reserved
**
*/

//---------------------------------INCLUDES---------------------------------------
#include <stdio.h>
#include <stdarg.h>

#include "MFLib\Dataton_Types.h"
#include "MFLib\TraceAssert.h"

#include "driver\srlDrv.h"

//---------------------------------DEFINES----------------------------------------
#define TRACE_BUF_SIZE (256)

//------------------------------GLOBAL VARIABLES----------------------------------
static char traceBuf[TRACE_BUF_SIZE];

//--------------------------------FUNCTIONS---------------------------------------
/*******************************************************************************
 * Function:	TracePrintf
 * Summary:		Print to debug
 *******************************************************************************/
void
TracePrintf(const char *fmt, ...) {
	int cnt;
	va_list va;
	
	va_start(va, flags);	
	cnt = vsprintf(traceBuf, fmt, va);
	ASSERT(cnt < sizeof(traceBuf), ("TracePrintf: message size > TRACE_BUF_SIZE"));
			
	//va_end(va);
	srlSendString(traceBuf);
}

/*******************************************************************************
 * Function:	MyAssert
 * Summary:		The ASSERT function for the firmware
 *******************************************************************************/
void
MyAssert(const char *msg) {
	TracePrintf(msg);
    while (1)
		;		// Stay here until watchdog reset
}

/*******************************************************************************
 * Function:	MySuperAssert
 * Summary:		The ASSERT function with variable arguments  
 *******************************************************************************/
void
MySuperAssert(const char *fmt, ...) {
	va_list vl;
	
	va_start(vl, flags);	
	vsprintf(traceBuf, fmt, vl);
	va_end(vl);
	
	MyAssert(traceBuf);
}
