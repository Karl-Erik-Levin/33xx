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
#ifndef _USB_MST_H_
#define _USB_MST_H_
#include "HCC/hcc_types.h"

extern void mst_init(void);
extern void mst_process(void);

/* Dataton specific */
typedef void (*datatonCB)(hcc_u8 whoCalled);
typedef void (*vendorReqCB)(hcc_u8 *usbFifoBuf);
hcc_u16 mst_req_bufsize_dataton(void);
void mst_init_dataton(void *bufPtr, datatonCB cb);
void mst_set_vendorReq(vendorReqCB cb);

#define CBW_FLAGS_DIR  (0x1u<<7)

/* Type to store command block wrappers. */
typedef hcc_u8 command_block_wrapper_t[31];
/* Macros to access CBW elements. */
#define CBW_SIGNATURE(cbp)       (*(((hcc_u32*)(cbp))+0x0))
#define CBW_TAG(cbp)             (*(((hcc_u32*)(cbp))+0x1))
#define CBW_TRANSFER_LENGTH(cbp) (*(((hcc_u32*)(cbp))+0x2))
#define CBW_FLAGS(cbp)           (*(((hcc_u8*)(cbp))+12))
#define CBW_LUN(cbp)             (*(((hcc_u8*)(cbp))+13))
#define CBW_CB_LENGTH(cbp)       (*(((hcc_u8*)(cbp))+14))
#define CBW_CB(cbp)              (((hcc_u8*)(cbp))+15)

/* Structure to store status_block_wrappers. */
typedef hcc_u8 status_block_wrapper_t[13];
/* Macros to access SBW elements. */
#define SBW_SIGNATURE(cbp)       (*(((hcc_u32*)(cbp))+0x0))
#define SBW_TAG(cbp)             (*(((hcc_u32*)(cbp))+0x1))
#define SBW_DATA_RESIDUE(cbp)    (*(((hcc_u32*)(cbp))+0x2))
#define SBW_STATUS(cbp)          (*(((hcc_u8*)(cbp))+12))

#endif
/****************************** END OF FILE **********************************/
