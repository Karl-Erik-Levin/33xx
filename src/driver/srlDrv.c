/* srlDrv.c - Dataton 3397 Transponder 
**   
** Driver to send C-style strings to debug serial port. Used for trace printout
**
** Created 09-09-24	Kalle
**
** (C) Copyright Dataton AB 2009, All Rights Reserved
**
*/

//---------------------------------INCLUDES---------------------------------------
#include "MFLib\Dataton_Types.h"

#include "driver\hwBoard.h"
#include "driver\srlDrv.h"

//----------------------------GLOBAL VARIABLES------------------------------------
static Boolean srlIsInit = false;

//--------------------------------FUNCTIONS---------------------------------------
/*******************************************************************************
 * Function:	srlInit
 * Returns:		void
 *
 * Summary:		Setup debug UART for asynchronous operation at 115.2 kbit, 8, N 1
 * NOTE:		Driver will be init on first call to srlSendString
 *******************************************************************************/
static void
srlInit(void)
{
	// Open PIO for DBGU
	AT91F_DBGU_CfgPIO();		// Disable pullup in pwrDrv.c
	
	// Reset Transmitter & receivier
	((AT91PS_USART)AT91C_BASE_DBGU)->US_CR = AT91C_US_RSTTX |AT91C_US_RSTRX;

	// Configure DBGU
	AT91F_US_Configure (
		(AT91PS_USART) AT91C_BASE_DBGU,       // DBGU base address
		MCK,
		AT91C_US_ASYNC_MODE ,                 // Mode Register to be programmed
		AT91C_DBGU_BAUD ,                     // Baudrate to be programmed
		0);                                   // Timeguard to be programmed

	// Enable Transmitter
	((AT91PS_USART)AT91C_BASE_DBGU)->US_CR = AT91C_US_TXEN; // | AT91C_US_RXEN;

	srlIsInit  = true;
}


/*******************************************************************************
 * Function:	srlSendString
 * Returns:		void
 *
 * Summary:		This function is used to send a string through the DBGU channel
 *******************************************************************************/
void srlSendString(char *buffer)
{
	if (!srlIsInit)
		srlInit();

    while(*buffer != '\0')
	{
		AT91F_US_PutChar((AT91PS_USART)AT91C_BASE_DBGU, *buffer++);
		while (!AT91F_US_TxReady((AT91PS_USART)AT91C_BASE_DBGU))
			;		// Wait for char to be sent
    }
}

