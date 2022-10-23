/* irMng.h - Dataton 3397 Transponder 
**
** IR Communication Manager.
**
**
** Created 09-09-24	Kalle
**
** (C) Copyright Dataton AB 2009, All Rights Reserved
**
*/

#ifndef _irMng_
#define _irMng_

//---------------------------------TYPEDEFS---------------------------------------
typedef enum
{
	KEHSC_IDRequest,			// Someone request my ID
	KEHSC_IDSet,				// Set my ID
	KEHSC_IDReceived,			// ID received from transponder
	KEHSC_Timeout,				// No ID received within 
	KEHSC_Timestamp,			// Timestamp received
	KEHSC_AutoPlay,				// Autoplay flags received.
	KEHSC_IDAck					// ID received from transponder
} irEvent;

typedef struct _IRCEvent
{
	irEvent		what;
	Word		id;	
} IRCEvent;

//-------------------------------DEFINITIONS--------------------------------------
void irInit();
void irSleep();
void irWake();
void irClose();

void irAnswReqID(Word id);
void irAnswSetID(Word id);

#endif	// _irMng_
