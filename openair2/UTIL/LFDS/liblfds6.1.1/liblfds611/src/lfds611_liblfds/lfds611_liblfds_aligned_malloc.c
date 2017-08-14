#include "lfds611_liblfds_internal.h"





/****************************************************************************/
void *lfds611_liblfds_aligned_malloc( size_t size, size_t align_in_bytes )
{
  void
  *original_memory,
  *memory;

  size_t
  offset;

  // TRD : size can be any value in its range
  // TRD : align_in_bytes can be any value in its range

  original_memory = memory = lfds611_abstraction_malloc( size + sizeof(void *) + align_in_bytes );

  if( memory != NULL ) {
    memory = (void **) memory + 1;
    offset = align_in_bytes - (size_t) memory % align_in_bytes;
    memory = (unsigned char *) memory + offset;
    *( (void **) memory - 1 ) = original_memory;
  }

  return( memory );
}

