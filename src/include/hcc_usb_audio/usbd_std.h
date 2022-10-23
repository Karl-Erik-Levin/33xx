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
#ifndef _USBD_STD_H_
#define _USBD_STD_H_

/*! \file
    \brief
    World-wide declarations for the STD module.

    This file contains all world-wide public declarations of the standard module.
    The standard mosule is responsible to implement handlers for standard
    USB requests and everyting tightly related to these. */

#include "src/lib/os/os.h"
#include "application/usbd_config.h"
/*! \brief Major version number of the module. */
#define USBD_STD_MAJOR  6
/*! brief Minor version number of the module. */
#define USBD_STD_MINOR  0

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************
 ************************ Macro definitions ***********************************
 *****************************************************************************/
/*! \brief Bitmask for the Request type field of the setup packet (bmRequestType) */
#define USBRQT_DIR_IN           (1u<<7)  /*!< \brief IN request */
#define USBRQT_DIR_OUT          (0u<<7)  /*!< \brief OUT request */
#define USBRQT_TYP_STD          (0u<<5)  /*!< \brief Standard request */
#define USBRQT_TYP_CLASS        (1u<<5)  /*!< \brief Class-specific request */
#define USBRQT_TYP_VENDOR       (2u<<5)  /*!< \brief Vendor-specific request */
#define USBRQT_TYP_MASK         (3u<<5)  /*!< \brief Mask for request type bits */
#define USBRQT_RCP_DEVICE       (0u<<0)  /*!< \brief Recipient is the device */
#define USBRQT_RCP_IFC          (1u<<0)  /*!< \brief Recipient is an interface */
#define USBRQT_RCP_EP           (2u<<0)  /*!< \brief Recipient is an endpoint */
#define USBRQT_RCP_OTHER        (3u<<0)  /*!< \brief Recipient is something else */
#define USBRQT_RCP_MASK         (3u<<0)  /*!< \brief Mask for recipient bits */

/*! Values for the request filed of the setup packet.(bmRequest). */
#define USBRQ_GET_STATUS         0u  /*!< \brief Get status of something */
#define USBRQ_CLEAR_FEATURE      1u  /*!< \brief Clear a feature */
#define USBRQ_SET_FEATURE        3u  /*!< \brief Set a feature */
#define USBRQ_SET_ADDRESS        5u  /*!< \brief Set USB address of the device */
#define USBRQ_GET_DESCRIPTOR     6u  /*!< \brief Read out a descriptor */
#define USBRQ_SET_DESCRIPTOR     7u  /*!< \brief Change a descriptor */
#define USBRQ_GET_CONFIGURATION  8u  /*!< \brief Read out the ID of the active configuration */
#define USBRQ_SET_CONFIGURATION  9u  /*!< \brief Change the active configuration */
#define USBRQ_GET_INTERFACE      10u /*!< \brief Read the state of an interface */
#define USBRQ_SET_INTERFACE      11u /*!< \brief Set alternate setting of an interface */
#define USBRQ_SYNCH_FRAME        12u /*!< \brief Set and report ISO endpoint sych frame */

/*! \brief Standard USB descriptor type vaules.

    Used in configuration data and in standared request processing.*/
#define STDD_DEVICE         0x1u /*!< \brief Device descriptor */
#define STDD_CONFIG         0x2u /*!< \brief Configuration descriptor */
#define STDD_STRING         0x3u /*!< \brief String descriptor */
#define STDD_INTERFACE      0x4u /*!< \brief Interface descriptor */
#define STDD_ENDPOINT       0x5u /*!< \brief Endpoint descriptor */
#define STDD_DEV_QUALIF     0x6u /*!< \brief Device qualifier descriptor */
#define STDD_OTHER_SPEED    0x7u /*!< \brief Other speed configuration descriptor */
#define STDD_IFC_ASSOC      0xbu /*!< \brief Interface association descriptor */

/*! \brief USB configuration descriptor attribute masks.

  Values for the attrib field of the configuration descriptor.
  \see USB_FILL_CFG_DESC*/
#define CFGD_ATTR_BUS_PWR  (3u<<6)  /*!< \brief devide ic BUS powered */
#define CFGD_ATTR_SELF_PWR (1u<<6)  /*!< \brief device is self powered */
#define CFGD_ATTR_RWAKEUP  (1u<<5)  /*!< \brief device can signal remote wakeup */

/*! Endpoint direction specifyers */
/*! \brief IN endpoint (device to host)
     \see USB_FILL_EP_DESC*/
#define EPD_DIR_TX                0x80
/*! \brief Out endpoint (host todevice)
     \see USB_FILL_EP_DESC*/
#define EPD_DIR_RX                0

/*! \brief Control endpoint
     \see USB_FILL_EP_DESC*/
#define EPD_ATTR_CTRL             0
/*! \brief Isochronous endpoint.
     \see USB_FILL_EP_DESC*/
#define EPD_ATTR_ISO              1
/*! \brief Bulk endpoint.
     \see USB_FILL_EP_DESC*/
#define EPD_ATTR_BULK             2
/*! \brief Interrupt endpoint.
     \see USB_FILL_EP_DESC*/
#define EPD_ATTR_INT              3
/*! \brief Iso endpoint synchronisation type: none
     \see USB_FILL_EP_DESC*/
#define EPD_ATTR_ISO_SYNC_NONE    (0 << 2)
/*! \brief Iso endpoint synchronisation type: asynchronous
     \see USB_FILL_EP_DESC*/
#define EPD_ATTR_ISO_SYNC_ASYNC   (1 << 2)
/*! \brief Iso endpoint synchronisation type: adaptive
     \see USB_FILL_EP_DESC*/
#define EPD_ATTR_ISO_SYNC_ADAPT   (2 << 2)
/*! \brief Iso endpoint synchronisation type: synchronous
     \see USB_FILL_EP_DESC*/
#define EPD_ATTR_ISO_SYNC_SYNC    (3 << 2)
/*! \brief Iso endpoint usage type: data endpoint
     \see USB_FILL_EP_DESC*/
#define EPD_ATTR_ISO_USAGE_DATA   (0 << 4)
/*! \brief Iso endpoint usage type: feedback endpoint
     \see USB_FILL_EP_DESC*/
#define EPD_ATTR_ISO_USAGE_FEEDB  (1 << 4)
/*! \brief Iso endpoint usage type: explicite feedback endpoint
     \see USB_FILL_EP_DESC*/
#define EPD_ATTR_ISO_USAGE_EFEEDB (2 << 4)

/*! \brief Standard USB feature selector values. */
#define FEAT_ENDPOINT_HALT        0u  /*!< \brief Endpoin halt feature selector */
#define FEAT_DEVICE_REMOTE_WAKEUP 1u  /*!< \brief Remote wakeup feature selector */

/******************************************************************************
 ************************ Exported functions **********************************
 *****************************************************************************/
/*! \brief The standard request handling task. */
OS_TASK_DEF(usbd_ep0_task);

/*! \brief USB bus reset event handling task. */
OS_TASK_DEF(usbd_reset_task);

/*! \brief Pointer to the current configuration data. */
extern usbd_config_t *config;

#ifdef __cplusplus
}
#endif


#endif
/****************************** END OF FILE **********************************/
