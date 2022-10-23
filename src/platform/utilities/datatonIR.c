/* datatonIR.c - Implementation of Dataton IR protocol specific functions

  Copyright Dataton AB 2004, All Rights Reserved
	
	Created: 2004-05-13 by Svante Arvedahl
	Modifyed:2004-09-14 by Kalle Levin.
					Implement "Dataton IR-Protocol v 0.5"
*/

//---------------------------------INCLUDES---------------------------------------
#include "MFLib\Dataton_Types.h"

#include "platform\FreeRTOS\FreeRTOS.h"
#include "platform\utilities\guideProtcol.h"
#include "platform\utilities\datatonIR.h"

//--------------------------------FUNCTIONS---------------------------------------
/*******************************************************************************
 * Function:	parseMsgBits
 * Returns:		LongWord
 * 
 * Summary:		
 *******************************************************************************/
static LongWord
parseMsgBits(
	Byte **byteBufPtr, 
	Byte *bit, 
	Byte numBit
) {
	Byte *byteBuf = *byteBufPtr, i;
	int mask = 0xFF;
	LongWord retValue = 0;

	for (i=0; i<numBit; i+=8) {
		if ((numBit-i) < 8) {
			mask >>= 8 - (numBit-i);
		}
		
		retValue |= (LongWord)(((byteBuf[0] + 256 * byteBuf[1]) >> *bit) & mask) << i;
		byteBuf++;
	}
	
	i = *bit + numBit;
	*byteBufPtr += i / 8;
	*bit = i % 8;
	
	return retValue;
}

/*******************************************************************************
 * Function:	assembleMsgBits
 * Returns:		void
 * 
 * Summary:		
 *******************************************************************************/
static void
assembleMsgBits(
	Byte **byteBufPtr, 
	Byte *bit, 
	Byte numBit, 
	LongWord propToSet
) {
	Byte *byteBuf = *byteBufPtr, i, j;
	Word mask = 0xFF << *bit, dataByte, bufByte;

	for (j=0, i=0; i<numBit; j++, i+=8) {
		if ((numBit-i) < 8) {
			mask &= mask >> (8 - (numBit-i));
		}
		
		dataByte = (propToSet << *bit) & mask;
		propToSet >>= 8;
		bufByte = byteBuf[j] + 256 * byteBuf[j+1];
		bufByte &= ~mask;
		bufByte |= dataByte;
		byteBuf[j]   = bufByte;
		byteBuf[j+1] = bufByte >> 8;
	}
	
	i = *bit + numBit;
	*byteBufPtr += i / 8;
	*bit = i % 8;
}

/*******************************************************************************
 * Function:	DirParseMsg
 * Returns:		Boolean
 * 
 * Summary:		
 *******************************************************************************/
Boolean
DirParseMsg(
	Byte *irMsgBuf, 
	Byte lenIrMsg, 
	irMsg *msg, 
	Byte requestedProductId
) {
	Boolean parseOK = false;
	Byte i, *currBufByte, currBufBit = 0;
	register Byte bm;

	switch (msg->productId) {				// Deallocate old custom data string
		case PICKUP_PRODID:
			if (msg->pe.customDataLen && msg->pe.customData) {
				vPortFree(msg->pe.customData);
			}
			break;
		case WIREHOTSPOT_PRODID:
			if (msg->we.customDataLen && msg->we.customData) {
				vPortFree(msg->we.customData);
			}
			break;
	}
	memset(msg, 0, sizeof(irMsg));			// Set default value in all fields
	msg->senderID = (LongWord) ((irMsgBuf[5]<<16) | (irMsgBuf[4]<<8) | irMsgBuf[3]);
	
	if (irMsgBuf[0] == kFrameAddr &&  irMsgBuf[1] == kFrameCtrl) {
	
		// This is old protocol style message
		msg->protocolType   = ORIG_PROTTYPE;
		msg->productId      = PICKUP_PRODID;
		msg->command        = irMsgBuf[2] & COMMAND_MASK;
		msg->irSendPower    = (irMsgBuf[2] & HIGH_POWER) ? true : false;
		msg->pe.newHotspotId= (LongWord) ((irMsgBuf[8]<<16) | (irMsgBuf[7]<<8) | irMsgBuf[6]);
		parseOK = true;
		
	} else {

		// This is the new protocol.
		msg->protocolType = irMsgBuf[0] & PROTTYPE_MASK;
		msg->productId = (irMsgBuf[0] & PRODUCTID_MASK) >> PRODUCTID_BIT_OFFSET;
		
		if ((msg->protocolType == CURR_PROTTYPE) &&
			(requestedProductId == 0) || (requestedProductId == msg->productId)) {
			
			msg->command = irMsgBuf[2] & COMMAND_MASK;
			msg->irSendPower = irMsgBuf[2] & HIGH_POWER;
			
			bm = msg->msgBitMap_0_7 = irMsgBuf[3];
			currBufByte = &(irMsgBuf[4]);
			
			switch (msg->productId) {
			case PICKUP_PRODID:
				parseOK = true;
				if (bm & PROP_TRANS_ID)
					msg->pe.transactionId = *currBufByte++;
			
				if (bm & PROP_RETR_BITMAP) {
					msg->pe.reqBitMap_0_7 = *currBufByte++;
					if (msg->pe.reqBitMap_0_7 & MORE_BITMAP)
						msg->pe.reqBitMap_8_14 = *currBufByte++;
				}
			
				if (bm & PROP_SERIAL_NUM)
					msg->pe.serialNumber = *currBufByte++ + 256 * *currBufByte++;
			
				if (bm & PROP_HARDWARE_NUM)
					msg->pe.hardwareNumber = *currBufByte++;
			
				if (bm & PROP_PU_NEW_HOTSPOT_ID)
					msg->pe.newHotspotId = parseMsgBits(&currBufByte, &currBufBit, PROP_PU_NEW_HOTSPOT_ID_SIZE);
					
				if (bm & PROP_PU_TOUR_ID)
					msg->pe.tourId = parseMsgBits(&currBufByte, &currBufBit, PROP_TOUR_ID_SIZE);
					
				if (bm & PROP_PU_BUTTON_PRESS)
					msg->pe.buttonPressed = parseMsgBits(&currBufByte, &currBufBit, PROP_PU_BUTTON_PRESS_SIZE);
					
				if (bm & PROP_PU_NEXT_BITMAP) {
					bm = msg->msgBitMap_8_14 = parseMsgBits(&currBufByte, &currBufBit, PROP_PU_NEXT_BITMAP_SIZE);

					if (bm & PROP_PU_CUSTOM_DATA) {
						msg->pe.customDataLen = parseMsgBits(&currBufByte, &currBufBit, PROP_PU_CUSTOM_DATA_SIZE);
						if (msg->pe.customDataLen) {
							msg->pe.customData = (Byte *) pvPortMalloc(msg->pe.customDataLen + 1);
							for (i=0; i<msg->pe.customDataLen; i++)
								msg->pe.customData[i] = parseMsgBits(&currBufByte, &currBufBit, 8);
							msg->pe.customData[i] = 0;
						}
					}
				}
				break;
			
			case WIREHOTSPOT_PRODID:
				parseOK = true;
				if (bm & PROP_TRANS_ID)
					msg->we.transactionId = *currBufByte++;
				
				if (bm & PROP_RETR_BITMAP) {
					msg->we.reqBitMap_0_7 = *currBufByte++;
					if (msg->we.reqBitMap_0_7 & MORE_BITMAP)
						msg->we.reqBitMap_8_14 = *currBufByte++;
				}
				
				if (bm & PROP_SERIAL_NUM)
					msg->we.serialNumber = *currBufByte++ + 256 * *currBufByte++;
				
				if (bm & PROP_HARDWARE_NUM)
					msg->we.hardwareNumber = *currBufByte++;
					
				if (bm & PROP_WH_HOTSPOT_ID) 
					msg->we.hotspotId = parseMsgBits(&currBufByte, &currBufBit, PROP_WH_HOTSPOT_ID_SIZE);

				if (bm & PROP_WH_TOUR_ID) 
					msg->we.tourId = parseMsgBits(&currBufByte, &currBufBit, PROP_TOUR_ID_SIZE);
					
				if (bm & PROP_WH_WO_STATE) {
					msg->we.woTimePos = parseMsgBits(&currBufByte, &currBufBit, PROP_WH_WO_STATE_SIZE);
					if (msg->we.woTimePos & (1L<<WO_STATE_LOAD_FLAG_OFFSET))  msg->we.woIsLoadingShow = true;
					if (msg->we.woTimePos & (1UL<<WO_STATE_RUN_FLAG_OFFSET))  msg->we.woIsRunningShow = true;
					msg->we.woTimePos &= WO_STATE_TIME_MASK;
				}
				
				if (bm & PROP_WH_NEXT_BITMAP) {
					bm = msg->msgBitMap_8_14 = parseMsgBits(&currBufByte, &currBufBit, PROP_WH_NEXT_BITMAP_SIZE);

					if (bm & PROP_WH_AUTO_PLAY_FLAGS) {
						msg->we.implicitPlay = parseMsgBits(&currBufByte, &currBufBit, PROP_WH_AUTO_PLAY_FLAGS_SIZE);
						msg->we.forceImplicitPlay = parseMsgBits(&currBufByte, &currBufBit, PROP_WH_AUTO_PLAY_FLAGS_SIZE);
					}
					
					if (bm & PROP_WH_CUSTOM_DATA) {
						msg->we.customDataLen = parseMsgBits(&currBufByte, &currBufBit, PROP_WH_CUSTOM_DATA_SIZE);
						if (msg->we.customDataLen) {
							msg->we.customData = (Byte *) pvPortMalloc(msg->we.customDataLen + 1);
							for (i=0; i<msg->we.customDataLen; i++)
								msg->we.customData[i] = parseMsgBits(&currBufByte, &currBufBit, 8);
							msg->we.customData[i] = 0;
						}
					}
				}
				break;
			}
		}
	}
	return (parseOK && ((currBufByte - irMsgBuf + 1) <= lenIrMsg));
}

/*******************************************************************************
 * Function:	DirMakeMsg
 * Returns:		void
 * 
 * Summary:		
 *******************************************************************************/
void
DirMakeMsg(
	Byte *irMsgBuf, 
	Byte *lenIrMsg, 
	irMsg *msg
) {
	Byte i, *currBufByte, currBufBit = 0;
	register Byte bm;

	*lenIrMsg = 0;
	
	if (msg->protocolType == ORIG_PROTTYPE) {
		*lenIrMsg = 6;
		irMsgBuf[0] = kFrameAddr;
		irMsgBuf[1] = kFrameCtrl;
		irMsgBuf[2] = msg->command;
		if (msg->irSendPower) {
			irMsgBuf[2] |= HIGH_POWER;
		}
		irMsgBuf[3] = msg->pe.serialNumber;
		irMsgBuf[4] = msg->pe.serialNumber >>  8;
		irMsgBuf[5] = 0;
		
		if (msg->command==kMsgSetId || msg->command==kMsgSetCal) {
			*lenIrMsg = 9;
			irMsgBuf[6] = msg->pe.newHotspotId;
			irMsgBuf[7] = msg->pe.newHotspotId >>  8;
			irMsgBuf[8] = 0;
		}
	} 
	else {
		irMsgBuf[0] = msg->protocolType & PROTTYPE_MASK;
		irMsgBuf[0] |= (msg->productId << PRODUCTID_BIT_OFFSET) & PRODUCTID_MASK;
	
		irMsgBuf[1] = IRLAP_TEST;
		
		irMsgBuf[2] = msg->command & COMMAND_MASK;
		if (msg->irSendPower) irMsgBuf[2] |= HIGH_POWER;
	
		bm = irMsgBuf[3] = msg->msgBitMap_0_7;
		currBufByte = &(irMsgBuf[4]);
		
		switch (msg->productId) {
			case PICKUP_PRODID:
				if (bm & PROP_TRANS_ID)
					*currBufByte++ = msg->pe.transactionId;
				
				if (bm & PROP_RETR_BITMAP) {
					*currBufByte++ = msg->pe.reqBitMap_0_7;
					if (msg->pe.reqBitMap_0_7 & MORE_BITMAP)
						*currBufByte++ = msg->pe.reqBitMap_8_14;
				}
				
				if (bm & PROP_SERIAL_NUM) {
					*currBufByte++ = msg->pe.serialNumber & 0xFF;
					*currBufByte++ = (msg->pe.serialNumber >> 8) & 0xFF;
				}
				
				if (bm & PROP_HARDWARE_NUM)
					*currBufByte++ = msg->pe.hardwareNumber;
				
				if (bm & PROP_PU_NEW_HOTSPOT_ID)
					assembleMsgBits(&currBufByte, &currBufBit, PROP_PU_NEW_HOTSPOT_ID_SIZE, msg->pe.newHotspotId);
					
				if (bm & PROP_PU_TOUR_ID)
					assembleMsgBits(&currBufByte, &currBufBit, PROP_TOUR_ID_SIZE, msg->pe.tourId);
					
				if (bm & PROP_PU_BUTTON_PRESS)
					assembleMsgBits(&currBufByte, &currBufBit, PROP_PU_BUTTON_PRESS_SIZE, msg->pe.buttonPressed);
					
				if (bm & PROP_PU_NEXT_BITMAP) {
					bm = msg->msgBitMap_8_14;
					assembleMsgBits(&currBufByte, &currBufBit, PROP_PU_NEXT_BITMAP_SIZE, msg->msgBitMap_8_14);
		
					if (bm & PROP_PU_CUSTOM_DATA) {
						assembleMsgBits(&currBufByte, &currBufBit, PROP_PU_CUSTOM_DATA_SIZE, msg->pe.customDataLen);
						if (msg->pe.customDataLen) {
							for (i=0; i<msg->pe.customDataLen; i++)
								assembleMsgBits(&currBufByte, &currBufBit, 8, msg->pe.customData[i]);
						}
					}
				}
				
				// Add padding bits to fill last byte
				if (currBufBit)
					assembleMsgBits(&currBufByte, &currBufBit, (8-currBufBit), 0);
				*lenIrMsg = currBufByte - irMsgBuf;
				break;
				
			case WIREHOTSPOT_PRODID:
				break;
		}
	}
}

