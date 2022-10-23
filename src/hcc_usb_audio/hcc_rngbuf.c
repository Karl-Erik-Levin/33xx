/****************************************************************************
*
*            Copyright (c) 2009 by HCC Embedded
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
#include "hcc_rngbuf.h"
#include <string.h>
#include <assert.h>
#include "src/lib/os/os.h"
#include "src/lib/compiler/compiler.h"

#if RNGBUF_MAJOR != 1 || RNGBUF_MINOR != 0
#error "Incorrect version number."
#endif

/*****************************************************************************
* Name: rngbuf_next_free
*
* In:
*    buf - buffer to work on
*    item - pointer to result
*
* Out:
*    item - pointer to next free item or NULL
*    0 - more free buffers available
*    1 - all buffers are busy
*
* Description:
*    allocate the next item for write
*
* Assumptions:
*    "rngbuf_init" was called earlyer.
*****************************************************************************/
int rngbuf_next_free(rngbuf_t *buf, rngbuf_item_t **item)
{
  *item=NULL;
  if (rngbfst_free==buf->wr->state)
  {
    *item=buf->wr;
    (*item)->state=rngbfst_wr;
    buf->wr=(*item)->next;
    if (rngbfst_free==buf->wr->state)
    {
      return 0;
    }
  }
  return 1;
}

/*****************************************************************************
* Name: rngbuf_next_filled
*
* In:
*    buf - buffer to work on
*    item - pointer to result
*
* Out:
*    item - pointer to next free item or NULL
*    0 - more free buffers available
*    1 - all buffers are busy
*
* Description:
*    allocate the next item for read
*
* Assumptions:
*    "rngbuf_init" was called earlyer.
*****************************************************************************/
int rngbuf_next_filled(rngbuf_t *buf, rngbuf_item_t **item)
{
  *item=NULL;
  if (rngbfst_filled==buf->rd->state)
  {
    *item=buf->rd;
    (*item)->state=rngbfst_rd;
    buf->rd=(*item)->next;
    buf->read_ctr++;
    if (rngbfst_filled==buf->rd->state)
    {
      return 0;
    }
  }
  return 1;
}

/*****************************************************************************
* Name: rngbuf_step
*
* In:
*    buf - buffer to operate on
*    item - item to free up
*
* Out:
*    n/a
*
* Description:
*    free an item allocated for write or read
*
* Assumptions:
*    "rngbuf_init" was called earlyer.
*    "item" was allocated by rngbuf_next_free() or rngbuf_next_filled()
*****************************************************************************/
void rngbuf_step(rngbuf_t *buf, rngbuf_item_t *item)
{
  assert(NULL != item);

  switch(item->state)
  {
  case rngbfst_wr:
    item->state=rngbfst_filled;
    buf->fill_ctr++;
    if (buf->rd==item && NULL != buf->out_cb.fn)
    {
      (*buf->out_cb.fn)(buf->out_cb.param);
    }
    break;
  case rngbfst_rd:
    item->state=rngbfst_free;
    if (buf->wr == item && NULL != buf->in_cb.fn)
    {
      (*buf->in_cb.fn)(buf->in_cb.param);
    }
    break;
  case rngbfst_free:
  case rngbfst_filled:
    break;
  default:
    assert(0);
  }
}

/*****************************************************************************
* Name: rngbuf_init
*
* In:
*    buf - buffer to initialize
*    item - first item to add
*
* Out:
*    OS_SUCCESS
*
* Description:
*    Initialize ring buffer
*
* Assumptions:
*    Must be called befor other functions.
*****************************************************************************/
int rngbuf_init(rngbuf_t *buf, rngbuf_item_t *item)
{
  buf->wr=buf->rd=item;
  item->next=item;
  rngbuf_set_incb(buf, NULL, 0);
  rngbuf_set_outcb(buf, NULL, 0);
  buf->fill_ctr=0;
  buf->read_ctr=0;
  return OS_SUCCESS;
}

/*****************************************************************************
* Name: rngbuf_add_item
*
* In:
*    buf - buffer to which item shall be added
*    item - item to be added
*
* Out:
*    OS_SUCCESS
*
* Description:
*    Add a buffer item to the buffer.
*
* Assumptions:
*    The buffer must not be used when this function is called.
*****************************************************************************/
int rngbuf_add_item(rngbuf_t *buf, rngbuf_item_t *item)
{
  item->state=rngbfst_free;
  item->next=buf->wr->next;
  buf->wr->next=item;
  return OS_SUCCESS;
}


/*****************************************************************************
* Name: rngbuf_set_incb
*
* In:
*    buf - buffer to operate on
*    fn - call-back function
*    param - parameteh of call-back function
*
* Out:
*    n/a
*
* Description:
*    Sets a call-back function which is called when the fifo becomes non full.
*
* Assumptions:
*    "rngbuf_init" was called earlyer.
*****************************************************************************/
void rngbuf_set_incb(rngbuf_t *buf, void (*fn)(hcc_u32), hcc_u32 param)
{
  buf->in_cb.fn=fn;
  buf->in_cb.param=param;
}

/*****************************************************************************
* Name: rngbuf_set_outcb
*
* In:
*    buf - buffer to operate on
*    fn - call-back function
*    param - parameteh of call-back function
*
* Out:
*    n/a
*
* Description:
*    Sets a call-back function which is called when the fifo becomes non empty.
*
* Assumptions:
*    "rngbuf_init" was called earlyer.
*****************************************************************************/
void rngbuf_set_outcb(rngbuf_t *buf, void (*fn)(hcc_u32), hcc_u32 param)
{
  buf->out_cb.fn=fn;
  buf->out_cb.param=param;
}

/*****************************************************************************
* Name: rngbuf_get_nfilled
*
* In:
*    buf - buffer to operate on
*
* Out:
*    number of filled items
*
* Description:
*    Return number of filled items.
*
* Assumptions:
*    "rngbuf_init" was called earlyer.
*****************************************************************************/
hcc_u16 rngbuf_get_nfilled(rngbuf_t *buf)
{
  return(buf->fill_ctr-buf->read_ctr);
}

/****************************** END OF FILE **********************************/
