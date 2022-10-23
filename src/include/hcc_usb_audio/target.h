/****************************************************************************
*
*            Copyright (c) 2008-2009 by HCC Embedded
*
* This software is copyrighted by and is the sole property of
* HCC.  All rights, title, ownership, or other interests
* in the software remain the property of HCC.  This
* software may only be used in accordance with the corresponding
* license agreement.  Any unauthorized use, duplication, transmission,
* distribution, or disclosure of this software is expressly forbidden.
*
* This Copyright notice may not be removed or modified without prior
* written consent of HCC.
*
* HCC reserves the right to modify this software without notice.
*
* HCC Embedded
* Budapest 1133
* Váci út 110.
* Hungary
*
* Tel:  +36 (1) 450 1302
* Fax:  +36 (1) 450 1303
* http: www.hcc-embedded.com
* email: info@hcc-embedded.com
*
***************************************************************************/


#ifndef _TARGET_H_
#define _TARGET_H_

#include "regs.h"
#include "src/lib/board/board.h"
/* Value of master clock set by target_init(). */
#define MCK 48000000ul
#define PLLCK 96000000ul
#define MAINCK EXT_CLOCK_FREQ

extern int target_init(void);
extern int target_start(void);
extern int target_stop(void);
extern int target_delete(void);

#endif

/****************************** END OF FILE **********************************/
