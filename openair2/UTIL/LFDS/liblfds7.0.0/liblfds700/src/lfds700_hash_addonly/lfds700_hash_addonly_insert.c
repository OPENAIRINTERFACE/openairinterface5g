/***** includes *****/
#include "lfds700_hash_addonly_internal.h"





/****************************************************************************/
enum lfds700_hash_a_insert_result lfds700_hash_a_insert( struct lfds700_hash_a_state *has,
                                                         struct lfds700_hash_a_element *hae,
                                                         struct lfds700_hash_a_element **existing_hae,
                                                         struct lfds700_misc_prng_state *ps )
{
  enum lfds700_hash_a_insert_result
    apr = LFDS700_HASH_A_PUT_RESULT_SUCCESS;

  enum lfds700_btree_au_insert_result
    alr;

  lfds700_pal_uint_t
    hash = 0;

  struct lfds700_btree_au_element
    *existing_baue;

  LFDS700_PAL_ASSERT( has != NULL );
  LFDS700_PAL_ASSERT( hae != NULL );
  LFDS700_PAL_ASSERT( (lfds700_pal_uint_t) &hae->value % LFDS700_PAL_ATOMIC_ISOLATION_IN_BYTES == 0 );
  // TRD : existing_hae can be NULL
  LFDS700_PAL_ASSERT( ps != NULL );

  // TRD : alignment checks
  LFDS700_PAL_ASSERT( (lfds700_pal_uint_t) &hae->baue % LFDS700_PAL_ALIGN_SINGLE_POINTER == 0 );

  has->key_hash_function( hae->key, &hash );

  LFDS700_BTREE_AU_SET_KEY_IN_ELEMENT( hae->baue, hae->key );
  LFDS700_BTREE_AU_SET_VALUE_IN_ELEMENT( hae->baue, hae );

  alr = lfds700_btree_au_insert( has->baus_array + (hash % has->array_size), &hae->baue, &existing_baue, ps );

  switch( alr )
  {
    case LFDS700_BTREE_AU_INSERT_RESULT_FAILURE_EXISTING_KEY:
      if( existing_hae != NULL )
        *existing_hae = LFDS700_BTREE_AU_GET_VALUE_FROM_ELEMENT( *existing_baue );

      apr = LFDS700_HASH_A_PUT_RESULT_FAILURE_EXISTING_KEY;
    break;

    case LFDS700_BTREE_AU_INSERT_RESULT_SUCCESS_OVERWRITE:
      apr = LFDS700_HASH_A_PUT_RESULT_SUCCESS_OVERWRITE;
    break;

    case LFDS700_BTREE_AU_INSERT_RESULT_SUCCESS:
      apr = LFDS700_HASH_A_PUT_RESULT_SUCCESS;
    break;
  }

  return( apr );
}

