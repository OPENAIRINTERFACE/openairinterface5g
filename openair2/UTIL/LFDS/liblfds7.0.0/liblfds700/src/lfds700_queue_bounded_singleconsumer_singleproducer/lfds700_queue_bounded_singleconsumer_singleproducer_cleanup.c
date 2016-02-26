/***** includes *****/
#include "lfds700_queue_bounded_singleconsumer_singleproducer_internal.h"





/****************************************************************************/
void lfds700_queue_bss_cleanup( struct lfds700_queue_bss_state *qbsss,
                                void (*element_cleanup_callback)(struct lfds700_queue_bss_state *qbsss, void *key, void *value) )
{
  int long long unsigned
    loop;

  struct lfds700_queue_bss_element
    *qbsse;

  LFDS700_PAL_ASSERT( qbsss != NULL );
  // TRD : element_cleanup_callback can be NULL

  if( element_cleanup_callback != NULL )
    for( loop = qbsss->read_index ; loop < qbsss->read_index + qbsss->number_elements ; loop++ )
    {
      qbsse = qbsss->element_array + (loop % qbsss->number_elements);
      element_cleanup_callback( qbsss, qbsse->key, qbsse->value );
    }

  return;
}

