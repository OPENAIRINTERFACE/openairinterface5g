#include "internal.h"





/****************************************************************************/
int main( int argc, char **argv )
{
  enum lfds611_test_operation
  operation = UNKNOWN;

  unsigned int
  loop,
  iterations = 1;

  assert( argc >= 1 );
  assert( argv != NULL );

  if( argc == 1 or argc >= 4 )
    operation = HELP;

  if( operation == UNKNOWN ) {
    if( 0 == strcmp(*(argv+1), "test") ) {
      operation = TEST;

      // TRD : sscanf() may fail, but iterations is initialised to 1, so it's okay
      if( argc == 3 )
        sscanf( *(argv+2), "%u", &iterations );
    }

    if( 0 == strcmp(*(argv+1), "benchmark") ) {
      operation = BENCHMARK;

      // TRD : sscanf() may fail, but iterations is initialised to 1, so it's okay
      if( argc == 3 )
        sscanf( *(argv+2), "%u", &iterations );
    }
  }

  switch( operation ) {
  case UNKNOWN:
  case HELP:
    printf( "test [test|benchmark] [iterations]\n"
            "  test       : run the test suite\n"
            "  benchmark  : run the benchmark suite\n"
            "  iterations : optional, default is 1\n" );
    break;

  case TEST:
    for( loop = 1 ; loop < iterations+1 ; loop++ ) {
      printf( "\n"
              "Test Iteration %02u\n"
              "=================\n", loop );

      test_lfds611_abstraction();
      test_lfds611_freelist();
      test_lfds611_queue();
      test_lfds611_ringbuffer();
      test_lfds611_slist();
      test_lfds611_stack();
    }

    break;

  case BENCHMARK:
    for( loop = 1 ; loop < iterations+1 ; loop++ ) {
      printf( "\n"
              "Benchmark Iteration %02u\n"
              "========================\n", loop );

      benchmark_lfds611_freelist();
      benchmark_lfds611_queue();
      benchmark_lfds611_ringbuffer();
      benchmark_lfds611_stack();
    }

    break;
  }

  return( EXIT_SUCCESS );
}

