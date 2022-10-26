/* i2cDrv.c - Dataton 33xx
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

//---------------------------------INCLUDES---------------------------------------
#include "MFLib\Dataton_Types.h"

#include "driver\hwBoard.h"
#include "driver\i2cDrv.h"
#include "driver\tmrDrv.h"

//#include "platform\FreeRTOS\projdefs.h"
#include "platform\FreeRTOS\FreeRTOS.h"
#include "platform\FreeRTOS\semphr.h"

//---------------------------------DEFINES----------------------------------------
// TWI clock select definitions. (Roughly 300 kbit/s @ 48 MHz MCK)
#define TWI_DIV_POW       		2		// T = ((TWI_DIV_SCAL * 2 ^ TWI_DIV_POW) + 3) * Tmck
#define TWI_DIV_SCAL			18   
#define TWI_WRITE_TIMEOUT_MS	2
#define TWI_READ_TIMEOUT_MS		2

//---------------------------------TYPEDEFS---------------------------------------
typedef struct _DeviceFlags
{
	Byte addr;
	LongWord mmrFlags;
} DeviceFlags;

//------------------------------GLOBAL VARIABLES----------------------------------
// Device address and size of internal device address for our devices on I2C bus
static DeviceFlags deviceFlags[_EI2CD_NumDevices] =
{
	0x63, 0,							// 0x20, EI2CD_4703 FM Transmitter and receiver
	0x13, AT91C_TWI_IADRSZ_1_BYTE,		// 0x26, EI2CD_9850 Audio DAC
	0x20, AT91C_TWI_IADRSZ_1_BYTE,		// 0x40, EI2CD_6964 LEDs
	0x09, 0,							// 0x12, EI2CD_3567	Power/Battery Manager
};

static xSemaphoreHandle i2cSem;

//--------------------------------FUNCTIONS---------------------------------------
void
i2cInit()
{
    // Enable Peripheral clock in PMC for TWI
    AT91F_TWI_CfgPMC();

	// Configure TWI I/O pins(System control)
	AT91F_PIO_CfgPeriph(AT91C_BASE_PIOA, TWI_PINS_MASK, 0);
	AT91C_BASE_PIOA->PIO_PPUDR = TWI_PINS_MASK;					// Pullup disable
	
	// Configure TWI for master mode.
	AT91F_TWI_Configure(AT91C_BASE_TWI);	

    AT91C_BASE_TWI->TWI_CWGR = (TWI_DIV_POW << 16) | (TWI_DIV_SCAL << 8) | TWI_DIV_SCAL;    // Square wave
	AT91C_BASE_TWI->TWI_CR = AT91C_TWI_MSEN;
	
	i2cSem = xSemaphoreCreateMutex();
}

void
i2cWrite(
		EI2CDevice device,
		LongWord iAddr,
		Byte *buf,
		Word numBytes
) {
	LongWord sr;
	LongWord sendTimeout;
	Word	 byteSent;

	if (xSemaphoreTake(i2cSem, 20) == pdFALSE)	{	// Aquire the semaphore.
		return;
	}
	
	if (deviceFlags[device].mmrFlags == 0) {	// Message to device without iaddr, must not be interrupted
	    portENTER_CRITICAL();					// Disable all interrupts
	}

	sendTimeout = tmrGetMS() + TWI_WRITE_TIMEOUT_MS;
	sr = AT91C_BASE_TWI->TWI_SR;
	
	// Send all data bytes from buffer buf
	for (byteSent = 0; byteSent < numBytes; ) {
		// Handle interface status code
		if (sr & AT91C_TWI_TXCOMP) {
			AT91C_BASE_TWI->TWI_MMR = deviceFlags[device].addr << 16 | deviceFlags[device].mmrFlags;
			AT91C_BASE_TWI->TWI_IADR = iAddr + byteSent;  
		} else if (sr & AT91C_TWI_NACK) {
			break;								// Device should always ack
		}
	
		// Send one byte
		AT91C_BASE_TWI->TWI_THR = buf[byteSent];
		byteSent++;

		// Wait until byte is transmitted
		do {
			sr = AT91C_BASE_TWI->TWI_SR & (AT91C_TWI_NACK | AT91C_TWI_TXRDY | AT91C_TWI_TXCOMP);
		} while (!sr && TM_LT(tmrGetMS(), sendTimeout));
		
		// Timeout ?
		if (TM_GE(tmrGetMS(), sendTimeout)) {
			break;
		}
	}
	
	// Wait until stop condition is sent
	do {
		sr = AT91C_BASE_TWI->TWI_SR & AT91C_TWI_TXCOMP;
	} while (!sr && TM_LT(tmrGetMS(), sendTimeout));
		
	if (deviceFlags[device].mmrFlags == 0) {
	    portEXIT_CRITICAL();					// Enable all interrupts
	}

	xSemaphoreGive(i2cSem);						// Release the semaphore.

	return;
}

/* Eftersom hårdvaran i Atmel processorn är dålig. I rev F av databladet tog Atmel bort statusflaggan
   AT91C_TWI_OVRE.
   Ulf Samuelsson på Atmel tror det beror på att hårdvaran för detta inte var tillförlitlig.
   
   Jag har haft stora problem att få I2C mottagningen att fungera beroende på att det kommer avbrott
   (interrupt) som gör att rutinen inte hinner hämta data från TWI_RHR innan nästa byte lagras där.

	Nu är det löst genom att disabla alla interupt medan man tar emot I2C data. Om man begär att 
	läsa 2 byte tar transaktionen ca 50 uS. Detta är ingen bra lösning då det kan störa tidsprecisionen
	för interrupt vilket påverkar sändningen av IR.
	
	I koden finns en annan lösning som kollar att det inte finns några pågående I2C transaktioner innan
	man börjar läsa. Lösningen var inte helt tillförlitlig och därför bortkommenterad.
*/
void
i2cRead(
		EI2CDevice device,
		LongWord iAddr,
		Byte *buf,
		Word numBytes
) {
	LongWord sr;
	LongWord rcvTimeout;
	Word	 byteReceived;
	Word	 retry;
	Boolean  failed;
	
	if (xSemaphoreTake(i2cSem, 20) == pdFALSE)	{	// Aquire the semaphore.
		return;
	}
	
	rcvTimeout = tmrGetMS() + TWI_READ_TIMEOUT_MS;
	retry = 5;									// We will retry 4 times if messages is interrupted
	
	do {
		byteReceived = 0;
		failed = false;
		retry--;

		AT91C_BASE_TWI->TWI_MMR = deviceFlags[device].addr << 16 | AT91C_TWI_MREAD | deviceFlags[device].mmrFlags;
	    AT91C_BASE_TWI->TWI_IADR = iAddr;
	    
	    portENTER_CRITICAL();		// Disable all interrupts
	    if (numBytes == 1)
			AT91C_BASE_TWI->TWI_CR = AT91C_TWI_START | AT91C_TWI_STOP;
	    else  
			AT91C_BASE_TWI->TWI_CR = AT91C_TWI_START;
	
		while ((byteReceived < numBytes) && !failed)
		{
			do {
				sr = AT91C_BASE_TWI->TWI_SR & (AT91C_TWI_OVRE | AT91C_TWI_RXRDY | AT91C_TWI_TXCOMP);
			} while (!sr && TM_LT(tmrGetMS(), rcvTimeout));
			
			if (sr & AT91C_TWI_OVRE) {
				failed = true;								// We came to late, probably interrupted by FreeRTOS
				AT91C_BASE_TWI->TWI_CR = AT91C_TWI_STOP;	// Set I2C stop condition after overrun error
			} else if (sr & AT91C_TWI_RXRDY) {
				buf[byteReceived] = AT91C_BASE_TWI->TWI_RHR;
				byteReceived++;
			}
			
			if ((numBytes - byteReceived) == 1)
				AT91C_BASE_TWI->TWI_CR = AT91C_TWI_STOP;	// Set I2C stop condition before last byte
				
			if (TM_GE(tmrGetMS(), rcvTimeout)) {
				failed = true;								// Timeout
				AT91C_BASE_TWI->TWI_CR = AT91C_TWI_STOP;	// Set I2C stop condition after timeout
			}
		}
		portEXIT_CRITICAL();
		
	} while (failed && retry && !TM_GE(tmrGetMS(), rcvTimeout));

	xSemaphoreGive(i2cSem);	// Release the semaphore.
	return;
}

void
i2cSleep()
{
	AT91F_PMC_DisablePeriphClock(AT91C_BASE_PMC, 1 << AT91C_ID_TWI);
}


void
i2cWake()
{
	AT91F_PMC_EnablePeriphClock(AT91C_BASE_PMC, 1 << AT91C_ID_TWI);
}



