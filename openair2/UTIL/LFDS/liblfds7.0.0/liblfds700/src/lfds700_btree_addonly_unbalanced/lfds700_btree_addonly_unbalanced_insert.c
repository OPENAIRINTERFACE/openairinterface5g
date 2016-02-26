/***** includes *****/
#include "lfds700_btree_addonly_unbalanced_internal.h"





/****************************************************************************/
enum lfds700_btree_au_insert_result lfds700_btree_au_insert( struct lfds700_btree_au_state *baus,
                                                             struct lfds700_btree_au_element *baue,
                                                             struct lfds700_btree_au_element **existing_baue,
                                                             struct lfds700_misc_prng_state *ps )
{
  char unsigned 
    result = 0;

  int
    compare_result = 0;

  lfds700_pal_uint_t
    backoff_iteration = LFDS700_MISC_ABSTRACTION_BACKOFF_INITIAL_VALUE;

  struct lfds700_btree_au_element
    *volatile compare = NULL,
    *volatile baue_next = NULL,
    *volatile baue_parent = NULL,
    *volatile baue_temp;

  LFDS700_PAL_ASSERT( baus != NULL );
  LFDS700_PAL_ASSERT( baue != NULL );
  LFDS700_PAL_ASSERT( (lfds700_pal_uint_t) &baue->left % LFDS700_PAL_ATOMIC_ISOLATION_IN_BYTES == 0 );
  LFDS700_PAL_ASSERT( (lfds700_pal_uint_t) &baue->right % LFDS700_PAL_ATOMIC_ISOLATION_IN_BYTES == 0 );
  LFDS700_PAL_ASSERT( (lfds700_pal_uint_t) &baue->up % LFDS700_PAL_ATOMIC_ISOLATION_IN_BYTES == 0 );
  LFDS700_PAL_ASSERT( (lfds700_pal_uint_t) &baue->value % LFDS700_PAL_ATOMIC_ISOLATION_IN_BYTES == 0 );
  LFDS700_PAL_ASSERT( (lfds700_pal_uint_t) &baue->key % LFDS700_PAL_ATOMIC_ISOLATION_IN_BYTES == 0 );
  // TRD : existing_baue can be NULL
  LFDS700_PAL_ASSERT( ps != NULL );

  /* TRD : we follow a normal search for the insert node and which side to insert

           the difference is that insertion may fail because someone else inserts
           there before we do

           in this case, we resume searching for the insert node from the node
           we were attempting to insert upon

           (if we attempted to insert the root node and this failed, i.e. we thought
            the tree was empty but then it wasn't, then we start searching from the
            new root)
  */

  baue->up = baue->left = baue->right = NULL;

  LFDS700_MISC_BARRIER_LOAD;

  baue_temp = baus->root;

  LFDS700_MISC_BARRIER_LOAD;

  while( result == 0 )
  {
    // TRD : first we find where to insert
    while( baue_temp != NULL )
    {
      compare_result = baus->key_compare_function( baue->key, baue_temp->key );

      if( compare_result == 0 )
      {
        if( existing_baue != NULL )
          *existing_baue = baue_temp;

        switch( baus->existing_key )
        {
          case LFDS700_BTREE_AU_EXISTING_KEY_OVERWRITE:
            LFDS700_BTREE_AU_SET_VALUE_IN_ELEMENT( *baue_temp, baue->value );
            return( LFDS700_BTREE_AU_INSERT_RESULT_SUCCESS_OVERWRITE );
          break;

          case LFDS700_BTREE_AU_EXISTING_KEY_FAIL:
            return( LFDS700_BTREE_AU_INSERT_RESULT_FAILURE_EXISTING_KEY );
          break;
        }
      }

      if( compare_result < 0 )
        baue_next = baue_temp->left;

      if( compare_result > 0 )
        baue_next = baue_temp->right;

      baue_parent = baue_temp;
      baue_temp = baue_next;
      if( baue_temp != NULL )
        LFDS700_MISC_BARRIER_LOAD;
    }

    /* TRD : second, we actually insert

             at this point baue_temp has come to NULL
             and baue_parent is the element to insert at
             and result of the last compare indicates
             the direction of insertion

             it may be that another tree has already inserted an element with
             the same key as ourselves, or other elements which mean our position
             is now wrong

             in this case, it is either inserted in the position we're trying
             to insert in now, in which case our insert will fail

             or, similarly, other elements will have come in where we are,
             and our insert will fail
    */

    if( baue_parent == NULL )
    {
      compare = NULL;
      baue->up = baus->root;
      LFDS700_MISC_BARRIER_STORE;
      LFDS700_PAL_ATOMIC_CAS_WITH_BACKOFF( &baus->root, &compare, baue, LFDS700_MISC_CAS_STRENGTH_WEAK, result, backoff_iteration, ps );

      if( result == 0 )
        baue_temp = baus->root;
    }

    if( baue_parent != NULL )
    {
      if( compare_result <= 0 )
      {
        compare = NULL;
        baue->up = baue_parent;
        LFDS700_MISC_BARRIER_STORE;
        LFDS700_PAL_ATOMIC_CAS_WITH_BACKOFF( &baue_parent->left, &compare, baue, LFDS700_MISC_CAS_STRENGTH_WEAK, result, backoff_iteration, ps );
      }

      if( compare_result > 0 )
      {
        compare = NULL;
        baue->up = baue_parent;
        LFDS700_MISC_BARRIER_STORE;
        LFDS700_PAL_ATOMIC_CAS_WITH_BACKOFF( &baue_parent->right, &compare, baue, LFDS700_MISC_CAS_STRENGTH_WEAK, result, backoff_iteration, ps );
      }

      // TRD : if the insert fails, resume searching at the insert node
      if( result == 0 )
        baue_temp = baue_parent;
    }
  }

  // TRD : if we get to here, we added (not failed or overwrite on exist) a new element
  if( existing_baue != NULL )
    *existing_baue = NULL;

  return( LFDS700_BTREE_AU_INSERT_RESULT_SUCCESS );
}

