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
#ifndef _USBD_STD_CONFIG_H_
#define _USBD_STD_CONFIG_H_

/* Maximum number of class driver to be supported. Some class drivers
   need multiple entries. */
#define MAX_NO_OF_CDRVS   3
/* Number of endpoints to be supported. Set it to match the capabilityes
   of the low-level driver. */
#define MAX_NO_OF_EP     32
/* Define this macro with value 1 if your device shall be able to execute
    remote wakeup signaling on the USB bus. */
#define USBD_REMOTE_WAKEUP  0

/* Maximum number of interfaces in the configuration. Interface ID:s shall
   start from 0 and must increase withouth holes. */
#define MAX_NO_OF_INTERFACES  3

/* Enable or disable SOF timer functionality. */
#define USBD_SOFTMR_SUPPORT 1

/* Enable or disable isochronous endpoint support. */
#define USBD_ISOCHRONOUS_SUPPORT  1

#endif
/****************************** END OF FILE **********************************/
