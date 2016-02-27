/***** includes *****/
#include "internal.h"





/****************************************************************************/
#pragma warning( disable : 4127 ) // TRD : disables MSVC warning for condition expressions being const

void test_lfds700_freelist_alignment()
{
  enum lfds700_misc_validity
    dvs = LFDS700_MISC_VALIDITY_VALID;

  /* TRD : these are compile time checks
           but we do them here because this is a test programme
           and it should indicate issues to users when it is *run*,
           not when it is compiled, because a compile error normally
           indicates a problem with the code itself and so is misleading
  */

  internal_display_test_name( "Alignment" );



  // TRD : struct lfds700_freelist_state
  if( offsetof(struct lfds700_freelist_state,top) % LFDS700_PAL_ATOMIC_ISOLATION_IN_BYTES != 0 )
    dvs = LFDS700_MISC_VALIDITY_INVALID_TEST_DATA;

  if( offsetof(struct lfds700_freelist_state,user_state) % LFDS700_PAL_ATOMIC_ISOLATION_IN_BYTES != 0 )
    dvs = LFDS700_MISC_VALIDITY_INVALID_TEST_DATA;



  // TRD : print the test result
  internal_display_test_result( 1, "freelist", dvs );

  return;
}

#pragma warning( default : 4127 )

