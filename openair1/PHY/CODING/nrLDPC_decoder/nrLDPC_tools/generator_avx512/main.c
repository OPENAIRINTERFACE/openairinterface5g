#include <stdio.h>
#include <immintrin.h>
//#include "../nrLDPCdecoder_defs.h"
#include "../../nrLDPC_types.h"
#include "../../nrLDPC_init.h"
#include "../../nrLDPC_bnProc.h"
#include "cnProc_gen_avx512.h"

int main(int argc, char *argv [])
{
  
  // Z=384, R=1/3
  nrLDPC_cnProc_BG1_generator_AVX512(384,0);
	
  return(0);
}
