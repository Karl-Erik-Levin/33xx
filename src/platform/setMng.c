/* setMng.c - Dataton 33xx
**
** Product setting stored in processor flash memory
**
**
** Created 09-09-24	Kalle
**
** (C) Copyright Dataton AB 2009, All Rights Reserved
**
*/

//---------------------------------INCLUDES---------------------------------------
#include "MFLib\Dataton_Types.h"
#include "MFLib\TraceAssert.h"

#include "platform\setMng.h"
#include "platform\utilities\flash.h"
#include "platform\utilities\crc16CCITT.h"
#include "platform\FreeRTOS\FreeRTOS.h"

#if (HARDWARE==3397)
  #define PRD_INFO_ADDRESS	0x13ff80			// 128 byte from top of flash memory
#else	// (HARDWARE==3356)
  #define PRD_INFO_ADDRESS	0x10ff80
#endif

#define PU_INFO_ADDRESS		0x10fe00
#define PU_HW_LOG_ADDRESS	0x10fd00
#define PU_HW_INFO_ADDRESS	0x10fc00

//---------------------------------TYPEDEFS---------------------------------------
// Pickup old hardware version
typedef struct _PUHWInfo
{
	Word 		hwVersion;			
	Word		blVersion;		// New from version 2
} PUHWInfo;

typedef struct _PUHWInfoBlock
{
	LongWord 	crc;
	Word 		version;		// Used version 1 and 2
	PUHWInfo	info;
} PUHWInfoBlock;

// Pickup old base information

typedef struct _PUInfo
{
	LongWord 	serial;			// Serial number of PU2.
	LongWord 	rtcBase;		// Quater second ticks since 2007-01-01 00:00.
	LongWord	featureMap;
} PUInfo;

typedef struct _PUInfoBlock
{
	LongWord crc;
	Word 	version;			// Used version 1
	PUInfo	info;
} PUInfoBlock;

typedef struct _prdInfoBlock
{
	LongWord crc;				// CRC of following fields.
	prdInfo info;
} prdInfoBlock;

//----------------------------GLOBAL VARIABLES------------------------------------
static prdInfoBlock prdInfoRAMCpy;

//--------------------------------FUNCTIONS---------------------------------------
/*******************************************************************************
 * Function:	prdInfoStore
 * 
 * Summary:		Calculate CRC and write prdInfo to flash
 *******************************************************************************/
void 
prdInfoStore()
{
	prdInfoRAMCpy.crc = crc16CCITT(0, (Byte *)&prdInfoRAMCpy.info, sizeof(prdInfo));

	portENTER_CRITICAL();		// Disable interrupt
	
	// Write to flash memory
	AT91F_Flash_Write_all((LongWord)PRD_INFO_ADDRESS, sizeof(prdInfoBlock), (void *)&prdInfoRAMCpy);
	
	portEXIT_CRITICAL();		// Enable interrupt
}

/*******************************************************************************
 * Function:	prdInfoGet
 * 
 * Summary:		Returns a pointer to application-info
 *******************************************************************************/
prdInfo* 
prdInfoGet()
{
	return &prdInfoRAMCpy.info;
}

/*******************************************************************************
 * Function:	prdInfoSet
 * 
 * Summary:		
 *******************************************************************************/
void 
prdInfoSet(prdInfo *newInfo)
{
	memcpy(&prdInfoRAMCpy.info, newInfo, sizeof(prdInfoRAMCpy));
}

/*******************************************************************************
 * Function:	prdInfoValidate
 * 
 * Summary:		
 *******************************************************************************/
Boolean 
prdInfoValidate()
{
	prdInfoBlock *ai = (prdInfoBlock *)PRD_INFO_ADDRESS;
	Word crc = crc16CCITT(0, (Byte *)&ai->info, sizeof(prdInfo));
	
	TRACE_AST(("prdInfoValidate %x %x\r\n", ai->crc, crc);)
	return (ai->crc == crc);
}

/*******************************************************************************
 * Function:	prdInfoInit
 * 
 * Summary:		Initialize application setting handler
 *******************************************************************************/
void 
prdInfoInit()
{
	if (prdInfoValidate()) {
		// Get data from flash memory
		memcpy(&prdInfoRAMCpy, (void *)PRD_INFO_ADDRESS, sizeof(prdInfoBlock));
	}
	else {
		// Set default value to all fields
		prdInfoRAMCpy.info.infoStructVersion = PRD_INFO_STRUCT_VERSION;
		prdInfoRAMCpy.info.hwVer = 0x010000;
		prdInfoRAMCpy.info.blVer = 0x000000;
		prdInfoRAMCpy.info.swVer = 0x010000;
		prdInfoRAMCpy.info.serialNumber = 10000;
		prdInfoRAMCpy.info.irID = 1000;
		
		prdInfoStore();
	}

  #if (HARDWARE==3356)
	{
		PUHWInfoBlock *hwi= (PUHWInfoBlock *)PU_HW_INFO_ADDRESS;
		PUInfoBlock   *bi = (PUInfoBlock *)PU_INFO_ADDRESS;
		Word size;
		
		if(hwi->version < 2)
			size = sizeof(Word);
		else
			size = sizeof(PUHWInfo);
		
		TRACE_AST(("setMng %x %x\n\r", hwi->crc, crc16CCITT(0, (Byte *)&hwi->info, size));)
		if (hwi->crc == crc16CCITT(0, (Byte *)&hwi->info, size)) {
			if (prdInfoRAMCpy.info.hwVer != hwi->info.hwVersion ||
				prdInfoRAMCpy.info.blVer != hwi->info.blVersion) {
				
				prdInfoRAMCpy.info.hwVer = hwi->info.hwVersion;
				prdInfoRAMCpy.info.blVer = hwi->info.hwVersion;
				if (hwi->version >= 2)
					prdInfoRAMCpy.info.blVer = hwi->info.blVersion;
				prdInfoStore();
			}
		}
		
		if ((prdInfoRAMCpy.info.serialNumber == 10000) &&
		    (bi->crc == crc16CCITT(0, (Byte *)&bi->info, sizeof(PUInfo)))) {
		
			prdInfoRAMCpy.info.serialNumber = bi->info.serial;
			prdInfoStore();
		}
	}
  #endif
}

