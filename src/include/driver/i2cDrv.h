/* i2cDrv.h - Dataton 33xx 
**
**	I2C Driver
** 		
**	Handle AT91SAM7S I2C interface. Transmitting and reciving data on I2C-bus
** 		
**
** Created 09-09-24	Kalle
**
** (C) Copyright Dataton AB 2009, All Rights Reserved
**
**/


#ifndef _i2cDrv_
#define _i2cDrv_

//---------------------------------DEFINES----------------------------------------
typedef enum
{
	EI2CD_4721,			// FM Radio
	EI2CD_9850,			// Audio D/A
	EI2CD_6964,			// LEDs
	EI2CD_3567,			// Power/Battery Manager
	_EI2CD_NumDevices
} EI2CDevice;

//--------------------------------PROTOTYPS---------------------------------------
void i2cInit();
void i2cWrite(EI2CDevice device, LongWord iAddr, Byte *buf, Word numBytes);
void  i2cRead(EI2CDevice device, LongWord iAddr, Byte *buf, Word numBytes);
void i2cSleep();
void i2cWake();

#endif	// _i2cDrv_ 
