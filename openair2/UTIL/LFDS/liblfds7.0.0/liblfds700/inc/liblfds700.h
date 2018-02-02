#ifndef LIBLFDS700_H

  /***** defines *****/
  #define LIBLFDS700_H

  /***** pragmas on *****/
//  #pragma warning( disable : 4324 )                                          // TRD : 4324 disables MSVC warnings for structure alignment padding due to alignment specifiers

//  #pragma prefast( disable : 28113 28182 28183, "blah" )

  /***** includes *****/
  #include "liblfds700/lfds700_porting_abstraction_layer_compiler.h"
  #include "liblfds700/lfds700_porting_abstraction_layer_operating_system.h"
  #include "liblfds700/lfds700_porting_abstraction_layer_processor.h"

  #include "liblfds700/lfds700_misc.h"                                       // TRD : everything after depends on misc
  #include "liblfds700/lfds700_btree_addonly_unbalanced.h"                   // TRD : hash_addonly depends on btree_addonly_unbalanced
  #include "liblfds700/lfds700_freelist.h"
  #include "liblfds700/lfds700_hash_addonly.h"
  #include "liblfds700/lfds700_list_addonly_ordered_singlylinked.h"
  #include "liblfds700/lfds700_list_addonly_singlylinked_unordered.h"
  #include "liblfds700/lfds700_queue.h"
  #include "liblfds700/lfds700_queue_bounded_singleconsumer_singleproducer.h"
  #include "liblfds700/lfds700_ringbuffer.h"
  #include "liblfds700/lfds700_stack.h"

  /***** pragmas off *****/
//  #pragma warning( default : 4324 )

#endif

