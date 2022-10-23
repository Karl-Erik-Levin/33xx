/* si4721.h - Dataton 3397 Transponder 
**
**	Silicon Labs chip Si4721
**
**
** Created 10-03-18	Kalle
**
** (C) Copyright Dataton AB 2010, All Rights Reserved
**
**/

#ifndef _si4721_
#define _si4721_

/*	FM/RDS Transmitter Command Summary
*/
#define	POWER_UP 		0x01		// Power up device and mode selection. Modes include FM transmit and analog/digital audio interface configuration.
#define	GET_REVISION	0x10		// Returns revision information on the device.
#define	POWER_DOWN		0x11		// Power down device.
#define	SET_PROPERTY	0x12		// Sets the value of a property.
#define	GET_PROPERTY	0x13		// Retrieves a propertyís value.
#define	GET_INT_STATUS	0x14		// Read interrupt status bits.
#define	PATCH_ARGS		0x15		// Reserved command used for patch file downloads.
#define	PATCH_DATA		0x16		// Reserved command used for patch file downloads.
#define	TX_TUNE_FREQ	0x30		// Tunes to given transmit frequency.
#define	TX_TUNE_POWER	0x31		// Sets the output power level and tunes the antenna capacitor.
#define	TX_TUNE_MEASURE	0x32		// Si4712/13/20/21 Only. Measure the received noise level at the specified frequency.
#define	TX_TUNE_STATUS	0x33		// Queries the status of a previously sent TX Tune Freq, TX Tune Power, or TX Tune Measure command.
#define	TX_ASQ_STATUS	0x34		// Queries the TX status and input audio signal metrics.
#define	TX_RDS_BUFF1	0x35		// Si4711/13/21 Only. Queries the status of the RDS Group Buffer and loads new data into buffer.
#define	TX_RDS_PS1		0x36		// Si4711/13/21 Only. Set up default PS strings.
#define	GPIO_CTL2		0x80		// Configures GPO1, 2, and 3 as output or Hi-Z.
#define	GPIO_SET2		0x81		// Sets GPO1, 2, and 3 output level (low or high).


/*	FM Transmitter Property Summary
*/
											// Default Comment
#define GPO_IEN					0x0001  	// 0x0000  Enables interrupt sources. 
#define DIGITAL_INPUT_FORMAT1	0x0101		// 0x0000  Configures the digital input format. 
#define DIGITAL_INPUT_SMP_RATE1	0x0103		// 0x0000  Configures the digital input sample rate in 1 Hz steps. Default is 0. 
#define REFCLK_FREQ				0x0201		// 0x8000  Sets frequency of the reference clock in Hz. The range is 31130 to 34406 Hz, or 0 to disable the AFC. Default is 32768 Hz. 
#define REFCLK_PRESCALE			0x0202		// 0x0001  Sets the prescaler value for the reference clock. 
#define TX_COMPONENT_ENABLE		0x2100		// 0x0003  Enable transmit multiplex signal components. Default has pilot and L-R enabled. 
#define TX_AUDIO_DEVIATION		0x2101		// 0x1AA9  Configures audio frequency deviation level. Units are in 10 Hz increments. Default is 6825 (68.25 kHz). 
#define TX_PILOT_DEVIATION		0x2102		// 0x02A3  Configures pilot tone frequency deviation level. Units are in 10 Hz increments. Default is 675 (6.75 kHz) 
#define TX_RDS_DEVIATION2		0x2103		// 0x00C8  Si4711/13/21 Only. Configures the RDS/RBDS fre-quency deviation level. Units are in 10 Hz increments. Default is 2 kHz. 
#define TX_LINE_INPUT_LEVEL		0x2104		// 0x327C  Configures maximum analog line input level to the LIN/RIN pins to reach the maximum deviation level programmed into the audio deviation property TX Audio Deviation. Default is 636 mVPK. 
#define TX_LINE_INPUT_MUTE		0x2105		// 0x0000  Sets line input mute. L and R inputs may be indepen-dently muted. Default is not muted. 
#define TX_PREEMPHASIS			0x2106		// 0x0000  Configures pre-emphasis time constant. Default is 0 (75 ÏS). 
#define TX_PILOT_FREQUENCY		0x2107		// 0x4A38  Configures the frequency of the stereo pilot. Default is 19000 Hz. 
#define TX_ACOMP_ENABLE3		0x2200		// 0x0002  Enables audio dynamic range control and limiter. Default is 2 (limiter is enabled, audio dynamic range control is disabled). 
#define TX_ACOMP_THRESHOLD		0x2201		// 0xFFD8  Sets the threshold level for audio dynamic range con-trol. Default is ñ40 dB. 
#define TX_ACOMP_ATTACK_TIME	0x2202		// 0x0000  Sets the attack time for audio dynamic range control. Default is 0 (0.5 ms). 
#define TX_ACOMP_RELEASE_TIME	0x2203		// 0x0004  Sets the release time for audio dynamic range control. Default is 4 (1000 ms). 
#define TX_ACOMP_GAIN			0x2204		// 0x000F  Sets the gain for audio dynamic range control. Default is 15 dB. 
#define TX_LIMITER_REL_TIME3	0x2205		// 0x0066  Sets the limiter release time. Default is 102 (5.01 ms) 
#define TX_ASQ_INTERRUPT_SOURCE	0x2300		// 0x0000  Configures measurements related to signal quality metrics. Default is none selected. 
#define TX_ASQ_LEVEL_LOW		0x2301		// 0x0000  Configures low audio input level detection threshold. This threshold can be used to detect silence on the incoming audio. 
#define TX_ASQ_DURATION_LOW		0x2302		// 0x0000  Configures the duration which the input audio level must be below the low threshold in order to detect a low audio condition. 
#define TX_ASQ_LEVEL_HIGH		0x2303		// 0x0000  Configures high audio input level detection threshold. This threshold can be used to detect activity on the incoming audio. 
#define TX_ASQ_DURATION_HIGH	0x2304		// 0x0000  Configures the duration which the input audio level must be above the high threshold in order to detect a high audio condition. 
#define TX_RDS_INTER_SOURCE2	0x2C00		// 0x0000  Si4721 Only. Configure RDS interrupt sources. Default is none selected. 
#define TX_RDS_PI2				0x2C01		// 0x40A7  Si4721 Only. Sets transmit RDS program identi-fier. 
#define TX_RDS_PS_MIX2			0x2C02		// 0x0003  Si4721 Only. Configures mix of RDS PS Group with RDS Group Buffer. 
#define TX_RDS_PS_MISC2			0x2C03		// 0x1008  Si4721 Only. Miscellaneous bits to transmit along with RDS_PS Groups. 
#define TX_RDS_PS_REPEAT_COUNT2	0x2C04		// 0x0003  Si4721 Only. Number of times to repeat trans-mission of a PS message before transmitting the next PS message. 
#define TX_RDS_PS_MESSA_COUNT2	0x2C05		// 0x0001  Si4721 Only. Number of PS messages in use. 
#define TX_RDS_PS_AF2			0x2C06		// 0xE0E0  Si4721 Only. RDS Program Service Alternate Frequency. This provides the ability to inform the receiver of a single alternate frequency using AF Method A coding and is transmitted along with the RDS_PS Groups. 
#define TX_RDS_FIFO_SIZE2		0x2C07		// 0x0000  Si4721 Only. Number of blocks reserved for the FIFO. Note that the value written must be one larger than the desired FIFO size. 



#define TX_INPUT_FORMAT_ANALOG           0x0050
#define TX_INPUT_FORMAT_DIGITAL          0x000b

#define TX_MIN_FREQ  7600
#define TX_MAX_FREQ 10800

#define TX_MIN_POWER  88  // dBuV
#define TX_MAX_POWER 120  // dBuV
#define TX_AUTO_ANTENNA_CAPACITOR        0x0000

#define TX_COMP_ENABLE_RDS_BIT           0x0001   // Set = RDS, not set = NO RDS
#define TX_COMP_ENABLE_STEREO_BIT        0x0002   // Set = L-R transmitted
#define TX_COMP_ENABLE_STEREO_PILOT_BIT  0x0004   // Set = Stereo pilot transmitted

#define TX_PRE_EMPH_MASK                 0x0003
#define TX_PRE_EMPH_75US                 0x0000
#define TX_PRE_EMPH_50US                 0x0001
#define TX_PRE_EMPH_OFF                  0x0002


#define TX_DIG_INP_FMT_SAMPLE_MODE_BIT      0x0040   // 0 = on rising edge, 1 = on falling

#define TX_DIG_INP_FMT_MODE_MASK            0x0078
#define TX_DIG_INP_FMT_DIG_MODE_I2S         0x0008
#define TX_DIG_INP_FMT_DIG_MODE_LEFT        0x0038
#define TX_DIG_INP_FMT_DIG_MODE_MSB_AT_1ST  0x0068
#define TX_DIG_INP_FMT_DIG_MODE_MSB_AT_2ND  0x0048

#define TX_DIG_INP_FMT_STEREO_MODE_BIT      0x0004   // 0 = Stereo, 1 = Mono

#define TX_DIG_INP_SAMPLE_PRECISION_MASK    0x0003
#define TX_DIG_INP_SAMPLE_PRECISION_16_BIT  0x0000
#define TX_DIG_INP_SAMPLE_PRECISION_20_BIT  0x0001
#define TX_DIG_INP_SAMPLE_PRECISION_24_BIT  0x0002
#define TX_DIG_INP_SAMPLE_PRECISION_8_BIT   0x0003


#define TX_SAMPLE_RATE_DISABLE     0
#define TX_MIN_SAMPLE_RATE     32000  
#define TX_MAX_SAMPLE_RATE     48000

#define TX_COMPRESSION_ENABLE_BIT 0x0001   // 1 = Audio compression (dunamic range control) enabled
#define TX_LIMITER_ENABLE_BIT     0x0002   // 1 = Audio limiter enabled

#define COMPOMENT_ENABLE_DEFAULTS ( TX_COMP_ENABLE_STEREO_BIT | TX_COMP_ENABLE_STEREO_PILOT_BIT )
#define DIGITAL_INPUT_DEFAULTS ( TX_DIG_INP_FMT_DIG_MODE_I2S | TX_DIG_INP_SAMPLE_PRECISION_16_BIT )

// Time to complete different commands:
#define	POWER_UP_TIME_MS         110
#define	GET_REVISION_TIME_MS       1
#define	POWER_DOWN_TIME_MS         1
#define	SET_PROPERTY_TIME_MS      11
#define	GET_PROPERTY_TIME_MS       1
#define	GET_INT_STATUS_TIME_MS     1
#define	PATCH_ARGS_TIME_MS         1
#define	PATCH_DATA_TIME_MS         1
#define	TX_TUNE_FREQ_TIME_MS     101
#define	TX_TUNE_POWER_TIME_MS     21
#define	TX_TUNE_MEASURE_TIME_MS  101
#define	TX_TUNE_STATUS_TIME_MS     1
#define	TX_ASQ_STATUS_TIME_MS      1
#define	TX_RDS_BUFF1_TIME_MS       1
#define	TX_RDS_PS1_TIME_MS         1
#define	GPIO_CTL2_TIME_MS          1
#define	GPIO_SET2_TIME_MS          1

#endif	// _si4721_





