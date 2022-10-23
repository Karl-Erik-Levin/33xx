/* fifo.c - Dataton 3397 Transponder
**
**
**  I/O request fifo (IORF).
**
**
** FredJ beskrivning
**   The I/O request fifo is a fifo that holds free or pending I/O requests. The IORF works just like a
**   normal fifo, data is gotten from the top, and pushed on the back. Operations are on pointers, and
**   the fifo is created using a double linked list, thus the operations can be made in constant time!
**
**   The normal is use that a driver holds one IORF for free and one IORF for pending requests, thus
**   the driver have a limitied max number of I/O requests, but operations are very effective.
**
**
** Created 09-09-24	Kalle
**
** (C) Copyright Dataton AB 2009, All Rights Reserved
**
*/

//---------------------------------INCLUDES---------------------------------------
#include "MFLib\Dataton_Types.h"

#include "platform\utilities\fifo.h"


//--------------------------------FUNCTIONS---------------------------------------
/* Initalize I/O requst fifo */
void
IORF_Initialize(IORequestFifo *fifo)
{
    fifo->first = fifo->last = NULL;
}

/* Get the first/top elelemnt in the IORF */
IORequestHeader *						// NULL if no more requests are available.
IORF_Get(IORequestFifo *fifo)
{
    register IORequestHeader *rh;
    
    if ((rh = fifo->first))
    {
        if (rh->next)
        {
            fifo->first = rh->next;
            fifo->first->prev = NULL;    
        } 
        else 
        {   
            fifo->last = fifo->first = NULL;
        }
        rh->next = rh->prev = NULL;
		
    }
        
    return rh;
}

/* Insert and element at the back/tail of the IORF */
void
IORF_Put(
    IORequestFifo *fifo,
    IORequestHeader *rh
) {    
    if (fifo->last)
    {
        rh->prev = fifo->last;
        rh->next = NULL;   
        rh->prev->next = fifo->last = rh;
    }
    else
    {
        fifo->last = fifo->first = rh;
        rh->next = rh->prev = NULL;
    }
}             

/* Check if there is any requests in IORF */
Boolean 								// True if empty
IORF_IsEmpty(IORequestFifo *fifo)
{   
    return fifo->last ? false : true;        
}
