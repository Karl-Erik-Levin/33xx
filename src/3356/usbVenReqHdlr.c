/* usbVenReqHdlr.c - Dataton 3356 Pickup
**
** Handles USB vendor specific requests.
** 
**
** Created 10-10-28	Kalle
**
** (C) Copyright Dataton AB 2010, All Rights Reserved
**
**/
//---------------------------------INCLUDES---------------------------------------
#include "MFLib\Dataton_Types.h"
#include "MFLib\TraceAssert.h"

#include "driver\rtcdrv.h"
#include "driver\hccusb\usb.h"

#include "platform\setMng.h"
#include "platform\utilities\byteSwap.h"
#include "platform\evtMng.h"
#include "platform\hccusb\usb_mst.h"

#include "3356\puusbAppl.h"
#include "3356\usbVenReq.h"
#include "3356\usbVenReqHdlr.h"

//--------------------------------PROTOTYPS---------------------------------------
typedef void (*datatonReceive)(Byte *data, Byte len);

//--------------------------------TYPEDEFS---------------------------------------
/* Structure used to hold the received requests. */
typedef struct _USBControlRequest
{                 
	Byte ucReqType;
	Byte ucRequest;
	Word usValue;
	Word usIndex;
	Word usLength;
} USBControlRequest;

typedef struct _venRcvData
{
	hcc_u8 rxBuf[8];
	hcc_u8 rxCnt;
	datatonReceive rxHandler;
	hcc_u8 rxInBuf;
} venRcvData;

//----------------------------GLOBAL VARIABLES------------------------------------
static venRcvData vendor;

//--------------------------------FUNCTIONS---------------------------------------
/*******************************************************************************
 * Function:	usbRxCallback
 * Returns:		void
 *
 * Summary:		We are in interrupt mode and cant store recevied data in flash
 *******************************************************************************/
static enum callback_state
usbRxCallback(void)
{
	if (vendor.rxHandler != (void *) 0)
	{
		vendor.rxInBuf = true;
	}
	return clbst_ok;
}

/*******************************************************************************
 * Function:	transmitNull
 * Returns:		void
 *
 * Summary:		A NULL (or zero length packet) is transmitted in acknowledge
 *				the reception of certain events from the host.
 *******************************************************************************/
static void
transmitNull(void)
{
	send_handshake(0);		// AT91C_UDP_CSR[0] |= AT91C_UDP_TXPKTRDY;
}

/*******************************************************************************
 * Function:	controlSendStall
 * Returns:		void
 *
 * Summary:		A stall condition is forced each time the host makes a
 *				request that is not supported
 *******************************************************************************/
static void
controlSendStall(void)
{
	usb_stop_ep_tx(0);		// AT91C_UDP_CSR[0] |= AT91C_UDP_FORCESTALL;
}

/*******************************************************************************
 * Function:	sendControlData
 * Returns:		void
 *
 * Summary:		Setup the Tx buffer to send data in response to a control request.
 *******************************************************************************/
static void
sendControlData(
		Byte *pucData,
		Word usRequestedLength,
		LongWord ulLengthToSend,
		long lSendingDescriptor)
{
	static hcc_u8 txbuf[20];
	
	memcpy(txbuf, pucData, ulLengthToSend);

	usb_send(0, (void *) 0, txbuf, ulLengthToSend, ulLengthToSend, usRequestedLength);
}

/*******************************************************************************
 * Function:	controlBeginRXData
 * Returns:		void
 *
 * Summary:		Setup receive data from USB control endpoint
 *******************************************************************************/
static void
controlBeginRXData(
		unsigned char count,
		datatonReceive dataRxCB,
		void (*dataAborted)())
{
	vendor.rxHandler = dataRxCB;
	vendor.rxCnt = count;
	vendor.rxInBuf = false;
	
	usb_receive(0, usbRxCallback, vendor.rxBuf, sizeof(vendor.rxBuf), count);
}

/*******************************************************************************
 * Function:	rtcBaseReceived
 * Returns:		void
 *
 * Summary:		
 *******************************************************************************/
static void 
rtcBaseReceived(
	Byte *newRTCBase, 
	Byte len
){
	LongWord rtcBase;
	
	ASSERT(len == 4, ("rtcBaseReceived: len != 4"));
	
	rtcBase = newRTCBase[0] << 24 |  newRTCBase[1] << 16 | newRTCBase[2] << 8 | newRTCBase[3] << 0;
	
	TRACE_USV(("\n\rrtcBaseReceived %x\n\r", rtcBase);)
	rtcSetRtValue(rtcBase);
}

/*******************************************************************************
 * Function:	serialNumReceived
 * Returns:		void
 *
 * Summary:		
 *******************************************************************************/
static void 
serialNumReceived(
	Byte *newSerial, 
	Byte len
){
	LongWord serial;
	ASSERT(len == 4, ("len != 4"));
	
	serial = newSerial[0] << 24 |  newSerial[1] << 16 | newSerial[2] << 8 | newSerial[3] << 0;
	
	TRACE_USV(("\n\rserialNumReceived %x\n\r", serial);)
	
	prdInfoGet()->serialNumber = serial;
	prdInfoStore();
}

/*******************************************************************************
 * Function:	vendorRequestHandler4hcc
 * Returns:		void
 *
 * Summary:		HCC interface for vendor request handler
 *******************************************************************************/
static void
vendorRequestHandler4hcc(hcc_u8 *pdata)
{
	Byte buf[20];
	EMEvent ev;
	
	USBControlRequest pxReq;
	USBControlRequest *pxRequest = &pxReq;

	pxRequest->ucReqType = STP_REQU_TYPE(pdata);
	pxRequest->ucRequest = STP_REQUEST(pdata);
	pxRequest->usValue   = STP_VALUE(pdata);
	pxRequest->usIndex   = STP_INDEX(pdata);
	pxRequest->usLength  = STP_LENGTH(pdata);
	
	TRACE_USV(("Vendor specific request %d %d %d\n\r", pxRequest->usLength, gChargePermitted, gLocked);)
	
	switch(pxRequest->ucRequest) {
	    case USB_VENREQ_GET_SERIAL:
			TRACE_USV(("USB_VENREQ_GET_SERIAL %x\n\r", PUSD_GetBaseInfo()->serial);)
		{
			LongWord serial = prdInfoGet()->serialNumber;
			buf[0] = (serial >> 24) & 0xff;
			buf[1] = (serial >> 16) & 0xff;
			buf[2] = (serial >> 8) & 0xff;
			buf[3] = (serial >> 0) & 0xff;
			
			sendControlData(buf, pxRequest->usLength, 4, true);
		}
			break;
			
		case USB_VENREQ_SET_SERIAL:
			TRACE_USV(("USB_VENREQ_SET_SERIAL\n\r");)
			controlBeginRXData(pxRequest->usLength, serialNumReceived, NULL);
			break;
			
		case USB_VENREQ_GET_RTC:
		{
			LongWord rtc = rtcGetRtValue();
			buf[0] = (rtc >> 24) & 0xff;
			buf[1] = (rtc >> 16) & 0xff;
			buf[2] = (rtc >> 8) & 0xff;
			buf[3] = (rtc >> 0) & 0xff;
			
			sendControlData((Byte *)buf, pxRequest->usLength, 4, true);
			TRACE_USV(("USB_VENREQ_GET_RTC %x\n\r", rtc);)
		}				
			break;
			
		case USB_VENREQ_SET_RTC:
			TRACE_USV(("USB_VENREQ_SET_RTC\n\r");)
			controlBeginRXData(pxRequest->usLength, rtcBaseReceived, NULL);
			break;
			
		case USB_VENREQ_RESET_HW_LOG:
			TRACE_USV(("USB_VENREQ_RESET_HW_LOG\n\r");)
			break;
			
		case USB_VENREQ_GET_HW_LOG:
			TRACE_USV(("USB_VENREQ_GET_HW_LOG\n\r");)
		{
			memset(buf, 0, sizeof(buf));		// Ska det vara 20 byte?
			sendControlData((Byte *)buf, pxRequest->usLength, sizeof(buf), true);
		}
			break;
			
		case USB_VENREQ_GET_HW_VERSION:
			TRACE_USV(("USB_VENREQ_GET_HW_VERSION\n\r");)
		{
			Word version = prdInfoGet()->hwVer;
			buf[0] = (version >> 8) & 0xff;			// MSB first
			buf[1] = (version >> 0) & 0xff;			// LSB later

			sendControlData((Byte *)buf, pxRequest->usLength, 2, true);
		}
			break;
			
		case USB_VENREQ_GET_SW_VERSION:
			TRACE_USV(("USB_VENREQ_GET_SW_VERSION\n\r");)
		{
			Word version = prdInfoGet()->swVer;
			buf[0] = (version >> 8) & 0xff;			// MSB first
			buf[1] = (version >> 0) & 0xff;			// LSB later

			sendControlData((Byte *)buf, pxRequest->usLength, 2, true);
		}
			break;
			
		case USB_VENREQ_RESET:
			TRACE_USV(("USB_VENREQ_RESET\n\r");)
			
			transmitNull();
			ev.source  = KEVS_VendorRequest;
			ev.vr.what = KVR_Reset;
			EM_PostEvent(&ev, 0);
			break;
			
		case USB_VENREQ_IS_CHARGING:
			TRACE_USV(("USB_VENREQ_IS_CHARGING\n\r");)
		{
			chgSt cs;
			puusbBattChgStatus(&cs);
			buf[0]  = cs.chargingAllowed  ? (1 << 0) : 0; 
			buf[0] |= cs.batteryIsCharged ? (1 << 1) : 0; 
			sendControlData((Byte *)buf, pxRequest->usLength, 1, true);
		}
			break;
			
		case USB_VENREQ_LOCK:
		case USB_VENREQ_UNLOCK:
			TRACE_USV(("USB_VENREQ_LOCK/USB_VENREQ_UNLOCK\n\r");)
			
			transmitNull();
			ev.source   = KEVS_VendorRequest;
			ev.vr.what  = KVR_IndData;
			ev.vr.value = (pxRequest->ucRequest == USB_VENREQ_LOCK);
			EM_PostEvent(&ev, 0);
			break;
			
		case USB_VENREQ_PERMIT_CHARGE:
		case USB_VENREQ_FORBID_CHARGE:
			TRACE_USV(("USB_VENREQ_PERMIT_CHARGE/USB_VENREQ_FORBID_CHARGE\n\r");)
			
			transmitNull();
			ev.source   = KEVS_VendorRequest;
			ev.vr.what  = KVR_BattChg;
			ev.vr.value = (pxRequest->ucRequest == USB_VENREQ_PERMIT_CHARGE);
			EM_PostEvent(&ev, 0);
			break;
			
		default:
            controlSendStall();
			break;
	}
}

/*******************************************************************************
 * Function:	USBVenReq_Init
 * Returns:		void
 *
 * Summary:		Initialize USBVendor Requests handler
 *******************************************************************************/
void usbVenReq_Init()
{
	// Global variable to defualt value
	vendor.rxInBuf = false;
	
	mst_set_vendorReq(vendorRequestHandler4hcc);
}

/*******************************************************************************
 * Function:	usbVenReq_Maintenace
 *
 * Summary:		Store received data from USB host
 *******************************************************************************/
void usbVenReq_Maintenace(void)
{
	if (vendor.rxInBuf)
	{
		vendor.rxHandler(vendor.rxBuf, vendor.rxCnt);
		vendor.rxHandler = (void *) 0;
		vendor.rxInBuf   = false;
	}
}

