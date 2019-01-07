/***** includes *****/
#include "internal.h"





/****************************************************************************/
void test_lfds700_queue_bss_enqueuing()
{
  enum lfds700_misc_validity
    dvs = LFDS700_MISC_VALIDITY_VALID;

  int
    rv;

  lfds700_pal_uint_t
    loop;

  struct lfds700_queue_bss_element
    element_array[128];

  struct lfds700_queue_bss_state
    qs;

  struct lfds700_misc_validation_info
    vi;

  /* TRD : create an empty queue
           enqueue 128 elements
           it's a single producer queue, so we just do this in our current thread
           it's an API test
  */

  internal_display_test_name( "Enqueuing" );

  lfds700_queue_bss_init_valid_on_current_logical_core( &qs, element_array, 128, NULL );

  for( loop = 0 ; loop < 127 ; loop++ )
    if( 1 != lfds700_queue_bss_enqueue(&qs, NULL, (void *) loop) )
      dvs = LFDS700_MISC_VALIDITY_INVALID_TEST_DATA;

  // TRD : at this point enqueuing one more should return 0
  rv = lfds700_queue_bss_enqueue( &qs, NULL, (void *) loop );

  if( rv != 0 )
    dvs = LFDS700_MISC_VALIDITY_INVALID_TEST_DATA;

  vi.min_elements = vi.max_elements = 127;

  lfds700_queue_bss_query( &qs, LFDS700_QUEUE_BSS_QUERY_VALIDATE, &vi, &dvs );

  lfds700_queue_bss_cleanup( &qs, NULL );

  internal_display_test_result( 1, "queue_bss", dvs );

  return;
}

