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
#ifndef _HCC_RNGBUF_CFG_H_
#define _HCC_RNGBUF_CFG_H_


#ifdef __cplusplus
extern "C" {
#endif

/* Specifyes tha data type hold by the buffer. */
typedef struct {
  void *data;
  hcc_u32 size;
  hcc_u32 nbytes;
  hcc_u32 rd_ndx;
} rngbuf_data_t;


#ifdef __cplusplus
}
#endif

#endif

/****************************** END OF FILE **********************************/
