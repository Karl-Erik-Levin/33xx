/******************************************************************************

  AV-lab tools library.
  © Copyright 2000-2003 by SPIRIT Corp.
  http://www.spiritcorp.com

  File Name:      WavWriter.c

*/ /** @file

  Functions to create WAV PCM file.

  For multichannel WAV's please use following channels layout:

   Front Left - FL                (L)
   Front Right - FR               (R)
   Front Center - FC              (C)
   Low Frequency - LF             (LFE)
   Back Left - BL                 (Ls)
   Back Right - BR                (Rs)

  see http://www.microsoft.com/whdc/hwdev/tech/audio/multichaud.mspx 
  for more details

*/ /*

  Revision History:

  1.0.0   07/24/2002   ASP     Initial Version

******************************************************************************/

#include "MFLib\Dataton_Types.h"
#include "platform\hccfat\api_f.h"
#include "3356\WavWriter.h"

//
//Standard WAV header size
//
#define WAV_HEADER_SIZE 44

//
//Magic constants for Little-endian machines
//
#define RIFF_MAGIC 0x46464952ul //'RIFF'
#define WAVE_MAGIC 0x45564157ul //'WAVE'
#define FMT_MAGIC  0x20746D66ul //'fmt '
#define DATA_MAGIC 0x61746164ul //'data'

#define WRITE_4(val)     {unsigned char t[4]; long x = (long)(val);\
							t[0] = (unsigned char)(x & 0xFF);\
							t[1] = (unsigned char)((x >> 8)  & 0xFF);\
							t[2] = (unsigned char)((x >> 16) & 0xFF);\
							t[3] = (unsigned char)((x >> 24) & 0xFF);\
							if (f_write(t,1,4,pF) != 4) return 0;}
							
#define WRITE_2(val)     {unsigned char t[2]; short x = (short)(val);\
							t[0] = (unsigned char)(x & 0xFF);\
							t[1] = (unsigned char)((x >> 8) & 0xFF);\
							if (f_write(t,1,2,pF) != 2) return 0;}


/*******************************************************************************
 * Function:	WAV_writeHeader
 * Summary:		Writes standard WAV header in the beginning of the file
 *
 * Param:		pF              F_FILE handle
 *				nSampleRateHz   Sample rate value (for ex. 44100)
 *				nChannels       Number of channels (1,2...)
 *				nBitsPerSample  Number of bits per sample (8,16...)
 *				nFileSize       Total file size in bytes (assume file was opened with WAV_writerOpen())

 * 
 * Return:		1 in case of success, or 0 in case of write error or invalid params
 * 
 *******************************************************************************/
static int
WAV_writeHeader(F_FILE * pF
                          ,unsigned long nSampleRateHz
                          ,unsigned int  nChannels
                          ,unsigned int  nBitsPerSample
                          ,unsigned long nFileSize
                          )
{
  if (!pF) return 0;
  f_rewind(pF);                                              //WAV header structure:
  WRITE_4(RIFF_MAGIC);                                     //  0: RiffHeader.Magic = 'RIFF'
  WRITE_4(nFileSize - 8);                                  //  4: RiffHeader.FileSize = File size - 8
  WRITE_4(WAVE_MAGIC);                                     //  8: RiffHeader.Type = 'WAVE'
  WRITE_4(FMT_MAGIC);                                      //  C: ChunkFmt.Id = 'fmt ' (format description)
  WRITE_4(16);                                             // 10: ChunkFmt.Size = 16   (descriptor size)
  WRITE_2(1);                                              // 14: Format.wFormatTag = 1 (WAV PCM)
  WRITE_2(nChannels);                                      // 16: Format.nChannels
  WRITE_4(nSampleRateHz);                                  // 18: Format.nSamplesPerSec
  WRITE_4(nSampleRateHz * nChannels * nBitsPerSample / 8); // 1C: Format.nAvgBytesPerSec
  WRITE_2(nBitsPerSample * nChannels / 8);                 // 20: Format.nBlockAlign
  WRITE_2(nBitsPerSample);                                 // 22: Format.BitsPerSample
  WRITE_4(DATA_MAGIC);                                     // 24: ChunkData.Id = 'data'
  WRITE_4(nFileSize - WAV_HEADER_SIZE);                    // 28: ChunkData.Size = File size - 44
  return 1;                                                //     Total size: 0x2C (44) bytes
}


/*******************************************************************************
 * Function:	WAV_writerOpen
 * Summary:		Creates new file for writing and writes dummy WAV header.
 *				Overvrites existing file
 *
 * Param:		pFileName	File name to create
 *
 * Return:		opened F_FILE handle
 * 
 *******************************************************************************/
F_FILE *
WAV_writerOpen(const char * pFileName)
{
  F_FILE * pF = f_open(pFileName, "w");
  
  if (pF) {
    if (!WAV_writeHeader(pF, 0, 0, 0, WAV_HEADER_SIZE)) {
      f_close(pF);
      pF = NULL;
    }
  }
  
  return pF;
}


/*-----------------------------------------------------------------------------

  Function:    WAV_writerClose

*/ /**

  Writes WAV header and close the file. Assumes file was opened with 
  WAV_writerOpen().

  \warning Some RTL's have ftell() badly implemented: 
  use WAV_writeHeader directly in this case.

  @param pF              F_FILE handle
  @param nSampleRateHz   Sample rate value (for ex. 44100)
  @param nChannels       Number of channels (1,2...)
  @param nBitsPerSample  Number of bits per sample (8,16...)
  @param lBytesWritten   Number of bytes, written to file, set to -1 for
                         autodetect with ftell() (may be buggy on some RTL's!)

  @return 1 in case of success, or 0 in case of write error or invalid params.

*/ /*
-----------------------------------------------------------------------------*/
int WAV_writerClose(F_FILE * pF                    //F_FILE handle
                   ,unsigned long nSampleRateHz  //Sample rate, Hz (44100, ...)
                   ,unsigned int  nChannels      //Number of channels (1,2 ...)
                   ,unsigned int  nBitsPerSample //Bits per sample (8,16 ...)
                   ,long          lBytesWritten  //Bytes, written to file
                   )
{
  int retval = 0;
  if (pF)
  {
    long size;
    if (lBytesWritten == -1)
    {
        f_seek(pF, 0, F_SEEK_END);
        size = f_tell(pF);   
    }
    else
    {
        size = lBytesWritten + WAV_HEADER_SIZE;
    }
    
    retval = WAV_writeHeader(pF, nSampleRateHz, nChannels, nBitsPerSample, size);
    f_close(pF);
  }
  return retval;
}


F_FILE * WAV_readerOpen(const char * pFileName, wavProp *prop)
{
	F_FILE *fp;
	LongWord tempL;
	Word     tempS, bytes, i, headSize;
	
	fp = f_open(pFileName, "r");
	if (fp != NULL) {
		bytes = f_read(&tempL, 1 , sizeof(LongWord), fp);
		if ((bytes != sizeof(LongWord)) || (tempL != RIFF_MAGIC)) goto error;
		
		for (i=0; i<4; i++) {
			bytes = f_read(&tempL, 1 , sizeof(LongWord), fp);
			if ((bytes != sizeof(LongWord))) goto error;
		}
		
		headSize = tempL + 8;
		bytes = f_read(&tempS, 1 , sizeof(Word), fp);
		if ((bytes != sizeof(Word)) || (tempS != 1)) goto error;	// wFormatTag = 1 (WAV PCM)
		headSize -=2;

		bytes = f_read(&tempS, 1 , sizeof(Word), fp);
		if ((bytes != sizeof(Word))) goto error;					// nChannels
		prop->nChannels = tempS;
		headSize -=2;

		bytes = f_read(&tempL, 1 , sizeof(LongWord), fp);
		if ((bytes != sizeof(LongWord))) goto error;				// nSamplesPerSec
		prop->nSampleRateHz = tempL;
		headSize -=4;

		bytes = f_read(&tempL, 1 , sizeof(LongWord), fp);
		if ((bytes != sizeof(LongWord))) goto error;				// nAvgBytesPerSec
		headSize -=4;

		bytes = f_read(&tempS, 1 , sizeof(Word), fp);
		if ((bytes != sizeof(Word))) goto error;					// nBlockAlign
		headSize -=2;

		bytes = f_read(&tempS, 1 , sizeof(Word), fp);
		if ((bytes != sizeof(Word))) goto error;					// BitsPerSample
		prop->nBitsPerSample = tempS;
		headSize -=2;
		
		// Read forward to first sample
		for (i=0; i<headSize; i++)
			if (f_getc(fp) == -1) goto error;
	}
	
	return fp;
	
error:
	f_close(fp);
	return NULL;
}

int WAV_readerClose(F_FILE * fp)
{
	return f_close(fp);
}


