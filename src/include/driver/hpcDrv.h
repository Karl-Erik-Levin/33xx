/* hpcDrv.h - Dataton 33xx
**
**	Headphone conector Driver
** 		
**
** Created 09-09-24	Kalle
**
** (C) Copyright Dataton AB 2009, All Rights Reserved
**
**/

#ifndef _hpcDrv_
#define _hpcDrv_

//---------------------------------TYPEDEFS---------------------------------------
typedef enum
{
	EHPC_VOID,
	EHPC_MIC,			// 3397
	EHPC_EARPHONE,		// 3356
	EHPC_USB
} EHPConnection;

//-------------------------------PROTOTYPES--------------------------------------
void hpcInit();
void hpcGoUSB();
void hpcReleaseUSB();
void hpcActivateUSBPullup(Boolean on);
EHPConnection hpcSense();
//void pup_on_off( hcc_u8 on );

#endif	// _hpcDrv_

