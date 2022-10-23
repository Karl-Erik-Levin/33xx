/* puusbAppl.h - Dataton 3356 Pickup
**
**	USB application
** 		
**
** Created 09-09-24	Kalle
**
** (C) Copyright Dataton AB 2009, All Rights Reserved
**
**/

#ifndef _puusbAppl_
#define _puusbAppl_

//---------------------------------TYPEDEFS---------------------------------------
typedef struct _chgSt
{
	Boolean		chargingAllowed;
	Boolean		batteryIsCharged;
} chgSt;

//-------------------------------DEFINITIONS--------------------------------------
void puusbAppl(void *bigBuffer);
void puusbBattChgStatus(chgSt *status);

#endif	//_puusbAppl_


