
#include <stdio.h>
#include <stdint.h>
#define NB_R  3
void nrLDPC_cnProc_BG1_generator_AVX512(int);
void nrLDPC_cnProc_BG2_generator_AVX512(int);
int main()
{
 int R[NB_R]={0,1,2};
        for(int i=0; i<NB_R;i++){

        nrLDPC_cnProc_BG1_generator_AVX512(R[i]);
	nrLDPC_cnProc_BG2_generator_AVX512(R[i]);

	}


  return(0);
}

