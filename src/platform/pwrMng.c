/* pwrMng.c - Dataton 33xx
**
** Power Manager - Handle hardware
**
**
** Created 10-10-30	Kalle
**
** (C) Copyright Dataton AB 2010, All Rights Reserved
**
*/

//---------------------------------INCLUDES---------------------------------------
#include "MFLib\Dataton_Types.h"
#include "MFLib\TraceAssert.h"

#include "driver\pwrDrv.h"

#include "platform\pwrMng.h"


static Boolean battChargeEnabled;


void
pwrMngInit(void)
{
	pwrBattCharge(false);
}

void
pwrBattCharge(Boolean enable)
{
	pwrBattChargeEnable(enable);
	battChargeEnabled = enable;
}

Boolean
pwrIsBattChargeEnabled(void)
{
	return battChargeEnabled;
}



