/***** includes *****/
#include "lfds700_list_addonly_singlylinked_unordered_internal.h"





/****************************************************************************/
void lfds700_list_asu_init_valid_on_current_logical_core( struct lfds700_list_asu_state *lasus,
                            int (*key_compare_function)(void const *new_key, void const *existing_key),
                            void *user_state )
{
  LFDS700_PAL_ASSERT( lasus != NULL );
  LFDS700_PAL_ASSERT( (lfds700_pal_uint_t) &lasus->end % LFDS700_PAL_ATOMIC_ISOLATION_IN_BYTES == 0 );
  LFDS700_PAL_ASSERT( (lfds700_pal_uint_t) &lasus->start % LFDS700_PAL_ATOMIC_ISOLATION_IN_BYTES == 0 );
  LFDS700_PAL_ASSERT( (lfds700_pal_uint_t) &lasus->dummy_element % LFDS700_PAL_ATOMIC_ISOLATION_IN_BYTES == 0 );
  LFDS700_PAL_ASSERT( (lfds700_pal_uint_t) &lasus->key_compare_function % LFDS700_PAL_ATOMIC_ISOLATION_IN_BYTES == 0 );
  // TRD : key_compare_function can be NULL
  // TRD : user_state can be NULL

  // TRD : dummy start element - makes code easier when you can always use ->next
  lasus->start = lasus->end = &lasus->dummy_element;

  lasus->start->next = NULL;
  lasus->start->value = NULL;
  lasus->key_compare_function = key_compare_function;
  lasus->user_state = user_state;

  LFDS700_MISC_BARRIER_STORE;

  lfds700_misc_force_store();

  return;
}

