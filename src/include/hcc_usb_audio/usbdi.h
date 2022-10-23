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
 * Váci út 110
 * Hungary
 *
 * Tel:  +36 (1) 450 1302
 * Fax:  +36 (1) 450 1303
 * http: www.hcc-embedded.com
 * email: info@hcc-embedded.com
 *
 ***************************************************************************/
#ifndef _USBDI_H_
#define _USBDI_H_

#include "usbd.h"
#include "usbdi_std.h"
#include "../usbdi_dev.h"

/*! \file
    \brief
    Driver internal common declarations.

    This file contains declarations common for all USB
    peripheral (device) drivers. These declarationsh shall only be used
    in other driver modules but not in class-drivers or user applications. */

#ifdef __cplusplus
extern "C" {
#endif

/*! \brief Transfer direction values */
#define USBDTRDIR_SETUP     2  /*!< \brief Setup transaction */
#define USBDTRDIR_HS_IN     3  /*!< \brief Hand-shake in (rx 0 length packet) */
#define USBDTRDIR_HS_OUT    4  /*!< \brief Hand-shake out (tx 0 length packet) */

/*! Endpoint handle management. */
#define USBD_EPH_AGE(eph)   ((hcc_u8)((eph)>>8))  /*!< \brief Endpoint handle -> age. */
#define USBD_EPH_NDX(eph)   ((hcc_u8)((eph) & 0x00ff)) /*!< \brief Endpoint handle -> index*/
#define USBD_EPH_CREATE(age, ndx)  (usbd_ep_handle_t)(((age)<<8) | (ndx)) /*!< \brief Cerate a handle value from the age and index. */

/*! \brief Endpoint list */
extern usbd_ep_info_t usbd_ep_list[];
/*! \brief Synchronization object to make reconfiguration of an endpoints
           atomic.*/
extern OS_MUTEX_TYPE *usbd_cfg_mutex;
/*! \brief Read out setup data. Implemented by the low-level driver. */
void usbd_get_setup_data(usbd_setup_data_t *setup_data);

#ifdef __cplusplus
}
#endif

#endif
/****************************** END OF FILE **********************************/


