/***** includes *****/
#include "lfds700_queue_internal.h"





/****************************************************************************/
void lfds700_queue_cleanup( struct lfds700_queue_state *qs,
                            void (*element_cleanup_callback)(struct lfds700_queue_state *qs, struct lfds700_queue_element *qe, enum lfds700_misc_flag dummy_element_flag) )
{
  struct lfds700_queue_element
    *qe;

  void
    *value;

  LFDS700_PAL_ASSERT( qs != NULL );
  // TRD : element_cleanup_callback can be NULL

  LFDS700_MISC_BARRIER_LOAD;

  if( element_cleanup_callback != NULL )
  {
    while( qs->dequeue[POINTER] != qs->enqueue[POINTER] )
    {
      // TRD : trailing dummy element, so the first real value is in the next element
      value = qs->dequeue[POINTER]->next[POINTER]->value;

      // TRD : user is given back *an* element, but not the one his user data was in
      qe = qs->dequeue[POINTER];

      // TRD : remove the element from queue
      qs->dequeue[POINTER] = qs->dequeue[POINTER]->next[POINTER];

      // TRD : write value into the qe we're going to give the user
      qe->value = value;

      element_cleanup_callback( qs, qe, LFDS700_MISC_FLAG_LOWERED );
    }

    // TRD : and now the final element
    element_cleanup_callback( qs, (struct lfds700_queue_element *) qs->dequeue[POINTER], LFDS700_MISC_FLAG_RAISED );
  }

  return;
}

