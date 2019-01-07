/***** includes *****/
#include "lfds700_queue_bounded_singleconsumer_singleproducer_internal.h"





/****************************************************************************/
int lfds700_queue_bss_enqueue( struct lfds700_queue_bss_state *qbsss, void *key, void *value )
{
  int
    rv = 0;

  struct lfds700_queue_bss_element
    *qbsse;

  LFDS700_PAL_ASSERT( qbsss != NULL );
  // TRD : key can be NULL
  // TRD : value can be NULL

  LFDS700_MISC_BARRIER_LOAD;

  if( ( (qbsss->write_index+1) & qbsss->mask ) != qbsss->read_index )
  {
    qbsse = qbsss->element_array + qbsss->write_index;

    qbsse->key = key;
    qbsse->value = value;

    LFDS700_MISC_BARRIER_STORE;

    qbsss->write_index = (qbsss->write_index + 1) & qbsss->mask;

    rv = 1;
  }

  return( rv );
}

