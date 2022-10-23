#ifndef _MMC_H_
#define _MMC_H_

/****************************************************************************
 *
 *            Copyright (c) 2003-2008 by HCC Embedded
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
 * Vaci Ut 110
 * Hungary
 *
 * Tel:  +36 (1) 450 1302
 * Fax:  +36 (1) 450 1303
 * http: www.hcc-embedded.com
 * email: info@hcc-embedded.com
 *
 ***************************************************************************/

#include "driver/hccfat/drv.h"
#include "platform/hccfat/common.h"

#ifdef __cplusplus
extern "C" {
#endif

#define SDHC_ENABLE		1	/* enable SDHC support */
#define USE_CRC			0	/* use CRC for communication */
#define CRC_ROM_TABLE	1	/* put CRC table in ROM */


extern F_DRIVER *mmc_initfunc(unsigned long driver_param);

#define MMC_ERR_NOTPLUGGED -1 /* for high level */

enum {
  MMC_NO_ERROR,
  MMC_ERR_NOTINITIALIZED=101,
  MMC_ERR_INIT,
  MMC_ERR_CMD,
  MMC_ERR_STARTBIT,
  MMC_ERR_BUSY,
  MMC_ERR_CRC,
  MMC_ERR_WRITE,
  MMC_ERR_WRITEPROTECT,
  MMC_ERR_NOTAVAILABLE
};


#ifdef __cplusplus
}
#endif

/******************************************************************************
 *
 * end of mmc.h
 *
 *****************************************************************************/

#endif /* _MMC_H_ */

