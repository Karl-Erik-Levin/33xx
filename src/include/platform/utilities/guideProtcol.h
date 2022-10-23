/*  GuideProtcol.h - Dataton Guide IR-protocol

	Structure of IR messages frame:
	
	BOF ADDR CTRL DATA DATA DATA DATA DATA DATA CRC CRC EOF
	
   		BOF		One or more "Beginig Of Frame"			  0xC0
   		ADDR	One byte, in Dataton guide system allways 0xFF
   		CTRL	One byte, in Dataton guide system allways 0xF3
   		DATA	Zero or more bytes
   		CRC		CCITT 16-bit crc
   		EOF		One "End Of Frame"						  0xC1


© Copyright Dataton AB 2001, All Rights Reserved
Created: 01-03-28 by Kalle Levin
  
*/

#ifndef _GuideProt_
#define _GuideProt_

/* IR message commands */
#define kMsgReqId	0x01	/* Pickup request transponder Id */
#define kMsgSetId	0x02	/* Pickup set a new Id in transponder */
#define kMsgReqVer	0x03	/* Pickup request transponder version */
#define kMsgReqCal	0x04	/* Pickup request transponder calibration byte */
#define kMsgSetCal	0x05	/* Pickup set transponder calibration byte */
#define kMsgAckId	0x06	/* Pickup send this messages to Transponder after received a valid trsp-Id */
#define kMsgHiPwr	0x80	/* Set MSB in messages command if sent in high
							   power mode */

/* CRC Constants */ 
#define kInitCrc16	0xFFFF	/* Initial CRC value */
#define kGoodCrc16	0xF0B8	/* Good final CRC value */
#define kPolyCrc16	0x8408	/* CCITT CRC-16 polynom */

/* Async-HDLC Constants (Frame format used by low speed IrDA) */
#define kBOF		0xC0	/* Beginig Of Frame */
#define kEOF		0xC1	/* End Of Frame */
#define kCE			0x7D	/* Control Escape */
#define kCExorValue	0x20	/* Control Escape xor value (byte after kCE
							   is xored with this value) */
#define kFrameAddr	0xFF	/* Frame address, broadcast in IrLAP */
#define kFrameCtrl	0xF3	/* Frame control, test in IrLAP */

#endif	// _GuideProt_

