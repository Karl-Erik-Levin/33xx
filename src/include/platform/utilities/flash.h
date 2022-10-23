/* flash.h - Dataton 3397 Transponder 
**
**	Write to AT91SAM7S flash memory and enable brownout detector
** 		
**
** Created 09-09-24	Kalle
**
** (C) Copyright Dataton AB 2009, All Rights Reserved
**
**/

#ifndef _flash_
#define _flash_


#define  FLASH_PAGE_SIZE_BYTE	256
#define  FLASH_PAGE_IN_LOCK_REG	 64
#define  FLASH_ERROR_NONE		 0
#define  FLASH_ERROR_STATUS		-1
#define  FLASH_ERROR_VERIFY		-2

int AT91F_Flash_Write( unsigned int Flash_Address ,int size ,unsigned int * buff);
int AT91F_Flash_Write_all( unsigned int Flash_Address ,int size ,unsigned int * buff);
void AT91F_Flash_Unlock(unsigned int Flash_Lock_Page);
void AT91F_Flash_Enable_Brownout(void);


#endif	// _flash_
