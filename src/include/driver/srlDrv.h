/* srlDrv.h - Dataton 3397 Transponder 
**
** Driver to send C-style strings to debug serial port. Used for trace printout
** Driver will init itself on first call to srlSendString
**
** Created 09-09-24	Kalle
**
** (C) Copyright Dataton AB 2009, All Rights Reserved
**
*/


#ifndef _srlDrv_
#define _srlDrv_

void srlSendString(char *buffer);


#endif	// _srlDrv_

