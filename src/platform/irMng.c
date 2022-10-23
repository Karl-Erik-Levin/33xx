/* irMng.c - Dataton 3397 Transponder 
**
** IR Communication Manager
**
**
** Created 09-09-24	Kalle
**
** (C) Copyright Dataton AB 2009, All Rights Reserved
**
*/

//---------------------------------INCLUDES---------------------------------------
#include "MFLib\Dataton_Types.h"

#include "driver\irdaDrv.h"
#include "driver\tmrDrv.h"

#include "platform\utilities\crc16CCITT.h"
#include "platform\utilities\datatonIR.h"
#include "platform\utilities\guideProtcol.h"

#include "platform\evtMng.h"
#include "platform\irMng.h"

#include "platform\FreeRTOS\FreeRTOS.h"
#include "platform\FreeRTOS\task.h"
#include "platform\FreeRTOS\queue.h"

//---------------------------------DEFINES----------------------------------------
#define IRTASK_STACK_SIZE 			140
#define IRTASK_NAME					"IRCT"
#define IRTASK_PRIORITY				(tskIDLE_PRIORITY + 3)


#define MAXTRSPID					9999		// Highest transponder ID

//---------------------------------TYPEDEFS---------------------------------------
/* Structure used to IR communicators global data */
typedef struct _irInfo
{
	Byte		protocolType;
	Byte		command;
	Boolean		irSendPower;
	LongWord	senderID;
} irInfo;

typedef struct _rxMsg
{
	Byte	msg[IR_MESSAGE_SIZE];
	Byte	msgSize;
} rxMsg;

/* IR communicator command model */
typedef enum
{
	IRCMD_ReceivedIrDA,
	IRCMD_RequestID,
	IRCMD_SetID,
	IRCMD_AnswReqID,
	IRCMD_AnswSetID
} irCmd;

typedef struct _IRCCommand
{
	irCmd	sel;
	Word	id;
} IRCCommand;

typedef enum
{
	EIRXS_DetectFrame,
	EIRXS_SkipFrameHeader,
	EIRXS_ReceiveFrame,
	EIRXS_ReceiveFrameEscape
} IrDaRXState;

//------------------------------GLOBAL VARIABLES----------------------------------
static xQueueHandle irQueue;
static xTaskHandle irTaskHandle;

static irInfo iri;
static rxMsg rx;
static Byte txBuf[IR_MESSAGE_SIZE];

//--------------------------------FUNCTIONS---------------------------------------
/*******************************************************************************
 * Function:	escapeAndInsert
 * 
 * Summary:		
 *******************************************************************************/
static Byte*
escapeAndInsert(
	Byte *txBuf,
	Byte msgData
) {
	if (msgData == kBOF || msgData == kEOF || msgData == kCE) {
		*txBuf = kCE;
		txBuf++;
		*txBuf = msgData ^ kCExorValue;
	}
	else {
		*txBuf = msgData;
	}

	return ++txBuf;
}

/*******************************************************************************
 * Function:	prepareMessage
 * 
 * Summary:		Prepare message for transmission by inserting sync header(BOF),
 *				calculate CRC and insert escape sequences for control characters.
 *******************************************************************************/
static Word
prepareMessage(
	Byte *source,
	Word length,
	Byte *dest
) {
	int i;
	Word crc;
	Byte *txDataPtr = dest + 3;
		
	dest[0] = kBOF;
	dest[1] = kBOF;
	dest[2] = kBOF;
	
	for (i = 0; i < length; i++) {
		txDataPtr = escapeAndInsert(txDataPtr, source[i]);
	}
	
	crc = crc16CCITT(kInitCrc16, source, length) ^ 0xFFFF;
	txDataPtr = escapeAndInsert(txDataPtr, crc & 0xff);				// CRC lo
	txDataPtr = escapeAndInsert(txDataPtr, (crc >> 8) & 0xff);		// CRC high
	*txDataPtr = kEOF;

	return  1 + txDataPtr - dest;
}

/*******************************************************************************
 * Function:	sendAnswReqID
 * 
 * Summary:		
 *******************************************************************************/
static void
sendAnswReqID(
	LongWord id,
	LongWord senderId,
	Boolean irSendPower		// True is high power
){
	Word len;
	Byte msg[9];

	msg[0] = kFrameAddr;
	msg[1] = kFrameCtrl;
	msg[2] = kMsgReqId;
	if (irSendPower)
		msg[2] |= 0x80;
		
	msg[3] = senderId & 0xff;
	msg[4] = (senderId >> 8) & 0xff;
	msg[5] = (senderId >> 16) & 0xff;
	
	msg[6] = id & 0xff;
	msg[7] = (id >> 8) & 0xff;
	msg[8] = (id >> 16) & 0xff;

	len = prepareMessage(msg, 9, txBuf);
	irdaSend(txBuf, len, irSendPower);
}

/*******************************************************************************
 * Function:	sendAnswSetID
 * 
 * Summary:		Send answer with old IR protocol to simulate battery transponde
 *******************************************************************************/
static void
sendAnswSetID(
	LongWord id,
	LongWord senderId,
	Boolean irSendPower		// True is high power
){
	Word len;
	Byte msg[9];

	msg[0] = kFrameAddr;
	msg[1] = kFrameCtrl;
	msg[2] = kMsgSetId;
	if (irSendPower)
		msg[2] |= 0x80;
		
	msg[3] = senderId & 0xff;
	msg[4] = (senderId >> 8) & 0xff;
	msg[5] = (senderId >> 16) & 0xff;
	
	msg[6] = id & 0xff;
	msg[7] = (id >> 8) & 0xff;
	msg[8] = (id >> 16) & 0xff;

	len = prepareMessage(msg, 9, txBuf);
	irdaSend(txBuf, len, irSendPower);
}

/*******************************************************************************
 * Function:	decodeMessage
 * 
 * Summary:		
 *******************************************************************************/
static void
decodeMessage(
		Byte *msg,
		Word msgLen
) {
	irMsg rxMsg;
	EMEvent ep;

	if (!DirParseMsg(msg, msgLen, &rxMsg, 0))
		return;
	
	if ((rxMsg.productId == PICKUP_PRODID) ||
		(rxMsg.productId == TOUCHDOWN_PRODID)
		) {
		
		iri.protocolType= rxMsg.protocolType;
		iri.command		= rxMsg.command;
		iri.irSendPower = rxMsg.irSendPower;
		iri.senderID	= rxMsg.senderID;
		
		memset(&ep, 0, sizeof(ep));

		ep.source = KEVS_IRCom;
				
		switch (rxMsg.command) {

		case kMsgReqId:
			ep.ir.what = KEHSC_IDRequest;
			EM_PostEvent(&ep, 5);
			break;
			
		case kMsgAckId:
			ep.ir.what = KEHSC_IDAck;
			EM_PostEvent(&ep, 5);
			break;
			
		case kMsgSetId:
			ep.ir.what = KEHSC_IDSet;
			ep.ir.id = rxMsg.pe.newHotspotId;
			EM_PostEvent(&ep, 5);
			break;
		}
	}
}

/*******************************************************************************
 * Function:	irReceivedCallback
 * Returns:		Boolean
 * 
 * Summary:		Callback from IRDA driver. Called in interrupt mode
 *				Returns TRUE if this call has caused a task to wake up, else FALSE
 * NOTE:		Called whenever a read request has completed. Will put the 
 *				completed request on a completed queue, to be handles by irTask
 *******************************************************************************/
static Boolean							
irReceivedCallback(
		Byte msgSize,			// Number of bytes received	
		Byte *msg				// Pointer to data buffer
) {
	portBASE_TYPE taskWoken = pdFALSE;
	IRCCommand cmd;

	if (msgSize>0 && msgSize<IR_MESSAGE_SIZE) {
		rx.msgSize = msgSize;
		memcpy(rx.msg, msg, msgSize);

		cmd.sel = IRCMD_ReceivedIrDA;
		taskWoken = xQueueSendFromISR(irQueue, &cmd, &taskWoken);
	}

	return taskWoken;
}

/*******************************************************************************
 * Function:	decodeIrDAMessage
 * 
 * Summary:		Remove esc-sequence from recevied IR messages and check CRC
 *******************************************************************************/
static void
decodeIrDAMessage()
{
	IrDaRXState rxState = EIRXS_DetectFrame;
	Word rxCrc, rxCnt;
	Byte rxData, rxBuf[IR_MESSAGE_SIZE];
	int i;
	
	if (rx.msgSize > IR_MESSAGE_SIZE) rx.msgSize = IR_MESSAGE_SIZE;
	
	for (i = 0; i < rx.msgSize; i++) {
		rxData = rx.msg[i];
		switch (rxState) {
			case EIRXS_DetectFrame:
				if (rx.msg[i] == kBOF) {
					rxCnt = 1;
					rxState = EIRXS_SkipFrameHeader;
					rxCrc = kInitCrc16;
				}
				break;

			case EIRXS_SkipFrameHeader:
				if (rxData == kBOF) {					// Still in frame header...
					rxCnt++;
					if (rxCnt > 5)	{					// Frame header is to big, this is probably an non corretly formmated message.
						rxState = EIRXS_DetectFrame;
					}
					break;
				}

				// State of frame data detected.
				rxCnt = 0;
				rxState = EIRXS_ReceiveFrame;
				// Fall through

			case EIRXS_ReceiveFrame:
				if (rxData == kBOF)						// Frame header in data... Illegal.
				{	
					rxState = EIRXS_DetectFrame;
					break;								// Change to goto?? or i--??
				}
				
				if (rxData == kCE) {					// Escaped byte dtected.
					rxState = EIRXS_ReceiveFrameEscape;
					break;
				}

				if (rxCnt < IR_MESSAGE_SIZE) {	// Receive data....
					rxBuf[rxCnt] = rxData;
					rxCnt++;
				}
				
				if (rxData == kEOF) {					// End of frame detected...
					rxState = EIRXS_DetectFrame;

					if (rxCrc == CRC16GOOD) {			// CRC OK
						decodeMessage(rxBuf, rxCnt);	// Execute message
					}
					
				}
				else {									// Accumulate CRC
					rxCrc = crc16CCITT(rxCrc, &rxData, 1);
				}
				break;
				
			case EIRXS_ReceiveFrameEscape:
				rxData ^= kCExorValue;
				rxState = EIRXS_ReceiveFrame;

				if (rxCnt < IR_MESSAGE_SIZE) {	// Receive data....
					rxBuf[rxCnt] = rxData;
					rxCnt++;
					rxCrc = crc16CCITT(rxCrc, &rxData, 1);
				}
				
				break;
		}
	}
}

/*******************************************************************************
 * Function:	irTask
 * 
 * Summary:		
 *******************************************************************************/
static void 
irTask(void* parameters)
{
	IRCCommand cmd;
	
	while (1) {
		if (xQueueReceive(irQueue, &cmd, 1000) == pdTRUE) {
			switch (cmd.sel) {
			case IRCMD_ReceivedIrDA:
				decodeIrDAMessage();
				break;

		    case IRCMD_RequestID:
				break;
			
			case IRCMD_SetID:
				break;
			
			case IRCMD_AnswReqID:
				sendAnswReqID(cmd.id, iri.senderID, iri.irSendPower);
				break;
			
			case IRCMD_AnswSetID:
				sendAnswSetID(cmd.id, iri.senderID, iri.irSendPower);
				break;
			}
		}
		else
		{
			// Enter here every 1000 ticks
		}
	}
}

/*******************************************************************************
 * Function:	irInit
 * 
 * Summary:		Start IR manager
 *******************************************************************************/
void
irInit()
{
	irdaInit(irReceivedCallback);		// Init IrDA driver
	
	memset(&iri, 0, sizeof(iri));

	irQueue = xQueueCreate(3, sizeof(IRCCommand));   
	xTaskCreate(irTask, 
                IRTASK_NAME, 
                IRTASK_STACK_SIZE, 
                NULL, 
                IRTASK_PRIORITY,
                &irTaskHandle);
}

/*******************************************************************************
 * Function:	irClose
 * 
 * Summary:		Stop IR manager
 *******************************************************************************/
void
irClose()
{
	irdaClose();					// Stop and close IrDA driver

	vTaskDelete(irTaskHandle);
	vQueueDelete(irQueue);
}

/*******************************************************************************
 * Function:	irSleep
 * 
 * Summary:		Put IR manager into hibernation
 *******************************************************************************/
void irSleep()
{
	irdaSleep();
}

/*******************************************************************************
 * Function:	irWake
 * 
 * Summary:		Wake IR manager from hibernation 
 *******************************************************************************/
void irWake()
{
	irdaWake();
}

/*******************************************************************************
 * Function:	irAnswReqID
 * 
 * Summary:		Send answer to PICKUP after Request ID
 *******************************************************************************/
void
irAnswReqID(Word id)
{
	IRCCommand cmd;
	
	cmd.sel = IRCMD_AnswReqID;
	if (id > MAXTRSPID) id = MAXTRSPID;
	cmd.id = id;
	
	xQueueSend(irQueue, &cmd, 0);
}

/*******************************************************************************
 * Function:	irAnswSetID
 * 
 * Summary:		Send answer to PICKUP after Set ID
 *******************************************************************************/
void
irAnswSetID(Word id)
{
	IRCCommand cmd;
	
	cmd.sel = IRCMD_AnswSetID;
	if (id > MAXTRSPID) id = MAXTRSPID;
	cmd.id = id;
	
	xQueueSend(irQueue, &cmd, 0);
}

