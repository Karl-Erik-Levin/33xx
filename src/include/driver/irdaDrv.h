/* irdaDrv.h - Dataton 33xx
**
** IR Communication Driver.
**
**
** Created 09-09-24	Kalle
**
** (C) Copyright Dataton AB 2009, All Rights Reserved
**
*/

#ifndef _irdaDrv_
#define _irdaDrv_

//---------------------------------DEFINES----------------------------------------
#define IR_MESSAGE_SIZE				32

//---------------------------------TYPEDEFS---------------------------------------
typedef Boolean (*IrDaEventFunc)(Byte msgSize, Byte *msg);

//--------------------------------PROTOTYPS---------------------------------------
void irdaInit(IrDaEventFunc msgRcvCB);
void irdaClose();
void irdaSleep();
void irdaWake();

Boolean irdaSend(Byte *msg,	Byte msgSize, Boolean irPwr);

#endif	// _irdaDrv_
