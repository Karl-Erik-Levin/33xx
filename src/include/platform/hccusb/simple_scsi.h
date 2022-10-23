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
#ifndef _SCSI_H_
#define _SCSI_H_
#include "usb_mst.h"

#define MMC       1
#define DATFLASH  2

#define SCSI_MEDIA  MMC
/*#define SCSI_MEDIA  DATAFLASH*/

typedef enum
{
  scsirv_ok,
  scsirv_phase_err,
  scsirv_pharse_error,
  scsirv_invalid_lun,
  scsirv_buffer_error,
  scsirv_invalid_filed_in_cdb,
  scsirv_lba_out_of_range,
  scsirv_saving_parameters_not_supported,
  scsirv_unknown_cmd,
  scsirv_no_medium,
  scsirv_medium_changed,
  scsirv_verify_failed,
  scsirv_read_error,
  scsirv_write_error,
  scsirv_write_protected
} scsi_ret_val;

#define DIR_RX    0u
#define DIR_TX    1u
#define DIR_NONE  2u

#define STSCSI_IDLE               0u
#define STSCSI_BUSY               1u
#define STSCSI_INQUIRY            2u
#define STSCSI_RD_FMT_CAPACITIES  3u
#define STSCSI_READ_CAPACITY      4u
#define STSCSI_READ_10            5u
#define STSCSI_MODE_SENSE_6       6u
#define STSCSI_REQUEST_SENSE      7u
#define STSCSI_TEST_UNIT_READY    8u
#define STSCSI_WRITE_10           9u
#define STSCSI_VERIFY             10u

extern hcc_u8 scsi_get_state(void);
extern hcc_u8 scsi_init(void);
extern hcc_u16 scsi_process(void);
extern hcc_u8 scsi_has_error(void);
extern scsi_ret_val scsi_setup_command(hcc_u8 lun, hcc_u8* cbw, hcc_u8 cbw_length, hcc_u8 dir, hcc_u32 host_length
                                 ,hcc_u8 *buf, hcc_u32 buf_length);
extern hcc_u8 scsi_get_nlun(void);
extern hcc_u32 scsi_get_remaining(void);
extern void scsi_new_buffer(hcc_u8 lun, hcc_u8 *buf, hcc_u32 buf_length);
#endif
/****************************** END OF FILE **********************************/
