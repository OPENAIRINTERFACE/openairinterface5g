/***** includes *****/
#include "internal.h"





/****************************************************************************/
void test_lfds700_queue_bss_dequeuing()
{
  enum lfds700_misc_validity
    dvs = LFDS700_MISC_VALIDITY_VALID;

  lfds700_pal_uint_t
    loop;

  struct lfds700_queue_bss_element
    element_array[128];

  struct lfds700_queue_bss_state
    qs;

  struct lfds700_misc_validation_info
    vi;

  void
    *value;

  /* TRD : create an empty queue
           enqueue 128 elements
           then dequeue the elements, in the same thread - we're API testing
           it's a single producer queue, so we just do this in our current thread
           since we're enqueuing and dequeuing in the same thread,
           
  */

  internal_display_test_name( "Dequeuing" );

  lfds700_queue_bss_init_valid_on_current_logical_core( &qs, element_array, 128, NULL );

  for( loop = 0 ; loop < 127 ; loop++ )
    lfds700_queue_bss_enqueue( &qs, NULL, (void *) loop );

  for( loop = 0 ; loop < 127 ; loop++ )
  {
    lfds700_queue_bss_dequeue( &qs, NULL, &value );
    if( (lfds700_pal_uint_t) value != 127 - loop )
      dvs = LFDS700_MISC_VALIDITY_INVALID_TEST_DATA;
  }

  vi.min_elements = vi.max_elements = 0;

  lfds700_queue_bss_query( &qs, LFDS700_QUEUE_BSS_QUERY_VALIDATE, &vi, &dvs );

  lfds700_queue_bss_cleanup( &qs, NULL );

  internal_display_test_result( 1, "queue_bss", dvs );

  return;
}

