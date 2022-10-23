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
#ifndef _MST_USB_CONFIG_H_
#define _MST_USB_CONFIG_H_

#include "driver/hccusb/usb.h"

#define USB_VENDOR_ID   0x0102u
#define USB_PRODUCT_ID  0x1515u
#define USB_DEVICE_REL_NUM  0x0100

const hcc_u8 string_descriptor0[4] = {4, 0x3, 0x09, 0x04 };
const hcc_u8 str_manufacturer[16] = { 16, 0x3,
									 'D',0, 'A',0, 'T',0, 'A',0, 'T',0, 'O',0, 'N',0};
									 
const hcc_u8 str_config[44] = { 44, 0x3, 'D',0, 'e',0, 'f',0, 'a',0, 'u',0
                  , 'l',0, 't',0, ' ',0, 'c',0, 'o',0, 'n',0, 'f',0, 'i',0
                  , 'g',0, 'u',0, 'r',0, 'a',0, 't',0, 'i',0, 'o',0, 'n',0};
                  
const hcc_u8 str_interface[26] = { 26, 0x3, 'M',0, 'a',0, 's',0, 's',0, '-',0
                  , 's',0, 't',0, 'o',0, 'r',0, 'a',0, 'g',0, 'e',0};
/* A bulk only mass storage device needs to have a minimum 12 digit long serial number. The last
   12 digits shall be unique. Valid characters are 0-9 and A-F.*/
const hcc_u8 str_serail_number[26] = { 26, 0x3,
									  '1',0, '2',0, '1',0, '1',0, '1',0, '1',0,
                   					  '1',0, '6',0, '1',0, '2',0, '2',0, '2',0};
const hcc_u8 str_product[24] = { 24, 0x3,
								'3',0, '3',0, '5',0, '6',0, ' ',0,
               					'P',0, 'i',0, 'c',0, 'k',0, 'u',0, 'p',0};

const hcc_u8 * const usb_string_descriptors[USB_NO_OF_STRING_DESC] = {
  string_descriptor0, str_manufacturer, str_product, str_serail_number
  , str_config, str_interface
};

const hcc_u8 usb_device_descriptor[] = {
  USB_FILL_DEV_DESC(0x0110, 0, 0, 0, 8, USB_VENDOR_ID, USB_PRODUCT_ID
      , USB_DEVICE_REL_NUM, 1, 2, 3, 1)
};

const hcc_u8 usb_config_descriptor[] = {
  USB_FILL_CFG_DESC(9+9+7+7, 1, 1, 4, CFGD_ATTR_BUS_PWR, 54),
  USB_FILL_IFC_DESC(0, 0, 2, 0x8, 0x6, 0x50, 5),
  USB_FILL_EP_DESC(0x1, 0, 0x2, 64, 0),
  USB_FILL_EP_DESC(0x2, 1, 0x2, 64, 0)
};

/* Problem to allign this variables on even address */
hcc_u16 ep0_buffer[4];
hcc_u16 ep1_buffer[16];
hcc_u16 ep2_buffer[16];
hcc_u8 * const ep_buffers[4] = {(hcc_u8 *)ep0_buffer, (hcc_u8 *)ep1_buffer, (hcc_u8 *)ep2_buffer, 0};
// hcc_u8 ep0_buffer[8];
// hcc_u8 ep1_buffer[32];
// hcc_u8 ep2_buffer[32];
//hcc_u8 * const ep_buffers[4] = {ep0_buffer, ep1_buffer, ep2_buffer, 0};
#endif
/****************************** END OF FILE **********************************/
