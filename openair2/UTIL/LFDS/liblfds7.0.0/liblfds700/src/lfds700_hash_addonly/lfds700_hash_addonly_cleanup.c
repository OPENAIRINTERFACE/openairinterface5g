/***** includes *****/
#include "lfds700_hash_addonly_internal.h"

/***** private prototypes*****/
static void btree_au_element_cleanup_function( struct lfds700_btree_au_state *baus, struct lfds700_btree_au_element *baue );





/****************************************************************************/
void lfds700_hash_a_cleanup( struct lfds700_hash_a_state *has,
                             void (*element_cleanup_callback)(struct lfds700_hash_a_state *has, struct lfds700_hash_a_element *hae) )
{
  lfds700_pal_uint_t
    loop;

  LFDS700_PAL_ASSERT( has != NULL );
  // TRD : element_cleanup_callback can be NULL

  if( element_cleanup_callback == NULL )
    return;

  LFDS700_MISC_BARRIER_LOAD;

  has->element_cleanup_callback = element_cleanup_callback;

  for( loop = 0 ; loop < has->array_size ; loop++ )
    lfds700_btree_au_cleanup( has->baus_array+loop, btree_au_element_cleanup_function );

  return;
}





/****************************************************************************/

static void btree_au_element_cleanup_function( struct lfds700_btree_au_state *baus, struct lfds700_btree_au_element *baue )
{
  struct lfds700_hash_a_state
    *has;

  struct lfds700_hash_a_element
    *hae;

  LFDS700_PAL_ASSERT( baus != NULL );
  LFDS700_PAL_ASSERT( baue != NULL );

  hae = (struct lfds700_hash_a_element *) LFDS700_BTREE_AU_GET_VALUE_FROM_ELEMENT( *baue );
  has = (struct lfds700_hash_a_state *) LFDS700_BTREE_AU_GET_USER_STATE_FROM_STATE( *baus );

  has->element_cleanup_callback( has, hae );

  return;
}


