/***** includes *****/
#include "internal.h"





/****************************************************************************/
#pragma warning( disable : 4127 ) // TRD : disables MSVC warning for condition expressions being const

void test_lfds700_hash_a( struct lfds700_list_asu_state *list_of_logical_processors, lfds700_pal_uint_t memory_in_megabytes )
{
  assert( list_of_logical_processors != NULL );
  // TRD : memory_in_megabytes can be any value in its range

  if( LFDS700_MISC_ATOMIC_SUPPORT_CAS )
  {
    printf( "\n"
            "Hash (add-only) Tests\n"
            "=====================\n" );

    test_lfds700_hash_a_alignment();
    test_lfds700_hash_a_fail_and_overwrite_on_existing_key();
    test_lfds700_hash_a_random_adds_fail_on_existing( list_of_logical_processors, memory_in_megabytes );
    test_lfds700_hash_a_random_adds_overwrite_on_existing( list_of_logical_processors, memory_in_megabytes );
    test_lfds700_hash_a_iterate();
  }

  return;
}

#pragma warning( default : 4127 )

