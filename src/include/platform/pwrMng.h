/* pwrMng.h - Dataton 33xx
**
** Power Manager - Handle hardware
**
**
** Created 10-10-30	Kalle
**
** (C) Copyright Dataton AB 2010, All Rights Reserved
**
*/


#ifndef _pwrMng_
#define _pwrMng_


void pwrMngInit(void);
void pwrBattCharge(Boolean enable);

Boolean pwrIsBattChargeEnabled(void);


#endif	// _pwrMng_

