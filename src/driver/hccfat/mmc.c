/****************************************************************************
 *
 *            Copyright (c) 2003-2008 by HCC Embedded
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
 * Vaci Ut 110
 * Hungary
 *
 * Tel:  +36 (1) 450 1302
 * Fax:  +36 (1) 450 1303
 * http: www.hcc-embedded.com
 * email: info@hcc-embedded.com
 *
 ***************************************************************************/

#include "driver/hccfat/mmc.h"

#define tNCR	(8+1)			/* command to response time */
#define tNCX	(8+1)			/* response to CSD time */
#define tNRC	(1+1)			/* response to next cmd time */
#define tNWR	(1+1)			/* response to data (write) */
#define tNBR	(1+1)			/* stop transition to busy time */
static unsigned long tNAC=0;		/* READ: response to data time (varies upon card) */
static unsigned long tBUSY=0;		/* WRITE: maximum write time */


#define CMD_GO_IDLE_STATE			0
#define CMD_SEND_OP_COND			1
#define CMD_SEND_IF_COND			8
#define CMD_SEND_CSD				9
#define CMD_STOP_TRANS				12
#define CMD_SEND_STATUS				13
#define CMD_SET_BLOCKLEN			16
#define CMD_READ_SINGLE_BLOCK		17
#define CMD_READ_MULTIPLE_BLOCK		18
#define CMD_SET_BLOCK_COUNT			23
#define CMD_WRITE_SINGLE_BLOCK		24
#define CMD_WRITE_MULTIPLE_BLOCK	25
#define CMD_APP_CMD					55
#define CMD_READ_OCR				58
#define CMD_CRC_ON_OFF				59

#define ACMD_SEND_OP_COND			41


#define BIT_IDLE_STATE			1
#define BIT_ILLEGAL_COMMAND		4



/* redefine CS high functions, add 8 bits after CS high */
#define SPI_CS_HI { spi_cs_hi(); spi_tx1(0xff); }
#define SPI_CS_LO spi_cs_lo()


static t_mmc_dsc mmc_dsc;		/* current MMC/SD card descriptor */
static F_DRIVER mmc_drv;

#if USE_CRC || SDHC_ENABLE
#if CRC_ROM_TABLE
const unsigned short CRCtbl[256] = {
  0x0000, 0x1021, 0x2042, 0x3063, 0x4084, 0x50a5, 0x60c6, 0x70e7,
  0x8108, 0x9129, 0xa14a, 0xb16b, 0xc18c, 0xd1ad, 0xe1ce, 0xf1ef,
  0x1231, 0x0210, 0x3273, 0x2252, 0x52b5, 0x4294, 0x72f7, 0x62d6,
  0x9339, 0x8318, 0xb37b, 0xa35a, 0xd3bd, 0xc39c, 0xf3ff, 0xe3de,
  0x2462, 0x3443, 0x0420, 0x1401, 0x64e6, 0x74c7, 0x44a4, 0x5485,
  0xa56a, 0xb54b, 0x8528, 0x9509, 0xe5ee, 0xf5cf, 0xc5ac, 0xd58d,
  0x3653, 0x2672, 0x1611, 0x0630, 0x76d7, 0x66f6, 0x5695, 0x46b4,
  0xb75b, 0xa77a, 0x9719, 0x8738, 0xf7df, 0xe7fe, 0xd79d, 0xc7bc,
  0x48c4, 0x58e5, 0x6886, 0x78a7, 0x0840, 0x1861, 0x2802, 0x3823,
  0xc9cc, 0xd9ed, 0xe98e, 0xf9af, 0x8948, 0x9969, 0xa90a, 0xb92b,
  0x5af5, 0x4ad4, 0x7ab7, 0x6a96, 0x1a71, 0x0a50, 0x3a33, 0x2a12,
  0xdbfd, 0xcbdc, 0xfbbf, 0xeb9e, 0x9b79, 0x8b58, 0xbb3b, 0xab1a,
  0x6ca6, 0x7c87, 0x4ce4, 0x5cc5, 0x2c22, 0x3c03, 0x0c60, 0x1c41,
  0xedae, 0xfd8f, 0xcdec, 0xddcd, 0xad2a, 0xbd0b, 0x8d68, 0x9d49,
  0x7e97, 0x6eb6, 0x5ed5, 0x4ef4, 0x3e13, 0x2e32, 0x1e51, 0x0e70,
  0xff9f, 0xefbe, 0xdfdd, 0xcffc, 0xbf1b, 0xaf3a, 0x9f59, 0x8f78,
  0x9188, 0x81a9, 0xb1ca, 0xa1eb, 0xd10c, 0xc12d, 0xf14e, 0xe16f,
  0x1080, 0x00a1, 0x30c2, 0x20e3, 0x5004, 0x4025, 0x7046, 0x6067,
  0x83b9, 0x9398, 0xa3fb, 0xb3da, 0xc33d, 0xd31c, 0xe37f, 0xf35e,
  0x02b1, 0x1290, 0x22f3, 0x32d2, 0x4235, 0x5214, 0x6277, 0x7256,
  0xb5ea, 0xa5cb, 0x95a8, 0x8589, 0xf56e, 0xe54f, 0xd52c, 0xc50d,
  0x34e2, 0x24c3, 0x14a0, 0x0481, 0x7466, 0x6447, 0x5424, 0x4405,
  0xa7db, 0xb7fa, 0x8799, 0x97b8, 0xe75f, 0xf77e, 0xc71d, 0xd73c,
  0x26d3, 0x36f2, 0x0691, 0x16b0, 0x6657, 0x7676, 0x4615, 0x5634,
  0xd94c, 0xc96d, 0xf90e, 0xe92f, 0x99c8, 0x89e9, 0xb98a, 0xa9ab,
  0x5844, 0x4865, 0x7806, 0x6827, 0x18c0, 0x08e1, 0x3882, 0x28a3,
  0xcb7d, 0xdb5c, 0xeb3f, 0xfb1e, 0x8bf9, 0x9bd8, 0xabbb, 0xbb9a,
  0x4a75, 0x5a54, 0x6a37, 0x7a16, 0x0af1, 0x1ad0, 0x2ab3, 0x3a92,
  0xfd2e, 0xed0f, 0xdd6c, 0xcd4d, 0xbdaa, 0xad8b, 0x9de8, 0x8dc9,
  0x7c26, 0x6c07, 0x5c64, 0x4c45, 0x3ca2, 0x2c83, 0x1ce0, 0x0cc1,
  0xef1f, 0xff3e, 0xcf5d, 0xdf7c, 0xaf9b, 0xbfba, 0x8fd9, 0x9ff8,
  0x6e17, 0x7e36, 0x4e55, 0x5e74, 0x2e93, 0x3eb2, 0x0ed1, 0x1ef0
};
#else
/****************************************************************************
 * crc16
 * calculates a CRC value
 ***************************************************************************/
unsigned short CRCtbl[256];
static unsigned long crc16( unsigned long crc, unsigned char c)
{
  int a;
  crc=crc^((unsigned long)c<<8UL);
  for(a=0;a<8;a++)
  {
    if(crc&0x8000) crc=(crc<<1)^0x1021;
              else crc<<=1;
  }
  return crc;
}
#endif
#endif


/****************************************************************************
 *
 * spiWaitStartBit
 *
 * Wait data start bit on spi and send one more clock
 *
 * RETURNS
 *
 * 0 - if received
 * other if time out
 *
 ***************************************************************************/
static int spiWaitStartBit (unsigned long a)
{
   while (a--)
   {
     if (spi_rx1()!=0xff) return 0;
   }
   return 1;
}

/****************************************************************************
 *
 * spiWaitBusy
 *
 * Wait until SPI datain line goes hi
 *
 * RETURNS
 *
 * 0 - if ok
 * other if time out
 *
 ***************************************************************************/
static int spiWaitBusy(void)
{
   unsigned long a=tBUSY;
   while(a--)
   {
     if (spi_rx1()==0xff) return 0;
   }
   return 1;
}

/****************************************************************************
 *
 * mmc_cmd
 *
 * Send command to SPI, it adds startbit and stopbit and calculates crc7 also
 * sent, and Wait response start bit on spi and r1 response
 *
 * INPUTS
 *
 * cmd - command to send
 * data - argument of the command
 *
 * RETURNS
 *
 * r1 if received or 0xff if any error
 *
 ***************************************************************************/
static unsigned char mmc_cmd(unsigned long cmd,unsigned long data)
{
   register int a;
   register unsigned char tmp=0;

   cmd|=0xffffff40;
   spi_tx4(cmd);

#if USE_CRC || SDHC_ENABLE
 #if SDHC_ENABLE
   if (mmc_dsc.use_crc)
 #endif
   {
     for (a=0;a<8;a++)
     {     /* send start and cmd */
       if (cmd&0x80) tmp^=0x40;
       cmd<<=1;
       if (tmp&0x40) tmp=(tmp<<1)^0x09;
                else tmp=tmp<<1;
     }
   }
#endif

   spi_tx4(data);  /* send data */
#if USE_CRC || SDHC_ENABLE
 #if SDHC_ENABLE
   if (mmc_dsc.use_crc)
 #endif
   {
     for (a=0;a<32;a++)
     {   /* send 32bit argument */
       if (data & 0x80000000) tmp^=0x40;
       data<<=1;
       if(tmp&0x40) tmp=(tmp<<1)^0x09;
               else tmp=tmp<<1;
     }

     tmp<<=1;  /* set tmp into correct position */
     tmp|=1;   /* add endbit   */

     spi_tx1(tmp); /* send tmp and endbit */
   }
 #if SDHC_ENABLE
   else
   {
     spi_tx1(0x95);
   }
 #endif
#else
   spi_tx1(0x95);
#endif

   for (a=0;a<tNCR;a++)
   {
     tmp=spi_rx1();
     if (tmp!=0xff) return tmp;
   }

   return 0xff;
}


/****************************************************************************
 *
 * mmc_read
 *
 * Getting a datablock from SD
 *
 * INPUTS
 *
 * driver - driver structure
 * dwaddr - long pointer where to store received data (512bytes)
 * sector - sector number where to start read card.
 * cnt - number of sectors
 *
 * RETURNS
 *
 * 0 - if successful
 * other if any error (crc,timeouts)
 *
 ***************************************************************************/

static int mmc_read(F_DRIVER *driver,unsigned char *dwaddr,unsigned long sector,int cnt)
{
#if USE_CRC || SDHC_ENABLE
   register unsigned short crc;
#endif
   int multi;
   unsigned char r1;

   if (!get_cd()) return MMC_ERR_NOTPLUGGED; /* card missing */

   SPI_CS_LO;

   if (cnt>1) multi=1; else multi=0;

   if (multi && mmc_dsc.bcs)
   {
     r1=mmc_cmd(CMD_SET_BLOCK_COUNT,cnt);
     if (r1)
     {
       SPI_CS_HI;
       if (r1&BIT_IDLE_STATE) mmc_dsc.initok=0;
       return MMC_ERR_CMD;
     }
   }

#if SDHC_ENABLE
   if ((mmc_dsc.cardtype&_T_SDHC)==0) sector<<=9;
#else
   sector<<=9;
#endif
   r1=mmc_cmd(multi?CMD_READ_MULTIPLE_BLOCK:CMD_READ_SINGLE_BLOCK,sector);
   if (r1)
   {
     SPI_CS_HI;
     if (r1&BIT_IDLE_STATE) mmc_dsc.initok=0;
     return MMC_ERR_CMD;
   }

   for (;cnt;cnt--)
   {
      register unsigned long a;
#if USE_CRC || SDHC_ENABLE
      crc=0;
#endif

      for (a=0;a<tNAC && (r1=spi_rx1())==0xff;a++);
      if (r1!=0xfe)
      {
         SPI_CS_HI;
         return MMC_ERR_STARTBIT;
      }

      spi_rx512(dwaddr);
#if USE_CRC || SDHC_ENABLE
 #if SDHC_ENABLE
      if (mmc_dsc.use_crc==0)
      {
        dwaddr+=512;
      }
      else
 #endif
      {
        for (a=0;a<512;a++) crc=(crc<<8)^CRCtbl[((crc>>8)^(*dwaddr++))&0xff];
      }
#else
      dwaddr+=512;
#endif

      {
#if USE_CRC || SDHC_ENABLE
        unsigned short rcrc=spi_rx1();
		rcrc<<=8;
        rcrc|=spi_rx1();
 #if SDHC_ENABLE
        if (mmc_dsc.use_crc)
 #endif
        {
          if (crc!=rcrc)
          {
            SPI_CS_HI;
            return MMC_ERR_CRC;
          }
        }
#else
        (void)spi_rx1(); /* crc lo */
        (void)spi_rx1(); /* crc hi */
#endif
      }
   }

   if (multi && mmc_dsc.bcs==0)
   {
     if (mmc_cmd(CMD_STOP_TRANS,0)) (void)spiWaitBusy();
   }

   SPI_CS_HI;

   return MMC_NO_ERROR;
}


/****************************************************************************
 *
 * mmc_write
 *
 * Write data into SD
 *
 * INPUTS
 *
 * driver - driver structure
 * dwaddr - long pointer where original data is (512bytes)
 * sector - sector number where to store in the card
 * cnt - number of sectors
 *
 * RETURNS
 *
 * 0 - if successful
 * other if any error
 *
 ***************************************************************************/
static int mmc_write(F_DRIVER *driver,unsigned char *dwaddr,unsigned long sector,int cnt)
{
#if USE_CRC || SDHC_ENABLE
   register unsigned short crc;
#endif
   int multi;
   unsigned char r1;

   if (get_cd()==0) return MMC_ERR_NOTPLUGGED;/* card is missing */
   if (get_wp()) return MMC_ERR_WRITEPROTECT; /* write protected!! */

   SPI_CS_LO;

   if (cnt>1) multi=1; else multi=0;

   if (multi && mmc_dsc.bcs)
   {
     r1=mmc_cmd(CMD_SET_BLOCK_COUNT,cnt);
     if (r1)
     {
       SPI_CS_HI;
       if (r1&BIT_IDLE_STATE) mmc_dsc.initok=0;
       return MMC_ERR_CMD;
     }
   }

#if SDHC_ENABLE
   if ((mmc_dsc.cardtype&_T_SDHC)==0) sector<<=9;
#else
   sector<<=9;
#endif
   r1=mmc_cmd(multi?CMD_WRITE_MULTIPLE_BLOCK:CMD_WRITE_SINGLE_BLOCK,sector);
   if (r1)
   {
     SPI_CS_HI;
     if (r1&BIT_IDLE_STATE) mmc_dsc.initok=0;
     return MMC_ERR_CMD;
   }

   for (;cnt;cnt--)
   {
      register unsigned int a;

      if (multi) spi_tx2(0xfffc); else spi_tx2(0xfffe);

      spi_tx512(dwaddr);
#if USE_CRC || SDHC_ENABLE
 #if SDHC_ENABLE
      if (mmc_dsc.use_crc==0)
      {
        dwaddr+=512;
        spi_tx2(0);
      }
      else
 #endif
      {
        crc=0;
        for (a=0;a<512;a++) crc=(crc<<8)^CRCtbl[((crc>>8)^(*dwaddr++))&0xff];
        spi_tx2(crc);
      }
#else
      dwaddr+=512;
      spi_tx2(0);
#endif

      for (a=0;a<tNCR;a++)
      {
        if (spi_rx1()==0xe5) break;
      }
      if (a==tNCR)
      {
        SPI_CS_HI;
        return MMC_ERR_WRITE;
      }

      spi_tx4(0xffffffff);
      if (spiWaitBusy())
      {
         SPI_CS_HI;
         return MMC_ERR_BUSY;
      }
   }

   if (multi)
   {
     if (mmc_dsc.bcs) spi_tx2(0xffff); else spi_tx2(0xfffd);
     spi_tx4(0xffffffff);
     if (spiWaitBusy())
     {
       SPI_CS_HI;
       return MMC_ERR_BUSY;
     }
   }

   SPI_CS_HI;

   return MMC_NO_ERROR;
}

/****************************************************************************
 *
 * mmc_readsector
 *
 * read one sector from the card
 *
 * INPUTS
 *
 * driver - driver structure
 * data - pointer where to store data
 * sector - which sector is needed
 *
 * RETURNS
 *
 * 0 - if successful
 * other if any error
 *
 ***************************************************************************/

int mmc_readsector(F_DRIVER *driver,void *data,unsigned long sector)
{
   return mmc_read(driver,data,sector,1);
}

/****************************************************************************
 * mmc_readmultiplesector
 * Reads multiple sectors
 *
 * INPUT - same as mmc_readsector
 *         cnt - number of sectors to read
 ***************************************************************************/
int mmc_readmultiplesector(F_DRIVER *driver,void *data,unsigned long sector,int cnt)
{
   return mmc_read(driver,data,sector,cnt);
}

/****************************************************************************
 *
 * mmc_writesector
 *
 * write one sector into the card
 *
 * INPUTS
 *
 * driver - driver structure
 * data - pointer where original data is
 * sector - which sector needs to be written
 *
 * RETURNS
 *
 * 0 - if successful
 * other if any error
 *
 ***************************************************************************/
int mmc_writesector(F_DRIVER *driver,void *data,unsigned long sector)
{
   return mmc_write(driver,data,sector,1);
}

/****************************************************************************
 * mmc_writemultiplesector
 * Writes multiple sectors
 *
 * INPUT - same as mmc_writesector
 *         cnt - number of sectors to write
 ***************************************************************************/
int mmc_writemultiplesector(F_DRIVER *driver,void *data,unsigned long sector,int cnt)
{
   return mmc_write(driver,data,sector,cnt);
}

/****************************************************************************
 *
 * _get_csd_bits
 *
 * Gets specified bits from bitspace g_mmc_type.CSD
 * Bits described in MMC specification. (e.g. CSD_STRUCTURE: st=126, len=2)
 *
 * INPUT: st - start bit (max. 127)
 *        len- length of data needed in bits.
 * OUTPUT:decoded value
 *
 ***************************************************************************/
unsigned short _get_csd_bits (unsigned char *src, unsigned char st, unsigned char len)
{
  unsigned long dst;
  unsigned char tmp,t_left,t_act;

  dst=0;

  tmp=127-(st+len-1);
  src+=(tmp>>3);
  tmp&=7;
  t_left=8-tmp;

  t_act=*src++;
  for (;tmp;tmp--) t_act<<=1;
  while (len)
  {
    dst<<=1;
    if (t_left==0)
    {
      t_left=8;
      t_act=*src++;
    }
    if (t_act&0x80) dst|=1;
    t_act<<=1;
    --t_left;
    --len;
  }

  return(dst);
}


/****************************************************************************
 *
 * mmc_getcsd
 *
 * Get Card Specific Data
 *
 * RETURNS
 *
 * 0 - if successful
 * other if any error
 *
 ***************************************************************************/
static const unsigned char m[15]={10,12,13,15,20,25,30,35,40,45,50,55,60,70,80};
static int mmc_getcsd (void)
{
  unsigned char a;
  unsigned char *csd=mmc_dsc.CSD;

  SPI_CS_LO;
  if (mmc_cmd(CMD_SEND_CSD,0))
  {
    SPI_CS_HI;
    return MMC_ERR_CMD; /*r1 any error*/
  }

  if (spiWaitStartBit(tNCX))
  {
    SPI_CS_HI;
    return MMC_ERR_STARTBIT;
  }

  for (a=0;a<16;*(csd+a++)=spi_rx1());
  SPI_CS_HI;

#if USE_CRC || SDHC_ENABLE
 #if SDHC_ENABLE
  if (mmc_dsc.use_crc)
 #endif
  {
     unsigned char b,v,crc=0;

     for (a=0;a<15;a++)
     {
       v=csd[a];
       for (b=0;b<8;b++)
       {
         if (v&0x80) crc^=0x40;
         if (crc&0x40) crc=(crc<<1)^0x09;
                  else crc<<=1;
         v<<=1;
       }
     }
     if ((crc&0x7f)!=(csd[15]>>1)) return MMC_ERR_CRC;
  }
#endif

  mmc_dsc.TRANSPEED=_get_csd_bits(csd,96,8);

  if ((mmc_dsc.cardtype&_T_SDHC)==0)
  {
    mmc_dsc.R_BL_LEN=_get_csd_bits(csd,80,4);
    mmc_dsc.CSIZE=_get_csd_bits(csd,62,12);
    mmc_dsc.CSIZE_M=_get_csd_bits(csd,47,3);
    mmc_dsc.NSAC=_get_csd_bits(csd,104,8);
    mmc_dsc.TAAC=_get_csd_bits(csd,112,8);
    mmc_dsc.R2W=_get_csd_bits(csd,26,3);

    mmc_dsc.number_of_sectors=1UL<<mmc_dsc.CSIZE_M;
    mmc_dsc.number_of_sectors<<=2;
    mmc_dsc.number_of_sectors<<=mmc_dsc.R_BL_LEN;
    mmc_dsc.number_of_sectors*=(mmc_dsc.CSIZE+1);
    mmc_dsc.number_of_sectors>>=9;
  }
  else
  {
    mmc_dsc.number_of_sectors=_get_csd_bits(csd,48,22)*1024;
  }

  {
    int i;
    unsigned long br;

    br=1;
    for (i=mmc_dsc.TRANSPEED&7;i;br*=10,i--);
    br*=m[((mmc_dsc.TRANSPEED>>3)&0xf)-1];
    spi_set_baudrate(br*10000);	/* pass speed in Hz */
    br=spi_get_baudrate();

    if ((mmc_dsc.cardtype&_T_SDHC)==0)
    {
      unsigned long div=100;
      for (i=7-((mmc_dsc.TAAC)&7);i;div*=10,i--);
      div*=10;
      div/=m[(((mmc_dsc.TAAC)>>3)&0xf)-1];
      tNAC=(10*(((br+div-1)/div)+100*(mmc_dsc.NSAC))+1)/8;
      tBUSY=tNAC*(1<<(mmc_dsc.R2W+1));
    }

    if (  (mmc_dsc.cardtype&_T_SDHC)
        || ( (mmc_dsc.cardtype&_T_SDV2) && (tNAC>(br/10/8)) ) )
    {
      tNAC=br/10/8;	/* use fixed 100ms (/8 to get spi cycles) */
      tBUSY=br/4/8;	/* use fixed 250ms */
    }
  }

  return MMC_NO_ERROR;
}




/****************************************************************************
 *
 * mmc_initcard
 *
 * inti Card protocols via spi
 *
 * RETURNS
 *
 * 0 - if successful
 * other if any error
 *
 ***************************************************************************/
static int mmc_initcard(void)
{
   long a;
   long retry;
   int ret;
   unsigned char r1;
   unsigned char t=0;

   mmc_dsc.initok=0;
   mmc_dsc.cardtype=0;
   mmc_dsc.use_crc=1;
   spi_set_baudrate(100000); /* set baudrate here to 100kHz */

   for (a=0; a<200; a++) spi_tx4(0xffffffff); /* wait some */

   for (retry=0;retry<100;retry++)
   {
      SPI_CS_LO;
      if (mmc_cmd(CMD_GO_IDLE_STATE,0)==BIT_IDLE_STATE) break;
      SPI_CS_HI;
   }
   SPI_CS_HI;
   if (retry==100) return MMC_ERR_INIT;

#if SDHC_ENABLE
   SPI_CS_LO;
   r1=mmc_cmd(CMD_SEND_IF_COND,0x1a5);
   a=spi_rx1();
   a<<=8;
   a|=spi_rx1();
   a<<=8;
   a|=spi_rx1();
   a<<=8;
   a|=spi_rx1();
   SPI_CS_HI;
   if (r1==BIT_IDLE_STATE && a==0x1a5) t=_T_SDV2;
#endif

   SPI_CS_LO;
   r1=mmc_cmd(CMD_APP_CMD,0);
   SPI_CS_HI;
   if (r1==BIT_IDLE_STATE)
   {
     SPI_CS_LO;
     r1=mmc_cmd(ACMD_SEND_OP_COND,(t&_T_SDV2)?0x40000000:0);
     SPI_CS_HI;
   }

   if ((t&_T_SDV2)==0)
   {
     if (r1==BIT_IDLE_STATE) t=_T_SD;
                        else t=_T_MMC;
   }

   for (retry=0,r1=BIT_IDLE_STATE;retry<5000 && r1;retry++)
   {
     if (t&_T_MMC)
     {
       SPI_CS_LO;
       r1=mmc_cmd(CMD_SEND_OP_COND,0);
       SPI_CS_HI;
     }
     else
     {
       SPI_CS_LO;
       r1=mmc_cmd(CMD_APP_CMD,0);
       SPI_CS_HI;
       SPI_CS_LO;
       r1=mmc_cmd(ACMD_SEND_OP_COND,a);
       SPI_CS_HI;
     }
   }
   if (retry==5000) return MMC_ERR_INIT;


   /* read OCR */
   SPI_CS_LO;
   if (mmc_cmd(CMD_READ_OCR,0))
   {
     SPI_CS_HI;
     return MMC_ERR_CMD;
   }

   a=spi_rx1();
   a<<=8;
   a|=spi_rx1();
   a<<=8;
   a|=spi_rx1();
   a<<=8;
   a|=spi_rx1();
   SPI_CS_HI;
   if ((a&0x80ff8000)>0x80000000)
   {
     if (a&(1<<7)) t|=_T_LOWVOLTAGE;
#if SDHC_ENABLE
     if (t&_T_SDV2)
     {
       if (a&0x40000000) t|=_T_SDHC;
     }
#endif
   }
   else return MMC_ERR_INIT;
   mmc_dsc.cardtype=t;

   SPI_CS_LO;
   if (mmc_cmd(CMD_SET_BLOCKLEN,512))
   { /* set blk lenght 512 byte */
     SPI_CS_HI;
     return MMC_ERR_CMD;
   }
   SPI_CS_HI;

   SPI_CS_LO;
   if (mmc_cmd(CMD_CRC_ON_OFF,USE_CRC))
   {  /* set crc on/off */
     SPI_CS_HI;
     return MMC_ERR_CMD;
   }
   SPI_CS_HI;
   mmc_dsc.use_crc=USE_CRC;

   ret=mmc_getcsd();
   if (ret) return ret ;

   SPI_CS_LO;
   r1=mmc_cmd(CMD_SET_BLOCK_COUNT,1);
   SPI_CS_HI;
   if (r1==BIT_ILLEGAL_COMMAND) mmc_dsc.bcs=0;
   else if (r1==0) mmc_dsc.bcs=1;
   else return MMC_ERR_CMD;
   mmc_dsc.initok=1;

   return MMC_NO_ERROR;
}

/****************************************************************************
 *
 * mmc_getstatus
 *
 * get status of card, missing or/and removed,changed,writeprotect
 *
 * INPUTS
 *
 * driver - driver structure
 *
 * RETURNS
 *
 * F_ST_xxx code for high level
 *
 ***************************************************************************/
static long mmc_getstatus(F_DRIVER *driver)
{
   long state=0;

   if (get_wp()) state|=F_ST_WRPROTECT;
   if (get_cd()==0)
   {
      mmc_dsc.initok=0;
      state|=F_ST_MISSING;
   }
   else
   {
      if (mmc_dsc.initok)
      {
        unsigned short rc;
        SPI_CS_LO;
        rc=mmc_cmd(CMD_SEND_STATUS,0);
        rc<<=8;
        rc|=spi_rx1();
        SPI_CS_HI;
        if (rc&(BIT_IDLE_STATE<<8))
        {
          state|=F_ST_CHANGED;
          (void)mmc_initcard();
        }
      }
      else
      {
        (void)mmc_initcard();
      }
   }

   if (!mmc_dsc.initok) state|=F_ST_MISSING; /* card is not initialized */

   return state;
}


/****************************************************************************
 *
 * mmc_getphy
 *
 * determinate flash card physicals
 *
 * INPUTS
 *
 * driver - driver structure
 * phy - this structure has to be filled with physical information
 *
 * RETURNS
 *
 * error code or zero if successful
 *
 ***************************************************************************/
static int mmc_getphy(F_DRIVER *driver,F_PHY *phy)
{
   if (mmc_dsc.initok==0) return MMC_ERR_NOTINITIALIZED;
   phy->number_of_cylinders=0;
   phy->sector_per_track=63;
   phy->number_of_heads=255;
   phy->number_of_sectors=mmc_dsc.number_of_sectors;
   phy->media_descriptor=0xf0;

   return MMC_NO_ERROR;
}

/****************************************************************************
 *
 * mmc_release
 *
 * Deletes a previously initialized driver
 *
 * INPUTS
 *
 * driver - driver structure
 *
 ***************************************************************************/

static void mmc_release(F_DRIVER *driver)
{
}

/****************************************************************************
 *
 * mmc_initfunc
 *
 * this init function has to be passed for highlevel to initiate the
 * driver functions
 *
 * INPUTS
 *
 * driver_param - driver parameters
 *
 * RETURNS
 *
 * driver structure pointer
 *
 ***************************************************************************/

F_DRIVER *mmc_initfunc(unsigned long driver_param)
{
#if USE_CRC || SDHC_ENABLE
 #if CRC_ROM_TABLE==0
   int a;
   for (a=0;a<256;a++) CRCtbl[a]=crc16(0,(unsigned char)a);
 #endif
#endif

   (void)_memset(&mmc_dsc,0,sizeof(t_mmc_dsc));
   (void)spi_init();

   (void)_memset(&mmc_drv,0,sizeof(mmc_drv));

   mmc_drv.readsector=mmc_readsector;
   mmc_drv.writesector=mmc_writesector;
   mmc_drv.readmultiplesector=mmc_readmultiplesector;
   mmc_drv.writemultiplesector=mmc_writemultiplesector;
   mmc_drv.getstatus=mmc_getstatus;
   mmc_drv.getphy=mmc_getphy;
   mmc_drv.release=mmc_release;

   return &mmc_drv;
}



