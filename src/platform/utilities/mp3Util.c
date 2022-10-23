/* mp3Util.c - Dataton 3356 Pickup
**
** This file contains helper functions to allow a user to easier interact and interface
** to MP3 data streams. Some it comes from the PU-classic source tree.
**
**
** Created 10-10-30	Kalle
**
** (C) Copyright Dataton AB 2010, All Rights Reserved
*/
//---------------------------------INCLUDES---------------------------------------
#include "MFLib\Dataton_Types.h"
#include "MFLib\TraceAssert.h"

#include "platform\hccfat\api_f.h"
#include "platform\utilities\mp3Util.h"

//----------------------------------DEFINES---------------------------------------
#define MAX_SEEK_MP3_FRAME_POS		50
#define MAX_SEEK_ID3_HEADER_POS		50

//---------------------------------TYPEDEFS---------------------------------------
typedef struct KBPS2BytesPerMS
{
	Word kbps;
	Word bytesPerMS;
} KBPS2BytesPerMS;

//------------------------------GLOBAL VARIABLES----------------------------------
/* Maps KPBS to bytes per MS table */
static KBPS2BytesPerMS gBitRateToBytesPerMSTab[]=
{
	{32, 4},
	{40, 5},
	{48, 6},
	{56, 7},
	{64, 8},
	{80, 10},
	{96, 12},
	{112, 14},
	{128, 16},
	{160, 20},
	{192, 24},
	{224, 28},
	{256, 32},
	{320, 40},
	{0,0 }
};

/*	MP3 frame size table

	From the mp3 frame header take MPEG version, bitrate index
	and samplerate index and merge to an index to this table.
	
	MP3 header:		AAAAAAAA AAABBCCD EEEEFFGH IIJJKLMM
	Table index:	EEEEFFBB
	
	MP3 frame size = fs[EEEEFFBB] + G
*/
static const Word fs[] = {	
				0, 
				0,
				0,
				0,
				0,
				0,
				0,
				0,
				0,
				0,
				0,
				0,
				0,
				0,
				0,
				0,
				104,
				0,
				52,
				104,
				96,
				0,
				48,
				96,
				144,
				0,
				72,
				144,
				0,
				0,
				0,
				0,
				208,
				0,
				104,
				130,
				192,
				0,
				96,
				120,
				288,
				0,
				144,
				180,
				0,
				0,
				0,
				0,
				313,
				0,
				156,
				156,
				288,
				0,
				144,
				144,
				432,
				0,
				216,
				216,
				0,
				0,
				0,
				0,
				417,
				0,
				208,
				182,
				384,
				0,
				192,
				168,
				576,
				0,
				288,
				252,
				0,
				0,
				0,
				0,
				522,
				0,
				261,
				208,
				480,
				0,
				240,
				192,
				720,
				0,
				360,
				288,
				0,
				0,
				0,
				0,
				626,
				0,
				313,
				261,
				576,
				0,
				288,
				240,
				864,
				0,
				432,
				360,
				0,
				0,
				0,
				0,
				731,
				0,
				365,
				313,
				672,
				0,
				336,
				288,
				1008,
				0,
				504,
				432,
				0,
				0,
				0,
				0,
				835,
				0,
				417,
				365,
				768,
				0,
				384,
				336,
				1152,
				0,
				576,
				504,
				0,
				0,
				0,
				0,
				1044,
				0,
				522,
				417,
				960,
				0,
				480,
				384,
				1440,
				0,
				720,
				576,
				0,
				0,
				0,
				0,
				1253,
				0,
				626,
				522,
				1152,
				0,
				576,
				480,
				1728,
				0,
				864,
				720,
				0,
				0,
				0,
				0,
				1462,
				0,
				731,
				626,
				1344,
				0,
				672,
				576,
				2016,
				0,
				1008,
				864,
				0,
				0,
				0,
				0,
				1671,
				0,
				835,
				731,
				1536,
				0,
				768,
				672,
				2304,
				0,
				1152,
				1008,
				0,
				0,
				0,
				0,
				1880,
				0,
				940,
				835,
				1728,
				0,
				864,
				768,
				2592,
				0,
				1296,
				1152,
				0,
				0,
				0,
				0,
				2089,
				0,
				1044,
				1044,
				1920,
				0,
				960,
				960,
				2880,
				0,
				1440,
				1440,
				0,
				0,
				0,
				0,
				0,
				0,
				0,
				0,
				0,
				0,
				0,
				0,
				0,
				0,
				0,
				0,
				0,
				0,
				0,
				0
};

static const Byte br[] = {	1, 4, 5, 6, 7, 8,10,12,14,16,20,24,28,32,40,1,
							1, 1, 2, 3, 4, 5, 6, 7, 8,10,12,14,16,18,20,1};

//--------------------------------FUNCTIONS---------------------------------------
/*******************************************************************************
 * Function:	synchsafeToNormal (from id3parser.c)
 * 
 * Summary:		Convert synchsafe to normal.
 *				Returns normal integer with size of the tag.
 * NOTE:		The ID3v2 tag size is encoded with four bytes where the most
 *				significant bit (bit 7) is set to zero in every byte, making a total
 *				of 28 bits. The zeroed bits are ignored, so a 257 bytes long tag is
 *				represented as $00 00 02 01.
 *******************************************************************************/
static LongWord 
synchsafeToNormal(
	Byte *tagSize		// Convert this byte-field	 from synchsafe size, MSB first
){
	return (tagSize[3] & 0x7F)	+
			((tagSize[2] & 0x7F) << 7)	+
			((tagSize[1] & 0x7F) << 14)	+
			((tagSize[0] & 0x7F) << 21);
 }

/*******************************************************************************
 * Function:	MP3_BytesPerMS
 * 
 * Summary:		Translate nKBPs second of audio data into 
 *				num bytes per milli second.
 *				Returns no. of bytes per milli seconds
 *******************************************************************************/
Word
MP3_BytesPerMS(Word inKBPS)
{
	Word i;
	
	for (i = 0; gBitRateToBytesPerMSTab[i].kbps != 0 && 
		 gBitRateToBytesPerMSTab[i].kbps != inKBPS; i++)
	{
		;
	}
	
	return gBitRateToBytesPerMSTab[i].bytesPerMS;
}

/*******************************************************************************
 * Function:	MP3_FindFirstFrame
 * 
 * Summary:		Find index of first MP3 audio frame in MP3 file
 * NOTE:		ID3 Header structure (10 bytes):
 * 				File identifier: 	"ID3"
 * 				Version:			$04 00
 * 				Flags:				%abcd0000
 * 				Size:			 	4 * %0xxxxxxx
 * NOTE:		Size is (total-size - header (and footer if it exists)
 * 
 * NOTE			MPEG AUDIO LAYER 3 HEADER
 *				AAAAAAAAA AAABBCCD EEEEFFGH IIJJKLMM
 *				A 11 bits 	Frame sync (all bits must be set)
 * 				B 2 bits	MPEG Audio Version ID
 *							00 - MPEG Version 2.5
 *							01 - reserved
 *							10 - MPEG Version 2
 *							11 - MPEG Version 1
 *				C 2 bits	Layer Description
 *							00 - reserved
 *							01 - Layer III
 *							10 - Layer II
 *							11 - Layer I
 *				D 1 bit		Protection bit
 *							0 - Protected by CRC
 *							1 - Not protected
 *				E 4 bits	Bitrate index		(see standard)
 *				F 2 bits	Sampling rate frequency index 	(see standard)
 *				G 1 bit		Padding bit
 *							0 - Frame is not padded
 *							1 - Frame is padded
 *				H 1 bit		Private bit
 *				I 2 bits	Channel Mode
 *							00 - Stereo
 *							01 - Joint Stereo
 *							10 - Dual Channel (2 mono channels)
 *							11 - Single Channel (mono)
 *				J 2 bits	Mode extension (for joint stereo)
 *				K 1 bit		Copyright
 *							0 - Audio is not copyrighted
 *							1 - Audio is copyrighted
 *				L 1 bit		Original
 *							0 - Copy of original media
 *							1 - Original media
 *				M 2 bits	Emphasis
 *							00 - none
 *							01 - 50/15 ms
 *							10 - reserved
 *							11 - CCIT J.17
 *******************************************************************************/
long
MP3_FindFirstFrame(F_FILE *fp)
{
	Byte idx, fh[3], mp3Ver;
	long currByte = 3;
	LongWord header_size = 0;
	Word frameSize = 0;
	int	 ch, err;
	
	/* Find ID3-header */
	Byte buf[10];								// Tag data buffer (10 bytes)
	ASSERT(fp, ("MP3_FindFirstFrame: didn't get any open file"));

	f_rewind(fp);								// Rewind file
	
	/* Read 9 bytes from file, store from second byte in buf. */
	
	if(err = f_read(buf+1, 1, sizeof(buf)-1, fp) != (sizeof(buf) - 1)) {
		TRACE_AUD(("Filesystem returned error: %d\n\r", err);)
		goto fileEnd;
	}

	while (f_tell(fp) < MAX_SEEK_ID3_HEADER_POS) {
		int b;
		memcpy(buf, buf + 1, sizeof(buf) - 1);	// Move 9 bytes forward in buf
		b = f_getc(fp);							// Read next byte from file

		if (b < 0)							
			goto fileEnd;
				
		buf[sizeof(buf) - 1] = b;				// Store b into buf's last position
		
		if (buf[0] != 'I')						// File Identifier
			continue;							
		if (buf[1] != 'D')
			continue;
		if (buf[2] != '3')
			continue;
		if (buf[3] > 0x04)						// Major number			
			continue;
		if (buf[4] == 0xff)						// Revision number can't be 0xFF
			continue;
		if (buf[6] >= 0x80)						// Syncsafe size byte, MSB
			continue;
		if (buf[7] >= 0x80)
			continue;
		if (buf[8] >= 0x80)
			continue;
		if (buf[9] >= 0x80)						// Syncsafe size byte, LSB
			continue;
		
		header_size = synchsafeToNormal(buf + 6) + f_tell(fp);

		if(buf[5] & 0x10)						// With footer
			header_size += 10;
	
		/* Jump to new position in file */	
		f_seek(fp, header_size, F_SEEK_SET);
		currByte += f_tell(fp);
	}
	
	/* Continue looking for MP3-frame */
	for (idx = 0; idx < 3; idx++) {				// Read next three bytes
		if ((ch = (int)f_getc(fp)) < 0) {
			goto fileEnd;
		}
		fh[idx] = ch;
	}
	
	while (currByte < (header_size + MAX_SEEK_MP3_FRAME_POS) && !frameSize)	{
		if ((fh[0] == 0xFF) && 
			((fh[1] & 0xE6) == 0xE2))							// Check only for Layer III frames
		{
			mp3Ver = (fh[1] >> 3) & 0x03;						// Get Layer Version, 2 lsb is saved
			idx = (fh[2] & 0xFC) | mp3Ver;						// Get frame size calculated by bitrate and sampling
			frameSize = fs[idx];
			if (frameSize) {
				frameSize += ((fh[2]>>1) & 0x01);				// Add padding (if padding-bit is set)

				if (f_seek(fp, currByte - 3 + frameSize, F_SEEK_SET) != F_NO_ERROR) {
					goto fileEnd;
				}
				if ((ch = (int)f_getc(fp)) < 0) {				// Get first byte from next frame header
					goto fileEnd;
				}
				
				if (ch != 0xFF) {								
					frameSize = 0;
					f_seek(fp, currByte, F_SEEK_SET);
				}
				else {
					f_seek(fp, currByte - 3, F_SEEK_SET);
					idx = (fh[2] >> 4) & 0x0F;		// Bitrate

					if (mp3Ver < 3) {
						idx += 16;					// MPEG version 2 or 2.5
					}
				}
			}
		}
		
		if (frameSize == 0) {						// Read next byte in file and continue from there
			fh[0] = fh[1];							
			fh[1] = fh[2];
			if ((ch = (int)f_getc(fp)) < 0) {
				goto fileEnd;
			}

			fh[2] = ch;
			currByte++;
		}
	}
	
fileEnd:
	if (currByte >= (header_size + MAX_SEEK_MP3_FRAME_POS) || frameSize == 0)		// Did we find frame?
		currByte = -1;								// No
	else
		currByte -= 3;								// Yes

	return currByte;		
}

/*******************************************************************************
 * Function:	MP3_FindFrame
 * Summary:		Find the next MP3-frame from a byte-postion in the file
 *******************************************************************************/
long
MP3_FindFrame(
	F_FILE *fp,
	LongWord fromPosition
) {
	Byte idx, fh[3], mp3Ver;
	Word frameSize = 0;
	long currByte = fromPosition;
	int	 ch;
	
	/* Move forward to fromPosition */
	f_seek(fp, fromPosition, F_SEEK_SET);
	
	/* Look for MP3-frame */
	for (idx = 0; idx < 3; idx++) {				// Read next three bytes
		if ((ch = (int)f_getc(fp)) < 0) {
			goto fileEnd;
		}
		fh[idx] = ch;
	}
	
	while (!frameSize)	{
		if ((fh[0] == 0xFF) && 
			((fh[1] & 0xE6) == 0xE2))							// Check only for Layer III frames
		{
			mp3Ver = (fh[1] >> 3) & 0x03;						// Get Layer Version, 2 lsb is saved
			idx = (fh[2] & 0xFC) | mp3Ver;						// Get frame size calculated by bitrate and sampling
			frameSize = fs[idx];
			if (frameSize) {
				frameSize += ((fh[2]>>1) & 0x01);				// Add padding (if padding-bit is set)
				if (f_seek(fp, currByte -3 + frameSize, F_SEEK_SET) != F_NO_ERROR) {
					goto fileEnd;
				}
				if ((ch = (int)f_getc(fp)) < 0) {				// Get first byte from next frame header
					goto fileEnd;
				}
				
				if (ch != 0xFF) {								
					frameSize = 0;
					f_seek(fp, currByte, F_SEEK_SET);
				}
				else {
					f_seek(fp, currByte - 3, F_SEEK_SET);
					idx = (fh[2] >> 4) & 0x0F;		// Bitrate

					if (mp3Ver < 3) {
						idx += 16;					// MPEG version 2 or 2.5
					}
				}
			}
		}
		
		if (frameSize == 0) {						// Read next byte in file and continue from there
			fh[0] = fh[1];							
			fh[1] = fh[2];
			if ((ch = (int)f_getc(fp)) < 0) {
				goto fileEnd;
			}

			fh[2] = ch;
			currByte++;
		}
	}
	
fileEnd:
	if (frameSize == 0)		// Did we find frame?
		currByte = 0;		// No
	else
		currByte -= 3;		// Yes

	return currByte;		
}
