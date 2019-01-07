/***** includes *****/
#include "lfds700_hash_addonly_internal.h"





/****************************************************************************/
int lfds700_hash_a_get_by_key( struct lfds700_hash_a_state *has,
                               void *key,
                               struct lfds700_hash_a_element **hae )
{
  int
    rv;

  lfds700_pal_uint_t
    hash = 0;

  struct lfds700_btree_au_element
    *baue;

  LFDS700_PAL_ASSERT( has != NULL );
  // TRD : key can be NULL
  LFDS700_PAL_ASSERT( hae != NULL );

  has->key_hash_function( key, &hash );

  rv = lfds700_btree_au_get_by_key( has->baus_array + (hash % has->array_size), key, &baue );

  if( rv == 1 )
    *hae = LFDS700_BTREE_AU_GET_VALUE_FROM_ELEMENT( *baue );
  else
    *hae = NULL;

  return( rv );
}

