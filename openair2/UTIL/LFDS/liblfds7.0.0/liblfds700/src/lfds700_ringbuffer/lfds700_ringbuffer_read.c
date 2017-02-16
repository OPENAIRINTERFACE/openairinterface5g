/***** includes *****/
#include "lfds700_ringbuffer_internal.h"





/****************************************************************************/
int lfds700_ringbuffer_read( struct lfds700_ringbuffer_state *rs,
                             void **key,
                             void **value,
                             struct lfds700_misc_prng_state *ps )
{
  int
    rv;

  struct lfds700_queue_element
    *qe;

  struct lfds700_ringbuffer_element
    *re;

  LFDS700_PAL_ASSERT( rs != NULL );
  // TRD : key can be NULL
  // TRD : value can be NULL
  LFDS700_PAL_ASSERT( ps != NULL );

  rv = lfds700_queue_dequeue( &rs->qs, &qe, ps );

  if( rv == 1 )
  {
    re = LFDS700_QUEUE_GET_VALUE_FROM_ELEMENT( *qe );
    re->qe_use = (struct lfds700_queue_element *) qe;
    if( key != NULL )
      *key = re->key;
    if( value != NULL )
      *value = re->value;
    LFDS700_FREELIST_SET_VALUE_IN_ELEMENT( re->fe, re );
    lfds700_freelist_push( &rs->fs, &re->fe, ps );
  }

  return( rv );
}

