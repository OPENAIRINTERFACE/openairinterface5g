/***** includes *****/
#include "internal.h"

/***** structs *****/
struct test_element
{
  struct lfds700_btree_au_element
    baue;

  lfds700_pal_uint_t
    datum;
};

struct test_state
{
  enum lfds700_misc_flag
    error_flag;

  struct lfds700_hash_a_state
    *has;

  struct test_element
    *element_array;
};

/***** private prototypes *****/
static int key_compare_function( void const *new_key, void const *existing_key );
static void key_hash_function( void const *key, lfds700_pal_uint_t *hash );





/****************************************************************************/
void test_lfds700_hash_a_iterate( void )
{
  enum lfds700_misc_validity
    dvs = LFDS700_MISC_VALIDITY_VALID;

  lfds700_pal_uint_t
    *counter_array,
    loop;

  struct lfds700_hash_a_element
    *hae;

  struct lfds700_hash_a_iterate
    hai;

  struct lfds700_hash_a_state
    has;

  struct lfds700_hash_a_element
    *element_array;

  struct lfds700_btree_au_state
    *baus;

  struct lfds700_misc_prng_state
    ps;

  void
    *value;

  /* TRD : single-threaded test
           we create a single hash_a
           we populate with 1000 elements
           where key and value is the number of the element (e.g. 0 to 999)
           we then allocate 1000 counters, init to 0
           we then iterate
           we increment each element as we see it in the iterate
           if any are missing or seen more than once, problemo!

           we do this once with a table of 10, to ensure each table has (or almost certainly has) something in
           and then a second tiem with a table of 10000, to ensure some empty tables exist
  */

  internal_display_test_name( "Iterate" );

  lfds700_misc_prng_init( &ps );

  element_array = util_aligned_malloc( sizeof(struct lfds700_hash_a_element) * 1000, LFDS700_PAL_ATOMIC_ISOLATION_IN_BYTES );

  counter_array = util_malloc_wrapper( sizeof(lfds700_pal_uint_t) * 1000 );

  // TRD : first time around
  baus = util_aligned_malloc( sizeof(struct lfds700_btree_au_state) * 10, LFDS700_PAL_ATOMIC_ISOLATION_IN_BYTES );

  lfds700_hash_a_init_valid_on_current_logical_core( &has, baus, 10, key_compare_function, key_hash_function, LFDS700_HASH_A_EXISTING_KEY_FAIL, NULL );

  for( loop = 0 ; loop < 1000 ; loop++ )
  {
    LFDS700_HASH_A_SET_KEY_IN_ELEMENT( *(element_array+loop), loop );
    LFDS700_HASH_A_SET_VALUE_IN_ELEMENT( *(element_array+loop), loop );
    lfds700_hash_a_insert( &has, element_array+loop, NULL, &ps );
  }

  for( loop = 0 ; loop < 1000 ; loop++ )
    *(counter_array+loop) = 0;

  lfds700_hash_a_iterate_init( &has, &hai );

  while( dvs == LFDS700_MISC_VALIDITY_VALID and lfds700_hash_a_iterate(&hai, &hae) )
  {
    value = LFDS700_HASH_A_GET_VALUE_FROM_ELEMENT( *hae );
    ( *(counter_array + (lfds700_pal_uint_t) value) )++;
  }

  if( dvs == LFDS700_MISC_VALIDITY_VALID )
    for( loop = 0 ; loop < 1000 ; loop++ )
    {
      if( *(counter_array+loop) > 1 )
        dvs = LFDS700_MISC_VALIDITY_INVALID_ADDITIONAL_ELEMENTS;

      if( *(counter_array+loop) == 0 )
        dvs = LFDS700_MISC_VALIDITY_INVALID_MISSING_ELEMENTS;
    }

  lfds700_hash_a_cleanup( &has, NULL );

  util_aligned_free( baus );

  // TRD : second time around
  if( dvs == LFDS700_MISC_VALIDITY_VALID )
  {
    baus = util_aligned_malloc( sizeof(struct lfds700_btree_au_state) * 10000, LFDS700_PAL_ATOMIC_ISOLATION_IN_BYTES );

    lfds700_hash_a_init_valid_on_current_logical_core( &has, baus, 10000, key_compare_function, key_hash_function, LFDS700_HASH_A_EXISTING_KEY_FAIL, NULL );

    for( loop = 0 ; loop < 1000 ; loop++ )
    {
      LFDS700_HASH_A_SET_KEY_IN_ELEMENT( *(element_array+loop), loop );
      LFDS700_HASH_A_SET_VALUE_IN_ELEMENT( *(element_array+loop), loop );
      lfds700_hash_a_insert( &has, element_array+loop, NULL, &ps );
    }

    for( loop = 0 ; loop < 1000 ; loop++ )
      *(counter_array+loop) = 0;

    lfds700_hash_a_iterate_init( &has, &hai );

    while( dvs == LFDS700_MISC_VALIDITY_VALID and lfds700_hash_a_iterate(&hai, &hae) )
    {
      value = LFDS700_HASH_A_GET_VALUE_FROM_ELEMENT( *hae );
      ( *(counter_array + (lfds700_pal_uint_t) value ) )++;
    }

    if( dvs == LFDS700_MISC_VALIDITY_VALID )
      for( loop = 0 ; loop < 1000 ; loop++ )
      {
        if( *(counter_array+loop) > 1 )
          dvs = LFDS700_MISC_VALIDITY_INVALID_ADDITIONAL_ELEMENTS;

        if( *(counter_array+loop) == 0 )
          dvs = LFDS700_MISC_VALIDITY_INVALID_MISSING_ELEMENTS;
      }

    lfds700_hash_a_cleanup( &has, NULL );

    util_aligned_free( baus );
  }

  // TRD : cleanup
  util_aligned_free( element_array );
  free( counter_array );

  // TRD : print the test result
  internal_display_test_result( 1, "hash_a", dvs );

  return;
}





/****************************************************************************/
#pragma warning( disable : 4100 )

static int key_compare_function( void const *new_key, void const *existing_key )
{
  int
    cr = 0;

  // TRD : new_key can be NULL (i.e. 0)
  // TRD : existing_key can be NULL (i.e. 0)

  if( (lfds700_pal_uint_t) new_key < (lfds700_pal_uint_t) existing_key )
    cr = -1;

  if( (lfds700_pal_uint_t) new_key > (lfds700_pal_uint_t) existing_key )
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
           and we are using the void pointer AS user data
           so here we need to pass in the addy of value
  */

  LFDS700_HASH_A_32BIT_HASH_FUNCTION( (void *) &key, sizeof(lfds700_pal_uint_t), *hash );

  return;
}

#pragma warning( default : 4100 )

