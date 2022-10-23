/* usbVenReq.h - Dataton 3356 Pickup
**
** This file defines and documents the vendor specific control requests that the
** Pickup can handle.
** 
**
** Created 10-10-28	Kalle
**
** (C) Copyright Dataton AB 2010, All Rights Reserved
**
**/

#ifndef _usbVenReq_
#define _usbVenReq_


// Get or set serial number of Pickup. Index and value = 0
// Parameter: A 4 byte serial number in the data stage, MSB first.
#define USB_VENREQ_GET_SERIAL			(1)
#define USB_VENREQ_SET_SERIAL			(2)

// Get or set value of Pickup real time clock. Index and value = 0
// Parameter: A 4 byte value representing the current time in 4 Hz ticks realtive to 20070101 00:00.00.
// MSB first.
#define USB_VENREQ_GET_RTC				(3)
#define USB_VENREQ_SET_RTC				(4)

// Reset the hardware LOG
#define USB_VENREQ_RESET_HW_LOG			(5)		

// Get the contents of the hardware log.
#define USB_VENREQ_GET_HW_LOG			(6)

// Set charger rack postion of Pickup, this is used for synchronized flassing.
// Index = position (row and column)
#define USB_VENREQ_SET_POSITION			(7)

// Enable or disable IDentify flasg mode.
// Index = 1 => enable, index = 0 => disable
#define USB_VENREQ_ID_FLASH				(8)				

// Get current battery level.
#define USB_VENREQ_GET_BAT_LEV			(9)			

// Index = which led, Value = level(0..15)
#define USB_VENREQ_SET_LED_LEVEL		(10)

#define USB_VENREQ_GET_VERSION			(11)

#define USB_VENREQ_GET_HW_VERSION		(12)
#define USB_VENREQ_GET_SW_VERSION		(13)

// Perform HW reset of Pickup (Restart)
#define USB_VENREQ_RESET				(14)			

// Service to be used by the charger to indicate busy.
#define USB_VENREQ_LOCK					(15)
#define USB_VENREQ_UNLOCK				(16)

// Permit / Forbid pickup to charge it's battery
#define USB_VENREQ_PERMIT_CHARGE		(17)
#define USB_VENREQ_FORBID_CHARGE		(18)

// Retuns a byte
// bit 0 = Is charging
// bit 1 = Is charged
#define USB_VENREQ_IS_CHARGING		(19)

// Returns two words, first word contains the current battery level
// in 100 mV steps. The second contains the maximum battery level in 100 mV
// steps.
// Only available in 1.7 or later versions of the hardware.
#define USB_VENREQ_GET_CHARGE_LEVEL		(20)		



#endif	// _usbVenReq_

