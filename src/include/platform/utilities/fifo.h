/* fifo.h - Dataton 3397 Transponder
**
**
**  I/O request fifo (IORF).
**
**
** Created 09-09-24	Kalle
**
** (C) Copyright Dataton AB 2009, All Rights Reserved
**
*/

#ifndef _fifo_
#define _fifo_


typedef struct _IORequestHeader
{
    struct _IORequestHeader *next; 
    struct _IORequestHeader *prev; 
} IORequestHeader;


typedef struct _IORequestFifo
{
    IORequestHeader *first, *last;
} IORequestFifo;


void IORF_Initialize(IORequestFifo *fifo);
void IORF_Put(IORequestFifo *fifo, IORequestHeader *r);       

IORequestHeader *IORF_Get(IORequestFifo *fifo);

Boolean IORF_IsEmpty(IORequestFifo *fifo);

#endif  // _fifo_

