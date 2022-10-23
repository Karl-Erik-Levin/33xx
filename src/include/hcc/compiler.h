/****************************************************************************
 *
 *            Copyright (c) 2006-2008 by HCC Embedded
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
 * Vaci ut 110
 * Hungary
 *
 * Tel:  +36 (1) 450 1302
 * Fax:  +36 (1) 450 1303
 * http: www.hcc-embedded.com
 * email: info@hcc-embedded.com
 *
 ***************************************************************************/
#ifndef _GHS_COMPILER_H_
#define _GHS_COMPILER_H_

#include "HCC/hcc_types.h"

#define ISR_DEF(name)    	void name(void)
#define ISR_PRE_DEF(name)	extern void name(void)

/* Map compiler specific endiness setting to our system. */
#if __LITTLE_ENDIAN__ == 1
#define BIGENDIAN 0
#else
#define BIGENDIAN 1
#endif

#endif
/****************************** END OF FILE **********************************/
