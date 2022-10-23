/* adcDrv.c - Dataton 33xx
**
**	uProcessor A/D converter Driver
** 		
** All measure are in 10-bit. Vref=3,3V. On all channels are 
** average value calculated as filter
**
** The channels are used like this:
** 
** 	AD4: Battery charge current	Ibatt=135 mA (max) ==> 1,0 V
** 	AD5: Headphones/USB sense	HP==>1,32V     NONE==>1,60V	USBPWR==>2,34V
** 	AD6: Not used
** 	AD7: Battery voltage		0,59 * Vbatt
**
** Created 09-09-24	Kalle
**
** (C) Copyright Dataton AB 2009, All Rights Reserved
**
*/
//---------------------------------INCLUDES---------------------------------------
#include "MFLib\Dataton_Types.h"

#include "driver\hwBoard.h"
#include "driver\adcDrv.h"
#include "driver\tmrDrv.h"

#include "platform\FreeRTOS\FreeRTOS.h"
#include "platform\FreeRTOS\task.h"

//---------------------------------DEFINES----------------------------------------
//#define ADC_BATTERY_CURRENT_CHANNEL		4
#define ADC_HEADPHONES_CHANNEL			5
#define ADC_VOLUME_CHANNEL				6
#define ADC_BATTERY_CHANNEL				7
#define ADC_SAMP_INTERVAL				100		// Time in ms

#define FILTER_NUM_SAMP_HP				3	
#define FILTER_NUM_SAMP_VL				5	
#define FILTER_NUM_SAMP_BL			   12	
#define FILTER_NUM_SAMP_BC				3

#define TRGEN    (0x00)			// Software triggering
#define TRGSEL   (0x00)			// Without effect in Software triggering
#define LOWRES   (0x00)			// 10-bit result output format
#define SLEEP    (0x00)			// Normal Mode (instead of SLEEP Mode)
#define PRESCAL  (0x09)			// ADCclk = MCK / ((PRESCAL+1)*2)		(0x04 ==> 2,4 MHz)
#define STARTUP  (0x10)			// STtime = (STARTUP+1)*8 / ADCclk		(0x0c ==> 56 us)
#define SHTIM    (0x0F)			// SHtime = (SHTIME + 1) / ADCclk		(0x0f ==> 6,6 us)

//----------------------------GLOBAL VARIABLES------------------------------------
static Word channelLastSample[_EADC_NumChannels];
static xTaskHandle potdTaskHandle;
static Boolean gIsSleeping, firstVL, firstHP;

//--------------------------------FUNCTIONS---------------------------------------
/*******************************************************************************
 * Function:	adcTask
 * 
 * Summary:		
 *******************************************************************************/
static void
adcTask(void * params)
{
	static Word accHP, accBL/*, accBC*/, hp[FILTER_NUM_SAMP_HP];
	static Byte cntHP, cntBL/*, cntBC*/;
  #if (HARDWARE == 3356)
	static Word accVL, vl[FILTER_NUM_SAMP_VL];
	static Byte cntVL;
  #endif
	
	accHP = accBL = /*accBC =*/ 0;
	cntHP = cntBL = /*cntBC =*/ 0;
	
	while (1)
	{	
		vTaskDelay(ADC_SAMP_INTERVAL);		// Sample rate

		// Sample battery level and headphones connector level.
	  #if (HARDWARE == 3397)
		AT91F_PIO_ClearOutput(AT91C_BASE_PIOA, BATT_MEASSURE_PIN);
		vTaskDelay(1);
	  #endif

//		AT91F_ADC_EnableChannel(AT91C_BASE_ADC, (1 << ADC_BATTERY_CURRENT_CHANNEL));
		AT91F_ADC_EnableChannel(AT91C_BASE_ADC, (1 << ADC_HEADPHONES_CHANNEL));
	  #if (HARDWARE == 3356)
		AT91F_ADC_EnableChannel(AT91C_BASE_ADC, (1 << ADC_VOLUME_CHANNEL));
	  #endif
		AT91F_ADC_EnableChannel(AT91C_BASE_ADC, (1 << ADC_BATTERY_CHANNEL));

		AT91F_ADC_StartConversion(AT91C_BASE_ADC);
		vTaskDelay(1);
		
#if 0	// Sample battery charge current
		if (AT91F_ADC_GetStatus(AT91C_BASE_ADC) & (1 << ADC_BATTERY_CURRENT_CHANNEL))
		{
			accBC += AT91F_ADC_GetConvertedDataCH4(AT91C_BASE_ADC);
			cntBC++;
			
			if (cntBC >= FILTER_NUM_SAMP_BC) {
				channelLastSample[EADC_BatteryCurrent] = accBC / FILTER_NUM_SAMP_BC;
				cntBC = accBC = 0;
			}
		}
#endif	// 0

		if (AT91F_ADC_GetStatus(AT91C_BASE_ADC) & (1 << ADC_HEADPHONES_CHANNEL))
		{
			if (firstHP) {
				firstHP = false;
				accHP = AT91F_ADC_GetConvertedDataCH5(AT91C_BASE_ADC);
				for (cntHP=0; cntHP < FILTER_NUM_SAMP_HP; cntHP++)
					hp[cntHP] = accHP;
				cntHP = 0;
				channelLastSample[EADC_Headphones] = accHP;
				accHP *= FILTER_NUM_SAMP_HP;
			} else {
				Word samp = AT91F_ADC_GetConvertedDataCH5(AT91C_BASE_ADC);
				accHP += (samp - hp[cntHP]);
				hp[cntHP] = samp;
				cntHP++;
				
				if (cntHP >= FILTER_NUM_SAMP_HP) {
					cntHP = 0;
				}
				channelLastSample[EADC_Headphones] = accHP / FILTER_NUM_SAMP_HP;
			}
		}
		
	  #if (HARDWARE == 3356)
		if (AT91F_ADC_GetStatus(AT91C_BASE_ADC) & (1 << ADC_VOLUME_CHANNEL))
		{
			if (firstVL) {
				firstVL = false;
				accVL = AT91F_ADC_GetConvertedDataCH6(AT91C_BASE_ADC);
				for (cntVL=0; cntVL < FILTER_NUM_SAMP_VL; cntVL++)
					vl[cntVL] = accVL;
				cntVL = 0;
				channelLastSample[EADC_Volume] = accVL;
				accVL *= FILTER_NUM_SAMP_VL;
			} else {
				Word samp = AT91F_ADC_GetConvertedDataCH6(AT91C_BASE_ADC);
				accVL += (samp - vl[cntVL]);
				vl[cntVL] = samp;
				cntVL++;
				
				if (cntVL >= FILTER_NUM_SAMP_VL) {
					cntVL = 0;
				}
				channelLastSample[EADC_Volume] = accVL / FILTER_NUM_SAMP_VL;
			}
		}
	  #endif
		
		if (AT91F_ADC_GetStatus(AT91C_BASE_ADC) & (1 << ADC_BATTERY_CHANNEL))
		{
			if (channelLastSample[EADC_BatteryLevel]==0 && cntBL==0) {	// First start value
				channelLastSample[EADC_BatteryLevel] = AT91F_ADC_GetConvertedDataCH7(AT91C_BASE_ADC);
			} else {														// Then average value 
				accBL += AT91F_ADC_GetConvertedDataCH7(AT91C_BASE_ADC);
				cntBL++;
			}
			
			if (cntBL >= FILTER_NUM_SAMP_BL) {
				channelLastSample[EADC_BatteryLevel] = accBL / FILTER_NUM_SAMP_BL;
				cntBL = accBL = 0;
			}
		}

	  #if (HARDWARE == 3397)
		AT91F_PIO_SetOutput(AT91C_BASE_PIOA, BATT_MEASSURE_PIN);
	  #endif
	}
}

/*******************************************************************************
 * Function:	adcInit
 * 
 * Summary:		
 *******************************************************************************/
void
adcInit()
{
	gIsSleeping = false;
	firstVL = firstHP = true;
	
	AT91F_PMC_EnablePeriphClock(
			AT91C_BASE_PMC,
			(unsigned int)1 << AT91C_ID_ADC);

  #if (HARDWARE == 3397)
	AT91F_PIO_CfgOutput(AT91C_BASE_PIOA, BATT_MEASSURE_PIN);
	AT91C_BASE_PIOA->PIO_PPUDR = BATT_MEASSURE_PIN;				// Pullup disable
  #endif

	/* Clear all previous setting and result */
	AT91F_ADC_SoftReset (AT91C_BASE_ADC);
       
    /* First Step: Set up by using ADC Mode register */
	AT91F_ADC_CfgModeReg (AT91C_BASE_ADC,
                            (SHTIM << 24) | (STARTUP << 16) | (PRESCAL << 8) | 
                            (SLEEP << 5) | (LOWRES <<4) | (TRGSEL << 1) | (TRGEN ));

	xTaskCreate(adcTask, 
                "ADCT", 
                100, 
                NULL,
				(tskIDLE_PRIORITY + 2),
                &potdTaskHandle);
}

/*******************************************************************************
 * Function:	adcGetLevel
 * 
 * Summary:		Return 10 bit value
 *******************************************************************************/
Word
adcGetLevel(
	EADCChannel channel
){
	if (gIsSleeping && (channel == EADC_Headphones))
	{
		AT91F_PMC_EnablePeriphClock(
				AT91C_BASE_PMC,
				(unsigned int)1 << AT91C_ID_ADC);

		AT91F_ADC_EnableChannel(
				AT91C_BASE_ADC,
				(1 << ADC_HEADPHONES_CHANNEL));

		tmrWaitMS(1);			// Wait 1 ms to get AD-converter ready.
								// NOTE Caller must wake Timer
		
		AT91F_ADC_StartConversion(AT91C_BASE_ADC);

		while (!(AT91C_BASE_ADC->ADC_SR & (1 << ADC_HEADPHONES_CHANNEL)))
				;

		channelLastSample[EADC_Headphones] = AT91F_ADC_GetConvertedDataCH5(AT91C_BASE_ADC);

		AT91F_ADC_DisableChannel(
				AT91C_BASE_ADC,
				(1 << ADC_HEADPHONES_CHANNEL));
		
		AT91F_PMC_DisablePeriphClock(
				AT91C_BASE_PMC,
				(unsigned int)1 << AT91C_ID_ADC);
	}

	return channelLastSample[channel];
}

/*******************************************************************************
 * Function:	adcSleep
 * 
 * Summary:		
 *******************************************************************************/
void
adcSleep()
{
	if (!gIsSleeping)
	{
		gIsSleeping = true;
		
		AT91F_ADC_DisableChannel(
				AT91C_BASE_ADC,
				(1 << ADC_HEADPHONES_CHANNEL)
			  | (1 << ADC_BATTERY_CHANNEL)
		#if (HARDWARE == 3356)
			  | (1 << ADC_VOLUME_CHANNEL)
		#endif
//			  |	(1 << ADC_BATTERY_CURRENT_CHANNEL)
				);

		AT91F_PMC_DisablePeriphClock(
				AT91C_BASE_PMC,
				(unsigned int)1 << AT91C_ID_ADC);
	}
}

/*******************************************************************************
 * Function:	adcWake
 * 
 * Summary:		
 *******************************************************************************/
void
adcWake()
{
	if (gIsSleeping)
	{
		gIsSleeping = false;
	
		AT91F_PMC_EnablePeriphClock(
				AT91C_BASE_PMC,
				(unsigned int)1 << AT91C_ID_ADC);
	
		AT91F_ADC_EnableChannel(
				AT91C_BASE_ADC,
				(1 << ADC_HEADPHONES_CHANNEL)
			  |	(1 << ADC_BATTERY_CHANNEL)
		#if (HARDWARE == 3356)
			  |	(1 << ADC_VOLUME_CHANNEL)
		#endif
//			  |	(1 << ADC_BATTERY_CURRENT_CHANNEL)
				);
	}
}
