/***** includes *****/
#include "lfds700_ringbuffer_internal.h"

/***** private prototypes *****/
static void lfds700_ringbuffer_internal_validate( struct lfds700_ringbuffer_state *rs, struct lfds700_misc_validation_info *vi, enum lfds700_misc_validity *lfds700_queue_validity, enum lfds700_misc_validity *lfds700_freelist_validity );



/****************************************************************************/
void lfds700_ringbuffer_query( struct lfds700_ringbuffer_state *rs, enum lfds700_ringbuffer_query query_type, void *query_input, void *query_output )
{
  LFDS700_PAL_ASSERT( rs != NULL );
  // TRD : query_type can be any value in its range

  LFDS700_MISC_BARRIER_LOAD;

  switch( query_type )
  {
    case LFDS700_RINGBUFFER_QUERY_SINGLETHREADED_GET_COUNT:
      LFDS700_PAL_ASSERT( query_input == NULL );
      LFDS700_PAL_ASSERT( query_output != NULL );

      lfds700_queue_query( &rs->qs, LFDS700_QUEUE_QUERY_SINGLETHREADED_GET_COUNT, NULL, query_output );
    break;

    case LFDS700_RINGBUFFER_QUERY_SINGLETHREADED_VALIDATE:
      // TRD : query_input can be NULL
      LFDS700_PAL_ASSERT( query_output != NULL );

      lfds700_ringbuffer_internal_validate( rs, (struct lfds700_misc_validation_info *) query_input, (enum lfds700_misc_validity *) query_output, ((enum lfds700_misc_validity *) query_output)+1 );
    break;
  }

  return;
}





/****************************************************************************/
static void lfds700_ringbuffer_internal_validate( struct lfds700_ringbuffer_state *rs, struct lfds700_misc_validation_info *vi, enum lfds700_misc_validity *lfds700_queue_validity, enum lfds700_misc_validity *lfds700_freelist_validity )
{
  LFDS700_PAL_ASSERT( rs != NULL );
  // TRD : vi can be NULL
  LFDS700_PAL_ASSERT( lfds700_queue_validity != NULL );
  LFDS700_PAL_ASSERT( lfds700_freelist_validity != NULL );

  if( vi == NULL )
  {
    lfds700_queue_query( &rs->qs, LFDS700_QUEUE_QUERY_SINGLETHREADED_VALIDATE, NULL, lfds700_queue_validity );
    lfds700_freelist_query( &rs->fs, LFDS700_FREELIST_QUERY_SINGLETHREADED_VALIDATE, NULL, lfds700_freelist_validity );
  }

  if( vi != NULL )
  {
    struct lfds700_misc_validation_info
      freelist_vi,
      queue_vi;

    queue_vi.min_elements = 0;
    freelist_vi.min_elements = 0;
    queue_vi.max_elements = vi->max_elements;
    freelist_vi.max_elements = vi->max_elements;

    lfds700_queue_query( &rs->qs, LFDS700_QUEUE_QUERY_SINGLETHREADED_VALIDATE, &queue_vi, lfds700_queue_validity );
    lfds700_freelist_query( &rs->fs, LFDS700_FREELIST_QUERY_SINGLETHREADED_VALIDATE, &freelist_vi, lfds700_freelist_validity );
  }

  return;
}

