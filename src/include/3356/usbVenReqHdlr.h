/* usbVenReqHdlr.h - Dataton 3356 Pickup
**
** Handles USB vendor specific requests.
** 
**
** Created 10-10-28	Kalle
**
** (C) Copyright Dataton AB 2010, All Rights Reserved
**
**/
#ifndef _usbVenReqHdlr_
#define _usbVenReqHdlr_

//---------------------------------TYPEDEFS---------------------------------------
typedef enum
{
	KVR_Reset,				// Request USB reset, enumeration
	KVR_BattChg,			// Permit/Forbid charging of battery
	KVR_IndData				// Force PU to indicate data
} vrEvent;

typedef struct _VREvent
{
	vrEvent		what;
	Boolean		value;
} VREvent;

//-------------------------------DEFINITIONS--------------------------------------
void usbVenReq_Init();
void usbVenReq_Maintenace(void);

#endif		// _usbVenReqHdlr_
