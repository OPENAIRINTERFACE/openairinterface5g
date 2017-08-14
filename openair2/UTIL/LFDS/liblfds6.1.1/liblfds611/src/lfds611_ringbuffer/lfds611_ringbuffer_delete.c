#include "lfds611_ringbuffer_internal.h"





/****************************************************************************/
void lfds611_ringbuffer_delete( struct lfds611_ringbuffer_state *rs, void (*user_data_delete_function)(void *user_data, void *user_state), void *user_state )
{
  assert( rs != NULL );
  // TRD : user_data_delete_function can be NULL
  // TRD : user_state can be NULL

  lfds611_queue_delete( rs->qs, NULL, NULL );

  lfds611_freelist_delete( rs->fs, user_data_delete_function, user_state );

  lfds611_liblfds_aligned_free( rs );

  return;
}

