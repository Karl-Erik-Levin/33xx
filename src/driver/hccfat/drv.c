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
 * Vaci ut 110
 * Hungary
 *
 * Tel:  +36 (1) 450 1302
 * Fax:  +36 (1) 450 1303
 * http: www.hcc-embedded.com
 * email: info@hcc-embedded.com
 *
 ***************************************************************************/
#include "hcc/target.h"
#include "driver/hccfat/drv.h"
#include "atmel/AT91SAM7S256.h"
#ifdef PICKUPHW
  #include "driver\hwBoard.h"
#endif
/****************************************************************************
 *
 * port defintions
 *
 ***************************************************************************/
#define CLK 48000000UL		/* clock speed in Hz */

#if 0
#define SPIPORT	        P0_DATA
/****************************************************************************
 *
 * Register definitions
 *
 ***************************************************************************/
typedef volatile unsigned int AT91_REG;

#define AT91C_PMC_PCER  ((AT91_REG *) 	0xFFFFFC10) /* (PMC) Peripheral Clock Enable Register */
#ifdef OLIMEXSAM7EX
#define AT91C_ID_SPI    ((unsigned int)  4) /* SPI0*/
#else
#define AT91C_ID_SPI    ((unsigned int)  5)
#endif
#define AT91C_ID_PIOA   ((unsigned int)  2) /* Parallel IO Controller */
#define AT91C_ID_PIOB   ((unsigned int)  3) /* Parallel IO Controller */

/****** SPI registers */
/* (SPI) Interrupt Enable Register */
#define AT91C_SPI_IER   ((AT91_REG *) 	0xFFFE0014)

/* (SPI) Status Register */
#define AT91C_SPI_SR    ((AT91_REG *) 	0xFFFE0010)
#define AT91C_SPI_RDRF        ((unsigned int) 0x1 <<  0) /* (SPI) Receive Data Register Full */
#define AT91C_SPI_TDRE        ((unsigned int) 0x1 <<  1) /* (SPI) Transmit Data Register Empty */
#define AT91C_SPI_MODF        ((unsigned int) 0x1 <<  2) /* (SPI) Mode Fault Error */
#define AT91C_SPI_OVRES       ((unsigned int) 0x1 <<  3) /* (SPI) Overrun Error Status */
#define AT91C_SPI_ENDRX       ((unsigned int) 0x1 <<  4) /* (SPI) End of Receiver Transfer */
#define AT91C_SPI_ENDTX       ((unsigned int) 0x1 <<  5) /* (SPI) End of Receiver Transfer */
#define AT91C_SPI_RXBUFF      ((unsigned int) 0x1 <<  6) /* (SPI) RXBUFF Interrupt */
#define AT91C_SPI_TXBUFE      ((unsigned int) 0x1 <<  7) /* (SPI) TXBUFE Interrupt */
#define AT91C_SPI_NSSR        ((unsigned int) 0x1 <<  8) /* (SPI) NSSR Interrupt */
#define AT91C_SPI_TXEMPTY     ((unsigned int) 0x1 <<  9) /* (SPI) TXEMPTY Interrupt */
#define AT91C_SPI_SPIENS      ((unsigned int) 0x1 << 16) /* (SPI) Enable Status */

/* (SPI) Interrupt Disable Register */
#define AT91C_SPI_IDR   ((AT91_REG *) 	0xFFFE0018)

/* (SPI) Control Register */
#define AT91C_SPI_CR    ((AT91_REG *) 	0xFFFE0000)
#define AT91C_SPI_SPIEN       ((unsigned int) 0x1 <<  0) /* (SPI) SPI Enable */
#define AT91C_SPI_SPIDIS      ((unsigned int) 0x1 <<  1) /* (SPI) SPI Disable */
#define AT91C_SPI_SWRST       ((unsigned int) 0x1 <<  7) /* (SPI) SPI Software reset */
#define AT91C_SPI_LASTXFER    ((unsigned int) 0x1 << 24) /* (SPI) SPI Last Transfer */

/* (SPI) Mode Register */
#define AT91C_SPI_MR    ((AT91_REG *) 	0xFFFE0004)
#define AT91C_SPI_MSTR        ((unsigned int) 0x1 <<  0) /* (SPI) Master/Slave Mode */
#define AT91C_SPI_PS          ((unsigned int) 0x1 <<  1) /* (SPI) Peripheral Select */
#define 	AT91C_SPI_PS_FIXED                ((unsigned int) 0x0 <<  1) /* (SPI) Fixed Peripheral Select */
#define 	AT91C_SPI_PS_VARIABLE             ((unsigned int) 0x1 <<  1) /* (SPI) Variable Peripheral Select */
#define AT91C_SPI_PCSDEC      ((unsigned int) 0x1 <<  2) /* (SPI) Chip Select Decode */
#define AT91C_SPI_FDIV        ((unsigned int) 0x1 <<  3) /* (SPI) Clock Selection */
#define AT91C_SPI_MODFDIS     ((unsigned int) 0x1 <<  4) /* (SPI) Mode Fault Detection */
#define AT91C_SPI_LLB         ((unsigned int) 0x1 <<  7) /* (SPI) Clock Selection */
#define AT91C_SPI_PCS         ((unsigned int) 0xF << 16) /* (SPI) Peripheral Chip Select */
#define AT91C_SPI_DLYBCS      ((unsigned int) 0xFF << 24) /* (SPI) Delay Between Chip Selects */

/* (SPI) Interrupt Mask Register */
#define AT91C_SPI_IMR   ((AT91_REG *) 	0xFFFE001C)

/* (SPI) Transmit Data Register */
#define AT91C_SPI_TDR   ((AT91_REG *) 	0xFFFE000C)
#define AT91C_SPI_TD          ((unsigned int) 0xFFFF <<  0) /* (SPI) Transmit Data */
#define AT91C_SPI_TPCS        ((unsigned int) 0xF << 16) /* (SPI) Peripheral Chip Select Status */

/* (SPI) Receive Data Register */
#define AT91C_SPI_RDR   ((AT91_REG *) 	0xFFFE0008)
#define AT91C_SPI_RD          ((unsigned int) 0xFFFF <<  0) /* (SPI) Receive Data */
#define AT91C_SPI_RPCS        ((unsigned int) 0xF << 16) /* (SPI) Peripheral Chip Select Status */

/* (SPI) Chip Select Register */
#define AT91C_SPI_CSR   ((AT91_REG *) 	0xFFFE0030)
#define AT91C_SPI_CPOL        ((unsigned int) 0x1 <<  0) /* (SPI) Clock Polarity */
#define AT91C_SPI_NCPHA       ((unsigned int) 0x1 <<  1) /* (SPI) Clock Phase */
#define AT91C_SPI_CSAAT       ((unsigned int) 0x1 <<  3) /* (SPI) Chip Select Active After Transfer */
#define AT91C_SPI_BITS        ((unsigned int) 0xF <<  4) /* (SPI) Bits Per Transfer */
#define 	AT91C_SPI_BITS_8                    ((unsigned int) 0x0 <<  4) /* (SPI) 8 Bits Per transfer */
#define 	AT91C_SPI_BITS_9                    ((unsigned int) 0x1 <<  4) /* (SPI) 9 Bits Per transfer */
#define 	AT91C_SPI_BITS_10                   ((unsigned int) 0x2 <<  4) /* (SPI) 10 Bits Per transfer */
#define 	AT91C_SPI_BITS_11                   ((unsigned int) 0x3 <<  4) /* (SPI) 11 Bits Per transfer */
#define 	AT91C_SPI_BITS_12                   ((unsigned int) 0x4 <<  4) /* (SPI) 12 Bits Per transfer */
#define 	AT91C_SPI_BITS_13                   ((unsigned int) 0x5 <<  4) /* (SPI) 13 Bits Per transfer */
#define 	AT91C_SPI_BITS_14                   ((unsigned int) 0x6 <<  4) /* (SPI) 14 Bits Per transfer */
#define 	AT91C_SPI_BITS_15                   ((unsigned int) 0x7 <<  4) /* (SPI) 15 Bits Per transfer */
#define 	AT91C_SPI_BITS_16                   ((unsigned int) 0x8 <<  4) /* (SPI) 16 Bits Per transfer */
#define AT91C_SPI_SCBR        ((unsigned int) 0xFF <<  8) /* (SPI) Serial Clock Baud Rate */
#define AT91C_SPI_DLYBS       ((unsigned int) 0xFF << 16) /* (SPI) Serial Clock Baud Rate */
#define AT91C_SPI_DLYBCT      ((unsigned int) 0xFF << 24) /* (SPI) Delay Between Consecutive Transfers */

/****** SPI DMA related definitions */
#define AT91C_SPI_PTCR  ((AT91_REG *) 	0xFFFE0120) /* (PDC_SPI) PDC Transfer Control Register */
#define AT91C_SPI_TPR   ((AT91_REG *) 	0xFFFE0108) /* (PDC_SPI) Transmit Pointer Register */
#define AT91C_SPI_TCR   ((AT91_REG *) 	0xFFFE010C) /* (PDC_SPI) Transmit Counter Register */
#define AT91C_SPI_RCR   ((AT91_REG *) 	0xFFFE0104) /* (PDC_SPI) Receive Counter Register */
#define AT91C_SPI_PTSR  ((AT91_REG *) 	0xFFFE0124) /* (PDC_SPI) PDC Transfer Status Register */
#define AT91C_SPI_RNPR  ((AT91_REG *) 	0xFFFE0110) /* (PDC_SPI) Receive Next Pointer Register */
#define AT91C_SPI_RPR   ((AT91_REG *) 	0xFFFE0100) /* (PDC_SPI) Receive Pointer Register */
#define AT91C_SPI_TNCR  ((AT91_REG *) 	0xFFFE011C) /* (PDC_SPI) Transmit Next Counter Register */
#define AT91C_SPI_RNCR  ((AT91_REG *) 	0xFFFE0114) /* (PDC_SPI) Receive Next Counter Register */
#define AT91C_SPI_TNPR  ((AT91_REG *) 	0xFFFE0118) /* (PDC_SPI) Transmit Next Pointer Register */

#define AT91C_PDC_RXTEN       ((unsigned int) 0x1 <<  0) /* (PDC) Receiver Transfer Enable */
#define AT91C_PDC_RXTDIS      ((unsigned int) 0x1 <<  1) /* (PDC) Receiver Transfer Disable */
#define AT91C_PDC_TXTEN       ((unsigned int) 0x1 <<  8) /* (PDC) Transmitter Transfer Enable */
#define AT91C_PDC_TXTDIS      ((unsigned int) 0x1 <<  9) /* (PDC) Transmitter Transfer Disable */


/****** PORT A registers */
#define AT91C_PIOA_ODR  ((AT91_REG *) 	0xFFFFF414) /* (PIOA) Output Disable Registerr */
#define AT91C_PIOA_SODR ((AT91_REG *) 	0xFFFFF430) /* (PIOA) Set Output Data Register */
#define AT91C_PIOA_ISR  ((AT91_REG *) 	0xFFFFF44C) /* (PIOA) Interrupt Status Register */
#define AT91C_PIOA_ABSR ((AT91_REG *) 	0xFFFFF478) /* (PIOA) AB Select Status Register */
#define AT91C_PIOA_IER  ((AT91_REG *) 	0xFFFFF440) /* (PIOA) Interrupt Enable Register */
#define AT91C_PIOA_PPUDR ((AT91_REG *) 	0xFFFFF460) /* (PIOA) Pull-up Disable Register */
#define AT91C_PIOA_IMR  ((AT91_REG *) 	0xFFFFF448) /* (PIOA) Interrupt Mask Register */
#define AT91C_PIOA_PER  ((AT91_REG *) 	0xFFFFF400) /* (PIOA) PIO Enable Register */
#define AT91C_PIOA_IFDR ((AT91_REG *) 	0xFFFFF424) /* (PIOA) Input Filter Disable Register */
#define AT91C_PIOA_OWDR ((AT91_REG *) 	0xFFFFF4A4) /* (PIOA) Output Write Disable Register */
#define AT91C_PIOA_MDSR ((AT91_REG *) 	0xFFFFF458) /* (PIOA) Multi-driver Status Register */
#define AT91C_PIOA_IDR  ((AT91_REG *) 	0xFFFFF444) /* (PIOA) Interrupt Disable Register */
#define AT91C_PIOA_ODSR ((AT91_REG *) 	0xFFFFF438) /* (PIOA) Output Data Status Register */
#define AT91C_PIOA_PPUSR ((AT91_REG *) 	0xFFFFF468) /* (PIOA) Pull-up Status Register */
#define AT91C_PIOA_OWSR ((AT91_REG *) 	0xFFFFF4A8) /* (PIOA) Output Write Status Register */
#define AT91C_PIOA_BSR  ((AT91_REG *) 	0xFFFFF474) /* (PIOA) Select B Register */
#define AT91C_PIOA_OWER ((AT91_REG *) 	0xFFFFF4A0) /* (PIOA) Output Write Enable Register */
#define AT91C_PIOA_IFER ((AT91_REG *) 	0xFFFFF420) /* (PIOA) Input Filter Enable Register */
#define AT91C_PIOA_PDSR ((AT91_REG *) 	0xFFFFF43C) /* (PIOA) Pin Data Status Register */
#define AT91C_PIOA_PPUER ((AT91_REG *) 	0xFFFFF464) /* (PIOA) Pull-up Enable Register */
#define AT91C_PIOA_OSR  ((AT91_REG *) 	0xFFFFF418) /* (PIOA) Output Status Register */
#define AT91C_PIOA_ASR  ((AT91_REG *) 	0xFFFFF470) /* (PIOA) Select A Register */
#define AT91C_PIOA_MDDR ((AT91_REG *) 	0xFFFFF454) /* (PIOA) Multi-driver Disable Register */
#define AT91C_PIOA_CODR ((AT91_REG *) 	0xFFFFF434) /* (PIOA) Clear Output Data Register */
#define AT91C_PIOA_MDER ((AT91_REG *) 	0xFFFFF450) /* (PIOA) Multi-driver Enable Register */
#define AT91C_PIOA_PDR  ((AT91_REG *) 	0xFFFFF404) /* (PIOA) PIO Disable Register */
#define AT91C_PIOA_IFSR ((AT91_REG *) 	0xFFFFF428) /* (PIOA) Input Filter Status Register */
#define AT91C_PIOA_OER  ((AT91_REG *) 	0xFFFFF410) /* (PIOA) Output Enable Register */
#define AT91C_PIOA_PSR  ((AT91_REG *) 	0xFFFFF408) /* (PIOA) PIO Status Register */

/****** PORT B registers */
#define AT91C_PIOB_OWSR ((AT91_REG *) 	0xFFFFF6A8) /* (PIOB) Output Write Status Register */
#define AT91C_PIOB_PPUSR ((AT91_REG *) 	0xFFFFF668) /* (PIOB) Pull-up Status Register */
#define AT91C_PIOB_PPUDR ((AT91_REG *) 	0xFFFFF660) /* (PIOB) Pull-up Disable Register */
#define AT91C_PIOB_MDSR ((AT91_REG *) 	0xFFFFF658) /* (PIOB) Multi-driver Status Register */
#define AT91C_PIOB_MDER ((AT91_REG *) 	0xFFFFF650) /* (PIOB) Multi-driver Enable Register */
#define AT91C_PIOB_IMR  ((AT91_REG *) 	0xFFFFF648) /* (PIOB) Interrupt Mask Register */
#define AT91C_PIOB_OSR  ((AT91_REG *) 	0xFFFFF618) /* (PIOB) Output Status Register */
#define AT91C_PIOB_OER  ((AT91_REG *) 	0xFFFFF610) /* (PIOB) Output Enable Register */
#define AT91C_PIOB_PSR  ((AT91_REG *) 	0xFFFFF608) /* (PIOB) PIO Status Register */
#define AT91C_PIOB_PER  ((AT91_REG *) 	0xFFFFF600) /* (PIOB) PIO Enable Register */
#define AT91C_PIOB_BSR  ((AT91_REG *) 	0xFFFFF674) /* (PIOB) Select B Register */
#define AT91C_PIOB_PPUER ((AT91_REG *) 	0xFFFFF664) /* (PIOB) Pull-up Enable Register */
#define AT91C_PIOB_IFDR ((AT91_REG *) 	0xFFFFF624) /* (PIOB) Input Filter Disable Register */
#define AT91C_PIOB_ODR  ((AT91_REG *) 	0xFFFFF614) /* (PIOB) Output Disable Registerr */
#define AT91C_PIOB_ABSR ((AT91_REG *) 	0xFFFFF678) /* (PIOB) AB Select Status Register */
#define AT91C_PIOB_ASR  ((AT91_REG *) 	0xFFFFF670) /* (PIOB) Select A Register */
#define AT91C_PIOB_IFER ((AT91_REG *) 	0xFFFFF620) /* (PIOB) Input Filter Enable Register */
#define AT91C_PIOB_IFSR ((AT91_REG *) 	0xFFFFF628) /* (PIOB) Input Filter Status Register */
#define AT91C_PIOB_SODR ((AT91_REG *) 	0xFFFFF630) /* (PIOB) Set Output Data Register */
#define AT91C_PIOB_ODSR ((AT91_REG *) 	0xFFFFF638) /* (PIOB) Output Data Status Register */
#define AT91C_PIOB_CODR ((AT91_REG *) 	0xFFFFF634) /* (PIOB) Clear Output Data Register */
#define AT91C_PIOB_PDSR ((AT91_REG *) 	0xFFFFF63C) /* (PIOB) Pin Data Status Register */
#define AT91C_PIOB_OWER ((AT91_REG *) 	0xFFFFF6A0) /* (PIOB) Output Write Enable Register */
#define AT91C_PIOB_IER  ((AT91_REG *) 	0xFFFFF640) /* (PIOB) Interrupt Enable Register */
#define AT91C_PIOB_OWDR ((AT91_REG *) 	0xFFFFF6A4) /* (PIOB) Output Write Disable Register */
#define AT91C_PIOB_MDDR ((AT91_REG *) 	0xFFFFF654) /* (PIOB) Multi-driver Disable Register */
#define AT91C_PIOB_ISR  ((AT91_REG *) 	0xFFFFF64C) /* (PIOB) Interrupt Status Register */
#define AT91C_PIOB_IDR  ((AT91_REG *) 	0xFFFFF644) /* (PIOB) Interrupt Disable Register */
#define AT91C_PIOB_PDR  ((AT91_REG *) 	0xFFFFF604) /* (PIOB) PIO Disable Register */
#endif

/************************************ SPI routines *****************************************/
/* transmit 1 byte */
void spi_tx1(unsigned char data8)
{
  *AT91C_SPI_TDR=data8;
  while((*AT91C_SPI_SR & AT91C_SPI_TXEMPTY) ==0)
    ;
}

void spi_tx2(unsigned short data16)
{
  *AT91C_SPI_TDR=data16>>8;
  while((*AT91C_SPI_SR & AT91C_SPI_TDRE) ==0)
    ;
  *AT91C_SPI_TDR=data16 & 0xff;
  while((*AT91C_SPI_SR & AT91C_SPI_TXEMPTY) ==0)
    ;
}

void spi_tx4( unsigned long data32)
{
  int x;

  for(x=0; x<4; x++)
  {
    *AT91C_SPI_TDR=data32>>((3-x)<<3);
    while((*AT91C_SPI_SR & AT91C_SPI_TDRE) ==0)
      ;
  }
  while((*AT91C_SPI_SR & AT91C_SPI_TXEMPTY) ==0)
    ;
}

void spi_tx512 (unsigned char *buf)
{
  /* Transmit using DMA */

    /* Disable RX and TX dma channel of SPI. */
  *AT91C_SPI_PTCR = AT91C_PDC_RXTDIS | AT91C_PDC_TXTDIS;

    /* set the second buffer to empty. */
  *AT91C_SPI_TNCR = 0;
    /* set the first buffer to be buf.*/
  *AT91C_SPI_TPR = (unsigned int)buf;
  *AT91C_SPI_TCR = 512;

  /* Start DMA. */
  *AT91C_SPI_PTCR = AT91C_PDC_TXTEN;
  /* Wait for DMA to finish. */
  while((*AT91C_SPI_SR & AT91C_SPI_TXBUFE) == 0)
    ;

  /* Wait till all characters are transmitted. */
  while((*AT91C_SPI_SR & AT91C_SPI_TXEMPTY) ==0)
    ;
  /* Clear receive buffer. */
  while(*AT91C_SPI_SR & AT91C_SPI_RDRF)
  {
    volatile unsigned char a=*AT91C_SPI_RDR;
  }
}

unsigned char spi_rx1 (void)
{
  *AT91C_SPI_TDR=0xff;
  while((*AT91C_SPI_SR & AT91C_SPI_TXEMPTY) ==0)
    ;
  while((*AT91C_SPI_SR & AT91C_SPI_RDRF) ==0)
    ;
  return(*AT91C_SPI_RDR);
}

void spi_rx512 (unsigned char *buf)
{
  int x;
  /***** Configure DMA to receive 512 bytes. */
  /* Disable RX and TX dma channel of SPI. */
  *AT91C_SPI_PTCR = AT91C_PDC_RXTDIS | AT91C_PDC_TXTDIS;

    /* set the first buffer to be buf.*/
  *AT91C_SPI_RPR = (unsigned int)buf;
  *AT91C_SPI_RCR = 512;
    /* set the second buffer to empty. */
  *AT91C_SPI_RNCR = 0;

  /* Receive using the DMA controller, and transmit using the MCU. */

  /* Enable receive channel. */
  *AT91C_SPI_PTCR = AT91C_PDC_RXTEN;

  for(x=0; x<512; x++)
  {
    *AT91C_SPI_TDR=0xffff;
    while((*AT91C_SPI_SR & AT91C_SPI_TDRE) ==0)
      ;
  }

  /* Wait for DMA to finish. */
  while((*AT91C_SPI_SR & AT91C_SPI_RXBUFF) == 0)
    ;
}

void spi_cs_lo (void)
{
#ifdef UC_DRIVE
  *AT91C_PIOA_CODR = 1u<<20;
#elif defined PICKUPHW
  *AT91C_PIOA_CODR = SDCARD_SELECT;
#elif defined AT91SAM7SEK
  *AT91C_PIOA_CODR = 1u<<10;
#elif defined OLIMEXSAM7EX
  *AT91C_PIOA_CODR = 1u<<13;  
#else
  *AT91C_PIOA_CODR = 1u<<11;
#endif
}

void spi_cs_hi (void)
{
#ifdef UC_DRIVE
  *AT91C_PIOA_SODR = 1u<<20;
#elif defined PICKUPHW
  *AT91C_PIOA_SODR = SDCARD_SELECT;
#elif defined AT91SAM7SEK
  *AT91C_PIOA_SODR = 1u<<10;
#elif defined OLIMEXSAM7EX
  *AT91C_PIOA_SODR = 1u<<13;
#else
  *AT91C_PIOA_SODR = 1u<<11;
#endif
}

int spi_init (void)
{
  /* Configure PIO to enable SPI pins. */
#ifndef OLIMEXSAM7EX
  /* Disable pull-up on: NPCS0, MISO, MOSI, SCLK. */  
  *AT91C_PIOA_PPUDR = (1<<12) | (1<<13) | (1<<14);
    /* Select SPI function, for SPI pins. */
  *AT91C_PIOA_ASR = (1<<12) | (1<<13) | (1<<14);
    /* Disable GPIO on SPI pins. */
  *AT91C_PIOA_PDR = (1<<12) | (1<<13) | (1<<14);
  spi_cs_hi();
#endif

#ifdef UC_DRIVE
  /* Enable peripheral clock */
  *AT91C_PMC_PCER =  (1u<<AT91C_ID_PIOA);
  /* Set PA20 to output (manual chip select). */
  *AT91C_PIOA_OER = 1<<20;
#elif defined OLIMEXSAM7PXXX
  /* Enable peripheral clock */
  *AT91C_PMC_PCER =  (1u<<AT91C_ID_PIOA);
  /* Configure WR pin and CD pins. */
  *AT91C_PIOA_PPUDR = (1<<16) | (1<<15);
  *AT91C_PIOA_PER = (1<<16) | (1<<15);
  /* Configure CS pin. */
  *AT91C_PIOA_OER = 1<<11;
  *AT91C_PIOA_PPUDR = 1<<11;
  *AT91C_PIOA_PER = (1<<11);
#elif defined OLIMEXSAM7EX
  /* Disable pull-up on: MISO, MOSI, SCLK. */  
  *AT91C_PIOA_PPUDR = (1<<16) | (1<<17) | (1<<18);
    /* Select SPI function, for SPI pins. */
  *AT91C_PIOA_ASR = (1<<16) | (1<<17) | (1<<18);
    /* Disable GPIO on SPI pins. */
  *AT91C_PIOA_PDR = (1<<16) | (1<<17) | (1<<18);
  spi_cs_hi();
  
  /* Enable peripheral clock */
  *AT91C_PMC_PCER =  (1u<<AT91C_ID_PIOA);
               //      | (1u << AT91C_ID_PIOB);
  /* Configure WP pin and CD pins. */
  *AT91C_PIOB_PPUDR = (1u<<22) | (1u<<23);
  *AT91C_PIOA_PER = (1u<<22) | (1u<<23);
  /* Configure CS pin. */
  *AT91C_PIOA_PPUDR = 1u<<13;
  *AT91C_PIOA_OER = 1u<<13;
  *AT91C_PIOA_PER = 1u<<13;  
#elif defined PICKUPHW
  *AT91C_PIOA_OER = SDCARD_SELECT;
  *AT91C_PIOA_PPUDR = SDCARD_SELECT;
#elif defined AT91SAM7SEK
  *AT91C_PIOA_OER = 1<<10;
  *AT91C_PIOA_PPUDR = 1<<10;
#else
  /* Configure CS pin. */
  *AT91C_PIOA_OER = 1<<11;
  *AT91C_PIOA_PPUDR = 1<<11;
#endif
  /* Configure interrupts in AIC. */
    /* not needed */
  /* Configure spi */
    /* enable SPI clock */
  *AT91C_PMC_PCER = (1u<<AT91C_ID_SPI);

    /* Reset the SPI. */
  *AT91C_SPI_CR = AT91C_SPI_SWRST;
    /* Select master mode, fixed CS to device 0, no CS decode, using MCLK,
       mode fault decetion on, LLB disabled, DLYBCS=6 clk. */
    /* Note: spi has a BUG (see errata). Do not use AT91C_SPI_FDIV=1. */
  *AT91C_SPI_MR = AT91C_SPI_MSTR | AT91C_SPI_MODFDIS;
    /* Disable all interrupts.*/
  *AT91C_SPI_IDR = -1UL;
  /* Sample at the starting edge. */
  AT91C_SPI_CSR[0] = AT91C_SPI_NCPHA;

    /* Start *AT91C_*/
  *AT91C_SPI_CR = AT91C_SPI_SPIEN;
    /* Wait till SPI starts. */
  while((*AT91C_SPI_SR & AT91C_SPI_SPIENS) == 0)
   ;

  /* Set baudrate to 1k. */
  spi_set_baudrate(1000);
  return(0);
}

void spi_set_baudrate (unsigned long br)
{
  unsigned long val=CLK/br;
  if ((CLK % br)> 0)
  {
    val++;
  }
  if (val > 0xff)
  {
    val=0xff;
  }
  if (val == 0)
  {
    val=1;
  }

  AT91C_SPI_CSR[0] &= ~AT91C_SPI_SCBR;
  AT91C_SPI_CSR[0] |= AT91C_SPI_SCBR & (val<<8);
}

unsigned long spi_get_baudrate (void)
{
  unsigned long val = CLK/((AT91C_SPI_CSR[0] & AT91C_SPI_SCBR) >>8);
  return(val);
}

/*
** Get Card Detect state
** RETURN: 0 - card is removed
**         1 - card present
*/
int get_cd (void)
{
#ifdef UC_DRIVE
  return((*AT91C_PIOA_ODSR & (1<<18))==0);
#elif defined OLIMEXSAM7PXXX
  return((*AT91C_PIOA_PDSR & (1<<15))==0);
#elif defined OLIMEXSAM7EX
  return((*AT91C_PIOB_PDSR & (1u<<23))==0);
#else
  /* Return card present. */
  return(1);
#endif
}

/*
** Get Write Protect state
** RETURN: 0 - not protected
**         1 - write protected
*/
int get_wp (void)
{
#ifdef UC_DIRVE
  return(*AT91C_PIOA_ODSR & (1u<<15));
#elif defined OLIMEXSAM7EX
  return((*AT91C_PIOB_PDSR & (1u<<22)) != 0);
#elif defined OLIMEXSAM7PXXX
  return((*AT91C_PIOA_PDSR & (1u<<16))!=0);
#else
  /* Return not write protected. */
  return(0);
#endif
}

/* Dataton extention */
void
spi_sleep(void)
{
	*AT91C_PMC_PCDR = (1u<<AT91C_ID_SPI);
}

void
spi_wake(void)
{
	*AT91C_PMC_PCER = (1u<<AT91C_ID_SPI);
}

