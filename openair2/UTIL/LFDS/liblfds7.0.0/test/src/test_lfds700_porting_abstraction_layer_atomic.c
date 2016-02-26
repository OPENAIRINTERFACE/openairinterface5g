/***** includes *****/
#include "internal.h"





/****************************************************************************/
#pragma warning( disable : 4127 ) // TRD : disables MSVC warning for condition expressions being const

void test_lfds700_pal_atomic( struct lfds700_list_asu_state *list_of_logical_processors, lfds700_pal_uint_t memory_in_megabytes )
{
  assert( list_of_logical_processors != NULL );
  // TRD : memory_in_megabytes can be any value in its range

  printf( "\n"
          "Abstraction Atomic Tests\n"
          "========================\n" );

  if( LFDS700_MISC_ATOMIC_SUPPORT_CAS )
    test_lfds700_pal_atomic_cas( list_of_logical_processors );

  if( LFDS700_MISC_ATOMIC_SUPPORT_DWCAS )
    test_lfds700_pal_atomic_dwcas( list_of_logical_processors );

  if( LFDS700_MISC_ATOMIC_SUPPORT_EXCHANGE )
    test_lfds700_pal_atomic_exchange( list_of_logical_processors, memory_in_megabytes );

  return;
}

#pragma warning( default : 4127 )

