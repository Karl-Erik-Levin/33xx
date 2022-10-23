/* setMng.h - Dataton 33xx
**
** Product setting stored in processor flash memory
**
**
** Created 09-09-24	Kalle
**
** (C) Copyright Dataton AB 2009, All Rights Reserved
**
*/

#ifndef _setMng_
#define _setMng_

//---------------------------------DEFINES----------------------------------------
#define PRD_INFO_STRUCT_VERSION	(1)

//---------------------------------TYPEDEFS---------------------------------------
typedef  struct _prdInfo
{
	Word	 infoStructVersion;		// Version this struct.
	LongWord hwVer;					// hardware version.
	LongWord blVer;					// Bootloader version
	LongWord swVer;					// Software version. (Application)
	LongWord serialNumber;			// Serial number for this device
	LongWord irID;					// IR transponder ID
} prdInfo;

//--------------------------------FUNCTIONS---------------------------------------
void	 prdInfoInit(void);
void	 prdInfoStore(void);
void	 prdInfoSet(prdInfo *newInfo);
prdInfo* prdInfoGet(void);
Boolean	 prdInfoValidate(void);


#endif 	// _setMng_

