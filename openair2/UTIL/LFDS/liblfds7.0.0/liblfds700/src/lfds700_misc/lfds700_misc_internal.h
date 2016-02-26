/***** the library wide include file *****/
#include "../liblfds700_internal.h"

/***** defines *****/
#define EXPONENTIAL_BACKOFF_TIMESLOT_LENGTH_IN_INCS_FOR_CAS    8
#define EXPONENTIAL_BACKOFF_TIMESLOT_LENGTH_IN_INCS_FOR_DWCAS  16

/***** private prototypes *****/
void lfds700_misc_prng_internal_big_slow_high_quality_init( int long long unsigned seed );

