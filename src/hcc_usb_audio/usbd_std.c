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
#include <assert.h>
#include <string.h>
#include "usbdi.h"
#include "src/lib/os/os.h"
#include "src/lib/compiler/compiler.h"
#include "usbd_std_config.h"

#if USBD_MAJOR != 5
#error "Invalis USBD version."
#endif

#if USBD_STD_MAJOR != 6 || USBD_STD_MINOR != 0
#error "Invalid USBD_STD version."
#endif

#if USBD_HWAPI_MAJOR != 3
#error "Incorrect low-level driver version."
#endif

#if OS_API_MAJOR != 2
#error "Incorrect OS API version."
#endif

/* This configures if remote wakeup is supported by the driver/device or not. */
#ifndef USBD_REMOTE_WAKEUP
#error "USBD_REMOTE_WAKEUP must be defined with value 0 or 1."
#endif

#if USBD_REMOTE_WAKEUP
hcc_u8 rw_en;
#endif

#define RD_CONFIG_LENGTH(c)  (((hcc_u8*)(c))[2] | (((hcc_u8*)(c))[3]<<8))
#define INVALID_DEV_ADDR 0x80
#define INVALID_CFG_VAL  0xff

#define BUILD_REQ16(rqt, rq)   ((hcc_u16)(((rqt) <<8) | (rq)))

#define MIN(a,b)  (((a) < (b)) ? (a) : (b))

OS_EVENT_BIT_TYPE usbd_stprx_event;
OS_EVENT_BIT_TYPE ep0_event;
usbd_ep_info_t usbd_ep_list[MAX_NO_OF_EP];
usbd_ep_handle_t ep0_handle;
usbd_config_t *config;

static usbd_transfer_t ep0_tr;
static usbd_setup_data_t setup_data;
static hcc_u8 current_cfg;
static hcc_u8 new_addr;
static hcc_u16 state;
int usbd_state;

typedef struct {
  usbd_cdrv_cb_t *cb;
  usbd_ep0_cb_t *ep0_cb;
  int param;
  hcc_u8 _class;
  hcc_u8 _sclass;
  hcc_u8 _proto;
} cdrv_info_t;

static cdrv_info_t cdrv_list[MAX_NO_OF_CDRVS];

static struct
{
  hcc_u8 as;
  usbd_ep_info_t *first_ep;
  usbd_cdrv_cb_t *old_cb;
} alt_setting_list[MAX_NO_OF_INTERFACES];

static hcc_u32 tmp_dsc_buf[64/4];

static usbd_ep_info_t *add_ep(hcc_u16 psize, hcc_u8 ep_type, hcc_u8 addr
                                   , usbd_ep_info_t *prev_ep);
static int usbd_set_config(hcc_u8 cid);
static void remove_ep(int index);

/*****************************************************************************
 * Name:
 *    usbd_clr_halt
 * In:
 *    ep - endpoint handle
 * Out:
 *    n/a
 *
 * Description:
 *    Clear halted flag of an endpoint.
 *
 * Assumptions:
 *
 *****************************************************************************/
void usbd_clr_halt(usbd_ep_handle_t ep)
{
  int endx=USBD_EPH_NDX(ep);
  if (ep != USBD_INVALID_EP_HANDLE_VALUE
       && USBD_EPH_AGE(ep)== usbd_ep_list[endx].age)
  {
    usbd_ep_list[endx].halted=0;
  }
}

/*****************************************************************************
 * Name:
 *    usbd_set_halt
 * In:
 *    ep - endpoint handle
 * Out:
 *    n/a
 *
 * Description:
 *    Set halted flag of an endpoint.
 *
 * Assumptions:
 *
 *****************************************************************************/
void usbd_set_halt(usbd_ep_handle_t ep)
{
  int endx=USBD_EPH_NDX(ep);
  if (ep != USBD_INVALID_EP_HANDLE_VALUE
       && USBD_EPH_AGE(ep)== usbd_ep_list[endx].age)
  {
    usbd_set_stall(endx);
    usbd_ep_list[endx].halted=1;

    /* If endpoint has an ongoing trnsfer, send error notification. */
    if (NULL != usbd_ep_list[endx].tr )
    {
      usbd_transfer_t *tr=usbd_ep_list[endx].tr;
      usbd_ep_list[endx].tr = NULL;
      tr->state=USBDTRST_EP_KILLED;
      if (tr->event)
      {
        os_event_set(*tr->event);
      }
    }
#if USBD_ISOCHRONOUS_SUPPORT
    else if (NULL != usbd_ep_list[endx].fifo)
    {
      usbd_drop_fifo(endx);
    }
#endif
  }
}

/*****************************************************************************
 * Name:
 *    usbd_get_halt
 * In:
 *    ep - endpoint handle
 * Out:
 *    0 - endpoint is not halted
 *    1 - endpoint is halted
 *
 * Description:
 *    Return halted status of an endpoiint
 *
 * Assumptions:
 *
 *****************************************************************************/
int usbd_get_halt(usbd_ep_handle_t ep)
{
  int r=0;
  int endx=USBD_EPH_NDX(ep);
  if (ep != USBD_INVALID_EP_HANDLE_VALUE
       && USBD_EPH_AGE(ep)== usbd_ep_list[endx].age)
  {
    r=usbd_ep_list[endx].halted || usbd_get_stall(endx);
    return(r);
  }
  return (r);
}

/*****************************************************************************
 * Name:
 *    usbd_std_init
 * In:
 *    n/a
 * Out:
 *    OS_SUCCESS
 *    OS_ERR
 *
 * Description:
 *    Init module.
 *
 * Assumptions:
 *
 *****************************************************************************/
int usbd_std_init(void)
{
  current_cfg=0;
  new_addr=INVALID_DEV_ADDR;
  usbd_state=USBDST_DEFAULT;
#if USBD_REMOTE_WAKEUP
  rw_en=0;
#endif
  config=NULL;
  ep0_handle=USBD_INVALID_EP_HANDLE_VALUE;
  memset(cdrv_list, 0 , sizeof(cdrv_list));
  if (OS_SUCCESS != os_event_create(&ep0_event))
  {
    return(OS_ERR);
  }
  ep0_tr.event=&ep0_event;
  if (OS_SUCCESS != os_event_create(&usbd_stprx_event))
  {
    return(OS_ERR);
  }

  return(OS_SUCCESS);
}

/*****************************************************************************
 * Name:
 *    usbd_std_start
 * In:
 *    n/a
 * Out:
 *    OS_SUCCESS
 *    OS_ERR
 *
 * Description:
 *    Start module.
 *
 * Assumptions:
 *
 *****************************************************************************/
int usbd_std_start(usbd_config_t *cfg)
{
  int ndx;

  for(ndx=0; ndx<sizeof(usbd_ep_list)/sizeof(usbd_ep_list[0]); ndx++)
  {
  	usbd_ep_list[ndx].ep_type=0xff;
  }

  usbd_state=USBDST_DEFAULT;

  current_cfg=0;
  new_addr=0;

  config=cfg;
  (void)add_ep(config->device_descriptor[7], EPD_ATTR_CTRL, 0, NULL);
  return(OS_SUCCESS);
}

/*****************************************************************************
 * Name:
 *    usbd_std_stop
 * In:
 *    n/a
 * Out:
 *    OS_SUCCESS
 *    OS_ERR
 *
 * Description:
 *    Stop module.
 *
 * Assumptions:
 *
 *****************************************************************************/
int usbd_std_stop(void)
{
  usbd_set_config(0);
  remove_ep(0);

  new_addr=INVALID_DEV_ADDR;
  usbd_state=USBDST_DISABLED;
#if USBD_REMOTE_WAKEUP
  rw_en=0;
#endif
  config=NULL;
  ep0_handle=USBD_INVALID_EP_HANDLE_VALUE;

  return(OS_SUCCESS);
}

/*****************************************************************************
 * Name:
 *    usbd_std_delete
 * In:
 *    n/a
 * Out:
 *    OS_SUCCESS
 *    OS_ERR
 *
 * Description:
 *    Delete module.
 *
 * Assumptions:
 *
 *****************************************************************************/
int usbd_std_delete(void)
{
  (void)os_event_delete(usbd_stprx_event);
  (void)os_event_delete(ep0_event);
  return(OS_SUCCESS);
}

/*****************************************************************************
 * Name:
 *    usdb_register_cdrv
 * In:
 *    _class - ifc class
 *    sclass - ifc sub class
 *    proto  - ifc protocol
 *    drv_cb - class driver call back
 *    param  - class driver call back parameter
 *    ep0_cb - ep0 call back
 * Out:
 *    OS_SUCCESS
 *    OS_FAILURE
 *
 * Description:
 *    Register a new class driver.
 * Assumptions:
 *
 *****************************************************************************/
int usdb_register_cdrv(hcc_u8 _class, hcc_u8 sclass, hcc_u8 proto
                                , usbd_cdrv_cb_t *drv_cb, int param, usbd_ep0_cb_t *ep0_cb)
{
  int ndx;
  for(ndx=0; ndx<sizeof(cdrv_list)/sizeof(cdrv_list[0]); ndx++)
  {
    if (NULL==cdrv_list[ndx].cb)
    {
      cdrv_list[ndx]._class=_class;
      cdrv_list[ndx]._sclass=sclass;
      cdrv_list[ndx]._proto=proto;
      cdrv_list[ndx].param=param;
      cdrv_list[ndx].cb=drv_cb;
      cdrv_list[ndx].ep0_cb=ep0_cb;
      return(OS_SUCCESS);
    }
  }
  return(OS_ERR);
}

/*****************************************************************************
 * Name:
 *    find_cdrv
 * In:
 *    _class - ifc class
 *    sclass - ifc sub class
 *    proto  - ifc protocol
 * Out:
 *    pointer to entry
 *    0 if not found
 *
 * Description:
 *    Find reg info for an interface
 * Assumptions:
 *
 *****************************************************************************/
static cdrv_info_t* find_cdrv(hcc_u8 _class, hcc_u8 sclass, hcc_u8 proto)
{
  int ndx;
  for(ndx=0; ndx<sizeof(cdrv_list)/sizeof(cdrv_list[0]); ndx++)
  {
    if (NULL != cdrv_list[ndx].cb
      && cdrv_list[ndx]._class==_class
      && cdrv_list[ndx]._sclass==sclass
      && cdrv_list[ndx]._proto==proto)
    {
      return(&cdrv_list[ndx]);
    }
  }
  return NULL;
}
/*****************************************************************************
 * Name:
 *    lang_supported
 * In:
 *    lng: USB spec defined language id
 * Out:
 *    index of language if found
 *    -1 otherwise
 *
 * Description:
 *    Check if the specified language is supported in the current config.
 * Assumptions:
 *
 *****************************************************************************/
static int lang_supported(hcc_u16 lng)
{
  int lndx;
  char *lang_p=(char *)(config->string_descriptor+2);
  for(lndx=0; lndx<config->number_of_languages; lndx++)
  {
    hcc_u16 clng=(hcc_u16)(*lang_p | (*(lang_p+1)<<8));
    if (clng==lng)
    {
      return(lndx);
    }
    lang_p+=2;
  }
  return(-1);
}

/*****************************************************************************
 * Name:
 *    find_string
 * In:
 *    ndx: string index
 *    lang: USB spec defined language id
 * Out:
 *    0 if error
 *    pointer to descriptor otherwise.
 *
 * Description:
 *    Return pointer to specified string descriptor.
 * Assumptions:
 *
 *****************************************************************************/
static hcc_u8 *find_string(hcc_u8 ndx, hcc_u16 lang)
{
  int lndx;

  if (ndx==0)
  {
    return((hcc_u8 *)config->string_descriptor);
  }

  if (config->number_of_strings < ndx)
  {
    return((hcc_u8 *)0);
  }

  ndx--;
  lndx=lang_supported(lang);
  if (lndx<0)
  {
    return((hcc_u8 *)0);
  }

  return((hcc_u8*)((config->strings[lndx])[ndx]));
}

/*****************************************************************************
 * Name:
 *    add_ep
 * In:
 *    psize: maximum packet size
 *    ep_type: endpoint type value
 *    addr: endpoint address
 *    prev_ep: index of previous endpoint
 * Out:
 *    ndx of allocated endpoint
 *
 * Description:
 *    Create a new endpoint.
 * Assumptions:
 *
 *****************************************************************************/
static usbd_ep_info_t *add_ep(hcc_u16 psize, hcc_u8 ep_type, hcc_u8 addr
                                     , usbd_ep_info_t *prev_ep)
{
  int ndx;

  /* Look for a free endpoint. */
  for(ndx=0;ndx<sizeof(usbd_ep_list)/sizeof(usbd_ep_list[0]); ndx++)
  {
  	if (0xff==usbd_ep_list[ndx].ep_type)
  	{
      usbd_ep_list[ndx].addr=addr;
      usbd_ep_list[ndx].psize=psize;
      usbd_ep_list[ndx].ep_type=(hcc_u8)(ep_type & 0x3);
      usbd_ep_list[ndx].halted=0;
#if USBD_ISOCHRONOUS_SUPPORT
      usbd_ep_list[ndx].fifo=NULL;
#endif
      if (prev_ep)
      {
        prev_ep->next=usbd_ep_list+ndx;
      }
      usbd_ep_list[ndx].next=NULL;

      if (addr==0)
      {
        usbd_ep_list[ndx].age=1;
        ep0_handle=USBD_EPH_CREATE(1, 0);
        usbd_ep_list[ndx].eph=ep0_handle;
      }
      else
      {
        usbd_ep_list[ndx].age++;
        usbd_ep_list[ndx].eph=(usbd_ep_handle_t)USBD_EPH_CREATE(usbd_ep_list[ndx].age, ndx);
      }

      /* never add endpoint 0 */
      if (addr)
      {
        if(OS_SUCCESS!=usbd_add_ep(ndx))
        {
          assert(0);
          return(NULL);
        }
      }
      return(usbd_ep_list+ndx);
    }
  }
  /* If getting here, then MAX_NO_OF_EP is too small. */
  assert(0);
  return(NULL);
}

/*****************************************************************************
 * Name:
 *    remove_ep
 * In:
 *    index: enpoint index
 * Out:
 *    none
 *
 * Description:
 *    Destroy the specified endpoint.
 * Assumptions:
 *
 *****************************************************************************/
static void remove_ep(int index)
{
  /* Check if the endpoint is allocated. */
  if (usbd_ep_list[index].ep_type!=0xff)
  {
    /* Kill endpoint in hardware. */
    usbd_remove_ep(index);
    /* Mark endpoint un-allocated. */
    usbd_ep_list[index].ep_type=0xff;
    /* Invalidate any existing endpoint handles. */
    usbd_ep_list[index].age++;
    /* If endpoint has an ongoing trnsfer, send error notitycation. */
    if (usbd_ep_list[index].tr != NULL)
    {
      usbd_transfer_t *tr=usbd_ep_list[index].tr;
      usbd_ep_list[index].tr = NULL;
      tr->state=USBDTRST_EP_KILLED;
      if (tr->event)
      {
        os_event_set(*tr->event);
      }
    }
#if USBD_ISOCHRONOUS_SUPPORT
    else if (usbd_ep_list[index].fifo)
    {
      usbd_drop_fifo(index);
      usbd_ep_list[index].fifo=NULL;
    }
#endif
  }
}

/*****************************************************************************
 * Name:
 *    find_ep
 * In:
 *    addr: enpoint address
 * Out:
 *    index if found
 *    -1 otherwise
 *
 * Description:
 *    Find the endpoint with the specified address
 *
 * Assumptions:
 *
 *****************************************************************************/
static int find_ep(hcc_u8 addr)
{
  int x;
  for(x=0; x< sizeof(usbd_ep_list)/sizeof(usbd_ep_list[0]); x++)
  {
    if (usbd_ep_list[x].ep_type != 0xff
        && usbd_ep_list[x].addr==addr)
    {
      return(x);
    }
  }
  return(-1);
}

/*****************************************************************************
 * Name:
 *    next_descriptor
 * In:
 *    start: start of current descriptor
 * Out:
 *    start of next descritor
 *
 * Description:
 *    Return the start address of the next descriptor
 *
 * Assumptions:
 *
 *****************************************************************************/
static hcc_u8 *next_descriptor(hcc_u8 *start)
{
  return(start+start[0]);
}

/*****************************************************************************
 * Name:
 *    find_descriptor
 * In:
 *    start: buffer start pointer
 *    end:   buffer end pointer
 *    type:  descriptor type value to look for
 * Out:
 *    start of descriptor if found
 *    value end otherwise
 *
 * Description:
 *    Return the start address of the next descriptor with mathcing type
 *
 * Assumptions:
 *
 *****************************************************************************/
static hcc_u8 *find_descriptor(hcc_u8 *start, hcc_u8 *end, hcc_u8 type)
{
  while(start<end)
  {
  	if (start[1] == type)
  	{
  	  return(start);
  	}
    start=next_descriptor(start);
  }

  return(end);
}

/*****************************************************************************
 * Name:
 *    init_ifc
 * In:
 *    id:    interface ID
 *    as:    alternate setting
 *    start: start of configuration descriptor.
 * Out:
 *    OS_SUCCESS
 *    OS_ERROR
 *
 * Description:
 *    initialize the specified interface
 *
 * Assumptions:
 *
 *****************************************************************************/
static int init_ifc(hcc_u8 id, hcc_u8 as, hcc_u8 *start)
{
  hcc_u8 *end=start+RD_CONFIG_LENGTH(start);

  for(;;)
  {
  	start=find_descriptor(start, end, STDD_INTERFACE);
  	/* Exit loop if not found. */
  	if (start == end)
  	{
  	  break;
  	}
    /* If ifc id and alternate setting matches. */
    if (start[2]==id && start[3] == as)
  	{
  	  hcc_u8* ifc_end;
      usbd_ep_info_t *this_ep=NULL;
  	  cdrv_info_t *cdrv_inf=find_cdrv(start[5], start[6], start[7]);

      assert(id<MAX_NO_OF_INTERFACES);
      /* Notofy class driver about configuration change. */
      if( alt_setting_list[id].old_cb )
      {
        alt_setting_list[id].old_cb( 0 , 0 , 0 );
        alt_setting_list[id].old_cb = 0;
      }

      alt_setting_list[id].as=as;
      os_mutex_get(usbd_cfg_mutex);
      while(alt_setting_list[start[2]].first_ep)
      {
        usbd_ep_info_t *p=alt_setting_list[start[2]].first_ep;
      	alt_setting_list[start[2]].first_ep=alt_setting_list[start[2]].first_ep->next;
      	remove_ep(USBD_EPH_NDX(p->eph));
      }
  	  alt_setting_list[start[2]].first_ep=NULL;
      os_mutex_put(usbd_cfg_mutex);


  	  /* If a class driver wants this interface. */
  	  if (NULL!=cdrv_inf)
  	  {
  	    hcc_u8 *ep_start=next_descriptor(start);
  	    /* Find end of configuration data of this interface. */
        ifc_end=find_descriptor(ep_start, end, STDD_INTERFACE);

        os_mutex_get(usbd_cfg_mutex);
  	    /* Look for endpoints. */
  	    do
  	    {
  	      /* Find next endpoint. */
  	      ep_start=find_descriptor(ep_start, ifc_end, STDD_ENDPOINT);
  	      if(ep_start < ifc_end)
  	      {
  	      	this_ep=add_ep((hcc_u16)(ep_start[4] | (ep_start[5]<<8))
                           ,(hcc_u8) (ep_start[3] & 0x3), ep_start[2]
                           , this_ep);
  	      }
  	      else
  	      {
  	      	break;
  	      }
  	      /* If this is the first endpoint, remember the start index. */
  	      if (NULL==alt_setting_list[start[2]].first_ep)
  	      {
  	        alt_setting_list[start[2]].first_ep=this_ep;
  	      }
  	      ep_start=next_descriptor(ep_start);
  	    }while(ep_start < ifc_end);

  	    /* Notify class driver about its configuration. */
        (*cdrv_inf->cb)(alt_setting_list[start[2]].first_ep, start[2], cdrv_inf->param);
        /* Remember which class driver is assigned to the interface currently.*/
        alt_setting_list[start[2]].old_cb = cdrv_inf->cb;
        os_mutex_put(usbd_cfg_mutex);
        return(OS_SUCCESS);
  	  }
  	}
  	/* Prepare looking for next interface. */
    start=next_descriptor(start);
  }
  return(OS_ERR);
}

/*****************************************************************************
 * Name:
 *    init_config
 * In:
 *    start: start of configuration descriptor.
 * Out:
 *    OS_SUCCESS
 *    OS_ERROR
 *
 * Description:
 *
 * Assumptions:
 *
 *****************************************************************************/
static int init_config(hcc_u8 *start)
{
  hcc_u8 *end=start+RD_CONFIG_LENGTH(start);
  int r=OS_ERR;

  for(;;)
  {
  	start=find_descriptor(start, end, STDD_INTERFACE);

  	/* Exit loop if not found. */
  	if (start == end)
  	{
  	  break;
  	}

    /* If this is the default alternate setting. */
    if (start[3] == 0)
  	{
  	  hcc_u8* ifc_end;
      usbd_ep_info_t *this_ep=NULL;
  	  cdrv_info_t *cdrv_inf=find_cdrv(start[5], start[6], start[7]);

      /* Set alternate setting of this interface. */
      assert(start[2]<MAX_NO_OF_INTERFACES);

      alt_setting_list[start[2]].as=0;
  	  alt_setting_list[start[2]].first_ep=NULL;
      alt_setting_list[start[2]].old_cb=0;

  	  /* If a class driver wants this interface. */
  	  if (NULL!=cdrv_inf)
  	  {
  	    hcc_u8 *ep_start=next_descriptor(start);
  	    /* Find end of configuration data of this interface. */
        ifc_end=find_descriptor(ep_start, end, STDD_INTERFACE);

  	    /* Look for endpoints. */
  	    do
  	    {
  	      /* Find next endpoint. */
  	      ep_start=find_descriptor(ep_start, ifc_end, STDD_ENDPOINT);
  	      if(ep_start < ifc_end)
  	      {
  	      	this_ep=add_ep((hcc_u16)(ep_start[4] | (ep_start[5]<<8)),(hcc_u8) (ep_start[3] & 0x3), ep_start[2], this_ep);
  	      }
  	      else
  	      {
  	      	break;
  	      }

  	      if (NULL==alt_setting_list[start[2]].first_ep)
  	      {
  	      	alt_setting_list[start[2]].first_ep=this_ep;
  	      }

  	      ep_start=next_descriptor(ep_start);
  	    }while(ep_start < ifc_end);

  	    /* Notifi class driver about its configuration. */
        (*cdrv_inf->cb)(alt_setting_list[start[2]].first_ep, start[2], cdrv_inf->param);
        /* Remember which class driver is assigned to the interface currently.*/
        alt_setting_list[start[2]].old_cb = cdrv_inf->cb;

        r=OS_SUCCESS;
  	  }
  	}
  	/* Prepare looking for next interface. */
    start=next_descriptor(start);
  }

  usbd_state=USBDST_CONFIGURED;
  return(r);
}

/*****************************************************************************
 * Name:
 *    usbd_set_config
 * In:
 *    cid: id of configuration
 * Out:
 *    OS_
 *    pointer to descriptor otherwise.
 *
 * Description:
 *    Return pointer to specified string descriptor.
 * Assumptions:
 *
 *****************************************************************************/
static int usbd_set_config(hcc_u8 cid)
{
  int x;
  const unsigned char **configs;

  /* Kill all endpoints except 0. */
  for(x=1; x<sizeof(usbd_ep_list)/sizeof(usbd_ep_list[0]); x++)
  {
    remove_ep(x);
  }

  /* Kill all class drivers. */
  for(x=0; x<sizeof(cdrv_list)/sizeof(cdrv_list[0]); x++)
  {
    if (NULL != cdrv_list[x].cb)
    {
      cdrv_list[x].cb(NULL, -1, cdrv_list[x].param);
    }
  }

  if (cid==0)
  {
    usbd_state=USBDST_ADDRESSED;
    return(OS_SUCCESS);
  }

  configs=usbd_at_high_speed() ? config->configurations_hs : config->configurations_fsls;

  /* Search for the configuration with matching ID. */
  for(x=0; x<config->device_descriptor[17]; x++)
  {

    if ((configs[x])[5]==cid)
    {
      int r=init_config((hcc_u8*)configs[x]);
      return(r);
    }
  }
  assert(0);
  return(OS_ERR);
}

/*****************************************************************************
 * Name:
 *    usbd_get_state
 * In:
 *    N/A
 * Out:
 *    N/A
 *
 * Description:
 *
 *
 * Assumptions:
 *
 *****************************************************************************/
int usbd_get_state(void)
{
  return(usbd_state);
}

#if USBD_ISOCHRONOUS_SUPPORT
/*****************************************************************************
 * Name:
 *    usbd_stream_attach
 * In:
 *    ep - endpoint handle
 *    fifo - FIFO 
 * Out:
 *    OS_SUCCESS - if all ok
 *    OS_ERR - on failure
 *
 * Description:
 *    Attach a FIFO to an endpoint.
 *
 * Assumptions:
 *
 *****************************************************************************/
int usbd_stream_attach(usbd_ep_handle_t ep, rngbuf_t *fifo)
{
  int endx=USBD_EPH_NDX(ep);
  if (ep != USBD_INVALID_EP_HANDLE_VALUE
       && USBD_EPH_AGE(ep)== usbd_ep_list[endx].age)
  {
    usbd_ep_list[endx].fifo=fifo;
    return(usbd_add_fifo(endx));
  }
  return(OS_ERR);
}

/*****************************************************************************
 * Name:
 *    usbd_stream_detach
 * In:
 *    ep - endpoint handle
 * Out:
 *    OS_SUCCESS - if all ok
 *    OS_ERR - on failure
 *
 * Description:
 *
 *
 * Assumptions:
 *
 *****************************************************************************/
int usbd_stream_detach(usbd_ep_handle_t ep)
{
  int endx=USBD_EPH_NDX(ep);
  if (ep != USBD_INVALID_EP_HANDLE_VALUE
       && USBD_EPH_AGE(ep)== usbd_ep_list[endx].age)
  {
    usbd_drop_fifo(endx);
    usbd_ep_list[endx].fifo=NULL;
    return OS_SUCCESS;
  }
  return OS_ERR;
}
#endif

/*****************************************************************************
 * Name:
 *    usbd_reset_task
 * In:
 *    N/A
 * Out:
 *    N/A
 *
 * Description:
 *    Handle USB reset event.
 *
 * Assumptions:
 *
 *****************************************************************************/
OS_TASK_FN(usbd_reset_task, param)
{
#if OS_TASK_POLL_MODE
  if (OS_SUCCESS==os_event_get(usbd_bus_event))
  {
#else
  while(1)
  {
    if (OS_SUCCESS!=os_event_get(usbd_bus_event))
    {
      continue;
    }
#endif
    if (usbd_state==USBDST_DEFAULT)
    {
      os_mutex_get(usbd_cfg_mutex);
      current_cfg=0;
      usbd_set_cfg_pre();
      usbd_set_config(current_cfg);
      /* Set config0 will incorrectly move us to adressed state. */
      usbd_state=USBDST_DEFAULT;
      usbd_set_cfg_post();
      os_mutex_put(usbd_cfg_mutex);
    }
  }
}

/*****************************************************************************
 * Name:
 *    send_two_part
 * In:
 *    type: descriptor type to change to
 *    buf:  buffer to be sent
 *    len:  buffer length
 *    requested: number of bytes requested
 * Out:
 *    N/A
 *
 * Description:
 *    Send a descriptor changing its type.
 *
 * Assumptions:
 *
 *****************************************************************************/
static void send_two_part(hcc_u8 type, hcc_u8* buf, hcc_u16 len, hcc_u16 requested)
{
  /* avoid copying more bytes than buffer can hold. */
  hcc_u16 l=(hcc_u16)MIN(len, sizeof(tmp_dsc_buf));
  memcpy(tmp_dsc_buf, buf, l);
  ((hcc_u8*)tmp_dsc_buf)[1]=type;
  ep0_tr.eph=ep0_handle;
  ep0_tr.buffer=(hcc_u8*)tmp_dsc_buf;
  /* do not send more bytes than requested */
  ep0_tr.length=(hcc_u32)MIN(l, requested);

  /* calculate length of whole transfer */
  l=(hcc_u16)MIN(requested, len);

  /* do not send zero length packet if second transfer is needed */
  if (ep0_tr.length < l)
  {
    ep0_tr.zp_needed=0;
  }
  else
  {
    ep0_tr.zp_needed=(hcc_u8)(len < requested);
  }
  if (USBDERR_NONE==usbd_transfer_b(&ep0_tr)
      && ep0_tr.length < l)
  {
    ep0_tr.buffer=buf+ep0_tr.length;
    ep0_tr.length = len - ep0_tr.length;
    ep0_tr.zp_needed=(hcc_u8)(len < requested);
    usbd_transfer_b(&ep0_tr);
  }
}

/*****************************************************************************
 * Name:
 *    usbd_ep0_task
 * In:
 *    N/A
 * Out:
 *    N/A
 *
 * Description:
 *    Handle transfers on the control channel. Started by an event sent by the
 *    driver.
 *
 * Assumptions:
 *
 *****************************************************************************/
OS_TASK_FN(usbd_ep0_task, param)
{
  int r;
  usbd_error_t tr_err;
#if OS_TASK_POLL_MODE
  if (OS_SUCCESS==os_event_get(usbd_stprx_event))
  {
#else
  while(1)
  {
    if (OS_SUCCESS!=os_event_get(usbd_stprx_event))
    {
      continue;
    }
#endif
    tr_err=USBDERR_NONE;
    usbd_get_setup_data(&setup_data);
    switch(BUILD_REQ16(setup_data.bmRequestType, setup_data.bRequest))
    {
    case BUILD_REQ16(USBRQT_DIR_IN | USBRQT_TYP_STD | USBRQT_RCP_DEVICE, USBRQ_GET_DESCRIPTOR):
      r=clbstc_in;
      ep0_tr.dir=USBDTRDIR_IN;
      switch(setup_data.wValue>>8)
      {
      case STDD_DEVICE:
        if (setup_data.wValue && setup_data.wIndex)
        {
          /* Invalid request. */
          r=clbstc_error;
        }
        else
        {
          ep0_tr.eph=ep0_handle;
          ep0_tr.buffer=(hcc_u8*)config->device_descriptor;
          ep0_tr.length=(hcc_u32)MIN(config->device_descriptor[0], setup_data.wLength);
          ep0_tr.zp_needed=(hcc_u8)(ep0_tr.length < setup_data.wLength);
          tr_err=usbd_transfer_b(&ep0_tr);
        }
        break;
      case STDD_CONFIG:
        /* Do we have a CFG descriptor with the requested index? */
        {
          const unsigned char **configs=usbd_at_high_speed() ?
                              config->configurations_hs : config->configurations_fsls;

          if (config->device_descriptor[17] > (hcc_u8)setup_data.wValue
              && setup_data.wIndex == 0)
          {
            hcc_u8 *cfg=(hcc_u8 *)configs[(hcc_u8)setup_data.wValue];
            ep0_tr.eph=ep0_handle;
            ep0_tr.buffer=cfg;
            ep0_tr.length=(hcc_u32)MIN(RD_CONFIG_LENGTH(cfg), setup_data.wLength);
            ep0_tr.zp_needed=(hcc_u8)(ep0_tr.length < setup_data.wLength);
            tr_err=usbd_transfer_b(&ep0_tr);
          }
          else
          {
            /* Invalid request. */
            r=clbstc_error;
          }
        }
        break;
      case STDD_DEV_QUALIF:
        if (setup_data.wValue && setup_data.wIndex)
        {
          /* Invalid request. */
          r=clbstc_error;
        }
        else
        {
          hcc_u8 *buf=(hcc_u8*)tmp_dsc_buf;
          /* length */
          buf[0]=10;
          /* descriptor type */
          buf[1]=STDD_DEV_QUALIF;
          /* USB version */
          /* 2.0 */
          buf[2]=0x0;
          buf[3]=0x2;
          /* class */
          buf[4]=((hcc_u8*)config->device_descriptor)[4];
          /* sub-class */
          buf[5]=((hcc_u8*)config->device_descriptor)[5];
          /* protocol */
          buf[6]=((hcc_u8*)config->device_descriptor)[6];
          /* Maximum packet size */
          buf[7]=((hcc_u8*)config->device_descriptor)[7];
          /* number of configurations */
          buf[8]=((hcc_u8*)config->device_descriptor)[17];
          /* reserved */
          buf[9]=0;
          ep0_tr.eph=ep0_handle;
          ep0_tr.buffer=buf;
          ep0_tr.length=(hcc_u32)MIN(10, setup_data.wLength);
          ep0_tr.zp_needed=(hcc_u8)(10 < setup_data.wLength);
          tr_err=usbd_transfer_b(&ep0_tr);
        }
        break;

      case STDD_OTHER_SPEED:
        /* Do we have a CFG descriptor with the requested index? */
        {
          const unsigned char **configs=usbd_at_high_speed() ?
                              config->configurations_fsls : config->configurations_hs;

          if (config->device_descriptor[17] >= (hcc_u8)setup_data.wValue
              && setup_data.wIndex == 0
              && NULL != configs)
          {
            hcc_u8 *cfg=(hcc_u8 *)configs[(hcc_u8)setup_data.wValue];
            send_two_part(STDD_OTHER_SPEED, cfg, (hcc_u16)RD_CONFIG_LENGTH(cfg), setup_data.wLength);
          }
          else
          {
            /* Invalid request. */
            r=clbstc_error;
          }
        }
        break;
      case STDD_STRING:
        /* See if te required descriptor exists. */
        {
          hcc_u8 *pstr=find_string((hcc_u8)setup_data.wValue, setup_data.wIndex);
          if (pstr != (hcc_u8 *)0)
          {
            ep0_tr.eph=ep0_handle;
            ep0_tr.buffer=pstr;
            ep0_tr.length=(hcc_u32)MIN(pstr[0], setup_data.wLength);
            ep0_tr.zp_needed=(hcc_u8)(ep0_tr.length < setup_data.wLength);
            tr_err=usbd_transfer_b(&ep0_tr);
          }
          else
          {
            /* Invalid request. */
            r=clbstc_error;
          }
        }
        break;

      /* Non standard descriptor type. */
      default:
        /* Call user callback. */
        goto call_usercb;
      }
      break;

    case BUILD_REQ16(USBRQT_DIR_IN | USBRQT_TYP_STD | USBRQT_RCP_DEVICE, USBRQ_GET_CONFIGURATION):
      r=clbstc_in;
      ep0_tr.dir=USBDTRDIR_IN;

      if (setup_data.wValue == 0 && setup_data.wLength == 1
          && setup_data.wIndex == 0)
      {
        ep0_tr.eph=ep0_handle;
        ep0_tr.buffer=(hcc_u8*)&current_cfg;
        ep0_tr.length=1;
        ep0_tr.zp_needed=0;
        tr_err=usbd_transfer_b(&ep0_tr);
      }
      else
      {
        /* Invalid request */
        r=clbstc_error;
      }
      break;
    case BUILD_REQ16(USBRQT_DIR_OUT | USBRQT_TYP_STD | USBRQT_RCP_DEVICE, USBRQ_SET_FEATURE):
  #if USBD_REMOTE_WAKEUP
      if(setup_data.wValue == FEAT_DEVICE_REMOTE_WAKEUP)
      {
        r=clbstc_out;
        rw_en=(hcc_u8)(1<<1);
      }
      else
  #endif
      {
        r=clbstc_error;
      }
      break;
    case BUILD_REQ16(USBRQT_DIR_OUT | USBRQT_TYP_STD | USBRQT_RCP_DEVICE, USBRQ_CLEAR_FEATURE):
  #if USBD_REMOTE_WAKEUP
      if(setup_data.wValue == FEAT_DEVICE_REMOTE_WAKEUP)
      {
        r=clbstc_out;
        rw_en=0;
      }
      else
  #endif
      {
        r=clbstc_error;
      }
      break;
    case BUILD_REQ16(USBRQT_DIR_IN | USBRQT_TYP_STD | USBRQT_RCP_DEVICE, USBRQ_GET_STATUS):
      if (setup_data.wValue == 0 && setup_data.wLength == 2)
      {
  #if USBD_REMOTE_WAKEUP
        state=LE16(rw_en | usbd_is_self_powered()); /* wakeup state, power state */
  #else
        state=LE16(usbd_is_self_powered()); /* no wakeup, power state */
  #endif
        ep0_tr.eph=ep0_handle;
        ep0_tr.buffer=(hcc_u8*)&state;
        ep0_tr.length=2;
        ep0_tr.zp_needed=0;
        ep0_tr.dir=USBDTRDIR_IN;

        tr_err=usbd_transfer_b(&ep0_tr);
        r=clbstc_in;
      }
      else
      {
        /* Invalid request */
        r=clbstc_error;
      }
      break;
    case BUILD_REQ16(USBRQT_DIR_OUT | USBRQT_TYP_STD | USBRQT_RCP_DEVICE, USBRQ_SET_ADDRESS):
      if (setup_data.wValue < 0x80 && setup_data.wLength == 0
           && setup_data.wIndex ==0)
      {
        new_addr=(hcc_u8)setup_data.wValue;
        usbd_set_addr_pre(new_addr);
        r=clbstc_out;
      }
      else
      {
        /* Invalid request */
        r=clbstc_error;
      }
      break;
    case BUILD_REQ16(USBRQT_DIR_OUT | USBRQT_TYP_STD | USBRQT_RCP_DEVICE, USBRQ_SET_CONFIGURATION):
      if (setup_data.wIndex == 0 && setup_data.wLength==0
               && config->device_descriptor[17] >= (hcc_u8)setup_data.wValue)
      {
        /*! Get usbd_cfg_mutex before changeing endpoint configuration. */
        os_mutex_get(usbd_cfg_mutex);
        current_cfg=(hcc_u8)setup_data.wValue;
        usbd_set_cfg_pre();
        usbd_set_config(current_cfg);
        usbd_set_cfg_post();
        os_mutex_put(usbd_cfg_mutex);
        r=clbstc_out;
      }
      else
      {
        r=clbstc_error;
      }
      break;
    case BUILD_REQ16(USBRQT_DIR_OUT | USBRQT_TYP_STD | USBRQT_RCP_IFC, USBRQ_SET_INTERFACE):
      if (setup_data.wIndex < MAX_NO_OF_INTERFACES)
      {
        int x;
        const unsigned char **configs=usbd_at_high_speed() ?
                              config->configurations_hs : config->configurations_fsls;

        /* Search for the configuration with matching ID. */
        for(x=0; x<config->device_descriptor[17]; x++)
        {
          if ((configs[x])[5]==current_cfg)
          {
            init_ifc((hcc_u8)setup_data.wIndex, (hcc_u8)setup_data.wValue, (hcc_u8*)configs[x]);
            break;
          }
        }
        r=clbstc_out;
      }
      else
      {
        r=clbstc_error;
      }
      break;
    case BUILD_REQ16(USBRQT_DIR_IN | USBRQT_TYP_STD | USBRQT_RCP_IFC, USBRQ_GET_INTERFACE):
      if (setup_data.wIndex < MAX_NO_OF_INTERFACES && setup_data.wLength ==1 )
      {
        ep0_tr.eph=ep0_handle;
        ep0_tr.buffer=&alt_setting_list[setup_data.wIndex].as;
        ep0_tr.length=1;
        ep0_tr.zp_needed=0;
        ep0_tr.dir=USBDTRDIR_IN;
        tr_err=usbd_transfer_b(&ep0_tr);
        r=clbstc_in;
      }
      else
      {
        r=clbstc_error;
      }
      break;
    case BUILD_REQ16(USBRQT_DIR_IN | USBRQT_TYP_STD | USBRQT_RCP_IFC, USBRQ_GET_STATUS):
      if (setup_data.wValue == 0 && setup_data.wLength == 2)
      {
        /* This request returns always 0. Defined by the usb spec... */
        state=0;
        ep0_tr.eph=ep0_handle;
        ep0_tr.buffer=(hcc_u8*)&state;
        ep0_tr.length=2;
        ep0_tr.zp_needed=0;
        ep0_tr.dir=USBDTRDIR_IN;
        tr_err=usbd_transfer_b(&ep0_tr);

        r=clbstc_out;
      }
      else
      {
        r=clbstc_error;
      }
      break;
    case BUILD_REQ16(USBRQT_DIR_OUT | USBRQT_TYP_STD | USBRQT_RCP_EP, USBRQ_CLEAR_FEATURE):
      r=clbstc_out;
      switch(setup_data.wValue)
      {
      case FEAT_ENDPOINT_HALT:
        {
          /* Find the endpoint with the address specified in setup packet. */
          int index=find_ep((hcc_u8)setup_data.wIndex);
          usbd_ep_info_t *ep=usbd_ep_list+index;
          if (index == -1 || ep->ep_type == 0xff)
          {
            r=clbstc_error;
            break;
          }
          if (usbd_ep_list[index].halted==0)
          {
            usbd_clr_stall(index);
          }
        }
        break;
      default:
         r=clbstc_error;
      }
      break;
    case BUILD_REQ16(USBRQT_DIR_OUT | USBRQT_TYP_STD | USBRQT_RCP_EP, USBRQ_SET_FEATURE):
      r=clbstc_out;
      switch(setup_data.wValue)
      {
      case FEAT_ENDPOINT_HALT:
        {
          /* Find the endpoint with the address specified in setup packet. */
          int index=find_ep((hcc_u8)setup_data.wIndex);
          usbd_ep_info_t *ep=usbd_ep_list+index;
          if (index == -1 || ep->ep_type == 0xff)
          {
            r=clbstc_error;
            break;
          }
          if (usbd_ep_list[index].halted==0)
          {
            usbd_ep_handle_t eph=USBD_EPH_CREATE(usbd_ep_list[index].age, index);
            usbd_set_halt(eph);
            usbd_clr_halt(eph);
          }
        }
        break;
      default:
         r=clbstc_error;
      }
      break;

    case BUILD_REQ16(USBRQT_DIR_IN | USBRQT_TYP_STD | USBRQT_RCP_EP, USBRQ_GET_STATUS):
      if (setup_data.wValue == 0 && setup_data.wLength == 2)
      {
        /* Find the endpoint with the address specified in setup packet. */
        int index=find_ep((hcc_u8)setup_data.wIndex);
        usbd_ep_info_t *ep=usbd_ep_list+index;
        if (index == -1 || ep->ep_type == 0xff)
        {
          r=clbstc_error;
          break;
        }
        r=clbstc_in;
        state=LE16((hcc_u16)((usbd_ep_list[index].halted || usbd_get_stall(index)) ? 1u : 0u));
        ep0_tr.eph=ep0_handle;
        ep0_tr.buffer=(hcc_u8*)&state;
        ep0_tr.length=2;
        ep0_tr.zp_needed=0;
        ep0_tr.dir=USBDTRDIR_IN;

        tr_err=usbd_transfer_b(&ep0_tr);
      }
      else
      {
        r=clbstc_error;
      }
      break;
    /* Unknown or not implemented request. */
    default:
      call_usercb:
      {
        int ndx;
        r=clbstc_error;
        tr_err=USBDERR_NONE;
        for(ndx=0; ndx<sizeof(cdrv_list)/sizeof(cdrv_list[0]); ndx++)
        {
          if (NULL != cdrv_list[ndx].ep0_cb)
          {
            r=(*cdrv_list[ndx].ep0_cb)(&setup_data, &ep0_tr);
            if (r != clbstc_error)
            {
              break;
            }
          }
        }
      }
    }

    if (USBDERR_NONE != tr_err)
    {
      r=clbstc_error;
    }

    if (USBDERR_NONE == tr_err)
    switch(r)
    {
    case clbstc_in:
      ep0_tr.eph=ep0_handle;
      ep0_tr.buffer=0;
      ep0_tr.length=0;
      ep0_tr.zp_needed=0;
      ep0_tr.dir=USBDTRDIR_HS_IN;
      (void)usbd_transfer_b(&ep0_tr);
      break;
    case clbstc_out:
      ep0_tr.eph=ep0_handle;
      ep0_tr.buffer=0;
      ep0_tr.length=0;
      ep0_tr.zp_needed=0;
      ep0_tr.dir=USBDTRDIR_HS_OUT;
      (void)usbd_transfer_b(&ep0_tr);
      if (new_addr != INVALID_DEV_ADDR)
      {
        usbd_set_addr_post(new_addr);
        new_addr=INVALID_DEV_ADDR;
        usbd_state=USBDST_ADDRESSED;
      }
      break;
    case clbstc_error:
      usbd_set_stall(USBD_EPH_NDX(ep0_handle));
      break;
    }
  }
}
/****************************** END OF FILE **********************************/
