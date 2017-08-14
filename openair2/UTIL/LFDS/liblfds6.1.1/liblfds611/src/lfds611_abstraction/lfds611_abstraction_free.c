#include "lfds611_abstraction_internal_wrapper.h"





/****************************************************************************/
#if (!defined WIN_KERNEL_BUILD)

/* TRD : any OS except Windows kernel on any CPU with any compiler

         !WIN_KERNEL_BUILD  indicates not Windows kernel
*/

void lfds611_abstraction_free( void *memory )
{
  free( memory );

  return;
}

#endif





/****************************************************************************/
#if (defined WIN_KERNEL_BUILD)

/* TRD : any Windows (kernel) on any CPU with the Microsoft C compiler

         WIN_KERNEL_BUILD  indicates Windows kernel
*/

void lfds611_abstraction_free( void *memory )
{
  ExFreePoolWithTag( memory, 'sdfl' );

  return;
}

#endif

