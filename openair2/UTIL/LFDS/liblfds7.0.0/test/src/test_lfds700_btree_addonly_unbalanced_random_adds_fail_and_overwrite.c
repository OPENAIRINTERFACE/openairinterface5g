/***** includes *****/
#include "internal.h"

/***** private prototypes *****/
static int key_compare_function( void const *new_value, void const *value_in_tree );





/****************************************************************************/
void test_lfds700_btree_au_fail_and_overwrite_on_existing_key()
{
  enum lfds700_btree_au_insert_result
    alr;

  enum lfds700_misc_validity
    dvs = LFDS700_MISC_VALIDITY_VALID;

  struct lfds700_btree_au_element
    baue_one,
    baue_two,
    *existing_baue;

  struct lfds700_btree_au_state
    baus;

  struct lfds700_misc_prng_state
    ps;

  void
    *value;

  /* TRD : the random_adds tests with fail and overwrite don't (can't, not in a performant manner)
           test that the fail and/or overwrite of user data has *actually* happened - they use the
           return value from the link function call, rather than empirically observing the final
           state of the tree

           as such, we now have a couple of single threaded tests where we check that the user data
           value really is being modified (or not modified, as the case may be)
  */

  internal_display_test_name( "Fail and overwrite on existing key" );

  lfds700_misc_prng_init( &ps );

  /* TRD : so, we make a tree which is fail on existing
           add one element, with a known user data
           we then try to add the same key again, with a different user data
           the call should fail, and then we get the element by its key
           and check its user data is unchanged
           (and confirm the failed link returned the correct existing_baue)
           that's the first test done
  */

  lfds700_btree_au_init_valid_on_current_logical_core( &baus, key_compare_function, LFDS700_BTREE_AU_EXISTING_KEY_FAIL, NULL );

  LFDS700_BTREE_AU_SET_KEY_IN_ELEMENT( baue_one, 0 );
  LFDS700_BTREE_AU_SET_VALUE_IN_ELEMENT( baue_one, 1 );
  alr = lfds700_btree_au_insert( &baus, &baue_one, NULL, &ps );

  if( alr != LFDS700_BTREE_AU_INSERT_RESULT_SUCCESS )
    dvs = LFDS700_MISC_VALIDITY_INVALID_TEST_DATA;

  LFDS700_BTREE_AU_SET_KEY_IN_ELEMENT( baue_two, 0 );
  LFDS700_BTREE_AU_SET_VALUE_IN_ELEMENT( baue_two, 2 );
  alr = lfds700_btree_au_insert( &baus, &baue_two, &existing_baue, &ps );

  if( alr != LFDS700_BTREE_AU_INSERT_RESULT_FAILURE_EXISTING_KEY )
    dvs = LFDS700_MISC_VALIDITY_INVALID_TEST_DATA;

  if( existing_baue != &baue_one )
    dvs = LFDS700_MISC_VALIDITY_INVALID_TEST_DATA;

  value = LFDS700_BTREE_AU_GET_VALUE_FROM_ELEMENT( *existing_baue );

  if( (void *) (lfds700_pal_uint_t) value != (void *) (lfds700_pal_uint_t) 1 )
    dvs = LFDS700_MISC_VALIDITY_INVALID_TEST_DATA;

  lfds700_btree_au_cleanup( &baus, NULL );

  /* TRD : second test, make a tree which is overwrite on existing
           add one element, with a known user data
           we then try to add the same key again, with a different user data
           the call should succeed, and then we get the element by its key
           and check its user data is changed
           (and confirm the failed link returned the correct existing_baue)
           that's the secondtest done
  */

  lfds700_btree_au_init_valid_on_current_logical_core( &baus, key_compare_function, LFDS700_BTREE_AU_EXISTING_KEY_OVERWRITE, NULL );

  LFDS700_BTREE_AU_SET_KEY_IN_ELEMENT( baue_one, 0 );
  LFDS700_BTREE_AU_SET_VALUE_IN_ELEMENT( baue_one, 1 );
  alr = lfds700_btree_au_insert( &baus, &baue_one, NULL, &ps );

  if( alr != LFDS700_BTREE_AU_INSERT_RESULT_SUCCESS )
    dvs = LFDS700_MISC_VALIDITY_INVALID_TEST_DATA;

  LFDS700_BTREE_AU_SET_KEY_IN_ELEMENT( baue_two, 0 );
  LFDS700_BTREE_AU_SET_VALUE_IN_ELEMENT( baue_two, 2 );
  alr = lfds700_btree_au_insert( &baus, &baue_two, NULL, &ps );

  if( alr != LFDS700_BTREE_AU_INSERT_RESULT_SUCCESS_OVERWRITE )
    dvs = LFDS700_MISC_VALIDITY_INVALID_TEST_DATA;

  lfds700_btree_au_cleanup( &baus, NULL );

  // TRD : print the test result
  internal_display_test_result( 1, "btree_au", dvs );

  return;
}





/****************************************************************************/
#pragma warning( disable : 4100 )

static int key_compare_function( void const *new_key, void const *key_in_tree )
{
  int
    cr = 0;

  // TRD : key_new can be any value in its range
  // TRD : key_in_tree can be any value in its range

  if( (lfds700_pal_uint_t) new_key < (lfds700_pal_uint_t) key_in_tree )
    cr = -1;

  if( (lfds700_pal_uint_t) new_key > (lfds700_pal_uint_t) key_in_tree )
    cr = 1;

  return( cr );
}

#pragma warning( default : 4100 )

