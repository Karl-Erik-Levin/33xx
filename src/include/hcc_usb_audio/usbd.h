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
#ifndef _USBD_H_
#define _USBD_H_

/*! \file
    \brief
    World-wide common declarations.

    This file contains world wide declarations common for all USB
    peripheral (device) drivers.*/

/*! \brief Major vVersion number of the module.  */
#define USBD_MAJOR  5
/*! \brief Minor version number of the module. */
#define USBD_MINOR  0

#include "src/usb-device/usb-drivers/usbd_dev.h"
#include "src/usb-device/usb-drivers/common/usbd_std.h"
#include "usbd_config.h"
#include "usbd_std_config.h"
#include "src/lib/os/os.h"
#if USBD_ISOCHRONOUS_SUPPORT
#include "src/lib/hcc_rngbuf/hcc_rngbuf.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************
 ************************ Type definitions ************************************
 *****************************************************************************/
/*! \brief Endpoint identifyer type. */
typedef hcc_u16 usbd_ep_handle_t;

/*! \brief Return values for ep0 callbacks. */
typedef enum {
  clbstc_error,    /*!< \brief Error encountered, stop endpoint. */
  clbstc_in,       /*!< \brief Start IN transfer. */
  clbstc_out       /*!< \brief Start OUT transfer. */
} usbd_callback_state_t;

/*! \brief Strucutre to hold setup transaction data.

    Used in ep0 callback functions. */
typedef struct {
   hcc_u8 bmRequestType;
   hcc_u8 bRequest;
   hcc_u16 wValue;
   hcc_u16 wIndex;
   hcc_u16 wLength;
} usbd_setup_data_t;

/*! \brief Type of return value for most API functions. */
typedef int usbd_error_t;

/*! \brief USB transfer descriptor.

    This structure is used to describe/manage an USB transfer.
*/
typedef struct {
  usbd_ep_handle_t eph;      /*!< \brief Endpoint handle. */
  hcc_u8 *buffer;            /*!< \brief Data buffer*/
  hcc_u32 length;            /*!< \brief Number of bytes to send/receive */
  hcc_u32 csize;             /*!< \brief Number of bytes already sent/received */
  hcc_u32 slength;           /*!< \brief Used when sending zero length packet. */
  hcc_u32 scsize;            /*!< \brief Used when sending zero length packet. */
  OS_EVENT_BIT_TYPE *event;  /*!< \brief Event to notify caller when transfer ends */
  volatile int state;        /*!< \brief State of transfer */
  hcc_u8 dir;                /*!< \brief Direction of transfer (#USBDTRDIR_IN #USBDTRDIR_OUT) */
  hcc_u8 zp_needed;          /*!< \brief Set to one if transfer shall be closed with short packet. */
} usbd_transfer_t;


/*! \brief Endpoint descriptor.

    Holds information about an endpoint. The driver uses this to manage the
    endpoint. It is only world wide public because class drivers need read only
    access to this information during initialization.
*/
typedef struct usbd_ep_info_s{
  usbd_transfer_t *tr;
  struct usbd_ep_info_s *next;
#if USBD_ISOCHRONOUS_SUPPORT
  rngbuf_t *fifo;
#endif
  usbd_ep_handle_t eph;
  usbd_hw_ep_info_t hw_info;
  hcc_u16 psize;
  hcc_u8 ep_type;
  hcc_u8 addr;
  hcc_u8 age;
  hcc_u8 halted;
} usbd_ep_info_t;

/*! \brief Class-driver call-back function type.

    This type defines the class-driver call-back function. It is called by
    the standard request handler task when the USB configuration of the
    device is changeing. The change can be triggered by the following events:
    <ul>
      <li>bus reset
      <li>set configuration request sent by the host
      <li>set interface request sent by the host
    </ul>
*/
typedef void usbd_cdrv_cb_t(const usbd_ep_info_t * eps /* array with endpoint information for this interface */
                            , const int ifc_ndx /* id field of interface descriptor */
                            , const int param); /* parameter spefified by usdb_register_cdrv() */

/*! \brief Endpoint 0 call-back type.

    This type defines the EP0 (default pipe) call-back function. It is called
    by the standard request handler task and is used to extend the set of
    request the device is ablt to handle on EP0. */
typedef usbd_callback_state_t usbd_ep0_cb_t(const usbd_setup_data_t *stp,
                                            usbd_transfer_t *tr);
/******************************************************************************
 ************************ Global variables ************************************
 *****************************************************************************/
/*! \brief Event to signal bus state chage. (suspended, resume, reset)

    The event is set by the low-level driver. Class-drivers and applications
    can monitor it to be able to react to these events. For example a bus
    powered device has 10 mS to decrease their current draw to 500 uA. The
    power-managemnt module can monitor this signal to see when it needs to
    start the procedure to enter low-power mode. */
extern OS_EVENT_BIT_TYPE usbd_bus_event;

/*! \brief Event to signal SOF received.

    This event is set by driver stack if when a SOF is received from the host.
    While the USB bus is not suspended, this event is set each mS.  */
extern OS_EVENT_BIT_TYPE usbd_sof_event;

/*! \brief Handle of endpoint 0.

    This variable can be used if some transfer needs to be done on EP0. Note:
    EP0 is a shared resource. The driver will lock the endpoint and report
    USBDERR_BUSY to avoid concurrency problems.*/
extern usbd_ep_handle_t ep0_handle;

/******************************************************************************
 ************************ Macro definitions ***********************************
 *****************************************************************************/
/*! \brief Envalid endpoint handle value.

    Use this value to initialize endpoint handles. */
#define USBD_INVALID_EP_HANDLE_VALUE ((hcc_u16 ) 0xff00)

/*! \brief USB driver state values.*/
#define USBDST_DISABLED       0    /*!< \brief State after usbd_init() */
#define USBDST_DEFAULT        1    /*!< \brief State after USB reset */
#define USBDST_ADDRESSED      2    /*!< \brief State after set address */
#define USBDST_CONFIGURED     3    /*!< \brief State after set config */
#define USBDST_SUSPENDED      4    /*!< \brief State after suspend */

/*! \brief USB transfer state values. Used by the low-level layer to
           talk to the common part. */
#define USBDTRST_DONE        0     /*!< \brief Transfer ended. */
#define USBDTRST_BUSY        1     /*!< \brief Low-level is busy. */
#define USBDTRST_CHK         2     /*!< \brief Check status. */
#define USBDTRST_SHORTPK     3     /*!< \brief Ended with short packet. */
#define USBDTRST_EP_KILLED   4     /*!< \brief Failed, endpoint gone. */
#define USBDTRST_COMM        5     /*!< \brief Communication error. */


/*! \brief Error codes. */
#define USBDERR_NONE        0      /*!< \brief no error */
#define USBDERR_BUSY        1      /*!< \brief endpoint is busy */
#define USBDERR_INVALIDEP   3      /*!< \brief invalid endpoint */
#define USBDERR_NOTREADY    5      /*!< \brief transfer can not be started */
#define USBDERR_INTERNAL    6      /*!< \brief internal error (shall only happen
                                        during development) */
#define USBDERR_COMM        7      /*!< \brief communication error */

/*! \brief Transfer direction values. */
#define USBDTRDIR_IN        0      /*!< \brief peripheral to host */
#define USBDTRDIR_OUT       1      /*!< \brief host to peripheral */

/******************************************************************************
 ************************ Inported functions **********************************
 *****************************************************************************/
/*! \brief Pull-up control call-back.

  Called by the driver to enable or disable the pull-up resistor. Some
  implementations hawe on-chip pull-up resistor. In this case this function
  will never be called. */
void usbd_pup_on_off(int on);
/*! \brief Power status call-back.

   Called by the driver to query if the device is currently running self
   powered or not (is powered from the USB). Bus powered only devices
   must never return nonzero value.*/
int usbd_is_self_powered(void);
/******************************************************************************
 ************************ Exported functions **********************************
 *****************************************************************************/
/*! \brief Inicialize.

    This function will allocate all dynamic resources (events, mutexes, etc)
    and preconfigure the hardware for normal operation. This functiol calls
    the _init() function of all USB modules. */
int usbd_init(usbd_hw_init_info_t *hi);
/*! \brief Start operating.

    This function enables normal driver operation.
    <ul>
       <li> pull-up resistor is enabled to make the host detect the device
       <li> processing of standard requests is enabled
       <li> enable USB related interrupt generation
    </ul> */
int usbd_start(usbd_config_t *conf);
/*! \brief Stop operating.

    Stop normal operation.
    <ul>
       <li> disable pull-up resistor (host detects device removal)
       <li> processing of standard requests is stopped
       <li> USB related interrupt generation is disabled
       <li> USB hardware is out to low-power mode if possible
    </ul> */
int usbd_stop(void);
/*! \brief Kill driver.

    Free allocated resources. Call to any other driver function than usbd_init()
    is illegal after this function is called. */
int usbd_delete(void);
/*! \brief Query current driver state.

    This function returns USBDST_XXX values. Please see the documentation of
    these for more details. */
int usbd_get_state(void);

/*! \brief Start a non-blocking transfer.

    Using this function a transfer can be requested to be done on the USB bus.
    The function will "enque" the request and return and will not wayt till the
    transfer is done.
    The return value can be used to determine if the transfer is successfully
    enqued or not. If yes, the event specifyed in the transfer descriptor
    will be set if the transfer end due to any reason. If the transfer can not
    be enqued the event will newer be set.*/
usbd_error_t usbd_transfer(usbd_transfer_t *tr);
/*! \brief Start a blocking transfer.

    Using this function a transfer can be requested to be done on the USB bus.
    The function will not return till the transfer is not finished and will
    block execution of the calling task.*/
usbd_error_t usbd_transfer_b(usbd_transfer_t *tr);
/*! \brief Query status of a transfer.

    This function is to be used with non-blocking transfers. It works well for
    blocking transfers too but uting it that way is pointless since the
    usbd_transfer_b() will return the same status as this call.
    This function allocates CPU time for transfer management and must be called
    by the transfer initiator (application or class-driver) when the event
    specifyed in the transfer descriptor becomes set. */
usbd_error_t usbd_transfer_status(usbd_transfer_t *tr);
/*! \brief Abort an ongoing transfer.

    An ongoing transfer can be aborted by calling this function.
    Note: This is dangerous. Race between host addressing the endpoint and
          firmware calling the function. */
int usbd_transfer_abort(usbd_transfer_t *tr);

/*! \brief Halt a bulk or interrupt endpoint.

    This function can be used to report an error to the USB host. After the call
    any communication to the specifyedendpoint will result in a STALL handshake
    and thus will fail with an error condition on the host side.*/
void usbd_set_halt(usbd_ep_handle_t ep);
/*! \brief Enable host to clear halted state.

    When the firmware is recovered the error condition due to the endpoint has
    been halted it shall call this function. This will enable the host to
    restart communication on the endpoint.*/
void usbd_clr_halt(usbd_ep_handle_t ep);
/*! \brief Check if endpoint is halted.

    A halted endpoint can be restarted ony by the host. This function can be
    used to see if the host restarted the endpoint or not. */
int usbd_get_halt(usbd_ep_handle_t ep);

/*! \brief Return current value of frame counter.

    While the bus is not suspended, the host is sending a SOF packet at the
    start of each frame. Any hadware counts the number of SOF:s seen on the
    bus. The counter is at least 11 bits wide.*/
hcc_u16 usbd_get_frame_ctr(void);

#if USBD_ISOCHRONOUS_SUPPORT
/* Attach a FIFO to an endpoint. */
int usbd_stream_attach(usbd_ep_handle_t ep, rngbuf_t *fifo);

/* Detach a FIFO from an endpoint. */
int usbd_stream_detach(usbd_ep_handle_t ep);
#endif

/*! \brief Register a class driver

    This function registers a class driver. The class driver can specify what
    interface type it is able to drive and what call-back functions the driver
    shall call. drv_cb is used to tell the class-driver which endpoint handles
    it shall use for communication. ep0_cb is called to enable extednig the set
    of requests the device is able to handle on EP0. */
int usdb_register_cdrv(hcc_u8 _class, hcc_u8 sclass, hcc_u8 proto
                                , usbd_cdrv_cb_t *drv_cb, int param, usbd_ep0_cb_t *ep0_cb);

#ifdef __cplusplus
}
#endif

#endif
/****************************** END OF FILE **********************************/


