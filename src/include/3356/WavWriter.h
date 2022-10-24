/******************************************************************************

  AV-lab tools library.
  © Copyright 2000-2003 by SPIRIT Corp.
  http://www.spiritcorp.com

  File Name:      WavWriter.h

*/ /** @file

  Functions to create wav file.

*/ /*

  Revision History:

  1.0.0   07/24/2002   ASP     Initial Version

******************************************************************************/
#ifndef _wavwriter_
#define _wavwriter_


typedef struct _wavProp {
	unsigned long nSampleRateHz;	//Sample rate, Hz (44100, ...)
	unsigned int  nChannels;		//Number of channels (1,2 ...)
	unsigned int  nBitsPerSample;	//Bits per sample (8,16 ...)
} wavProp;

/*
// Opens file for writing, and appends header template.
// Overwrites existing files.
// Returns F_FILE handle.
// 
// Example:

  F_FILE * pWavFile = WAV_writerOpen("MyFile.wav");

*/
F_FILE * WAV_writerOpen(const char * pFileName);

/*
// Closes wav file, and write WAV header according to given info.
// Returns 1 in case of succsess, 0 otherwise.
// 
// Example:

  WAV_writerClose(pWavFile, 44100, 2, 16); //CD-format audio

*/
int WAV_writerClose(F_FILE * pF						//F_FILE handle
                   ,unsigned long nSampleRateHz		//Sample rate, Hz (44100, ...)
                   ,unsigned int  nChannels			//Number of channels (1,2 ...)
                   ,unsigned int  nBitsPerSample	//Bits per sample (8,16 ...)
                   ,long          lBytesWritten		//Bytes, written to file (-1 = Autodetect)
                   );

F_FILE * WAV_readerOpen(const char * pFileName, wavProp *prop);
int WAV_readerClose(F_FILE *fp);


#endif // _wavwriter_
