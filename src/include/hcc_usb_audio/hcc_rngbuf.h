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
#ifndef _HCC_RNGBUF_H_
#define _HCC_RNGBUF_H_

#include "src/lib/os/os.h"
#include "src/lib/compiler/compiler.h"
#include "hcc_rngbuf_cfg.h"

#define RNGBUF_MAJOR 1
#define RNGBUF_MINOR 0

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
  rngbfst_free,
  rngbfst_wr,
  rngbfst_filled,
  rngbfst_rd
} rngbuffer_state_t;

typedef struct rngbuf_info_tag{
  struct rngbuf_info_tag *next;
  rngbuf_data_t data;
  rngbuffer_state_t state;
} rngbuf_item_t;

typedef struct {
  hcc_u32 param;
  void (*fn)(hcc_u32 param);
}cb_info_t;

typedef struct {
  rngbuf_item_t *wr;
  rngbuf_item_t *rd;
  cb_info_t in_cb;
  cb_info_t out_cb;
  hcc_u16 fill_ctr;
  hcc_u16 read_ctr;
} rngbuf_t;

int rngbuf_init(rngbuf_t *buf, rngbuf_item_t *item);
int rngbuf_add_item(rngbuf_t *buf, rngbuf_item_t *item);
void rngbuf_set_outcb(rngbuf_t *buf, void (*fn)(hcc_u32), hcc_u32 param);
void rngbuf_set_incb(rngbuf_t *buf, void (*fn)(hcc_u32), hcc_u32 param);

void rngbuf_step(rngbuf_t *buf, rngbuf_item_t *item);
int rngbuf_next_filled(rngbuf_t *buf, rngbuf_item_t **item);
int rngbuf_next_free(rngbuf_t *buf, rngbuf_item_t **item);

hcc_u16 rngbuf_get_nfilled(rngbuf_t *buf);

#ifdef __cplusplus
}
#endif

#endif

/****************************** END OF FILE **********************************/
