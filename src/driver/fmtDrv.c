/* fmtDrv.c - Dataton 3397 Transponder 
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

//---------------------------------INCLUDES---------------------------------------
#include "MFLib\Dataton_Types.h"
#include "MFLib\TraceAssert.h"

#include "driver\hwBoard.h"
#include "driver\fmtDrv.h"
#include "driver\i2cDrv.h"
#include "driver\si4721.h"
#include "driver\tmrDrv.h"


//---------------------------------DEFINES----------------------------------------

//---------------------------------TYPEDEFS---------------------------------------

typedef struct CompressionParams_ {
	Word thresholdDb;    // 16-bit 2's compl of thr (-40 to 0 dB range). Valid range 0x00, 0xffd8-0xffff
	Word attackTime;     // 0 = 0.5 ms, 1 = 1 ms, ..., 9 = 5 ms. Valid range 0-9
	Word releaseTime;    // 0 = 100 ms, 1 = 200 ms, 2 = 350 ms, 3 = 525 ms, 4 = 1000 ms
	Word gainDb;         // 0 = 0dB, 1 = 1dB, ..., 20 = 20dB. Valid range 1-20
} CompressionParams;

//------------------------------GLOBAL VARIABLES----------------------------------

static Word gComponentEnable;
static Word gDigitalInputFormat;

static CompressionParams gCompressionLevels[3] = {
	{ (65536 - 40), 9, 0, 15 },   // EFMT_LittleCompression  (-40 dB, 5   ms,  100 ms, 15 dB)
	{ (65536 - 25), 5, 2, 10 },   // EFMT_MediumCompression  (-25 dB, 3   ms,  350 ms, 10 dB)
	{ (65536 - 15), 0, 4,  5 }    // EFMT_HighCompression    (-15 dB, 0.5 ms, 1000 ms,  5 dB)
};

//-------------------------LOCAL FUNCTION DECLARATIONS----------------------------

static void SetProperty( Word property, Word propArg, char* idStr );


//--------------------------------FUNCTIONS---------------------------------------

void
fmtInit(void)
{
	// Configure port pins for Si4721 reset and interrupt signals
	AT91F_PIO_CfgOutput(AT91C_BASE_PIOA, SI4721_RESET_PIN);			// Reset signal as output
	AT91F_PIO_CfgInput(AT91C_BASE_PIOA, SI4721_INT) ;				// Interrupt signal as input
	AT91C_BASE_PIOA->PIO_PPUDR = SI4721_RESET_PIN | SI4721_INT;		// Disable pullup on both
	
	// Send hardware reset signal to Si4721 chip to set it in I2C mode
	AT91F_PIO_ClearOutput(AT91C_BASE_PIOA, SI4721_RESET_PIN);
	tmrWaitMS(10);
	AT91F_PIO_SetOutput(AT91C_BASE_PIOA, SI4721_RESET_PIN);
	tmrWaitMS(10);
	
	gComponentEnable = COMPOMENT_ENABLE_DEFAULTS;  // RDS off and stereo on per default
	gDigitalInputFormat = DIGITAL_INPUT_DEFAULTS;  // Sample on rising, I2S, stereo, 16-bit per default 
}


void
fmtOn(fmtMode mode)
{
	Byte buf[10];
	
	// Power up
	buf[0] = POWER_UP;				// Cmd
	buf[1] = 0x02;					// Arg1: Use external RCLK, transmit
	
	switch ( mode ) {
	case EFMT_TxAnalogInput:
		buf[2] = TX_INPUT_FORMAT_ANALOG;
		break;
		
	case EFMT_TxDigitalInput:
		buf[2] = TX_INPUT_FORMAT_DIGITAL;
		break;
		
	default:
		TRACE_FMT( ( "fmtOn: invalid input format %d\r\n", (Word)mode ) );
		return;
	}
	
	i2cWrite(EI2CD_4721, 0, buf, 3);
	tmrWaitMS( POWER_UP_TIME_MS );
	i2cRead(EI2CD_4721, 0, buf, 1);
	
	// Get chip revision
	buf[0] = GET_REVISION;			// Cmd
	i2cWrite(EI2CD_4721, 0, buf, 1);
	tmrWaitMS( GET_REVISION_TIME_MS );
	i2cRead(EI2CD_4721, 0, buf, 9);	// Status + resp1..resp8
	TRACE_FMT(("fmtOn  status %02x", buf[0]);)
	TRACE_FMT(("  PN %02x  FW 0x%02x%02x", buf[1], buf[2], buf[3]);)
	TRACE_FMT(("  Pa 0x%02x%02x  CPM0x%02x%02x", buf[4], buf[5], buf[6], buf[7]);)
	TRACE_FMT(("  CR %02x\r\n", buf[8]);)
	
	// Set defaults
	fmtSetTransmitPower( TX_MAX_POWER );
	fmtSetOutputStereoMode( EFMT_StereoOutput );
	fmtSetRdsMode( EFMT_RdsOff );
	fmtSetPreEmphasis( EFMT_PreEmphasis50us );
	fmtSetCompression( EFMT_NoCompression );
	fmtSetInputStereoMode( EFMT_StereoInput );
	fmtSetInputSamplePrecision( EFMT_16Bit );
}


void
fmtOff(void)
{
	Byte buf[1];
	
	// Power down
	buf[0] = POWER_DOWN;				// Cmd
	i2cWrite(EI2CD_4721, 0, buf, 1);
	tmrWaitMS( POWER_DOWN_TIME_MS );
	i2cRead(EI2CD_4721, 0, buf, 1);
	TRACE_FMT(("fmtOff status %02x\r\n", buf[0]);)
}


void
fmtSetFreq(Word freq)
{
	Byte buf[4];

	// Limit frequency to valid range, which is 76 to 108 MHz
	if ( ( freq < TX_MIN_FREQ ) || ( freq > TX_MAX_FREQ ) ) {
		TRACE_FMT( ( "fmtSetFreq: freq %d out of range\r\n", freq ) );
		return;
	}
	
	// Frequency must be a multiple of 50 kHz, if not, don't try to set it. 
	if ( ( freq % 5 ) != 0 ) {
		TRACE_FMT( ( "fmSetFreq: freq %d is not a multiple of 50 kHz\r\n" ) );
		return;
	}
	
	// Set frequency
	buf[0] = TX_TUNE_FREQ;
	buf[1] = 0;                      // Should always be set to zero
	buf[2] = ( freq >> 8 ) & 0xff;   // High byte of frequency
	buf[3] = freq & 0xff;            // Low byte of frequency
	i2cWrite( EI2CD_4721, 0, buf, 4 );
	tmrWaitMS( TX_TUNE_FREQ_TIME_MS );
	i2cRead( EI2CD_4721, 0, buf, 1 );
	TRACE_FMT( ( "fmtSetFreq status %02x\r\n", buf[0] ) );
}


void fmtSetTransmitPower( Word power )
{
	Byte buf[5];
	
	// Limit transmit power to the valid range (88-120)
	if ( ( power < TX_MIN_POWER ) || ( power > TX_MAX_POWER ) ) {
		TRACE_FMT( ( "fmtSetTransmitPower: Power %d is out of range\r\n", power ) );
		return;
	}
	
	// Set power, use automatic tuning of antenna capacitor
	buf[0] = TX_TUNE_POWER;
	buf[1] = 0;              // Should always be set to zero
	buf[2] = 0;              // Should always be set to zero
	buf[3] = (Byte)power;    // Power in dBuV
	buf[4] = TX_AUTO_ANTENNA_CAPACITOR;  // Use automatic tuning of antenna capacitor
	i2cWrite( EI2CD_4721, 0, buf, 5 );
	tmrWaitMS( TX_TUNE_POWER_TIME_MS );
	i2cRead( EI2CD_4721, 0, buf, 1 );
	TRACE_FMT( ( "fmtSetTransmitPower status %02x\r\n", buf[0] ) );
}


void fmtSetOutputStereoMode( FmtOutputStereoMode stereoMode ) 
{
	switch ( stereoMode ) {
	case EFMT_MonoOutput:
		gComponentEnable &= ~TX_COMP_ENABLE_STEREO_BIT;
		gComponentEnable &= ~TX_COMP_ENABLE_STEREO_PILOT_BIT;
		break;
	
	case EFMT_StereoOutput:
		gComponentEnable |= TX_COMP_ENABLE_STEREO_BIT;
		gComponentEnable |= TX_COMP_ENABLE_STEREO_PILOT_BIT;
		break;
		
	default:
		TRACE_FMT( ( "fmtSetOutputStereoMode: unknown mode: %d\r\n", (Word)stereoMode ) );
		return;
	}
	
	SetProperty( TX_COMPONENT_ENABLE, gComponentEnable, "OutputStereoMode" );
}

	
	
void fmtSetRdsMode( FmtRdsMode rdsMode )
{
	switch ( rdsMode ) {
	case EFMT_RdsOff:
		gComponentEnable &= ~TX_COMP_ENABLE_RDS_BIT;
		break;
	
	case EFMT_RdsOn:
		gComponentEnable |= TX_COMP_ENABLE_RDS_BIT;
		break;
		
	default:
		TRACE_FMT( ( "fmtSetRdsMode: unknown mode: %d\r\n", (Word)rdsMode ) );
		return;
	}
	
	SetProperty( TX_COMPONENT_ENABLE, gComponentEnable, "SetRdsMode" );
}	



void fmtSetPreEmphasis( FmtPreEmphasisMode preEmphMode )
{
	Word propParam = 0;
	
	switch ( preEmphMode ) {
	case EFMT_PreEmphasis75us:
		propParam |= TX_PRE_EMPH_75US;
		break;
		
	case EFMT_PreEmphasis50us:
		propParam |= TX_PRE_EMPH_50US;
		break;
		
	case EFMT_PreEmphasisOff:
		propParam |= TX_PRE_EMPH_OFF;
		break;
		
	default:
		// Unknown pre emphasis mode, don't try to do anything
		TRACE_FMT( ( "fmtSetPreEmphasis: Unknown pre emph. mode %d\r\n", (Word)preEmphMode ) );
		return;
	}

	SetProperty( TX_PREEMPHASIS, propParam, "PreEmphasis" );
}


void fmtSetCompression( FmtCompressionMode compressionMode )
{
	Word propParam = 0;

	if ( compressionMode == EFMT_NoCompression ) {
		// No compression, but leave the limiter on, which is the default mode
		propParam |= TX_LIMITER_ENABLE_BIT;
	}
	else if ( ( compressionMode == EFMT_LittleCompression ) ||
	          ( compressionMode == EFMT_MediumCompression ) ||
	          ( compressionMode == EFMT_HighCompression ) ) {
		// Turn audio compressio on, and leave the limiter on (default)
		propParam |= TX_COMPRESSION_ENABLE_BIT | TX_LIMITER_ENABLE_BIT;
	}
	else {
		TRACE_FMT( ( "fmtSetCompression: Unknown mode %d\r\n", (Word)compressionMode ) );
		return;
	}
	SetProperty( TX_ACOMP_ENABLE3, propParam, "EnableCompression" );
	
	if ( compressionMode != EFMT_NoCompression ) {
		// Compression should be on, set the correct levels of the four 
		// compression related parameters (threshold, attack time, release time and gain
		SetProperty( TX_ACOMP_THRESHOLD,    gCompressionLevels[(Word)compressionMode].thresholdDb, "CompressionThreshold" );
		SetProperty( TX_ACOMP_ATTACK_TIME,  gCompressionLevels[(Word)compressionMode].attackTime,  "CompressionAttackTime" );
		SetProperty( TX_ACOMP_RELEASE_TIME, gCompressionLevels[(Word)compressionMode].releaseTime, "CompressionReleaseTime" );
		SetProperty( TX_ACOMP_GAIN,         gCompressionLevels[(Word)compressionMode].gainDb,      "CompressionGain" );
	}
}
		


void fmtSetInputStereoMode( FmtInputStereoMode stereoMode )
{
	switch ( stereoMode ) {
	case EFMT_MonoInput:
		gDigitalInputFormat |= TX_DIG_INP_FMT_STEREO_MODE_BIT;
		break;
	
	case EFMT_StereoInput:
		gDigitalInputFormat &= ~TX_DIG_INP_FMT_STEREO_MODE_BIT;
		break;
	
	default: 
		TRACE_FMT( ( "fmtSetInputStereoMode: Unknown stereo mode %d\r\n", (Word)stereoMode ) );
		return;
	}

	SetProperty( DIGITAL_INPUT_FORMAT1, gDigitalInputFormat, "InputStereoMode" );
}



void fmtSetInputSamplePrecision( FmtInputSamplePrecision samplePrec ) 
{
	gDigitalInputFormat &= ~TX_DIG_INP_SAMPLE_PRECISION_MASK;
	switch ( samplePrec ) {
	case EFMT_16Bit:
		gDigitalInputFormat |= TX_DIG_INP_SAMPLE_PRECISION_16_BIT;
		break;
		
	case EFMT_20Bit:
		gDigitalInputFormat |= TX_DIG_INP_SAMPLE_PRECISION_20_BIT;
		break;
		
	case EFMT_24Bit:
		gDigitalInputFormat |= TX_DIG_INP_SAMPLE_PRECISION_24_BIT;
		break;
		
	case EFMT_8Bit:
		gDigitalInputFormat |= TX_DIG_INP_SAMPLE_PRECISION_8_BIT;
		break;
		
	default:
	 	TRACE_FMT( ( "fmtSetInputSamplePrecision: Unknown sample precision %d\r\n", (Word)samplePrec ) );
	 	return;
	}
	
	SetProperty( DIGITAL_INPUT_FORMAT1, gDigitalInputFormat, "Inputsampleprecision" );
}		
		
		
		
void fmtSetSampleRate( Word sampleRate ) 
{
	if ( ( sampleRate != TX_SAMPLE_RATE_DISABLE ) && 
		( ( sampleRate < TX_MIN_SAMPLE_RATE ) || ( sampleRate > TX_MAX_SAMPLE_RATE ) ) ) {
		// Sample rate is outside possible interval, and is not disabled.
		TRACE_FMT( ( "fmtSetSampleRate: illegal sample rate %d\r\n", sampleRate ) );
		return;
	}
	
	SetProperty( DIGITAL_INPUT_SMP_RATE1, sampleRate, "SetSampleRate" );
}
	


void SetProperty( Word property, Word propArg, char* idStr ) 
{
	Byte buf[6];
	
	buf[0] = SET_PROPERTY;
	buf[1] = 0;
	buf[2] = ( property >> 8 ) & 0xff;
	buf[3] = ( property >> 0 ) & 0xff;
	buf[4] = ( propArg >> 8 ) & 0xff;
	buf[5] = ( propArg >> 0 ) & 0xff;
	
	i2cWrite( EI2CD_4721, 0, buf, 6 );
	tmrWaitMS( SET_PROPERTY_TIME_MS );
	i2cRead( EI2CD_4721, 0, buf, 1 );
	TRACE_FMT( ( "Set property %s status: %02x\r\n", idStr, buf[0] ) );
}

