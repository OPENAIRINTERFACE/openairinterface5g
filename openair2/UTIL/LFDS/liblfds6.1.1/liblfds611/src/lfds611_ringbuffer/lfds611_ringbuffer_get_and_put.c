#include "lfds611_ringbuffer_internal.h"





/****************************************************************************/
struct lfds611_freelist_element *lfds611_ringbuffer_get_read_element( struct lfds611_ringbuffer_state *rs, struct lfds611_freelist_element **fe )
{
  assert( rs != NULL );
  assert( fe != NULL );

  lfds611_queue_dequeue( rs->qs, (void **) fe );

  return( *fe );
}





/****************************************************************************/
struct lfds611_freelist_element *lfds611_ringbuffer_get_write_element( struct lfds611_ringbuffer_state *rs, struct lfds611_freelist_element **fe, int *overwrite_flag )
{
  assert( rs != NULL );
  assert( fe != NULL );
  // TRD : overwrite_flag can be NULL

  /* TRD : we try to obtain an element from the lfds611_freelist
           if we can, we populate it and add it to the lfds611_queue

           if we cannot, then the lfds611_ringbuffer is full
           so instead we grab the current read element and
           use that instead

           dequeue may fail since the lfds611_queue may be emptied
           during our dequeue attempt

           so what we actually do here is a loop, attempting
           the lfds611_freelist and if it fails then a dequeue, until
           we obtain an element

           once we have an element, we lfds611_queue it

           you may be wondering why this operation is in a loop
           remember - these operations are lock-free; anything
           can happen in between

           so for example the pop could fail because the lfds611_freelist
           is empty; but by the time we go to get an element from
           the lfds611_queue, the whole lfds611_queue has been emptied back into
           the lfds611_freelist!

           if overwrite_flag is provided, we set it to 0 if we
           obtained a new element from the lfds611_freelist, 1 if we
           stole an element from the lfds611_queue
  */

  do {
    if( overwrite_flag != NULL )
      *overwrite_flag = 0;

    lfds611_freelist_pop( rs->fs, fe );

    if( *fe == NULL ) {
      lfds611_ringbuffer_get_read_element( rs, fe );

      if( overwrite_flag != NULL and *fe != NULL )
        *overwrite_flag = 1;
    }
  } while( *fe == NULL );

  return( *fe );
}





/****************************************************************************/
void lfds611_ringbuffer_put_read_element( struct lfds611_ringbuffer_state *rs, struct lfds611_freelist_element *fe )
{
  assert( rs != NULL );
  assert( fe != NULL );

  lfds611_freelist_push( rs->fs, fe );

  return;
}





/****************************************************************************/
void lfds611_ringbuffer_put_write_element( struct lfds611_ringbuffer_state *rs, struct lfds611_freelist_element *fe )
{
  assert( rs != NULL );
  assert( fe != NULL );

  lfds611_queue_enqueue( rs->qs, fe );

  return;
}

