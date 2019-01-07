/***** includes *****/
#include "lfds700_list_addonly_ordered_singlylinked_internal.h"





/****************************************************************************/
void lfds700_list_aos_init_valid_on_current_logical_core( struct lfds700_list_aos_state *laoss,
                            int (*key_compare_function)(void const *new_key, void const *existing_key),
                            enum lfds700_list_aos_existing_key existing_key,
                            void *user_state )
{
  LFDS700_PAL_ASSERT( laoss != NULL );
  LFDS700_PAL_ASSERT( (lfds700_pal_uint_t) &laoss->start % LFDS700_PAL_ATOMIC_ISOLATION_IN_BYTES == 0 );
  LFDS700_PAL_ASSERT( (lfds700_pal_uint_t) &laoss->dummy_element % LFDS700_PAL_ATOMIC_ISOLATION_IN_BYTES == 0 );
  LFDS700_PAL_ASSERT( (lfds700_pal_uint_t) &laoss->key_compare_function % LFDS700_PAL_ATOMIC_ISOLATION_IN_BYTES == 0 );
  LFDS700_PAL_ASSERT( key_compare_function != NULL );
  // TRD : existing_key can be any value in its range
  // TRD : user_state can be NULL

  // TRD : dummy start element - makes code easier when you can always use ->next
  laoss->start = &laoss->dummy_element;

  laoss->start->next = NULL;
  laoss->start->value = NULL;
  laoss->key_compare_function = key_compare_function;
  laoss->existing_key = existing_key;
  laoss->user_state = user_state;

  LFDS700_MISC_BARRIER_STORE;

  lfds700_misc_force_store();

  return;
}

