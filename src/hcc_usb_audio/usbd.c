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


#include <assert.h>
#include "usbdi.h"

#if USBD_STD_MAJOR != 6
#error "Invalis USBD_STD version."
#endif

#if USBD_MAJOR != 5 || USBD_MINOR != 0
#error "Invalid USBD version."
#endif

#if USBD_HWAPI_MAJOR != 3
#error "Incorrect low-level driver version."
#endif

#if OS_API_MAJOR != 2
#error "Incorrect OS API version."
#endif

/*! \file
    \brief
    Common module implementing transfer handling.

    This module implements transfer handling functions. */

/*! This even is set by the low-level driver if BUS state changes. The following
    state changes are flaged:
    <ul>
      <li>bus reset
      <li>suspend and resume
      <li>?VBUS state change
    </ul>*/
OS_EVENT_BIT_TYPE usbd_bus_event;

/*! This even is set by the low level driver when a SOF has been received. */
OS_EVENT_BIT_TYPE usbd_sof_event;

/*! This synchronisation object is used to solve the following race condition.
    If a task is starting a transfer and at the same time the standard request
    handler task is processing a set configuration or set interface request,
    the following may happen:
    <ol>
     <li>execution of usbd_transfer() passes endpoint age check
     <li>reception of setup packet for set configuration finishes
     <li>interrupt handler triggers usbd_ep0_task()
     <li>usbd_ep0_task() reconfigures the hardware and endpoint usage is changed
     <li>execution of usbd_transfer() is resumed and endpoint hardware is
         programed to execute a transaction while the usage of the endpoint
         already changed
    </ol>
    To solve the problem the mutex will delay one of the conflicting tasks and
    thus either usbd_transfer() will fail due to age check (usbd_ep0_task() got
    hold of synchronisation object first), endpoint hardware is reconfigured
    after it was programmed for the transaction (usbd_transfer() got hold of
    synchronisation object first). Either case the transaction will not be
    carried out on the BUS. */
OS_MUTEX_TYPE *usbd_cfg_mutex;

/*! Allocate resources and call initialization functions of the other modules
    in the right order. */
int usbd_init(usbd_hw_init_info_t *hi)
{
  int r=OS_SUCCESS;
  if (r==OS_SUCCESS) r=os_event_create(&usbd_bus_event);
  if (r==OS_SUCCESS) r=os_event_create(&usbd_sof_event);
  if (r==OS_SUCCESS) r=os_mutex_create(&usbd_cfg_mutex);
  if (r==OS_SUCCESS) r=usbd_std_init();
  if (r==OS_SUCCESS) r=usbd_hw_init(hi);
  return(r);
}

/*! Start operation of the driver. Start function of other modules is
    called in the right order. The hardware is started last to avoid an
    interrupt to be triggered before other modules are ready to process any
    resulting event.*/
int usbd_start(usbd_config_t *conf)
{
  int r=OS_SUCCESS;
  if (r==OS_SUCCESS) r=usbd_std_start(conf);
  if (r==OS_SUCCESS) r=usbd_hw_start();
  return(r);
}

/*! Stop operation of the driver. Stop function of other modules is
    called in the right order. The hardware is stoped first to avoid an
    interrupt to be triggered before other modules are ready to process any
    resulting event.*/
int usbd_stop(void)
{
  int r=OS_SUCCESS;
  if (r==OS_SUCCESS) r=usbd_hw_stop();
  if (r==OS_SUCCESS) r=usbd_std_stop();
  return(r);
}

int usbd_delete(void)
{
  usbd_hw_delete();
  usbd_std_delete();
  (void)os_mutex_delete(usbd_cfg_mutex);
  os_event_delete(usbd_sof_event);
  os_event_delete(usbd_bus_event);
  return(OS_SUCCESS);
}

/*! This helper function will split execution of transfer processing based on
    transfer direction. */
static usbd_error_t _transfer(usbd_transfer_t *tr, usbd_ep_info_t *eph)
{
  int r;

  if (tr->dir == USBDTRDIR_OUT || tr->dir == USBDTRDIR_SETUP ||
      tr->dir == USBDTRDIR_HS_IN)
  {
    r=usbd_receive(tr);
  }
  else
  {
    r=usbd_send(tr);
  }

  /*! The low-level driver will not return a valid USBDER_XXX value if the
      the usbd_send() or usbd_receive() call fails. So we need to convert the
      return value. Also it is up to the common module to assign/separate an
      endpoint to a transfer. */
  if (r)
  {
    r=USBDERR_COMM;
    eph->tr=NULL;
  }
  else
  {
    r=USBDERR_BUSY;
  }

  return(r);
}

usbd_error_t usbd_transfer(usbd_transfer_t *tr)
{
  usbd_ep_info_t *eph;
  int r=0;

  /*! Delay endpoint reconfiguration to avoid sending data on an endpoint
      that is reconfigured after we checked the age of the endpoint handle.
      See usbd_cfg_mutex for more info. */
  if (os_mutex_get(usbd_cfg_mutex))
  {
    return(USBDERR_INTERNAL);
  }

  /*! Convert the endpoint handle to a pointer to the endpoint descriptor. */
  eph=usbd_ep_list+USBD_EPH_NDX(tr->eph);

  /*! Check if the endpoint handle is valid. */
  if (tr->eph==USBD_INVALID_EP_HANDLE_VALUE
      || ( eph->age != USBD_EPH_AGE(tr->eph)))
  {
    r=USBDERR_INVALIDEP;
  }
  else
  {
    /*! Clear completed size. */
    tr->csize=0;

    /*! Check if endpoint is ready to do new transactions.
        If the endpoint has a transfer assigned to it, then the endpoint
        is not ready.*/
    if (eph->tr)
    {
      r=USBDERR_NOTREADY;
    }
    else
    {
      /*! Set busy state otherwise */
      eph->tr=tr;
      tr->slength=0;

      /*! , and call low-level driver. */
      r=_transfer(tr, eph);
    }
  }
  /*! Release usbd_cfg_mutex and return. */
  os_mutex_put(usbd_cfg_mutex);
  return(r);
}

usbd_error_t usbd_transfer_status(usbd_transfer_t *tr)
{
  int r;
  usbd_ep_info_t *eph=usbd_ep_list+USBD_EPH_NDX(tr->eph);

  /*! Process status of transfer. */
  switch (tr->state)
  {
    /* Transfer finished. */
  case USBDTRST_DONE:
    r=USBDERR_NONE;
    break;
    /* status is USBDTRST_BUSY */
  case USBDTRST_BUSY:
    r=USBDERR_BUSY;
    break;
    /* low-level asked for processing. */
  case USBDTRST_CHK:
    /*! and not all bytes transferred call _transfer(). */
    if (tr->length > tr->csize)
    {
      r=_transfer(tr, eph);
      break;
    }

    /*! if all bytes transferred see if a zero length packet is needed */
    if (tr->zp_needed && tr->dir==USBDTRDIR_IN)
    {
      /*! Zero length packet already sent? */
      if (tr->slength == 0)
      {
        /* no, send it. */
        tr->slength=tr->length;
        tr->scsize=tr->csize;
        tr->length=0;
        tr->csize=0;
        r=_transfer(tr, eph);
        break;
      }
    }
    /* Fall trough next case. */
    /*! If status is USBDTRST_SHORTPK */
  case USBDTRST_SHORTPK:
    /*! Zero length packet already sent? */
    if (tr->slength)
    {
      /* restore saved values. */
      tr->length=tr->slength;
      tr->csize=tr->scsize;
    }

    /*!Transfer ended. */
    r=USBDERR_NONE;
    tr->state=USBDTRST_DONE;
    eph->tr=NULL;
    break;
  case USBDTRST_EP_KILLED:
    /*!Transfer ended. */
    r=USBDERR_INVALIDEP;
    tr->state=USBDTRST_DONE;
    eph->tr=NULL;
    break;
  case USBDTRST_COMM:
    /*! Make endpoint free for next transfer. */
    r=USBDERR_COMM;
    tr->state=USBDTRST_DONE;
    eph->tr=NULL;
    break;
  default:
    assert(0);
  }
  return(r);
}

usbd_error_t usbd_transfer_b(usbd_transfer_t *tr)
{
  int r;

  r=usbd_transfer(tr);
  while(USBDERR_BUSY==r)
  {
    /*! Check if the low-level driver sent a notification. */
    if (tr->event)
    {
      (void)os_event_get(*tr->event);
    }
    r=usbd_transfer_status(tr);
  }
  assert(NULL == usbd_ep_list[USBD_EPH_NDX(tr->eph)].tr);
  return(r);
}


/****************************** END OF FILE **********************************/
