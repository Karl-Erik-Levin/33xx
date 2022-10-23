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
#ifndef _USB_H_
#define _USB_H_
#include "HCC/hcc_types.h"
#include "Platform/hccusb/usb_config.h"
#include "HCC/endiness.h"
#include "HCC/compiler.h"

#define USB_NUM_OF_ENDPOINTS 4

/******************************************************************************
 ************************ Type definitions ************************************
 *****************************************************************************/
/* Return values for callback functions. */
enum callback_state {
  clbst_ok,       /* Callback executed ok. */
  clbst_in,       /* Start IN transfer. */
  clbst_out,      /* Start out transfer. */
  clbst_error,    /* Error encountered, stop endpoint. */
  clbst_not_ready /* No buffer avaiable, pause endpoint. */
};
/* Callback function type. */
typedef enum callback_state (* usb_callback_t)(void);

/* USB low level callback table. */
typedef struct {
  void (* reset)(void);
  void (* bus_error)(void);
  void (* suspend)(void);
  void (* wakeup)(void);
  void (* pup_on_off)(hcc_u8 on);
  usb_callback_t ep_callback[8];
} usb_callback_table_t;

/******************************************************************************
 ************************ Exported functions **********************************
 *****************************************************************************/
extern hcc_u8 usb_init(hcc_u8 ip);
extern void usb_stop(void);

#if IT_ROUTINE_IS_ISR == 1
  ISR_PRE_DEF(usb_it_handler);
#else
  extern void usb_it_handler(void);
#endif

extern void usb_resume_tx(hcc_u8 ep, hcc_u8* data, hcc_u16 buffer_size);
extern void usb_resume_rx(hcc_u8 ep, hcc_u8* data, hcc_u16 buffer_size);
extern hcc_u8 usb_ep_is_busy(hcc_u8 ep);
extern hcc_u8 usb_ep_error(hcc_u8 ep);
extern hcc_u32 usb_get_remaining(hcc_u8 ep);
extern hcc_u32 usb_get_done(hcc_u8 ep);
extern hcc_u8 usb_send(hcc_u8 ep, usb_callback_t f, hcc_u8* data, hcc_u16 buffer_size, hcc_u32 tr_length, hcc_u32 req_length);
extern hcc_u8 usb_receive(hcc_u8 ep, usb_callback_t f, hcc_u8* data, hcc_u16 buffer_size, hcc_u32 tr_length);
extern hcc_u8 usb_get_state(void);

extern hcc_u8* usb_get_rx_pptr(hcc_u8 ep);
extern hcc_u16 usb_get_rx_plength(hcc_u8 ep);
extern void usb_set_halt(hcc_u8 ep);
extern void usb_clr_halt(hcc_u8 ep);
extern hcc_u8 usb_ep_is_halted(hcc_u8 ep);
extern hcc_u16 usb_get_frame_ctr(void);
extern hcc_u8 usb_ack_config(hcc_u8 cfg);
hcc_u8 usg_get_configuration(void);

/* Dataton extention of API */
void send_handshake(hcc_u8 ep);
void send_zero_packet(hcc_u8 ep);
void usb_stop_ep_tx(hcc_u8 ep);

/******************************************************************************
 ************************ Macro definitions ***********************************
 *****************************************************************************/
 /* These definitions access fileds of a setup packet. The setup packet shall
   be a character array on length 8. */
#define STP_REQU_TYPE(a)    (((hcc_u8*)(a))[0])
#define STP_REQUEST(a)      (((hcc_u8*)(a))[1])
#define STP_VALUE(a)        LE16(((hcc_u16*)(a))[1])
#define STP_INDEX(a)        LE16(((hcc_u16*)(a))[2])
#define STP_LENGTH(a)       LE16(((hcc_u16*)(a))[3])

/* Driver states. */
#define USBST_DISABLED      (0u)
#define USBST_DEFAULT       (1u) /* Only standard requests are handled on
                                     then default pipe. */
#define USBST_ADDRESSED     (2u) /* USB has a unique address, but no
                                    configuration is active. */
#define USBST_PRECONFIGURED (3u) /* set_configuration standard request
                                  has been issued by the host, and the
                                  device did not acknowledged the request
                                  yet. */
#define USBST_CONFIGURED    (4u) /* Device is configured, and fully
                                     functional. */
#define USBST_SUSPENDED     (5u) /* Device has been suspended by the master. */

/* Error flags returned by usb_ep_error(). */
#define USBEPERR_NONE             (0) /* No error. */
#define USBEPERR_TO_MANY_DATA     (BIT0) /* To many data received. */
#define USBEPERR_PROTOCOL_ERROR   (BIT1) /* Protocol error. */
#define USBEPERR_USER_ABORT       (BIT2) /* Transfer was aborted by the
                                             application. */
#define USBEPERR_HOST_ABORT       (BIT3) /* Host aborted the transfer. */

/* STP_REQU_TYPE fileds.(bmRequestType). */
#define USBRQT_DIR_IN           (1u<<7)
#define USBRQT_DIR_OUT          (0u<<7)
#define USBRQT_STD              (0u<<5)
#define USBRQT_CLASS            (1u<<5)
#define USBRQT_VENDOR           (2u<<5)
#define USBRQT_DEVICE           (0u<<0)
#define USBRQT_IFC              (1u<<0)
#define USBRQT_EP               (2u<<0)
#define USBRQT_OTHER            (3u<<0)

/* STP_REQUEST() values for standard requests.(bmRequest). */
#define USBRQ_GET_STATUS         0u
#define USBRQ_CLEAR_FEATURE      1u
#define USBRQ_SET_FEATURE        3u
#define USBRQ_SET_ADDRESS        5u
#define USBRQ_GET_DESCRIPTOR     6u
#define USBRQ_SET_DESCRIPTOR     7u
#define USBRQ_GET_CONFIGURATION  8u
#define USBRQ_SET_CONFIGURATION  9u
#define USBRQ_GET_INTERFACE      10u
#define USBRQ_SET_INTERFACE      11u
#define USBRQ_SYNCH_FRAME        12u

/* Standard USB descriptor type vaules. Used in configuration data. */
#define STDD_DEVICE         0x1u
#define STDD_CONFIG         0x2u
#define STDD_STRING         0x3u
#define STDD_INTERFACE      0x4u
#define STDD_ENDPOINT       0x5u
#define STDD_IFC_ASSOC      0xbu

/* Endpoint type vaues. Used in configuration data. */
#define EP_TYPE_BULK    0u
#define EP_TYPE_CONTROL 1u
#define EP_TYPE_ISO     2u
#define EP_TYPE_IT      3u
#define EP_TYPE_DISABLE 4u

/* This macro will evaluate to an array inicializer list with values of a
   device descriptor. */
#define USB_FILL_DEV_DESC(usb_ver, dclass, dsubclass, dproto, psize, vid,\
                          pid, relno, mstr, pstr, sstr, ncfg) \
  (hcc_u8)0x12u, STDD_DEVICE, (hcc_u8)(usb_ver)\
  , (hcc_u8)((usb_ver) >> 8), (hcc_u8)(dclass), (hcc_u8)(dsubclass)\
  , (hcc_u8)(dproto), (hcc_u8)(psize), (hcc_u8)(vid), (hcc_u8)((vid) >> 8)\
  , (hcc_u8)(pid), (hcc_u8)((pid) >> 8), (hcc_u8)(relno)\
  , (hcc_u8)((relno) >> 8), (hcc_u8)(mstr), (hcc_u8)(pstr), (hcc_u8)(sstr)\
  , (hcc_u8)(ncfg)

/* This macro will evaluate to an array inicializer list with values of a
   configuration descriptor. */
#define USB_FILL_CFG_DESC(size, nifc, cfg_id, str_ndx, attrib, pow) \
  (hcc_u8)0x09u, STDD_CONFIG, (hcc_u8)(size), (hcc_u8)((size) >> 8)\
  , (hcc_u8)(nifc), (hcc_u8)(cfg_id), (hcc_u8)(str_ndx), (hcc_u8)((attrib)|(1<<7)), (hcc_u8)(pow)
/* Values for the attrib field of the configuration descriptor. */
/* Devide ic BUS powered. */
#define CFGD_ATTR_BUS_PWR  (1u<<7)
/* Device is self powered. */
#define CFGD_ATTR_SELF_PWR (1u<<6)
/* Device can wake up the BUS. */
#define CFGD_ATTR_RWAKEUP  (1u<<5)


/* This macro will evaluate to an array inicializer list with values of an
   interface association descriptor. */
#define FILL_IFC_ASSOC_DESC(first_ifc, nifc, fclass, fsubclass, fproto, strndx)\
  (hcc_u8)0x08u, STDD_IFC_ASSOC, (hcc_u8) first_ifc, (hcc_u8) nifc\
  , (hcc_u8)(fclass), (hcc_u8)(fsubclass), (hcc_u8)(fproto), (hcc_u8)(strndx)

/* This macro will evaluate to an array inicializer list with values of a
   interface descriptor. */
#define USB_FILL_IFC_DESC(ifc_id, alt_set, no_ep, iclass, isubclass, iproto, strndx) \
  (hcc_u8)0x09u, STDD_INTERFACE, (hcc_u8)(ifc_id), (hcc_u8)(alt_set), (hcc_u8)(no_ep)\
  , (hcc_u8)(iclass), (hcc_u8)(isubclass), (hcc_u8)(iproto), (hcc_u8)(strndx)

/* This macro will evaluate to an array inicializer list with values of a
   endpoint descriptor. */
#define USB_FILL_EP_DESC(addr, dir, attrib, psize, interval) \
  (hcc_u8)0x07u, STDD_ENDPOINT, (hcc_u8)((addr)&0x7f) | (((hcc_u8)(dir))<<0x7)\
  , (hcc_u8)(attrib), (hcc_u8)((psize) & 0xff), (hcc_u8)(((psize) >> 8) & 0xff)\
  , (interval)

/* IN endpoint (device to host) */
#define EPD_DIR_TX                1
/* Out endpoint (host todevice) */
#define EPD_DIR_RX                0

/* Control endpoint */
#define EPD_ATTR_CTRL             0
/* Isochronous endpoint. */
#define EPD_ATTR_ISO              1
/* Bulk endpoint. */
#define EPD_ATTR_BULK             2
/* Interrupt endpoint. */
#define EPD_ATTR_INT              3
/* Iso endpoint synchronisation type: none */
#define EPD_ATTR_ISO_SYNC_NONE    (0 << 2)
/* Iso endpoint synchronisation type: asynchronous */
#define EPD_ATTR_ISO_SYNC_ASYNC   (1 << 2)
/* Iso endpoint synchronisation type: adaptive */
#define EPD_ATTR_ISO_SYNC_ADAPT   (2 << 2)
/* Iso endpoint synchronisation type: synchronous */
#define EPD_ATTR_ISO_SYNC_SYNC    (3 << 2)
/* Iso endpoint usage type: data endpoint */
#define EPD_ATTR_ISO_USAGE_DATA   (0 << 4)
/* Iso endpoint usage type: feedback endpoint */
#define EPD_ATTR_ISO_USAGE_FEEDB  (1 << 4)
/* Iso endpoint usage type: explicite feedback endpoint */
#define EPD_ATTR_ISO_USAGE_EFEEDB (2 << 4)

/* Standard USB feature selector values. */
#define FEAT_ENDPOINT_HALT        0u
#define FEAT_DEVICE_REMOTE_WAKEUP 1u

#endif

/****************************** END OF FILE **********************************/

