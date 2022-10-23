/* comdef.h - Common defines for all targets

© Copyright Dataton AB 2009, All Rights Reserved

	Created: 09-09-23 	Kalle Levin

*/

#define	NULL 0

#define GHS_AT91SAM7				// FreeRTOS portable.h

#define TC1				765
#define PIT				547
#define FREERTOS_TIMER	TC1			// FreeRTOS portable.h (TC1 or PIT)
#define PICKUP						// HCC

#define FILEOPENOPT					// Dataton extension in HCC FAT

#define KALLE_MW					// portmacro.h

extern int flags;					// To make TracePrintf() happy


