/***** includes *****/
#include "internal.h"





/****************************************************************************/
#pragma warning( disable : 4127 ) // TRD : disables MSVC warning for condition expressions being const

void test_lfds700_ringbuffer( struct lfds700_list_asu_state *list_of_logical_processors, lfds700_pal_uint_t memory_in_megabytes )
{
  assert( list_of_logical_processors != NULL );
  // TRD : memory_in_megabytes can be any value in its range

  if( LFDS700_MISC_ATOMIC_SUPPORT_DWCAS )
  {
    printf( "\n"
            "Ringbuffer Tests\n"
            "================\n" );

    test_lfds700_ringbuffer_reading( list_of_logical_processors, memory_in_megabytes );
    test_lfds700_ringbuffer_reading_and_writing( list_of_logical_processors, memory_in_megabytes );
    test_lfds700_ringbuffer_writing( list_of_logical_processors, memory_in_megabytes );
  }

  return;
}

#pragma warning( default : 4127 )

