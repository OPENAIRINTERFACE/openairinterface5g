#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <stdint.h>

#include "PHY/sse_intrin.h"

void nr_polar_kernal_operation(uint8_t *u, uint8_t *d, uint16_t N)
{
	// Martino's algorithm to avoid multiplication for the generating matrix of polar codes
	
	uint32_t i,j;

	simde__m256i A,B,C,D,E,U,zerosOnly, OUT;
	simde__m256i inc;
	uint32_t dTest[8];
	uint32_t uArray[8];
	uint32_t k;	
	uint32_t incArray[8];
	
	//initialisation
	for(k=0; k<8; k++)
		incArray[k]=k;
	inc=simde_mm256_loadu_si256((simde__m256i const*)incArray); // 0, 1, ..., 7 to increase
	
	zerosOnly=simde_mm256_setzero_si256(); // for comparison

	for(i=0; i<N; i+=8)
        {
		B=simde_mm256_set1_epi32((int)i); // i, ..., i
		B=simde_mm256_add_epi32(B, inc); // i, i+1, ..., i+7
		
		OUT=simde_mm256_setzero_si256(); // it will contain the result of all the XORs for the d(i)s
                
		for(j=0; j<N; j++)
		{
			A=simde_mm256_set1_epi32((int)(j)); //j, j,  ..., j
			A=simde_mm256_sub_epi32(A, B); //(j-i), (j-(i+1)), ... (j-(i+7))  
			
			U=simde_mm256_set1_epi32((int)u[j]);
			simde_mm256_storeu_si256((simde__m256i*)uArray, U); //u(j) ... u(j) for the maskload

			C=simde_mm256_and_si256(A, B); //(j-i)&i -> If zero, then XOR with the u(j)
			D=simde_mm256_cmpeq_epi32(C, zerosOnly); // compare with zero and use the result as mask
			
			E=simde_mm256_maskload_epi32((int const*)uArray, D); // load only some u(j)s for the XOR
			OUT=simde_mm256_xor_si256(OUT, E); //32 bit x 8

		}
		simde_mm256_storeu_si256((simde__m256i*)dTest, OUT);

		for(k=0; k<8; k++) // Conversion from 32 bits to 8 bits
                {	
		        d[i+k]=(uint8_t)dTest[k]; // With AVX512 there is an intrinsic to do it
                }

	}

}
