#include <stdio.h>
#include <immintrin.h>
#include "../../nrLDPC_types.h"
#include "../../nrLDPC_init.h"

#include "../../nrLDPC_bnProc.h"
#include "cnProc_gen_avx2.h"

int main(int argc, char *argv [])
{
 
  // Z=384, R=1/3
  nrLDPC_cnProc_BG1_generator_AVX2(384,0);


  return(0);
}
