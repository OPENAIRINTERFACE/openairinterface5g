/***** includes *****/
#include "internal.h"





/****************************************************************************/
void *util_aligned_malloc( lfds700_pal_uint_t size, lfds700_pal_uint_t align_in_bytes )
{
  lfds700_pal_uint_t
    offset;

  void
    *memory,
    *original_memory;

  // TRD : size can be any value in its range
  // TRD : align_in_bytes can be any value in its range

  /* TRD : helper function to provide aligned allocations
           no porting required
  */

  original_memory = memory = util_malloc_wrapper( size + sizeof(void *) + align_in_bytes );

  if( memory != NULL )
  {
    memory = (void **) memory + 1;
    offset = align_in_bytes - (lfds700_pal_uint_t) memory % align_in_bytes;
    memory = (char unsigned *) memory + offset;
    *( (void **) memory - 1 ) = original_memory;
  }

  return( memory );
}





/****************************************************************************/
void util_aligned_free( void *memory )
{
  assert( memory != NULL );

  // TRD : the "void *" stored above memory points to the root of the allocation
  free( *( (void **) memory - 1 ) );

  return;
}





/****************************************************************************/
void *util_malloc_wrapper( lfds700_pal_uint_t size )
{
  void
    *memory;

  // TRD : size can be any value in its range

  memory = malloc( size );

  if( memory == NULL )
  {
    puts( "malloc() failed, exiting." );
    exit( EXIT_FAILURE );
  }

  return( memory );
}

