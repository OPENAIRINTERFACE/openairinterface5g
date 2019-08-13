/***** includes *****/
#include "lfds700_ringbuffer_internal.h"

/***** private prototypes *****/
static void lfds700_ringbuffer_internal_queue_element_cleanup_callback( struct lfds700_queue_state *qs, struct lfds700_queue_element *qe, enum lfds700_misc_flag dummy_element_flag );
static void lfds700_ringbuffer_internal_freelist_element_cleanup_callback( struct lfds700_freelist_state *fs, struct lfds700_freelist_element *fe );





/****************************************************************************/
void lfds700_ringbuffer_cleanup( struct lfds700_ringbuffer_state *rs,
                                 void (*element_cleanup_callback)(struct lfds700_ringbuffer_state *rs, void *key, void *value, enum lfds700_misc_flag unread_flag) )
{
  LFDS700_PAL_ASSERT( rs != NULL );
  // TRD : element_cleanup_callback can be NULL

  if( element_cleanup_callback != NULL )
  {
    rs->element_cleanup_callback = element_cleanup_callback;
    lfds700_queue_cleanup( &rs->qs, lfds700_ringbuffer_internal_queue_element_cleanup_callback );
    lfds700_freelist_cleanup( &rs->fs, lfds700_ringbuffer_internal_freelist_element_cleanup_callback );
  }

  return;
}





/****************************************************************************/
static void lfds700_ringbuffer_internal_queue_element_cleanup_callback( struct lfds700_queue_state *qs, struct lfds700_queue_element *qe, enum lfds700_misc_flag dummy_element_flag )
{
  struct lfds700_ringbuffer_element
    *re;

  struct lfds700_ringbuffer_state
    *rs;

  LFDS700_PAL_ASSERT( qs != NULL );
  LFDS700_PAL_ASSERT( qe != NULL );
  // TRD : dummy_element can be any value in its range

  rs = (struct lfds700_ringbuffer_state *) LFDS700_QUEUE_GET_USER_STATE_FROM_STATE( *qs );
  re = (struct lfds700_ringbuffer_element *) LFDS700_QUEUE_GET_VALUE_FROM_ELEMENT( *qe );

  if( dummy_element_flag == LFDS700_MISC_FLAG_LOWERED )
    rs->element_cleanup_callback( rs, re->key, re->value, LFDS700_MISC_FLAG_RAISED );

  return;
}

//#pragma warning( default : 4100 )


/****************************************************************************/
//#pragma warning( disable : 4100 )

static void lfds700_ringbuffer_internal_freelist_element_cleanup_callback( struct lfds700_freelist_state *fs, struct lfds700_freelist_element *fe )
{
  struct lfds700_ringbuffer_element
    *re;

  struct lfds700_ringbuffer_state
    *rs;

  LFDS700_PAL_ASSERT( fs != NULL );
  LFDS700_PAL_ASSERT( fe != NULL );

  rs = (struct lfds700_ringbuffer_state *) LFDS700_FREELIST_GET_USER_STATE_FROM_STATE( *fs );
  re = (struct lfds700_ringbuffer_element *) LFDS700_FREELIST_GET_VALUE_FROM_ELEMENT( *fe );

  rs->element_cleanup_callback( rs, re->key, re->value, LFDS700_MISC_FLAG_LOWERED );

  return;
}

//#pragma warning( default : 4100 )

