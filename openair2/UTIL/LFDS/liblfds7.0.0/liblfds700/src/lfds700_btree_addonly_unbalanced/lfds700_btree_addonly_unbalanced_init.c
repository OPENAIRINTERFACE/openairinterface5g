/***** includes *****/
#include "lfds700_btree_addonly_unbalanced_internal.h"





/****************************************************************************/
void lfds700_btree_au_init_valid_on_current_logical_core( struct lfds700_btree_au_state *baus,
                                                          int (*key_compare_function)(void const *new_key, void const *existing_key),
                                                          enum lfds700_btree_au_existing_key existing_key,
                                                          void *user_state )
{
  LFDS700_PAL_ASSERT( baus != NULL );
  LFDS700_PAL_ASSERT( (lfds700_pal_uint_t) &baus->root % LFDS700_PAL_ATOMIC_ISOLATION_IN_BYTES == 0 );
  LFDS700_PAL_ASSERT( (lfds700_pal_uint_t) &baus->key_compare_function % LFDS700_PAL_ATOMIC_ISOLATION_IN_BYTES == 0 );
  LFDS700_PAL_ASSERT( key_compare_function != NULL );
  // TRD : existing_key can be any value in its range
  // TRD : user_state can be NULL

  baus->root = NULL;
  baus->key_compare_function = key_compare_function;
  baus->existing_key = existing_key;
  baus->user_state = user_state;

  LFDS700_MISC_BARRIER_STORE;

  lfds700_misc_force_store();

  return;
}

