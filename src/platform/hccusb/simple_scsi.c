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
#include "platform/hccusb/simple_scsi.h"
#include "driver/hccfat/mmc.h"

/********************************************************************
 ************************ MACRO definitions *************************
 *******************************************************************/
#define BLOCK_SIZE    512u

/* write 4 bytes in big endian format. */
#define WR32_BE(adr, dat)      (((hcc_u8*)(adr))[0]=(hcc_u8)((dat) >> 24),((hcc_u8*)(adr))[1]=(hcc_u8)((dat) >> 16),\
                                ((hcc_u8*)(adr))[2]=(hcc_u8)((dat) >> 8),((hcc_u8*)(adr))[3]=(hcc_u8)((dat) >> 0))
/* write 2 bytes in big endian format. */
#define WR16_BE(adr, dat)      (((hcc_u8*)(adr))[2]=(hcc_u8)((dat) >> 8),((hcc_u8*)(adr))[3]=(hcc_u8)((dat) >> 0))

/* Clear 36 bytes from addr. */
#define ZERO36(addr) \
  {\
    int x=9;\
    while(x--)\
    {\
      ((hcc_u32 *)(addr))[x]=0;\
    }\
  }

/* Copy 28 bytes from src to dst. */
/*     for(x=0; x<8; x++)\   ! Orginal will copy 32 byte*/
#define COPY28(src,dst) \
  {\
    int x;\
    for(x=0; x<7; x++)\
    {\
      ((hcc_u32 *)(dst))[x]=((hcc_u32 *)(src))[x];\
    }\
  }

/**************** Mandatory SCSI Commands *********************
 * The document "UniversalSerial Bus Mas Storage Class
 * Compilance Test Specification" (MSC-compilance-0_9a.pdf)
 * available from www.usb.org states the following SCSI
 * commands as mandatory for scsi devices with Pheriperal
 * Device Type (PDT) 0x0/0x7/0xe.
 **************************************************************/
#define SC_INQUIRY			0x12
#define SC_REQUEST_SENSE		0x03
#define SC_TEST_UNIT_READY		0x00
#define SC_READ_10			0x28
#define SC_READ_CAPACITY		0x25
#define SC_WRITE_10			0x2a
/* This is not mandatory command, but windows needs it to be
   able to format the media. */
#define SC_VERIFY			0x2f
/* Dataton implement to make linux and Mac happy */
#define SC_MODE_SENSE_6		0x1a

/*
These commands are optional and curretly not supported.
#define SC_READ_FORMAT_CAPACITIES	0x23
#define SC_SEND_DIAGNOSTIC		0x1d
#define SC_REPORT_LUNS                  0xA0
#define SC_PREVENT_ALLOW_MEDIUM_REMOVAL	0x1e
#define SC_MODE_SELECT_6		0x15
#define SC_MODE_SELECT_10		0x55
#define SC_MODE_SENSE_10		0x5a
#define SC_RELEASE_6			0x17
#define SC_RELEASE_10			0x57
#define SC_RESERVE_6			0x16
#define SC_RESERVE_10			0x56
#define SC_FORMAT_UNIT			0x04
#define SC_READ_6			0x08
#define SC_READ_12			0xa8
#define SC_START_STOP_UNIT		0x1b
#define SC_SYNCHRONIZE_CACHE		0x35
#define SC_WRITE_6			0x0a
#define SC_WRITE_12			0xaa
#define SC_MODE_SENSE_6			0x1a
*/

/* SCSI Sense Key/Additional Sense Code/ASC Qualifier values */
#define SK_NO_SENSE				0x000000
#define SK_INVALID_COMMAND			0x052000
#define SK_INVALID_FIELD_IN_CDB			0x052400
#define SK_LOGICAL_BLOCK_ADDRESS_OUT_OF_RANGE	0x052100
#define SK_LOGICAL_UNIT_NOT_SUPPORTED		0x052500
#define SK_MEDIUM_NOT_PRESENT			0x023a00
#define SK_NOT_READY_TO_READY_TRANSITION	0x062800
#define SK_RESET_OCCURRED			0x062900
#define SK_SAVING_PARAMETERS_NOT_SUPPORTED	0x053900
#define SK_UNRECOVERED_READ_ERROR		0x031100
#define SK_WRITE_ERROR				0x030c02
#define SK_WRITE_PROTECTED			0x072700
#define SK_HARDWARE_ERROR                       0x040000
#define SK_COMMAND_PAHSE_ERROR                  0x064a00
#define SK_MISCOMPARE                           0x0e0800  /* LUN communication failure. */

#define SK(x)		((hcc_u8) ((x) >> 16))	
#define ASC(x)		((hcc_u8) ((x) >> 8))
#define ASCQ(x)		((hcc_u8) (x))

/********************************************************************
 ************************ Type definitions **************************
 *******************************************************************/
typedef struct {
  hcc_u8 op_code;
  hcc_u8 info;
  hcc_u8 lba0;
  hcc_u8 lba1;
  hcc_u8 length;
  hcc_u8 control;
} cdb6_t;

typedef struct {
  hcc_u8 op_code;
  hcc_u8 info;
  hcc_u8 lba_3;
  hcc_u8 lba_2;
  hcc_u8 lba_1;
  hcc_u8 lba_0;
  hcc_u8 info1;
  hcc_u8 length1;
  hcc_u8 length0;
  hcc_u8 control;
} cdb10_t;

typedef struct {
  hcc_u8 op_code;
  hcc_u8 info;
  hcc_u8 lba_3;
  hcc_u8 lba_2;
  hcc_u8 lba_1;
  hcc_u8 lba_0;
  hcc_u8 info1;
  hcc_u8 length3;
  hcc_u8 length2;
  hcc_u8 length1;
  hcc_u8 length0;
  hcc_u8 control;
} cdb12_t;

/********************************************************************
 ************************ External dependencies *********************
 *******************************************************************/
/* none */

/********************************************************************
 ************************ Function predefinitions********************
 *******************************************************************/
scsi_ret_val scsi_check_medium(void);

/********************************************************************
 ************************ Module variables **************************
 *******************************************************************/
//static const char mst_info_text[28] =
static char mst_info_text[28] =
"DATATON "          /* 8 byte vendor id */
"3356 Pickup     "  /* 16 byte product name */
"1000";             /* 4 byte version number. */

static struct {
  hcc_u32 tlength;
  hcc_u32 blength;
  hcc_u32 lba;
  hcc_u32 sense_data;
/*  hcc_u32 sense_data_info; this is not used. */
  F_DRIVER *media_funcs;
  F_PHY media_params;
  hcc_u8 *buffer;
  hcc_u8 state;
  hcc_u8 lun;
  hcc_u8 no_medium;
  hcc_u8 wrprotect;
  union {
    cdb6_t  cdb6;
    cdb10_t cdb10;
    cdb12_t cdb12;
  } *cdb;
} scsi_info;
/********************************************************************
 ************************ Functions *********************************
 *******************************************************************/

/*************************************************************
 * Function "scsi_get_nlun".
 * In:
 *   N/A
 *
 * Return values:
 *   The number of avaiable logical units.
 ************************************************************/
hcc_u8 scsi_get_nlun(void)
{
  return(1);
}

/*************************************************************
 * Function "valid_lun".
 * In:
 *   hcc_u8 lun: number of logical unit. (starting form 0)
 *
 * Return values:
 *   0: if logical unit does not exist
 *   1: if logical  unit exists
 ************************************************************/
static hcc_u8 valid_lun(hcc_u8 lun)
{
  return(lun == 0);
}

/*************************************************************
 * Function "scsi_check_cmd".
 * In:
 *   hcc_u8 lun: logical unit number. (starting form 0)
 *   hcc_u8 *cbw: command block wrapper from USB layer
 *   hcc_u8 cbw_length: length of cbw
 *   hcc_u8 dir: direstion of the transfer (DIR_NONE, DIR_TX, DIR_RX)
 *   hcc_u32 host_length: transfer length from USB layer.
 *   hcc_u8 cmd_length: expected length of the cbw.
 *   hcc_u8 needs_medium: nonzero if the execution of the command
 *                        needs the medium to be present.
 * Return values:
 *   see type scsi_ret_val
 ************************************************************/
static scsi_ret_val scsi_check_cmd(hcc_u8 lun, hcc_u8 cbw_length
                            , hcc_u8 usb_dir, hcc_u8 scsi_dir
                            , hcc_u32 host_length, hcc_u8 cmd_length
                            , hcc_u8 needs_medium)
{
  scsi_ret_val r=scsirv_ok;

  /* First check the data size. */
  if (host_length < scsi_info.tlength)
  {
    /* Host data size < Device data size is a phase error.
     * Carry out the command, but only transfer as much
     * as we are allowed. */
    r=scsirv_phase_err;
  }

  /* If transfer length is 0 the data direction is none. */
  if (scsi_info.tlength == 0)
  {
    scsi_dir=DIR_NONE;
  }
  /* Conflicting data directions is an error. */
  if(usb_dir != scsi_dir)
  {
    r=scsirv_phase_err;
  }

  /* Verify the length of the command. */
  if(cbw_length != cmd_length)
  {
    return(scsirv_pharse_error);
  }

  /* Verify LUN. Note: INQUIRY and REQUEST SENSE are ok with an
     invalud LUN. */
  if(!valid_lun(lun) && (*(hcc_u8 *)scsi_info.cdb != SC_INQUIRY)
     && (*(hcc_u8 *)scsi_info.cdb != SC_REQUEST_SENSE))
  {
    return(scsirv_invalid_lun);
  }


  /* Check if the state of the medium if needed. */
  if (needs_medium)
  {
    scsi_ret_val r1=scsi_check_medium();
    if (r1 != scsirv_ok)
    {
      return(r1);
    }
  }
  return(r);
}


/*************************************************************
 * Function "scsi_set_error".
 * In:
 *   r - error code
 * Return values:
 *   N./A
 * Description:
 *   Will set scsi_info.sense_data according to r.
 ************************************************************/
static void scsi_set_error(scsi_ret_val r)
{
  switch(r)
  {
  case scsirv_ok:
    scsi_info.sense_data=SK_NO_SENSE;
    break;
  case scsirv_invalid_lun:
    scsi_info.sense_data=SK_LOGICAL_UNIT_NOT_SUPPORTED;
    break;
  case scsirv_buffer_error:
    scsi_info.sense_data=SK_HARDWARE_ERROR;
    break;
  case scsirv_lba_out_of_range:
    scsi_info.sense_data=SK_LOGICAL_BLOCK_ADDRESS_OUT_OF_RANGE;
    break;
  case scsirv_unknown_cmd:
    scsi_info.sense_data=SK_INVALID_COMMAND;
    break;
  case scsirv_saving_parameters_not_supported:
    scsi_info.sense_data=SK_SAVING_PARAMETERS_NOT_SUPPORTED;
    break;
  case scsirv_pharse_error:
  case scsirv_invalid_filed_in_cdb:
    scsi_info.sense_data=SK_INVALID_FIELD_IN_CDB;
    break;
  case scsirv_phase_err:
    scsi_info.sense_data=SK_COMMAND_PAHSE_ERROR;
    break;
  case scsirv_no_medium:
    scsi_info.sense_data=SK_MEDIUM_NOT_PRESENT;
    break;
  case scsirv_medium_changed:
    scsi_info.sense_data=SK_RESET_OCCURRED;
    break;
  case scsirv_verify_failed:
    scsi_info.sense_data=SK_MISCOMPARE;
    break;
  case scsirv_read_error:
  case scsirv_write_error:
    scsi_info.sense_data=SK_HARDWARE_ERROR;
    break;
  case scsirv_write_protected:
    scsi_info.sense_data=SK_WRITE_PROTECTED;
    break;
  default:
    /* TODO: handle unknown error. */
    HCC_ASSERT(0);
  }
}

/*************************************************************
 * Function "scsi_get_state".
 * In:
 *   N/A
 * Return values:
 *   N./A
 * Description:
 *   Returns SCSI status.
 ************************************************************/
hcc_u8 scsi_get_state(void)
{
  return(scsi_info.state);
}

/*************************************************************
 * Function "scsi_check_medium".
 * In:
 *   N/A
 * Return values:
 *   Media status.
 * Description:
 *   Returns media status like no medium, medium changed ok.
 ************************************************************/
scsi_ret_val scsi_check_medium(void)
{
  hcc_u8 r;

  HCC_ASSERT(scsi_info.media_funcs->getstatus != 0);

  r=scsi_info.media_funcs->getstatus(0);

  if (r & F_ST_MISSING)
  {
    scsi_info.no_medium=1;
    scsi_set_error(scsirv_no_medium);
    return(scsirv_no_medium);
  }

  if (r & F_ST_WRPROTECT)
  {
    scsi_info.wrprotect=1;
  }
  else
  {
    scsi_info.wrprotect=0;
  }

  if (scsi_info.no_medium || (r & F_ST_CHANGED))
  {
    HCC_ASSERT(scsi_info.media_funcs->getstatus != 0);

    scsi_info.no_medium=0;
    r=scsi_info.media_funcs->getphy(scsi_info.media_funcs,&scsi_info.media_params);

    if (r != MMC_NO_ERROR)
    {
      scsi_set_error(scsirv_no_medium);
      return(scsirv_no_medium);
    }
    scsi_set_error(scsirv_medium_changed);
    return(scsirv_medium_changed);

  }
  scsi_set_error(scsirv_ok);
  return(scsirv_ok);
}

/*************************************************************
 * Function "scsi_init".
 * In:
 *   N/A
 * Return values:
 *   0  - success
 *   !0 - no medium
 * Description:
 *   Returns media status like no medium, medium changed ok.
 ************************************************************/
hcc_u8 scsi_init(void)
{
  /* Initialise global variables. */
  scsi_info.tlength=scsi_info.blength=scsi_info.lba=scsi_info.lun=0;
  scsi_info.no_medium=1;
  scsi_info.wrprotect=0;
  scsi_info.state=STSCSI_IDLE;

  /* Read mmc card propertyes. */
  scsi_info.media_funcs=mmc_initfunc(0);
  if(scsi_info.media_funcs==NULL)
  {
    scsi_set_error(scsirv_no_medium);
    return(1);
  }
  scsi_check_medium();

  return(0);
}

/*************************************************************
 * Function "scsi_setup_command".
 * In:
 *   lun          -logical unit number
 *   cbw          -scsi command bloack wrapper
 *   cbw_length   -length of CBW
 *   dir          -expected transfer direction
 *   host_length  -expected transfer length
 *   buf          -buffer address
 *   buf_length   -buffer length
 * Return values:
 *   scsirv_xxx values
 * Description:
 *   Configures SCSI layer to process commands. It will check
 *   command parameters and will report error if needed.
 ************************************************************/
scsi_ret_val scsi_setup_command(hcc_u8 lun, hcc_u8* cbw, hcc_u8 cbw_length, hcc_u8 dir, hcc_u32 host_length
                                 ,hcc_u8 *buf, hcc_u32 buf_length)
{
  scsi_info.cdb = (void*)cbw;

  /* Check MMC status. */

  if (scsi_info.state != STSCSI_IDLE)
  {
    /* abort current transfer. */
    /* TODO: .....*/
    scsi_info.state = STSCSI_IDLE;
  }

  scsi_info.blength = buf_length;
  scsi_info.buffer = buf;

  /* First select cdb group. This will tell the cdb type (6, 10, 12, etc..). */
  switch(*cbw)
  {
  case SC_INQUIRY:
    {
      scsi_ret_val r;
      scsi_info.tlength=scsi_info.cdb->cdb6.length;
      r = scsi_check_cmd(lun, cbw_length, dir, DIR_TX, host_length, 6, 0);
      scsi_set_error(r);
      if (r != scsirv_ok)
      {
        return(r);
      }

      if (buf_length < 36)
      {
        scsi_set_error(scsirv_buffer_error);
        return(scsirv_buffer_error);
      }
      scsi_info.state = STSCSI_INQUIRY;
    }
    break;
  case SC_MODE_SENSE_6:
    {
      scsi_set_error(scsirv_ok);
      scsi_info.state = STSCSI_MODE_SENSE_6;
    }
    break;
  case SC_READ_CAPACITY:
    {
      hcc_u8 pmi=scsi_info.cdb->cdb10.length0;
      scsi_ret_val r;

      /* Read capacity command has no transfer length filed. Thus we force
         scsi transfer length to 8. */
      scsi_info.tlength = 8;

      r=scsi_check_cmd(lun,cbw_length, dir, DIR_TX, host_length, 10, 1);
      scsi_set_error(r);
      if (r != scsirv_ok)
      {
        return(r);
      }

      scsi_info.lba=((hcc_u32)scsi_info.cdb->cdb10.lba_3 << 24)
                    | ((hcc_u32)scsi_info.cdb->cdb10.lba_2 << 16)
                    | ((hcc_u32)scsi_info.cdb->cdb10.lba_1 << 8)
                    |    scsi_info.cdb->cdb10.lba_0;


      /* Check the PMI and LBA fields */
      if (pmi > 1 || (pmi == 0 && scsi_info.lun != 0))
      {
        scsi_set_error(scsirv_invalid_filed_in_cdb);
        return(scsirv_invalid_filed_in_cdb);
      }

      if (buf_length < 8)
      {
        scsi_set_error(scsirv_buffer_error);
        return(scsirv_buffer_error);
      }

      scsi_info.state = STSCSI_READ_CAPACITY;
    }
    break;
  case SC_WRITE_10:
    {
      scsi_ret_val r;

      scsi_info.tlength = ((hcc_u16)(scsi_info.cdb->cdb10.length1) << 8) | scsi_info.cdb->cdb10.length0;

#if BLOCK_SIZE == 512
      /* SCSI sends transfer length in blocks. To be able to compare the length from
      the CBW and the SCSI right, we convert the CBW length to blocks. */
      r = scsi_check_cmd(lun, cbw_length, dir, DIR_RX, host_length >> 9, 10, 1);
#else
      r = scsi_check_cmd(lun, cbw_length, dir, DIR_RX, host_length / BLOCK_SIZE, 10, 1);
#endif
      scsi_set_error(r);
      if (r != scsirv_ok && r != scsirv_phase_err)
      {
        return(r);
      }
      /* Windows is sending an invalid SCSI command block. The length filed is not filled.
      as a workaround we use the length from the CBW. */
      scsi_info.tlength = ((hcc_u16)(scsi_info.cdb->cdb10.length1) << 8) | scsi_info.cdb->cdb10.length0;
      scsi_info.lba=((hcc_u32)scsi_info.cdb->cdb10.lba_3 << 24)
                    | ((hcc_u32)scsi_info.cdb->cdb10.lba_2 << 16)
                    | ((hcc_u32)scsi_info.cdb->cdb10.lba_1 << 8)
                    |    scsi_info.cdb->cdb10.lba_0;

      if (scsi_info.wrprotect)
      {
        scsi_set_error(scsirv_write_protected);
        return(scsirv_write_protected);
      }
      /* We don't care about DPO and FUA bits, since we don't hawe any chache,
         and we allways write directly to the media.
         We don't support relative adressing. */

      if (scsi_info.cdb->cdb10.info & (1<<0))
      {
        scsi_set_error(scsirv_invalid_filed_in_cdb);
        return scsirv_invalid_filed_in_cdb;
      }

      if (scsi_info.lba + (scsi_info.tlength-1) > scsi_info.media_params.number_of_sectors)
      {
        scsi_set_error(scsirv_lba_out_of_range);
        return scsirv_lba_out_of_range;
      }
#if BLOCK_SIZE == 512
      /* Convert transfer length to bytes. */
      scsi_info.tlength =scsi_info.tlength << 9;
#else
      scsi_info.tlength =scsi_info.tlength * BLOCK_SIZE;
#endif
      scsi_info.state=STSCSI_WRITE_10;
    }
    break;
  case SC_READ_10:
    {
      scsi_ret_val r;

      scsi_info.tlength = ((hcc_u16)(scsi_info.cdb->cdb10.length1) << 8) | scsi_info.cdb->cdb10.length0;

      r = scsi_check_cmd(lun, cbw_length, dir, DIR_TX, host_length, 10, 1);
      scsi_set_error(r);

      if (r != scsirv_ok)
      {
        return(r);
      }
      /* Windows is sending an invalid SCSI command block. The length filed is not filled.
      as a workaround we use the length from the CBW. */
      scsi_info.tlength = ((hcc_u16)(scsi_info.cdb->cdb10.length1) << 8) | scsi_info.cdb->cdb10.length0;
      scsi_info.lba=((hcc_u32)scsi_info.cdb->cdb10.lba_3 << 24)
                    | ((hcc_u32)scsi_info.cdb->cdb10.lba_2 << 16)
                    | ((hcc_u32)scsi_info.cdb->cdb10.lba_1 << 8)
                    |    scsi_info.cdb->cdb10.lba_0;

	/* We don't implement FUA and DPO.  */
      if ((scsi_info.cdb->cdb10.info & ~0x18) != 0)
      {
        scsi_set_error(scsirv_invalid_filed_in_cdb);
        return(scsirv_invalid_filed_in_cdb);
      }

      if (scsi_info.lba + (scsi_info.tlength-1) > scsi_info.media_params.number_of_sectors)
      {
        scsi_set_error(scsirv_lba_out_of_range);
        return scsirv_lba_out_of_range;
      }

#if BLOCK_SIZE == 512
      /* Convert transfer length to bytes. */
      scsi_info.tlength =scsi_info.tlength << 9;
#else
      scsi_info.tlength =scsi_info.tlength * BLOCK_SIZE;
#endif

      scsi_info.state=STSCSI_READ_10;
    }
    break;

  case SC_REQUEST_SENSE:
    {
      scsi_ret_val r;

      /* BUG, BUG, BUG: stupid windows (5.1.2600 Service Pack 2 Build 2600)
         will send a 12 byte CB instead of a 6 byte. To have more fun it sets
         allocation length to 0 and host_length to 0x18. This is a phase error
         (usb dir != scsi dir). As a workaround accept 12 byte CB and force
         allocation length to 0x18. */

      switch (cbw_length)
      {
      case 6:
        scsi_info.tlength=scsi_info.cdb->cdb6.length;
        r= scsi_check_cmd(lun, cbw_length, dir, DIR_TX, host_length, 6, 0);
        break;
      case 12:
        scsi_info.tlength = 18;
        r= scsi_check_cmd(lun, cbw_length, dir, DIR_TX, host_length, 12, 0);
        break;
      }

      /* Request sense shall only alter the current error code, if there is a
         serious error during processing of the command. */
      if (r != scsirv_ok)
      {
        scsi_set_error(r);
        return(r);
      }

      if (buf_length < 18)
      {
        scsi_set_error(scsirv_buffer_error);
        return(scsirv_buffer_error);
      }
    }
    scsi_info.state = STSCSI_REQUEST_SENSE;
    /* If there is no error, this command shall not alter the current error state. */
    return(scsirv_ok);

  case SC_TEST_UNIT_READY:
    {
      scsi_ret_val r;

      scsi_info.tlength = 0;
      r = scsi_check_cmd(lun, cbw_length, dir, DIR_NONE, host_length, 6, 1);
      scsi_set_error(r);
      if (r != scsirv_ok && r != scsirv_phase_err)
      {
        return(r);
      }
      scsi_info.state = STSCSI_IDLE;
    }
    break;
  case SC_VERIFY:
    {
      scsi_ret_val r;

      /* BUG, BUG, BUG: windows will set the transfer length in the CBW to 0 and
         it will send the transfer length in the SCSI commad block to 397. So the
         CBW and SCSI transfer length is not the same, and thus we will report phase
         error. To avoid this, we override phase errors here. */
      scsi_info.tlength = ((hcc_u16)(scsi_info.cdb->cdb10.length1) << 8)
                            | scsi_info.cdb->cdb10.length0;
      r = scsi_check_cmd(lun, cbw_length, dir, DIR_NONE, host_length, 10, 1);
      if (r==scsirv_phase_err)
      {
        r=scsirv_ok;
      }

      scsi_set_error(r);

      if (r != scsirv_ok && r != scsirv_phase_err)
      {
        return(r);
      }

      /* Check reserved bits. */
      if (((scsi_info.cdb->cdb10.info & 0xc) != 0)
        || (scsi_info.cdb->cdb10.info1 != 0))
      {
        scsi_set_error(scsirv_phase_err);
        return(scsirv_phase_err);
      }

      /* We don't support relative addressing and byte-by-byte comparison.
         DPO bit is simply ignored since we don't have any chache. */
      if ((scsi_info.cdb->cdb10.info & 0x3) != 0)
      {
        scsi_set_error(scsirv_invalid_filed_in_cdb);
        return(scsirv_invalid_filed_in_cdb);
      }
      /* Set start address. */
      scsi_info.lba=((hcc_u32)scsi_info.cdb->cdb10.lba_3 << 24)
                    | ((hcc_u32)scsi_info.cdb->cdb10.lba_2 << 16)
                    | ((hcc_u32)scsi_info.cdb->cdb10.lba_1 << 8)
                    |    scsi_info.cdb->cdb10.lba_0;

      /* Check address is not out of bounds. */
      if (scsi_info.lba + (scsi_info.tlength-1) > scsi_info.media_params.number_of_sectors)
      {
        scsi_set_error(scsirv_lba_out_of_range);
        return scsirv_lba_out_of_range;
      }

#if BLOCK_SIZE == 512
      /* Convert transfer length to bytes. */
      scsi_info.tlength =scsi_info.tlength << 9;
#else
      scsi_info.tlength =scsi_info.tlength * BLOCK_SIZE;
#endif

      scsi_info.state = STSCSI_VERIFY;
    }
    break;
  default:
    /* Unknown or not implemented command. Return error. */
    scsi_info.tlength=0;
    scsi_set_error(scsirv_unknown_cmd);
    return(scsirv_unknown_cmd);
  }
  return(scsirv_ok);
}

/*************************************************************
 * Function "scsi_process".
 * In:
 *   N/A
 * Return values:
 *   Depends on processed command.
 * Description:
 *   After a scsi_setup_comand call will process the command.
 ************************************************************/
hcc_u16 scsi_process()
{
  switch(scsi_info.state)
  {
  case STSCSI_INQUIRY:
    if (!valid_lun(scsi_info.lun))
    { /* Unsupported LUNs are okay */
      ZERO36(scsi_info.buffer);
      /* Unsupported, no device-type */
      scsi_info.buffer[0] = 0x7f;
      scsi_info.state = STSCSI_IDLE;
    }
    else
    {
      scsi_info.buffer[0] = 0;              /* direct-access device */
      /* Note: the FAT12 driver of windows XP (2600.xpsp_sp2_gdr.050301-1519)
       seems to have a BUG. Mass storage is not working if the filesysytem is FAT12
       and the device is not remowable. */
      scsi_info.buffer[1] = 0x80;     /* Removable media. */
//      scsi_info.buffer[2] = 2;        /* We clainm to be ANSI SCSI 2 compatible. */
      scsi_info.buffer[2] = 4;        /* Jonsson kör med 4 */
      scsi_info.buffer[3] = 2;        /* SCSI-2 INQUIRY data format. */
//      scsi_info.buffer[4] = 31;	    /* Additional length. */
      scsi_info.buffer[4] = 32;	    /* Jonsson kör med 32 */
      scsi_info.buffer[5] = scsi_info.buffer[6] = 0;  /* 2 reserved bytes. */
      scsi_info.buffer[7] = 0;        /* No special options. */
        /* The vendor ID, product ID, and release number. */
      COPY28(mst_info_text ,&scsi_info.buffer[8]);
      scsi_info.state = STSCSI_IDLE;
    }
    return (scsi_info.tlength < 36 ? scsi_info.tlength : 36);
  case STSCSI_MODE_SENSE_6:
      scsi_info.state = STSCSI_IDLE;
      scsi_info.buffer[0] = 0x03;
      scsi_info.buffer[1] = scsi_info.buffer[2] = scsi_info.buffer[3] = 0;
    return 4;
  case STSCSI_READ_CAPACITY:
    /* Max logical block */
    WR32_BE(&scsi_info.buffer[0], scsi_info.media_params.number_of_sectors-1);
    WR32_BE(&scsi_info.buffer[4], BLOCK_SIZE);   /* Block length */
    scsi_info.state = STSCSI_IDLE;
    /* The SCSI command block of read capacity has no transfer length
       filed. Thus we return always 8 bytes. */
    return 8;
  case STSCSI_READ_10:
    {
      hcc_u16 length = scsi_info.tlength < scsi_info.blength ? scsi_info.tlength : scsi_info.blength;
      if (length > 0)
      {
        int r;

        HCC_ASSERT(scsi_info.media_funcs->getstatus != 0);

        r=scsi_info.media_funcs->readmultiplesector(scsi_info.media_funcs, scsi_info.buffer, scsi_info.lba, length >> 9);

        if (r != MMC_NO_ERROR)
        {
          if (r==MMC_ERR_NOTPLUGGED)
          {
            scsi_check_medium();
          }
          scsi_info.state=STSCSI_IDLE;
          scsi_set_error(scsirv_read_error);
          return(0);
        }

        scsi_info.tlength -= length;
        scsi_info.lba+= length >>9;
        if (scsi_info.tlength == 0)
        {
          scsi_info.state=STSCSI_IDLE;
        }
      }
      else
      {
        scsi_info.state=STSCSI_IDLE;
      }
      return (length);
    }
  case STSCSI_WRITE_10:
    {
      hcc_u16 length = scsi_info.tlength < scsi_info.blength ? scsi_info.tlength : scsi_info.blength;
      if (length > 0)
      {
        int r;
        HCC_ASSERT(scsi_info.media_funcs->getstatus != 0);

        r=scsi_info.media_funcs->writemultiplesector(scsi_info.media_funcs, scsi_info.buffer, scsi_info.lba, length >> 9);

        if (r != MMC_NO_ERROR)
        {
          if (r==MMC_ERR_NOTPLUGGED)
          {
            scsi_check_medium();
          }
          scsi_info.state=STSCSI_IDLE;
          scsi_set_error(scsirv_read_error);
          return(1);
        }

        scsi_info.tlength -= length;

#if BLOCK_SIZE == 512
      /* Convert transfer length to bytes. */
        scsi_info.lba += length>>9;
#else
        scsi_info.lba += length / BLOCK_SIZE;
#endif

        if (scsi_info.tlength == 0)
        {
          scsi_info.state=STSCSI_IDLE;
        }
      }
      else
      {
        scsi_info.state=STSCSI_IDLE;
      }
      return (0);
    }
  case STSCSI_REQUEST_SENSE:
    {
      /* We use vendor specific sense data format. */
      scsi_info.buffer[0] = 0x80 | 0x70;        /* Valid, current error */

      if (!valid_lun(scsi_info.lun))
      {
        scsi_info.buffer[2] = (hcc_u8)(SK_LOGICAL_UNIT_NOT_SUPPORTED >> 16);
        scsi_info.buffer[12] = (hcc_u8)(SK_LOGICAL_UNIT_NOT_SUPPORTED >> 8);
        scsi_info.buffer[13] = (hcc_u8)(SK_LOGICAL_UNIT_NOT_SUPPORTED);
        WR32_BE(&scsi_info.buffer[0], 0);	/* Sense information */
      }
      else
      {
        scsi_info.buffer[2] = (hcc_u8)(scsi_info.sense_data>>16);
        scsi_info.buffer[12] = (hcc_u8)(scsi_info.sense_data>>8);
        scsi_info.buffer[13] = (hcc_u8)(scsi_info.sense_data);
        /* TODO: sense data info could return offset values in some cases. */
        /*WR32_BE(&scsi_info.buffer[3], scsi_info.sense_data_info);
        scsi_info.sense_data_info = 0;*/
        WR32_BE(&scsi_info.buffer[3], 0);	/* Sense information */
        scsi_set_error(scsirv_ok);
      }

      scsi_info.buffer[7] = 18 - 8;		/* Additional sense length */
      scsi_info.buffer[1] = scsi_info.buffer[8] = scsi_info.buffer[9]
        = scsi_info.buffer[10] = scsi_info.buffer[11] = 0;
    }
      scsi_info.state = STSCSI_IDLE;
    /* Clear error indicators. Request sense shall do this, and shall not
       return the same error code when called several times. */
    scsi_set_error(scsirv_ok);
    return 18;

  case STSCSI_VERIFY:
    {
      /* Simply read sectors and dropp data.
         We assure this way, that these sectors can be accessed (read). */
      do {
        int r;

        HCC_ASSERT(scsi_info.media_funcs->getstatus != 0);
        r=scsi_info.media_funcs->readsector(scsi_info.media_funcs, scsi_info.buffer, scsi_info.lba);

        if (r != MMC_NO_ERROR)
        {
          /* In case of an error we have to return CHECK CONDITION status
             with the sense key set to MISCOMPARE. */
          scsi_set_error(scsirv_verify_failed);
          break;
        }

        scsi_info.tlength -= BLOCK_SIZE;
        scsi_info.lba++;
      } while(scsi_info.tlength != 0);
      scsi_info.state = STSCSI_IDLE;
    }
    return(0);
  }
  return(0);
}


/*************************************************************
 * Function "scsi_has_error".
 * In:
 *   N/A
 * Return values:
 *   0  -no error
 *   !0 -error encountered
 * Description:
 *   Returns true if an error occured druing processing the
 *   last command.
 ************************************************************/
hcc_u8 scsi_has_error(void)
{
  return(scsi_info.sense_data != SK_NO_SENSE);
}

/*************************************************************
 * Function "scsi_get_remaining".
 * In:
 *   N/A
 * Return values:
 *   Number of bytes to be read/write.
 * Description:
 *   Returns the number of bytes to be read or write by the
 *   current or last command.
 ************************************************************/
hcc_u32 scsi_get_remaining(void)
{
  return(scsi_info.tlength);
}

/*************************************************************
 * Function "scsi_new_buffer".
 * In:
 *   N/A
 * Return values:
 *   N/A
 * Description:
 *   Will change the buffer length and address. Call it a
 *   call to scsi_process().
 ************************************************************/
void scsi_new_buffer(hcc_u8 lun, hcc_u8 *buf, hcc_u32 buf_length)
{
  scsi_info.blength=buf_length;
  scsi_info.buffer=buf;
}
/****************************** END OF FILE **********************************/
