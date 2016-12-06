/***** includes *****/
#include "internal.h"





/****************************************************************************/
#pragma warning( disable : 4127 ) // TRD : disables MSVC warning for condition expressions being const

void test_lfds700_freelist( struct lfds700_list_asu_state *list_of_logical_processors, lfds700_pal_uint_t memory_in_megabytes )
{
  assert( list_of_logical_processors != NULL );
  // TRD : memory_in_megabytes can be any value in its range

  if( LFDS700_MISC_ATOMIC_SUPPORT_DWCAS )
  {
    printf( "\n"
            "Freelist Tests\n"
            "==============\n" );

    test_lfds700_freelist_alignment();
    test_lfds700_freelist_popping( list_of_logical_processors, memory_in_megabytes );
    test_lfds700_freelist_pushing( list_of_logical_processors, memory_in_megabytes );
    test_lfds700_freelist_popping_and_pushing( list_of_logical_processors, memory_in_megabytes );
    test_lfds700_freelist_rapid_popping_and_pushing( list_of_logical_processors );
  }

  return;
}

#pragma warning( default : 4127 )

