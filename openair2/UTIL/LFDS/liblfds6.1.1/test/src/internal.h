/***** ANSI includes *****/
/* TRD : _GNU_SOURCE is required by sched.h for pthread_attr_setaffinity_np, CPU_ZERO and CPU_SET
         however it has to be defined very early as even the ANSI headers pull in stuff
         which uses _GNU_SOURCE and which I think must be protected against multiple inclusion,
         which basically means if you set it too late, it's not seen, because the headers
         have already been parsed with _GNU_SOURCE unset
*/

#define _GNU_SOURCE

#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/***** internal includes *****/
#include "abstraction.h"

/***** external includes *****/
#include "liblfds611.h"

/***** defines *****/
#define and &&
#define or  ||

#define RAISED   1
#define LOWERED  0

#define NO_FLAGS 0x0

/***** enums *****/
enum lfds611_test_operation {
  UNKNOWN,
  HELP,
  TEST,
  BENCHMARK
};

/***** structs *****/
#include "structures.h"

/***** prototypes *****/
int main( int argc, char **argv );

void internal_display_test_name( char *test_name );
void internal_display_test_result( unsigned int number_name_dvs_pairs, ... );
void internal_display_lfds611_data_structure_validity( enum lfds611_data_structure_validity dvs );

void benchmark_lfds611_freelist( void );
thread_return_t CALLING_CONVENTION benchmark_lfds611_freelist_thread_pop_and_push( void *freelist_benchmark );

void benchmark_lfds611_queue( void );
thread_return_t CALLING_CONVENTION benchmark_lfds611_queue_thread_delfds611_queue_and_enqueue( void *queue_benchmark );

void benchmark_lfds611_ringbuffer( void );
thread_return_t CALLING_CONVENTION benchmark_lfds611_ringbuffer_thread_write_and_read( void *ringbuffer_benchmark );

void benchmark_lfds611_stack( void );
thread_return_t CALLING_CONVENTION benchmark_lfds611_stack_thread_push_and_pop( void *stack_benchmark );

void test_lfds611_abstraction( void );
void abstraction_test_increment( void );
thread_return_t CALLING_CONVENTION abstraction_test_internal_thread_increment( void *shared_counter );
thread_return_t CALLING_CONVENTION abstraction_test_internal_thread_atomic_increment( void *shared_counter );
void abstraction_test_cas( void );
thread_return_t CALLING_CONVENTION abstraction_test_internal_thread_cas( void *abstraction_test_cas_state );
void abstraction_test_dcas( void );
thread_return_t CALLING_CONVENTION abstraction_test_internal_thread_dcas( void *abstraction_test_dcas_state );

void test_lfds611_freelist( void );
void freelist_test_internal_popping( void );
int freelist_test_internal_popping_init( void **user_data, void *user_state );
thread_return_t CALLING_CONVENTION freelist_test_internal_thread_popping( void *freelist_test_popping_state );
void freelist_test_internal_pushing( void );
int freelist_test_internal_pushing_init( void **user_data, void *user_state );
void freelist_test_internal_pushing_delete( void *user_data, void *user_state );
thread_return_t CALLING_CONVENTION freelist_test_internal_thread_pushing( void *freelist_test_pushing_state );
void freelist_test_internal_popping_and_pushing( void );
thread_return_t CALLING_CONVENTION freelist_test_internal_thread_popping_and_pushing_start_popping( void *freelist_test_popping_and_pushing_state );
thread_return_t CALLING_CONVENTION freelist_test_internal_thread_popping_and_pushing_start_pushing( void *freelist_test_popping_and_pushing_state );
void freelist_test_internal_rapid_popping_and_pushing( void );
thread_return_t CALLING_CONVENTION freelist_test_internal_thread_rapid_popping_and_pushing( void *lfds611_freelist_state );

void test_lfds611_queue( void );
void queue_test_enqueuing( void );
thread_return_t CALLING_CONVENTION queue_test_internal_thread_simple_enqueuer( void *queue_test_enqueuing_state );
void queue_test_dequeuing( void );
thread_return_t CALLING_CONVENTION queue_test_internal_thread_simple_dequeuer( void *queue_test_dequeuing_state );
void queue_test_enqueuing_and_dequeuing( void );
thread_return_t CALLING_CONVENTION queue_test_internal_thread_enqueuer_and_dequeuer( void *queue_test_rapid_enqueuing_and_dequeuing_state );
void queue_test_rapid_enqueuing_and_dequeuing( void );
thread_return_t CALLING_CONVENTION queue_test_internal_thread_rapid_enqueuer_and_dequeuer( void *queue_test_rapid_enqueuing_and_dequeuing_state );

void test_lfds611_ringbuffer( void );
void ringbuffer_test_reading( void );
thread_return_t CALLING_CONVENTION ringbuffer_test_thread_simple_reader( void *ringbuffer_test_reading_state );
void ringbuffer_test_writing( void );
thread_return_t CALLING_CONVENTION ringbuffer_test_thread_simple_writer( void *ringbuffer_test_writing_state );
void ringbuffer_test_reading_and_writing( void );
thread_return_t CALLING_CONVENTION ringbuffer_test_thread_reader_writer( void *ringbuffer_test_reading_and_writing_state );

void test_lfds611_slist( void );
void test_slist_new_delete_get( void );
thread_return_t CALLING_CONVENTION slist_test_internal_thread_new_delete_get_new_head_and_next( void *slist_test_state );
thread_return_t CALLING_CONVENTION slist_test_internal_thread_new_delete_get_delete_and_get( void *slist_test_state );
void test_slist_get_set_user_data( void );
thread_return_t CALLING_CONVENTION slist_test_internal_thread_get_set_user_data( void *slist_test_state );
void test_slist_delete_all_elements( void );

void test_lfds611_stack( void );
void stack_test_internal_popping( void );
thread_return_t CALLING_CONVENTION stack_test_internal_thread_popping( void *stack_test_popping_state );
void stack_test_internal_pushing( void );
thread_return_t CALLING_CONVENTION stack_test_internal_thread_pushing( void *stack_test_pushing_state );
void stack_test_internal_popping_and_pushing( void );
thread_return_t CALLING_CONVENTION stack_test_internal_thread_popping_and_pushing_start_popping( void *stack_test_popping_and_pushing_state );
thread_return_t CALLING_CONVENTION stack_test_internal_thread_popping_and_pushing_start_pushing( void *stack_test_popping_and_pushing_state );
void stack_test_internal_rapid_popping_and_pushing( void );
thread_return_t CALLING_CONVENTION stack_test_internal_thread_rapid_popping_and_pushing( void *stack_state );

