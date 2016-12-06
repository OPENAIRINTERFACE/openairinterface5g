/***** includes *****/
#include "lfds700_queue_bounded_singleconsumer_singleproducer_internal.h"





/****************************************************************************/
int lfds700_queue_bss_dequeue( struct lfds700_queue_bss_state *qbsss, void **key, void **value )
{
  int
    rv = 0;

  struct lfds700_queue_bss_element
    *qbsse;

  LFDS700_PAL_ASSERT( qbsss != NULL );
  // TRD : key can be NULL
  // TRD : value can be NULL

  LFDS700_MISC_BARRIER_LOAD;

  if( qbsss->read_index != qbsss->write_index )
  {
    qbsse = qbsss->element_array + qbsss->read_index;

    if( key != NULL )
      *key = qbsse->key;

    if( value != NULL )
      *value = qbsse->value;

    qbsss->read_index = (qbsss->read_index + 1) & qbsss->mask;

    LFDS700_MISC_BARRIER_STORE;

    rv = 1;
  }

  return( rv );
}

