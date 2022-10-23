/* pubFileUtil.c - Pickup automatic program update

	(C) Copyright Dataton AB 2009, All Rights Reserved
	Created:  09-06-25 by Jon Månsson

*/
//---------------------------------INCLUDES---------------------------------------
#include "MFLib\Dataton_Types.h"
#include "MFLIb\TraceAssert.h"

#include "platform\hccfat\api_f.h"
#include "platform\utilities\ByteSwap.h"
#include "platform\utilities\pubFileUtil.h"

//---------------------------------DEFINES----------------------------------------
#define BL_LOAD_ADDRESS		0x100000			
#define PU_HW_INFO_ADDRESS	0x10fc00
#define APP_END_ADDRESS		0x140000		// 192 K

#define COPYRIGHT_2003	"Copyright 2003 Dataton AB, Sweden. All rights reserved"
#define COPYRIGHT_2008	"Copyright 2008 Dataton AB, Sweden. All rights reserved"
#define kCRC16XmodemPolynomial	0x1021UL	// (x^16 +) x^12 + x^5 + x^0
#define kCRCInit	0						// Default lastCRC param to calcCRC

//---------------------------------TYPEDEFS---------------------------------------
typedef struct {
	LongWord type;					
	LongWord size;				
	LongWord crc;				
} blkHdr;

enum {  kBTCopyright = 1, 
		kBTComment, 
		kBTBinData, 
		kBTVersion,
		kBTMP3Data
};
//--------------------------------FUNCTIONS---------------------------------------
/*******************************************************************************
 * Function:	calcCRC
 * Summary:		Returns a 16 bit checksum in upper word and 16 bit CRC in 
 *				lower word of return value. When calling calcCRC repeatedly over 
 *				a block of data, then set lastCRC to KRMInitCRCfirst time called, 
 *				then pass the CRC value returned from the last calcCRC to 
 *				subsequent calcCRC calls.
 *******************************************************************************/
static LongWord
calcCRC(Ptr theData, LongWord bytes, LongWord lastCRC)
{
	Byte *dataPtr = theData, data, bitCnt;
	Word crc, checkSum;
	
	crc = lastCRC & 0xffff;
	checkSum = lastCRC >> 16;
	
	while (bytes--) {
		data = *dataPtr++;
		crc ^= (unsigned short)(data) << 8;
		for (bitCnt = 8; bitCnt; --bitCnt) {
			if (crc & 0x8000)
				crc = (crc << 1) ^ kCRC16XmodemPolynomial;
			else
				crc <<= 1;
		}
		checkSum += data;
	}
	
	return (LongWord)checkSum << 16 | crc;
}

/*******************************************************************************
 * Function:	seekFileBlkHdr
 * Summary:		Finds specifics headers for the pub-file
 *******************************************************************************/
static Boolean
seekFileBlkHdr(F_FILE *fp, blkHdr *fbh, LongWord type, Boolean fromBegining)
{
	Boolean readMore = true;
	LongWord currFilePos;
	
	if (fromBegining)
		currFilePos = 0;
	else
		currFilePos = f_tell(fp);
		
	while (readMore) {
		readMore = false;
		fbh->type = 0xFFFFFFFF;
		if (f_seek(fp, currFilePos, F_SEEK_SET) == F_NO_ERROR)
			if (f_read(fbh, 1, sizeof(blkHdr), fp) == sizeof(blkHdr)) {
				fbh->size = LE2H32(fbh->size);
				if (fbh->size < 128000) {
					currFilePos += sizeof(blkHdr) + fbh->size;
					fbh->type = LE2H32(fbh->type);
					fbh->crc  = LE2H32(fbh->crc);
					readMore = (type != fbh->type);
				}
			}
	}
	
	return (type == fbh->type);
}

/*******************************************************************************
 * Function:	getFileVer
 * Summary:		Return 0 if unable to read file version number 
 *******************************************************************************/
static Word
getFileVer(F_FILE *fp)
{
	Word fileVer, retValue = 0x0;
	char copyRightStr[64];
	blkHdr fileBlkHdr;
	
	if (seekFileBlkHdr(fp, &fileBlkHdr, kBTCopyright, true)) {
		if (f_read(copyRightStr, 1, fileBlkHdr.size, fp) == fileBlkHdr.size) {
			copyRightStr[fileBlkHdr.size] = 0;		// Set end of string mark
			if (strcmp(copyRightStr, COPYRIGHT_2003) != 0 &&
				strcmp(copyRightStr, COPYRIGHT_2008) != 0) {
				return retValue;
			}
		}
	}
	
	if (seekFileBlkHdr(fp, &fileBlkHdr, kBTVersion, true)) {
		if (f_read(&fileVer, 1, sizeof(Word), fp) == sizeof(Word)) {
			fileVer = LE2H16(fileVer);
			retValue = fileVer;
		}
	}
	return retValue;		
}

/*******************************************************************************
 * Function:	validateFile
 * Summary:		Return false if not valid crc in  
 *******************************************************************************/
static Boolean
validateFile(char* filename)
{
	F_FILE *fp;
	blkHdr fileBlkHdr;

	Boolean validFile = false, error = false;
	Byte dataBuf[256];
	Word readBytes, bytesInBuf;
	LongWord crc = kCRCInit, loadAddr, endAddr, numBytes;
			
	if ((fp = f_open(filename, "r")) != false) {

	    if(seekFileBlkHdr(fp, &fileBlkHdr, kBTBinData, true)) {
	    	
	    	if (f_read(&loadAddr, 1, sizeof(loadAddr), fp) == sizeof(loadAddr)) {
	    	
	    		loadAddr = LE2H32(loadAddr) + BL_LOAD_ADDRESS;						
	    		endAddr  = (loadAddr == BL_LOAD_ADDRESS) ? PU_HW_INFO_ADDRESS : APP_END_ADDRESS;
	    		numBytes = fileBlkHdr.size - sizeof(loadAddr);
	    		    		
	    		while (numBytes > 0) {
	    			readBytes  = MIN(numBytes, sizeof(dataBuf));
	    			bytesInBuf = f_read(dataBuf, 1, readBytes, fp);
	    			
	    			if (bytesInBuf == readBytes) {
	    				crc = calcCRC(dataBuf, bytesInBuf, crc);	// Calculate CRC 
						loadAddr += bytesInBuf;
						numBytes -= bytesInBuf;
						
						if (loadAddr >= endAddr) {
							error = true;
							break;		// UE_EndOfMemoryError
						}		
	    			}
	    			else {
	    				error = true;
	    				break;			// UE_FileReadError
	    			}
	    		}
	
				if (!error &&
					(numBytes == 0) &&
					(crc == fileBlkHdr.crc)) {
					
					validFile = true;
				} 
	    	}
	    }

		f_close(fp);
	}
	
	return validFile;
}

/*******************************************************************************
 * Function:	isUpdateAvailable
 * Summary:		Go through the ROOT-directory and searches for files which has
 *				the extension set to "pub" and the correct copyright-string
 *				Returns TRUE if a file was found.
 *******************************************************************************/
static Boolean
isUpdateAvailable(char* retFilename, char *searchFileExt)
{
	Boolean fileFound = false;
	Boolean isPubFile = false;
	F_FILE *tmpFile;
	F_FIND find;
	Word fileVer = 0;
	
	if (strncmp("PUB", searchFileExt, 3) == 0)
		isPubFile = true;
		
	if (!f_findfirst("A:/*.*", &find)) {
		do {
			if (strncmp(find.ext, searchFileExt, 3) == 0) {
				if ((tmpFile = f_open(find.filename, "r")) != false) {
					Word tmpVer = getFileVer(tmpFile);
					
					// Pickup firmware 
					if (isPubFile) {
					
						// Avoid version number between 0.70 and 0.84
						if (tmpVer >= 0x0700 && tmpVer <= 0x0804) {
							tmpVer = 0;
						}
						
						// Convert version number > 8.4 to 0.84
						if (tmpVer >= 0x0805) {
							tmpVer = ((tmpVer & 0xFF00) >> 4) + (tmpVer & 0xFF);
						}
					}
					
					// Only update if version is higher than previous
					if (tmpVer > fileVer) {
						strcpy(retFilename, find.filename);
						fileFound = true;
						fileVer = tmpVer;
					}
					f_close(tmpFile);		
				}
			}
		}
		while (!f_findnext(&find));
	}
	
	if (fileFound) {
		fileFound = validateFile(retFilename);
	}
	
	return fileFound;
}

/*******************************************************************************
 * Function:	getFileVersion
 *******************************************************************************/
static Word
getFileVersion(char* filename)
{
	F_FILE *fp;
	Word fileVer = 0;
	
	f_chdir("A:/");
	if ((fp = f_open(filename, "r")) != false) {
		fileVer = getFileVer(fp);
		f_close(fp);
	}
	return fileVer;
}

/*******************************************************************************
 * Function:	Need2UpdateFirmware
 * Summary:		Search SDcard for file holding pickup program with different version number
 *******************************************************************************/
Boolean
Need2UpdateFirmware(Word currentVersion)
{
	Word fileVer;
	char filename[80];

	if (!isUpdateAvailable(filename, "PUB"))
		return false;
	
	fileVer = getFileVersion(filename);
	return fileVer != currentVersion;
}

