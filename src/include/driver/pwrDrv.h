/* pwrDrv.h - Dataton 33xx
**
**	Power Driver
** 		
**	Handle power and battery charger circuit LTC3567
**
**
** (C) Copyright Dataton AB 2009, All Rights Reserved
**
*/

#ifndef _pwrDrv_
#define _pwrDrv_

//---------------------------------DEFINES----------------------------------------
typedef enum {
	ECRM_100MA,
	ECRM_1000MA,
	ECRM_SUSP,
	ECRM_500MA
} USBCurrentMode;

typedef enum {
	ESWK_KeyAction,
	ESWK_KeyLeft,
	ESWK_KeyRight
} SWkey;

typedef enum {
	ESWE_KeyDown,
	ESWE_KeyUp
} SWevt;

typedef struct _SWEvent
{
	SWkey		key;
	SWevt		event;	
} SWEvent;

//--------------------------------PROTOTYPS---------------------------------------
void pwrInit(void);
void pwrDownRequest(void);
void pwrBattChargeEnable(Boolean enable);

void LTC3567BuckBoostEnable(Boolean enable);
void LTC3567BurstMode(Boolean burstOn);
void LTC3567CurrentMode(USBCurrentMode mode);
void LTC3567ServoVoltage(Byte mode);

#endif	// _pwrDrv_



