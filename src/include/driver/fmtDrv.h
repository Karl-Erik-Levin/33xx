/* fmtDrv.h - Dataton 3397 Transponder 
**
**	FM Transiver Driver
** 		
**	Handle Silicon Labs chip Si4721, Broadcast FM Transceiver.
**
**
** Created 10-03-18	Kalle
**
** (C) Copyright Dataton AB 2010, All Rights Reserved
**
**/

#ifndef _fmtDrv_
#define _fmtDrv_

//---------------------------------TYPEDEFS---------------------------------------
typedef enum {
	EFMT_TxAnalogInput,
	EFMT_TxDigitalInput,
	EFMT_Rx
} fmtMode;


typedef enum {
	EFMT_RdsOff,
	EFMT_RdsOn
} FmtRdsMode;


typedef enum {
	EFMT_MonoOutput,
	EFMT_StereoOutput 
} FmtOutputStereoMode;


typedef enum {
	EFMT_PreEmphasis75us,   // Used in USA
	EFMT_PreEmphasis50us,   // Used in Europe, Australia and Japan
	EFMT_PreEmphasisOff     // Turn off
} FmtPreEmphasisMode;


typedef enum {
	EFMT_SampleOnRising,
	EFMT_SampleOnFalling
} FmtInputSampleMode;


typedef enum {
	EFMT_I2S,
	EFMT_LeftJustified,
	EFMT_MsbAtFirst,
	EFMT_MsbAtSecond
} FmtInputDigitalMode;


typedef enum {
	EFMT_MonoInput,
	EFMT_StereoInput
} FmtInputStereoMode;


typedef enum {
	EFMT_16Bit, 
	EFMT_20Bit, 
	EFMT_24Bit, 
	EFMT_8Bit
} FmtInputSamplePrecision;


typedef enum {
	EFMT_NoCompression     = -1,   // Don't mess with these numbers, they are 
	EFMT_LittleCompression =  0,   // used to index the gCompressionLevels 
	EFMT_MediumCompression =  1,   // array. 
	EFMT_HighCompression   =  2 
} FmtCompressionMode;


//---------------------------------PROTOTYPES---------------------------------------
void fmtInit(void);
void fmtOn(fmtMode mode);
void fmtOff(void);

// freq in MHz * 100, 98.2 MHz = 9820, valid range is 7600 to 10800 
void fmtSetFreq(Word freq);       

// Power in dBuV, valid range is 88-120. Values >= 115 may
// in some cases cause some distortion.
void fmtSetTransmitPower( Word power );
void fmtSetOutputStereoMode( FmtOutputStereoMode stereoMode );
void fmtSetRdsMode( FmtRdsMode rdsMode );
void fmtSetPreEmphasis( FmtPreEmphasisMode preEmphMode );
void fmtSetCompression( FmtCompressionMode compressionMode );
void fmtSetInputStereoMode( FmtInputStereoMode stereoMode );
void fmtSetInputSamplePrecision( FmtInputSamplePrecision samplePrec );

// fmtSetsampleRate must be called after fmtInit, fmtOn and fmtSetFreq
// so that the internal clocking has been started. 
void fmtSetSampleRate( Word sampleRate );   


#endif	//_fmtDrv_




