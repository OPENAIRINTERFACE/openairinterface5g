/***** includes *****/
#define _GNU_SOURCE
#include <assert.h>
#include <ctype.h>
#include <limits.h>
#include <math.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "../../liblfds700/inc/liblfds700.h"
#include "test_porting_abstraction_layer_operating_system.h"

/***** defines *****/
#define and &&
#define or  ||

#define NO_FLAGS 0x0

#define BITS_PER_BYTE 8

#define TEST_DURATION_IN_SECONDS             5
#define TIME_LOOP_COUNT                      10000
#define REDUCED_TIME_LOOP_COUNT              1000
#define NUMBER_OF_NANOSECONDS_IN_ONE_SECOND  1000000000LLU
#define ONE_MEGABYTE_IN_BYTES                (1024 * 1024)
#define DEFAULT_TEST_MEMORY_IN_MEGABYTES     512U
#define TEST_PAL_DEFAULT_NUMA_NODE_ID        0
#define LFDS700_TEST_VERSION_STRING          "7.0.0"
#define LFDS700_TEST_VERSION_INTEGER         700

#if( defined _KERNEL_MODE )
  #define MODE_TYPE_STRING "kernel-mode"
#endif

#if( !defined _KERNEL_MODE )
  #define MODE_TYPE_STRING "user-mode"
#endif

#if( defined NDEBUG && !defined COVERAGE && !defined TSAN )
  #define BUILD_TYPE_STRING "release"
#endif

#if( !defined NDEBUG && !defined COVERAGE && !defined TSAN )
  #define BUILD_TYPE_STRING "debug"
#endif

#if( !defined NDEBUG && defined COVERAGE && !defined TSAN )
  #define BUILD_TYPE_STRING "coverage"
#endif

#if( !defined NDEBUG && !defined COVERAGE && defined TSAN )
  #define BUILD_TYPE_STRING "threadsanitizer"
#endif

/***** enums *****/
enum flag
{
  LOWERED,
  RAISED
};

/***** structs *****/
struct test_pal_logical_processor
{
  lfds700_pal_uint_t
    logical_processor_number,
    windows_logical_processor_group_number;

  struct lfds700_list_asu_element
    lasue;
};

/***** prototypes *****/
int main( int argc, char **argv );

void internal_display_test_name( char *format_string, ... );
void internal_display_test_result( lfds700_pal_uint_t number_name_dvs_pairs, ... );
void internal_display_data_structure_validity( enum lfds700_misc_validity dvs );
void internal_show_version( void );
void internal_logical_core_id_element_cleanup_callback( struct lfds700_list_asu_state *lasus, struct lfds700_list_asu_element *lasue );

int test_pal_thread_start( test_pal_thread_state_t *thread_state, struct test_pal_logical_processor *lp, test_pal_thread_return_t (TEST_PAL_CALLING_CONVENTION *thread_function)(void *thread_user_state), void *thread_user_state );
void test_pal_thread_wait( test_pal_thread_state_t thread_state );
void test_pal_get_logical_core_ids( struct lfds700_list_asu_state *lasus );

void test_lfds700_pal_atomic( struct lfds700_list_asu_state *list_of_logical_processors, lfds700_pal_uint_t memory_in_megabytes );
  void test_lfds700_pal_atomic_cas( struct lfds700_list_asu_state *list_of_logical_processors );
  void test_lfds700_pal_atomic_dwcas( struct lfds700_list_asu_state *list_of_logical_processors );
  void test_lfds700_pal_atomic_exchange( struct lfds700_list_asu_state *list_of_logical_processors, lfds700_pal_uint_t memory_in_megabytes );

void test_lfds700_hash_a( struct lfds700_list_asu_state *list_of_logical_processors, lfds700_pal_uint_t memory_in_megabytes );
  void test_lfds700_hash_a_alignment( void );
  void test_lfds700_hash_a_fail_and_overwrite_on_existing_key( void );
  void test_lfds700_hash_a_random_adds_fail_on_existing( struct lfds700_list_asu_state *list_of_logical_processors, lfds700_pal_uint_t memory_in_megabytes );
  void test_lfds700_hash_a_random_adds_overwrite_on_existing( struct lfds700_list_asu_state *list_of_logical_processors, lfds700_pal_uint_t memory_in_megabytes );
  void test_lfds700_hash_a_iterate( void );

void test_lfds700_list_aos( struct lfds700_list_asu_state *list_of_logical_processors, lfds700_pal_uint_t memory_in_megabytes );
  void test_lfds700_list_aos_alignment( void );
  void test_lfds700_list_aos_new_ordered( struct lfds700_list_asu_state *list_of_logical_processors, lfds700_pal_uint_t memory_in_megabytes );
  void test_lfds700_list_aos_new_ordered_with_cursor( struct lfds700_list_asu_state *list_of_logical_processors, lfds700_pal_uint_t memory_in_megabytes );

void test_lfds700_list_asu( struct lfds700_list_asu_state *list_of_logical_processors, lfds700_pal_uint_t memory_in_megabytes );
  void test_lfds700_list_asu_alignment( void );
  void test_lfds700_list_asu_new_start( struct lfds700_list_asu_state *list_of_logical_processors, lfds700_pal_uint_t memory_in_megabytes );
  void test_lfds700_list_asu_new_end( struct lfds700_list_asu_state *list_of_logical_processors, lfds700_pal_uint_t memory_in_megabytes );
  void test_lfds700_list_asu_new_after( struct lfds700_list_asu_state *list_of_logical_processors, lfds700_pal_uint_t memory_in_megabytes );

void test_lfds700_btree_au( struct lfds700_list_asu_state *list_of_logical_processors, lfds700_pal_uint_t memory_in_megabytes );
  void test_lfds700_btree_au_alignment( void );
  void test_lfds700_btree_au_fail_and_overwrite_on_existing_key( void );
  void test_lfds700_btree_au_random_adds_fail_on_existing( struct lfds700_list_asu_state *list_of_logical_processors, lfds700_pal_uint_t memory_in_megabytes );
  void test_lfds700_btree_au_random_adds_overwrite_on_existing( struct lfds700_list_asu_state *list_of_logical_processors, lfds700_pal_uint_t memory_in_megabytes );

void test_lfds700_freelist( struct lfds700_list_asu_state *list_of_logical_processors, lfds700_pal_uint_t memory_in_megabytes );
  void test_lfds700_freelist_alignment( void );
  void test_lfds700_freelist_popping( struct lfds700_list_asu_state *list_of_logical_processors, lfds700_pal_uint_t memory_in_megabytes );
  void test_lfds700_freelist_pushing( struct lfds700_list_asu_state *list_of_logical_processors, lfds700_pal_uint_t memory_in_megabytes );
  void test_lfds700_freelist_popping_and_pushing( struct lfds700_list_asu_state *list_of_logical_processors, lfds700_pal_uint_t memory_in_megabytes );
  void test_lfds700_freelist_rapid_popping_and_pushing( struct lfds700_list_asu_state *list_of_logical_processors );
  void test_lfds700_freelist_pushing_array( void );

void test_lfds700_queue( struct lfds700_list_asu_state *list_of_logical_processors, lfds700_pal_uint_t memory_in_megabytes );
  void test_lfds700_queue_alignment( void );
  void test_lfds700_queue_enqueuing( struct lfds700_list_asu_state *list_of_logical_processors, lfds700_pal_uint_t memory_in_megabytes );
  void test_lfds700_queue_dequeuing( struct lfds700_list_asu_state *list_of_logical_processors, lfds700_pal_uint_t memory_in_megabytes );
  void test_lfds700_queue_enqueuing_and_dequeuing( struct lfds700_list_asu_state *list_of_logical_processors );
  void test_lfds700_queue_rapid_enqueuing_and_dequeuing( struct lfds700_list_asu_state *list_of_logical_processors, lfds700_pal_uint_t memory_in_megabytes );
  void test_lfds700_queue_enqueuing_and_dequeuing_with_free( struct lfds700_list_asu_state *list_of_logical_processors, lfds700_pal_uint_t memory_in_megabytes );
  void test_lfds700_queue_enqueuing_with_malloc_and_dequeuing_with_free( struct lfds700_list_asu_state *list_of_logical_processors );

void test_lfds700_queue_bss( struct lfds700_list_asu_state *list_of_logical_processors );
  void test_lfds700_queue_bss_enqueuing( void );
  void test_lfds700_queue_bss_dequeuing( void );
  void test_lfds700_queue_bss_enqueuing_and_dequeuing( struct lfds700_list_asu_state *list_of_logical_processors );

void test_lfds700_ringbuffer( struct lfds700_list_asu_state *list_of_logical_processors, lfds700_pal_uint_t memory_in_megabytes );
  void test_lfds700_ringbuffer_reading( struct lfds700_list_asu_state *list_of_logical_processors, lfds700_pal_uint_t memory_in_megabytes );
  void test_lfds700_ringbuffer_reading_and_writing( struct lfds700_list_asu_state *list_of_logical_processors, lfds700_pal_uint_t memory_in_megabytes );
  void test_lfds700_ringbuffer_writing( struct lfds700_list_asu_state *list_of_logical_processors, lfds700_pal_uint_t memory_in_megabytes );

void test_lfds700_stack( struct lfds700_list_asu_state *list_of_logical_processors, lfds700_pal_uint_t memory_in_megabytes );
  void test_lfds700_stack_alignment( void );
  void test_lfds700_stack_pushing( struct lfds700_list_asu_state *list_of_logical_processors, lfds700_pal_uint_t memory_in_megabytes );
  void test_lfds700_stack_popping( struct lfds700_list_asu_state *list_of_logical_processors, lfds700_pal_uint_t memory_in_megabytes );
  void test_lfds700_stack_popping_and_pushing( struct lfds700_list_asu_state *list_of_logical_processors, lfds700_pal_uint_t memory_in_megabytes );
  void test_lfds700_stack_rapid_popping_and_pushing( struct lfds700_list_asu_state *list_of_logical_processors );
  void test_lfds700_stack_pushing_array( void );

/***** late includes *****/
#include "util_cmdline.h"
#include "util_memory_helpers.h"
#include "util_thread_starter.h"

