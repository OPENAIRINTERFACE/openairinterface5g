/***** includes *****/
#include "internal.h"





/****************************************************************************/
void test_lfds700_queue_bss( struct lfds700_list_asu_state *list_of_logical_processors )
{
  assert( list_of_logical_processors != NULL );
  // TRD : memory_in_megabytes can be any value in its range

  printf( "\n"
          "Queue (bounded, single consumer, single producer) Tests\n"
          "=======================================================\n" );

  // TRD : no alignment checks are required for queue_bss
  test_lfds700_queue_bss_enqueuing();
  test_lfds700_queue_bss_dequeuing();
  test_lfds700_queue_bss_enqueuing_and_dequeuing( list_of_logical_processors );

  return;
}

