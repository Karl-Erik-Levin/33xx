/* hwBoard.h - Dataton 33xx
**
**	Define hardware in 3356 Pickup and 3397 UniTag
**
**
** 		PICKUP HARDWARE VERSIONS
** 		------------------------
** 		070806 1.7.0 Note 600              U14 MAX4958 (not used)
** 		071205 1.8.0 Propoint approx 2000  U14 AJ41-3  (not used)
** 		080616 1.9.0 Propoint approx 3000  U14 MAX14508
** 		090625 2.1.0 Propoint 3000+3000    U04 MAX14508
**
** 		UNITAG HARDWARE VERSIONS
** 		------------------------
** 		091006 1.0.0 Propoint 8 prototypes
**
** 		
**
** Created 09-09-24	Kalle
**
** (C) Copyright Dataton AB 2009, All Rights Reserved
**
**/

#ifndef _hwBoard_
#define _hwBoard_

#include "atmel\AT91SAM7S256.h"
#include "atmel\lib_AT91SAM7S256.h"

/*-------------------------------*/
/* SAM7Board Memories Definition */
/*-------------------------------*/
// The AT91SAM7S256 embeds a 64-Kbyte SRAM bank, and 256 K-Byte Flash
#define  INT_SARM           0x00200000
#define  INT_SARM_REMAP	    0x00000000
#define  INT_FLASH          0x00000000
#define  INT_FLASH_REMAP    0x00100000

#define  FLASH_PAGE_NB	    1024
#define  FLASH_PAGE_LOCK    64
#define  FLASH_PAGE_SIZE    256

/*----------------*/
/* PIO Definition */
/*----------------*/
#define IRDA_TX_PIN				AT91C_PA0_TIOA0		// PA0  OUT IrDA Tx
#define TWI_TWD_PIN				AT91C_PA3_TWD		// PA3  I2C SDA
#define TWI_TWCK_PIN			AT91C_PA4_TWCK		// PA4  I2C SCL
#define IRDA_RX_PIN				AT91C_PA5_RXD0		// PA5  IN  IrDA Rx
#define AUD_USB_SWT_NEW_PIN		AT91C_PIO_PA6		// PA6  OUT Ctrl new USB/audio switch MAX14508. Drive AOR high to
													//		connect COM_ to ANO_. AOR has 1M pulldown resistor to GND.
#define DBGU_RX					AT91C_PA9_DRXD		// PA9  --- Debug connector pin 1
#define DBGU_TX					AT91C_PA10_DTXD		// PA10 --- Debug connector pin 3 (TracePrintf)
#define SI4721_RESET_PIN		AT91C_PIO_PA11		// PA11 OUT Radion Reset, Active low
#define I2S_LRCLK_PIN			AT91C_PA15_TF		// PA15 SCC LRCLK
#define I2S_BCLK_PIN			AT91C_PA16_TK		// PA16 SCC BCLK
#define I2S_DATA_PIN			AT91C_PA17_TD		// PA17 SCC DATA
#define POWD_BATT_CHARGE_IND	AT91C_PIO_PA22		// PA22 IN  BATT CHG IND, Low indicates when battery charge current
													//			has dropped to ten percent of its programmed value (C/10).
#define USB_PULLUP_CONTROL_PIN	AT91C_PIO_PA24		// PA24 OUT Controll USB PullUp, Active low
#define RADIO_32KHZ_CLK_PIN		AT91C_PA26_TIOA2	// PA26 OUT Radio 32 kHz clk
#define	SI4721_INT				AT91C_PIO_PA28		// PA28 IN  Radio GPIO2/INT
#define KB_RIGHT_PIN			AT91C_PIO_PA29		// PA29 IN  RIGHT Switch, Active low
#define KB_LEFT_PIN				AT91C_PIO_PA30		// PA30 IN  LEFT Switch, Active low

#if (HARDWARE==3356)
  #define TWI_LED_RESET_PIN		AT91C_PIO_PA1		// PA1  OUT LED Reset, Active low
													// PA2  IN  SDCARD DATA1, not used
  #define AUD_USB_SWT_OLD_PIN	AT91C_PIO_PA7		// PA7  OUT Ctrl USB/audio switch U4
													//		IN1/2 pins on MAX4762 used to switch in/out USB.
													//      NC 2.0 ->
  #define BATT_CHARGE_ENABLE_PIN AT91C_PIO_PA8		// PA8  OUT BATT CHG ENABLE, Active low
													//      NC 2.0 ->
  #define SPI_MISO_PIN			AT91C_PA12_MISO		// PA12 SPI SDCARD DATA0, DataOut
  #define SPI_MOSI_PIN			AT91C_PA13_MOSI		// PA13 SPI SDCARD CMD,   DataIn
  #define SPI_CLK_PIN			AT91C_PA14_SPCK		// PA14 SPI SDCARD CLK,   Clk
  #define KB_PLAY_PIN			AT91C_PIO_PA18		// PA18 IN  Action Knob-Switch, Active low
  #define IRDA_BOOST_PIN		AT91C_PIO_PA19		// PA19 OUT Tx IR-boost, Active low
  #define IRDA_SD_PIN			AT91C_PIO_PA20		// PA20 OUT IrDA SHDN, Active high
  #define POWD_USB_POWER_OK		AT91C_PIO_PA21		// PA21 IN  1.5 NC, 1.7 -- 1.9 USB Power IND
													//			Low when pickup is supplyed with 5V from USB
													//          NC 2.0 ->
  #define I2S_MCLK_PIN			AT91C_PA23_SCK1		// PA23 --- I2S MCLK
  #define ACC_INTERRUPT_PIN		AT91C_PIO_PA25		// PA25 IN  NC, not used -> 1.9, Interrupt from ADXL345 2.0 ->
  #define AD_SPEAKER_CTRL_PIN	AT91C_PIO_PA27		// PA27 OUT Speaker Amp SHDN, Active low
													// PA28 IN  1.5 NC 1.7 GND 1.8 -- 1.9 Radio GPIO2/INT, not used
  #define SDCARD_SELECT			AT91C_PIO_PA31		// PA31 OUT SDCARD CS
#elif (HARDWARE==3397)
  #define BATT_MEASSURE_PIN		AT91C_PIO_PA1		// PA1  OUT Set low to meassure battery voltage
  #define LED_01				AT91C_PIO_PA2		// PA2  OUT LED 01
  #define LED_02				AT91C_PIO_PA7		// PA7  OUT LED 02
  #define LED_03				AT91C_PIO_PA8		// PA8  OUT LED 03
  #define LED_04				AT91C_PIO_PA12		// PA12 OUT LED 04
  #define LED_05				AT91C_PIO_PA13		// PA13 OUT LED 05
  #define LED_06				AT91C_PIO_PA14		// PA14 OUT LED 06
  #define LED_07				AT91C_PIO_PA18		// PA18 OUT LED 07
  #define LED_08				AT91C_PIO_PA19		// PA19 OUT LED 08
  #define IRDA_BOOST_PIN		AT91C_PIO_PA20		// PA20 OUT IrDA boost, Active low
  #define LED_09				AT91C_PIO_PA21		// PA21 OUT LED 09
  #define LED_10				AT91C_PIO_PA23		// PA23 OUT LED 10
  #define LED_11				AT91C_PIO_PA25		// PA25 OUT LED 11
  #define LED_12				AT91C_PIO_PA27		// PA27 OUT LED 12
  #define LED_13				AT91C_PIO_PA31		// PA31 OUT LED 13
  #define KB_PLAY_PIN			((unsigned int) 0)	// Not used in 3397
  #define AUD_USB_SWT_OLD_PIN	((unsigned int) 0)	// Not used in 3397
#else
  #error "HARDWARE must be 3356 or 3397"
#endif

#define SCC_PINS_MASK      (I2S_BCLK_PIN | I2S_DATA_PIN | I2S_LRCLK_PIN)
#define KB_PINS_MASK	   (KB_PLAY_PIN  | KB_RIGHT_PIN | KB_LEFT_PIN)
#define TWI_PINS_MASK      (TWI_TWCK_PIN | TWI_TWD_PIN)
#define AT91C_DBGU_BAUD	   115200					// Baud rate for debug serial port

/*--------------*/
/* Master Clock */
/*--------------*/
#define EXT_OC          18432000          // Exetrnal ocilator MAINCK
#define MCK             47923200          // MCK (PLLRC div by 2)
#define MCKKHz          (MCK/1000)        // MCK in KHz

/*---------------------------------*/
/* Flash Microseconds Cycle Number */
/*---------------------------------*/
#define FMCN_NON_VOLATILE	(MCK/1000000 + 2)				// Rounded upwards
#define FMCN_NORMAL			(FMCN_NON_VOLATILE * 3) / 2		// Rounded upwards

#endif	// _hwBoard_
