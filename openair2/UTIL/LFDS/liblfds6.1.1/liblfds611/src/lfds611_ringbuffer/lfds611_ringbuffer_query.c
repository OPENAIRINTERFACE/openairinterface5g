#include "lfds611_ringbuffer_internal.h"





/****************************************************************************/
//#pragma warning( disable : 4100 )

void lfds611_ringbuffer_query( struct lfds611_ringbuffer_state *rs, enum lfds611_ringbuffer_query_type query_type, void *query_input, void *query_output )
{
  assert( rs != NULL );
  // TRD : query_type can be any value in its range
  // TRD : query_input can be NULL
  assert( query_output != NULL );

  switch( query_type ) {
  case LFDS611_RINGBUFFER_QUERY_VALIDATE:
    // TRD : query_input can be NULL

    lfds611_ringbuffer_internal_validate( rs, (struct lfds611_validation_info *) query_input, (enum lfds611_data_structure_validity *) query_output,
                                          ((enum lfds611_data_structure_validity *) query_output)+2 );
    break;
  }

  return;
}

//#pragma warning( default : 4100 )





/****************************************************************************/
void lfds611_ringbuffer_internal_validate( struct lfds611_ringbuffer_state *rs, struct lfds611_validation_info *vi, enum lfds611_data_structure_validity *lfds611_queue_validity,
    enum lfds611_data_structure_validity *lfds611_freelist_validity )
{
  assert( rs != NULL );
  // TRD : vi can be NULL
  assert( lfds611_queue_validity != NULL );
  assert( lfds611_freelist_validity != NULL );

  lfds611_queue_query( rs->qs, LFDS611_QUEUE_QUERY_VALIDATE, vi, lfds611_queue_validity );

  if( vi != NULL ) {
    struct lfds611_validation_info
        lfds611_freelist_vi;

    lfds611_atom_t
    total_elements;

    lfds611_freelist_query( rs->fs, LFDS611_FREELIST_QUERY_ELEMENT_COUNT, NULL, (void *) &total_elements );
    lfds611_freelist_vi.min_elements = total_elements - vi->max_elements;
    lfds611_freelist_vi.max_elements = total_elements - vi->min_elements;
    lfds611_freelist_query( rs->fs, LFDS611_FREELIST_QUERY_VALIDATE, (void *) &lfds611_freelist_vi, (void *) lfds611_freelist_validity );
  }

  if( vi == NULL )
    lfds611_freelist_query( rs->fs, LFDS611_FREELIST_QUERY_VALIDATE, NULL, (void *) lfds611_freelist_validity );

  return;
}

