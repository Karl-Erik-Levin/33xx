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
#ifndef _USB_CONFIG_H_
#define _USB_CONFIG_H_
#include "HCC/hcc_types.h"

/*****************************************************************************/
/* Configuration values for the booloader. */
#define USB_NO_OF_STRING_DESC  6u

extern const hcc_u8 usb_device_descriptor[];
extern const hcc_u8 usb_config_descriptor[];
extern const hcc_u8 * const usb_string_descriptors[USB_NO_OF_STRING_DESC];
extern hcc_u8 * const ep_buffers[];
/******************************************************************************
 * Configuration data. (Change these to suit your needs.)
 *
 *****************************************************************************/
/* Set this to one, if the interrupt routine is called as an interrupt service
   routine. (In this case EIC_IVR will be set by usb_init, and the interrupt
   routine will clear the it pending bit in EIC_IPR.) */
#define IT_ROUTINE_IS_ISR           1

#define AIC_IN_PROTECTED_MODE       0			// Delivered with set to 1. Change by Kalle 081020

#define SUPPORTS_DOUBE_BUFFERS      1

#define GET_DEV_DESCRIPTOR()                      usb_device_descriptor
#define IS_CFGD_INDEX(cndx)                       ((cndx) == 1 ? 1 : 0)
#define GET_CFG_DESCRIPTOR(cndx)                  usb_config_descriptor
#define IS_STR_INDEX(sndx)                        ((sizeof(usb_string_descriptors)/\
                                                    sizeof(usb_string_descriptors[0]))\
                                                    > (sndx) ? 1 : 0 )
#define GET_STR_DESCRIPTOR(sndx)                  usb_string_descriptors[(sndx)]
#define IS_IFC_NDX(cndx, indx, iset)              (((indx) + (cndx) + (iset)) == 1 ? 1 : 0)
#define GET_IFC_DESCRIPTOR(cndx, indx, iset)      &usb_config_descriptor[9]
#define IS_EP_NDX(cndx, indx, iset, endx)         (endx < 2 ? 1 : 0)
#define GET_EP_DESCRIPTOR(cndx, indx, iset, endx) (&usb_config_descriptor[9+9+(7*endx)])
#define IS_EP_DBUFFERED(cndx, indx, iset, endx)   ((ep>0 && ep<3) ? 1 : 0)

#define GET_EP_BUFFER(cndx, indx, iset, endx)     (ep_buffers[ep])

#endif
