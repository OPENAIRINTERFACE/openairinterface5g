/***** includes *****/
#include "internal.h"

/***** private prototypes *****/
static int key_compare_function( void const *new_value, void const *value_in_tree );
static void key_hash_function( void const *key, lfds700_pal_uint_t *hash );





/****************************************************************************/
void test_lfds700_hash_a_fail_and_overwrite_on_existing_key()
{
  enum lfds700_hash_a_insert_result
    apr;

  enum lfds700_misc_validity
    dvs = LFDS700_MISC_VALIDITY_VALID;

  struct lfds700_hash_a_element
    hae_one,
    hae_two;

  struct lfds700_hash_a_state
    has;

  struct lfds700_btree_au_state
    *baus;

  struct lfds700_misc_prng_state
    ps;

  internal_display_test_name( "Fail and overwrite on existing key" );

  lfds700_misc_prng_init( &ps );

  baus = util_aligned_malloc( sizeof(struct lfds700_btree_au_state) * 10, LFDS700_PAL_ATOMIC_ISOLATION_IN_BYTES );

  // TRD : fail on overwrite
  lfds700_hash_a_init_valid_on_current_logical_core( &has, baus, 10, key_compare_function, key_hash_function, LFDS700_HASH_A_EXISTING_KEY_FAIL, NULL );

  LFDS700_HASH_A_SET_KEY_IN_ELEMENT( hae_one, 1 );
  LFDS700_HASH_A_SET_VALUE_IN_ELEMENT( hae_one, 0 );
  apr = lfds700_hash_a_insert( &has, &hae_one, NULL, &ps );

  if( apr != LFDS700_HASH_A_PUT_RESULT_SUCCESS )
    dvs = LFDS700_MISC_VALIDITY_INVALID_TEST_DATA;

  LFDS700_HASH_A_SET_KEY_IN_ELEMENT( hae_two, 1 );
  LFDS700_HASH_A_SET_VALUE_IN_ELEMENT( hae_two, 1 );
  apr = lfds700_hash_a_insert( &has, &hae_two, NULL, &ps );

  if( apr != LFDS700_HASH_A_PUT_RESULT_FAILURE_EXISTING_KEY )
    dvs = LFDS700_MISC_VALIDITY_INVALID_TEST_DATA;

  lfds700_hash_a_cleanup( &has, NULL );

  // TRD : success on overwrite
  lfds700_hash_a_init_valid_on_current_logical_core( &has, baus, 10, key_compare_function, key_hash_function, LFDS700_HASH_A_EXISTING_KEY_OVERWRITE, NULL );

  LFDS700_HASH_A_SET_KEY_IN_ELEMENT( hae_one, 1 );
  LFDS700_HASH_A_SET_VALUE_IN_ELEMENT( hae_one, 1 );
  apr = lfds700_hash_a_insert( &has, &hae_one, NULL, &ps );

  if( apr != LFDS700_HASH_A_PUT_RESULT_SUCCESS )
    dvs = LFDS700_MISC_VALIDITY_INVALID_TEST_DATA;

  LFDS700_HASH_A_SET_KEY_IN_ELEMENT( hae_two, 1 );
  LFDS700_HASH_A_SET_VALUE_IN_ELEMENT( hae_two, 1 );
  apr = lfds700_hash_a_insert( &has, &hae_two, NULL, &ps );

  if( apr != LFDS700_HASH_A_PUT_RESULT_SUCCESS_OVERWRITE )
    dvs = LFDS700_MISC_VALIDITY_INVALID_TEST_DATA;

  lfds700_hash_a_cleanup( &has, NULL );

  util_aligned_free( baus );

  // TRD : print the test result
  internal_display_test_result( 1, "hash_a", dvs );

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





/****************************************************************************/
#pragma warning( disable : 4100 )

static void key_hash_function( void const *key, lfds700_pal_uint_t *hash )
{
  // TRD : key can be NULL
  assert( hash != NULL );

  *hash = 0;

  /* TRD : this function iterates over the user data
           and we are using the void pointer *as* key data
           so here we need to pass in the addy of key
  */

  LFDS700_HASH_A_32BIT_HASH_FUNCTION( (void *) &key, sizeof(lfds700_pal_uint_t), *hash );

  return;
}

#pragma warning( default : 4100 )

