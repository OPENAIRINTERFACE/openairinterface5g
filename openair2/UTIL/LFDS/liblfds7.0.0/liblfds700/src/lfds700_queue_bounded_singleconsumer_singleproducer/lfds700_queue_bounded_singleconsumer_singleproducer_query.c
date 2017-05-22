/***** includes *****/
#include "lfds700_queue_bounded_singleconsumer_singleproducer_internal.h"

/***** private prototypes *****/
static void lfds700_queue_bss_internal_validate( struct lfds700_queue_bss_state *qbsss, struct lfds700_misc_validation_info *vi, enum lfds700_misc_validity *lfds700_validity );





/****************************************************************************/
void lfds700_queue_bss_query( struct lfds700_queue_bss_state *qbsss, enum lfds700_queue_bss_query query_type, void *query_input, void *query_output )
{
  LFDS700_PAL_ASSERT( qbsss != NULL );
  // TRD : query_type can be any value in its range

  switch( query_type )
  {
    case LFDS700_QUEUE_BSS_QUERY_GET_POTENTIALLY_INACCURATE_COUNT:
      LFDS700_PAL_ASSERT( query_input == NULL );
      LFDS700_PAL_ASSERT( query_output != NULL );

      LFDS700_MISC_BARRIER_LOAD;

      *(lfds700_pal_uint_t *) query_output = +( qbsss->write_index - qbsss->read_index );
      if( qbsss->read_index > qbsss->write_index )
        *(lfds700_pal_uint_t *) query_output = qbsss->number_elements - *(lfds700_pal_uint_t *) query_output;
    break;

    case LFDS700_QUEUE_BSS_QUERY_VALIDATE:
      // TRD : query_input can be NULL
      LFDS700_PAL_ASSERT( query_output != NULL );

      lfds700_queue_bss_internal_validate( qbsss, (struct lfds700_misc_validation_info *) query_input, (enum lfds700_misc_validity *) query_output );
    break;
  }

  return;
}





/****************************************************************************/
static void lfds700_queue_bss_internal_validate( struct lfds700_queue_bss_state *qbsss, struct lfds700_misc_validation_info *vi, enum lfds700_misc_validity *lfds700_validity )
{
  LFDS700_PAL_ASSERT( qbsss != NULL );
  // TRD : vi can be NULL
  LFDS700_PAL_ASSERT( lfds700_validity != NULL );

  *lfds700_validity = LFDS700_MISC_VALIDITY_VALID;

  if( vi != NULL )
  {
    lfds700_pal_uint_t
      number_elements;

    lfds700_queue_bss_query( qbsss, LFDS700_QUEUE_BSS_QUERY_GET_POTENTIALLY_INACCURATE_COUNT, NULL, (void *) &number_elements );

    if( number_elements < vi->min_elements )
      *lfds700_validity = LFDS700_MISC_VALIDITY_INVALID_MISSING_ELEMENTS;

    if( number_elements > vi->max_elements )
      *lfds700_validity = LFDS700_MISC_VALIDITY_INVALID_ADDITIONAL_ELEMENTS;
  }

  return;
}

