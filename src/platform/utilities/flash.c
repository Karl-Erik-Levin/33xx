/* flash.c - Dataton 3397 Transponder 
**
**	Write to AT91SAM7S flash memory and enable brownout detector
** 		
**
** Created 09-09-24	Kalle
**
** (C) Copyright Dataton AB 2009, All Rights Reserved
**
**/

//---------------------------------INCLUDES---------------------------------------
#include "MFLib\Dataton_Types.h"
#include "ghs\arm_ghs.h"

#include "driver\hwBoard.h"
#include "platform\utilities\flash.h"

//--------------------------------FUNCTIONS---------------------------------------
/*******************************************************************************
 * Function:	AT91F_Flash_Unlock
 * 
 * Summary:		Unlocks the page in flash for writing.
 *				Clear the Non Volatile Memory Bits at the page address.
 *				Returns 1 if success.
 *******************************************************************************/
void 
AT91F_Flash_Unlock(
	unsigned int Flash_Lock_Page	// Page number 
){
	unsigned int FMRcopy, lockBitMask;
	
	Flash_Lock_Page &= 0x3FF;		// 0..1023
	lockBitMask = 1 << (16 + (Flash_Lock_Page / FLASH_PAGE_IN_LOCK_REG));
	
	// Only unlock if page is in a locked region
	if (AT91C_BASE_MC->MC_FSR & lockBitMask) {
		
		// Set FMCN to NON_VOLATILE
		FMRcopy = AT91C_BASE_MC->MC_FMR & ~AT91C_MC_FMCN;	// Clear FMCN bits
		AT91C_BASE_MC->MC_FMR = ((AT91C_MC_FMCN)&(FMCN_NON_VOLATILE << 16)) | FMRcopy;
		
		// Disable interrupts
		__DIR();
		
	    // Unlock page (write in command register)
	    AT91C_BASE_MC->MC_FCR = AT91C_MC_CORRECT_KEY | AT91C_MC_FCMD_UNLOCK | (AT91C_MC_PAGEN & (Flash_Lock_Page << 8) ) ;

	    // Wait for end of command
		while ((AT91C_BASE_MC->MC_FSR & AT91C_MC_FRDY) != AT91C_MC_FRDY ) {
			;
		}
		
		// Enable interrupts
		__EIR();
		
		// Set FMCN back to NORMAL
		AT91C_BASE_MC->MC_FMR = ((AT91C_MC_FMCN)&(FMCN_NORMAL << 16)) | FMRcopy;
	}
	
	return;
}

/*******************************************************************************
 * Function:	AT91F_Flash_Write
 * 
 * Summary:		Write in one Flash page located in AT91C_IFLASH, size in 32 bits.
 *				Returns 0 if successful, otherwise <0
 * NOTE:		No locking of page after a write /JM
 *******************************************************************************/
int 
AT91F_Flash_Write(
	unsigned int Flash_Address,	// Starts at 0x00100000
	int size,					// Size in bytes
	unsigned int *Buff_Address	// Buffer address
){
    // Set the Flash controller base address
    int i, page, numBytes;
    unsigned int *Flash, *Buff;
    unsigned int FSRcopy;
	
	// Init flash pointer
    Flash = (unsigned int *) Flash_Address;
	Buff  = Buff_Address;
	numBytes = size;
	
	// Get the Flash page number
    page = ((Flash_Address - (unsigned int)AT91C_IFLASH ) / FLASH_PAGE_SIZE_BYTE);

	// Copy the new value
	for (i=0; (i < FLASH_PAGE_SIZE_BYTE) && (numBytes > 0); Flash++, Buff++, i+=sizeof(unsigned int), numBytes-=sizeof(unsigned int)) {
		// Copy the flash to the write buffer ensure that code generation
	    *Flash = *Buff;			// Copy unsigned int (4 byte)
	}
	
	// Clear NEBP bit (No Erase Berfore Programming)
	AT91C_BASE_MC->MC_FMR &= ~AT91C_MC_NEBP;
	
	// Disable interrupts
	__DIR();
	
    // Write page (set command in Flash Command Register)
    AT91C_BASE_MC->MC_FCR = AT91C_MC_CORRECT_KEY | AT91C_MC_FCMD_START_PROG | (AT91C_MC_PAGEN & (page <<8)) ;
    
    // Wait for end of command
    do {
		 FSRcopy = AT91C_BASE_MC->MC_FSR;
	} while ((FSRcopy & AT91C_MC_FRDY) != AT91C_MC_FRDY);
	
	// Enable interrupts
	__EIR();
	
	// Check the result
    if ((FSRcopy & ( AT91C_MC_PROGE | AT91C_MC_LOCKE )) != 0) {
		return FLASH_ERROR_STATUS;
	}
	
	// Verify data in flash memory
    Flash = (unsigned int *) Flash_Address;
	Buff  = Buff_Address;
	numBytes = size;
	
	for (i=0; (i < FLASH_PAGE_SIZE_BYTE) && (numBytes > 0); Flash++, Buff++, i+=sizeof(unsigned int), numBytes-=sizeof(unsigned int)) {
		if (*Flash != *Buff)
	    	return FLASH_ERROR_VERIFY;
	}
	
	return FLASH_ERROR_NONE;
}

/*******************************************************************************
 * Function:	AT91F_Flash_Write_all
 * 
 * Summary:		Write consecutive pages in Flash
 *				Returns 0 if successful, otherwise <0
 *******************************************************************************/
int 
AT91F_Flash_Write_all( 
	unsigned int Flash_Address,	// Start address, (base = AT91C_IFLASH)
	int size,					// Size in bytes
	unsigned int * buff			// Buffer address
){
    int   next, status;
    unsigned int  dest;
    unsigned int * src;

    dest = Flash_Address;
    src = buff;
    status = FLASH_ERROR_NONE;

    while((status == FLASH_ERROR_NONE) && (size > 0)) {
        // Check the size
        if (size <= FLASH_PAGE_SIZE_BYTE)
			next = size;
        else
			next = FLASH_PAGE_SIZE_BYTE;

        // Unlock current sector base address - current address by sector size (page address)
        AT91F_Flash_Unlock((dest - (unsigned int)AT91C_IFLASH ) / FLASH_PAGE_SIZE_BYTE);

        // Write page and get status
        status = AT91F_Flash_Write(dest, next, src);
		
        // Get next page param
        size -= next;
        src += FLASH_PAGE_SIZE_BYTE/4;
        dest +=  FLASH_PAGE_SIZE_BYTE;
	}

    return status;
}

/**********************************************************************************************************
 * Function:	AT91F_Flash_Enable_Brownout
 * 
 * Summary:		Non-volatile Brownout Detector Control
 *				Two general purpose NVM (GPNVM) bits are used for controlling the brownout detector (BOD),
 *				so that even after a power loss, the brownout detector operations remain in their state.
 *				These two GPNVM bits can be cleared or set respectively through the commands "Clear General-
 *				purpose NVM Bit" and "Set General-purpose NVM Bit" of the EFC User Interface.
 *
 *				* GPNVM Bit 0 is used as a brownout detector enable bit. Setting the GPNVM Bit 0 enables
 *				the BOD, clearing it disables the BOD. Asserting ERASE clears the GPNVM Bit 0 and thus
 *				disables the brownout detector by default.
 *
 *				* The GPNVM Bit 1 is used as a brownout reset enable signal for the reset controller. Setting
 *				the GPNVM Bit 1 enables the brownout reset when a brownout is detected, Clearing the
 *				GPNVM Bit 1 disables the brownout reset. Asserting ERASE disables the brownout reset by
 *				default.
 *
 **********************************************************************************************************/

void
AT91F_Flash_Enable_Brownout(void)
{
	unsigned int FMRcopy;
	// Only perform enable of BOD if it's disabled
	if ((AT91C_BASE_MC->MC_FSR & AT91C_MC_GPNVM0) != AT91C_MC_GPNVM0)
	{	
		// Set FMCN to NON_VOLATILE
		FMRcopy = AT91C_BASE_MC->MC_FMR & ~AT91C_MC_FMCN;	// Clear FMCN bits
		AT91C_BASE_MC->MC_FMR = ((AT91C_MC_FMCN)&(FMCN_NON_VOLATILE << 16)) | FMRcopy;
		
		// Disable interrupts
		__DIR();
		
		// Activate GPNVM0 (Bit 0 in PAGEN)
		AT91C_BASE_MC->MC_FCR = AT91C_MC_CORRECT_KEY | AT91C_MC_FCMD_SET_GP_NVM | (0 << 8);
	    
		// Wait for end of command
		while ((AT91C_BASE_MC->MC_FSR & AT91C_MC_FRDY) != AT91C_MC_FRDY ) {
			;
		}
		
		// Enable interrupts
		__EIR();
		
		// Set FMCN back to NORMAL
		AT91C_BASE_MC->MC_FMR = ((AT91C_MC_FMCN)&(FMCN_NORMAL << 16)) | FMRcopy;
	}
	
	if ((AT91C_BASE_MC->MC_FSR & AT91C_MC_GPNVM1) != AT91C_MC_GPNVM1)
	{	
		// Set FMCN to NON_VOLATILE
		FMRcopy = AT91C_BASE_MC->MC_FMR & ~AT91C_MC_FMCN;	// Clear FMCN bits
		AT91C_BASE_MC->MC_FMR = ((AT91C_MC_FMCN)&(FMCN_NON_VOLATILE << 16)) | FMRcopy;
		
		// Disable interrupts
		__DIR();
		
		// Activate GPNVM1 (Bit 1 in PAGEN)
	    AT91C_BASE_MC->MC_FCR = AT91C_MC_CORRECT_KEY | AT91C_MC_FCMD_SET_GP_NVM | (1 << 8);
	    
		// Wait for end of command
		while ((AT91C_BASE_MC->MC_FSR & AT91C_MC_FRDY) != AT91C_MC_FRDY ) {
			;
		}
		
		// Enable interrupts
		__EIR();
		
		// Set FMCN back to NORMAL
		AT91C_BASE_MC->MC_FMR = ((AT91C_MC_FMCN)&(FMCN_NORMAL << 16)) | FMRcopy;
	}
}



