/* crc16CCITT.h - Calculate a 16-bit CRC for a byte array

	Table driven CRC implementation example from RFC1549
	The other variant without table is adjusted by Dataton

© Copyright Dataton AB 2001, All Rights Reserved

Created: 01-02-07 by Kalle Levin

*/

#ifndef _CRC16_
#define _CRC16_

#define CRC16INIT 0xffff		// Initial FCS value
#define CRC16GOOD 0xf0b8		// Good final FCS value

Word crc16CCITT(Word fcs, Byte *cp, int len);

#ifdef USECRCTABLE
  Word Crc16CcittTable(Word fcs, Byte *cp, int len);
#endif

#endif	// _CRC16_


