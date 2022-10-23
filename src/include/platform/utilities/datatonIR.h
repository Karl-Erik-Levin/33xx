/** datatonIR.h - Declarations connected to the Dataton IR protocol

	© Copyright Dataton AB 2004, All Rights Reserved
	
	Created: 2004-05-13 by Svante Arvedahl
	Modifyed:2004-09-14 by Kalle Levin. Implement "Dataton IR-Protocol v 0.5"
*/

#ifndef _DATATONIR_
#define _DATATONIR_

// Protocol Type
#define	PROTTYPE_MASK			0x0f
#define	ORIG_PROTTYPE			0xf
#define	CURR_PROTTYPE			0xe

// Product ids
#define PRODUCTID_MASK			0xf0
#define PRODUCTID_BIT_OFFSET	4
#define PICKUP_PRODID			1
#define HOTSPOT_PRODID			2
#define WIREHOTSPOT_PRODID		3
#define TOUCHDOWN_PRODID		4

#define IRLAP_TEST				0xf3

// Commands
#define COMMAND_MASK			0x7f
#define CMD_NONE				0
#define CMD_START_WO			1
#define CMD_REQ_HS_ID			1
#define CMD_SET_NEW_HS_ID		2
#define CMD_REQ_HS_VER			3
#define CMD_REQ_HS_CAL			4
#define CMD_SET_NEW_HS_CAL		5

// Pickup buttons
#define PUBUT_PLAY				(1<<0)		// Play
#define PUBUT_REW				(1<<0)		// Rewind
#define PUBUT_FWD				(1<<0)		// Forward
#define PUBUT_INCVOL			(1<<0)		// Increase volume
#define PUBUT_DECVOL			(1<<0)		// Decrease volume
#define PUBUT_INCTONE			(1<<0)		// Increase tone
#define PUBUT_DECTONE			(1<<0)		// Decrease tone

/// Indicate that high power ir transmission is used
#define HIGH_POWER				0x80

/// The bit size of a HOTSPOT ID
#define HOTSPOT_ID_SIZE			14
#define MORE_BITMAP				0x80

// Common properties
#define PROP_TRANS_ID			(1<<0)		// BitMap_0_7
#define PROP_RETR_BITMAP		(1<<1)		// BitMap_0_7
#define PROP_SERIAL_NUM			(1<<2)		// BitMap_0_7
#define PROP_HARDWARE_NUM		(1<<3)		// BitMap_0_7

// PICKUP properties
#define PROP_PU_NEW_HOTSPOT_ID	(1<<4)		// BitMap_0_7
#define PROP_PU_TOUR_ID			(1<<5)		// BitMap_0_7
#define PROP_PU_BUTTON_PRESS	(1<<6)		// BitMap_0_7
#define PROP_PU_NEXT_BITMAP		(1<<7)		// BitMap_0_7
#define PROP_PU_CUSTOM_DATA		(1<<0)		// BitMap_8_14

// WireHOTSPOT properties
#define PROP_WH_HOTSPOT_ID		(1<<4)		// BitMap_0_7
#define PROP_WH_TOUR_ID			(1<<5)		// BitMap_0_7
#define PROP_WH_WO_STATE		(1<<6)		// BitMap_0_7
#define PROP_WH_NEXT_BITMAP		(1<<7)		// BitMap_0_7
#define PROP_WH_AUTO_PLAY_FLAGS	(1<<0)		// BitMap_8_14
#define PROP_WH_CUSTOM_DATA		(1<<1)		// BitMap_8_14

// Common property sizes
#define PROP_TOUR_ID_SIZE				HOTSPOT_ID_SIZE

// PICKUP property bit sizes
#define PROP_PU_BUTTON_PRESS_SIZE		7
#define PROP_PU_NEW_HOTSPOT_ID_SIZE		HOTSPOT_ID_SIZE
#define PROP_PU_NEXT_BITMAP_SIZE		8
#define PROP_PU_CUSTOM_DATA_SIZE		8

// WireHOTSPOT property sizes
#define PROP_WH_HOTSPOT_ID_SIZE			HOTSPOT_ID_SIZE
#define PROP_WH_WO_STATE_SIZE			32
#define PROP_WH_NEXT_BITMAP_SIZE		8
#define PROP_WH_AUTO_PLAY_FLAGS_SIZE	1
#define PROP_WH_CUSTOM_DATA_SIZE		8

// Other WireHOTSPOT defines
#define WO_STATE_LOAD_FLAG_OFFSET		30
#define WO_STATE_RUN_FLAG_OFFSET		31
#define WO_STATE_TIME_MASK				0x3fffffff


/** Declarations */
typedef Word HotspotIDType;
typedef Word SerialNumberType;
typedef Byte HardwareNumberType;

typedef struct _PickupE {
	Byte				transactionId;
	Byte				reqBitMap_0_7;
	Byte				reqBitMap_8_14;
	SerialNumberType	serialNumber;
	HardwareNumberType	hardwareNumber;
	HotspotIDType		newHotspotId;
	HotspotIDType		tourId;
	Byte				buttonPressed;
	Byte				customDataLen;
	Byte				*customData;
} PickupE;

typedef struct _WireHSE {
	Byte				transactionId;
	Byte				reqBitMap_0_7;
	Byte				reqBitMap_8_14;
	SerialNumberType	serialNumber;
	HardwareNumberType	hardwareNumber;
	LongWord			hotspotId;		// To handle old protcoll
	HotspotIDType		tourId;
	LongWord			woTimePos;
	Boolean				woIsLoadingShow;
	Boolean				woIsRunningShow;
	Boolean				implicitPlay;
	Boolean				forceImplicitPlay;
	Byte				customDataLen;
	Byte				*customData;
} WireHSE;

typedef struct _irMsg {
	Byte		protocolType;
	Byte		productId;
	Byte		command;
	Boolean		irSendPower;		// True is high power
	Byte		msgBitMap_0_7;
	Byte		msgBitMap_8_14;
	LongWord	senderID;			// Old type protocol
	union {
		PickupE	pe;
		WireHSE	we;
	};
} irMsg;


/** Functions */
Boolean DirParseMsg(Byte *irMsgBuf, Byte lenIrMsg, irMsg *msg, Byte requestedProductId);
void DirMakeMsg(Byte *irMsgBuf, Byte *lenIrMsg, irMsg *msg);

#endif /// _DATATONIR_
