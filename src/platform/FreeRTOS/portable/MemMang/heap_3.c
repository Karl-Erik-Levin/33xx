/*
	FreeRTOS V3.0.0 - Copyright (C) 2003 - 2005 Richard Barry.

	This file is part of the FreeRTOS distribution.

	FreeRTOS is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	FreeRTOS is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with FreeRTOS; if not, write to the Free Software
	Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

	A special exception to the GPL can be applied should you wish to distribute
	a combined work that includes FreeRTOS, without being obliged to provide
	the source code for any proprietary components.  See the licensing section 
	of http://www.FreeRTOS.org for full details of how and when the exception
	can be applied.

	***************************************************************************
	See http://www.FreeRTOS.org for documentation, latest information, license 
	and contact details.  Please ensure to read the configuration and relevant 
	port sections of the online documentation.
	***************************************************************************

	History:
	2007-09-27	Fred J	Added memory allocation and deallocation tracing.
						Note: Impacts the IDLE task, as vPortFree is called from it.
*/


/*
 * Implementation of pvPortMalloc() and vPortFree() that relies on the
 * compilers own malloc() and free() implementations.
 *
 * This file can only be used if the linker is configured to to generate
 * a heap memory area.
 *
 * See heap_2.c and heap_1.c for alternative implementations, and the memory
 * management pages of http://www.FreeRTOS.org for more information.
 */

#include <stdlib.h>

#include "Platform\FreeRTOS\FreeRTOS.h"
#include "Platform\FreeRTOS\task.h"


#ifdef OPTION_DEBUG_MEMORY_ALLOCATION
#include "MFLIb/AssertAndTrace.h"
#include <MFLib/Dataton_Types.h>

static LongWord gUsedMemory = 0;
#endif


/*-----------------------------------------------------------*/

void *pvPortMalloc( size_t xWantedSize )
{
void *pvReturn;

	vTaskSuspendAll();
	{
		pvReturn = malloc( xWantedSize );

#ifdef OPTION_DEBUG_MEMORY_ALLOCATION
	if (pvReturn)
	{
		LongWord *size = pvReturn;
		size--;

		TracePrintf("pvPortMalloc %x %d %d %d %d\n\r", pvReturn, xWantedSize, *size, gUsedMemory, gUsedMemory + *size);
		gUsedMemory += *size;
	}
#endif
	}
	xTaskResumeAll();

	
	return pvReturn;
}
/*-----------------------------------------------------------*/

void vPortFree( void *pv )
{
	if( pv )
	{
		vTaskSuspendAll();
		{
#ifdef OPTION_DEBUG_MEMORY_ALLOCATION
			LongWord *size = pv;
			size--;
			gUsedMemory -= *size;

			TracePrintf("vPortFree %x %d %d %d\n\r", pv, *size, gUsedMemory, gUsedMemory + *size);
#endif			
			free( pv );
			
		}
		xTaskResumeAll();
	}
}



