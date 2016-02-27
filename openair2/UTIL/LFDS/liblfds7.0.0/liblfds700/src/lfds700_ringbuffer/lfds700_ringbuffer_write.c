/***** includes *****/
#include "lfds700_ringbuffer_internal.h"





/****************************************************************************/
void lfds700_ringbuffer_write( struct lfds700_ringbuffer_state *rs,
                               void *key,
                               void *value,
                               enum lfds700_misc_flag *overwrite_occurred_flag,
                               void **overwritten_key,
                               void **overwritten_value,
                               struct lfds700_misc_prng_state *ps )
{
  int
    rv = 0;

  struct lfds700_freelist_element
    *fe;

  struct lfds700_queue_element
    *qe;

  struct lfds700_ringbuffer_element
    *re = NULL;

  LFDS700_PAL_ASSERT( rs != NULL );
  // TRD : key can be NULL
  // TRD : value can be NULL
  // TRD : overwrite_occurred_flag can be NULL
  // TRD : overwritten_key can be NULL
  // TRD : overwritten_value can be NULL
  LFDS700_PAL_ASSERT( ps != NULL );

  if( overwrite_occurred_flag != NULL )
    *overwrite_occurred_flag = LFDS700_MISC_FLAG_LOWERED;

  do
  {
    rv = lfds700_freelist_pop( &rs->fs, &fe, ps );

    if( rv == 1 )
      re = LFDS700_FREELIST_GET_VALUE_FROM_ELEMENT( *fe );

    if( rv == 0 )
    {
      // TRD : the queue can return empty as well - remember, we're lock-free; anything could have happened since the previous instruction
      rv = lfds700_queue_dequeue( &rs->qs, &qe, ps );

      if( rv == 1 )
      {
        re = LFDS700_QUEUE_GET_VALUE_FROM_ELEMENT( *qe );
        re->qe_use = (struct lfds700_queue_element *) qe;

        if( overwrite_occurred_flag != NULL )
          *overwrite_occurred_flag = LFDS700_MISC_FLAG_RAISED;

        if( overwritten_key != NULL )
          *overwritten_key = re->key;

        if( overwritten_value != NULL )
          *overwritten_value = re->value;
      }
    }
  }
  while( rv == 0 );

  re->key = key;
  re->value = value;

  LFDS700_QUEUE_SET_VALUE_IN_ELEMENT( *re->qe_use, re );
  lfds700_queue_enqueue( &rs->qs, re->qe_use, ps );

  return;
}

