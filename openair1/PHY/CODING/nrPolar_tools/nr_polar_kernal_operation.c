#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <stdint.h>

#include <immintrin.h>

void nr_polar_kernal_operation(uint8_t *u, uint8_t *d, uint16_t N)
{
	// Martino's algorithm to avoid multiplication for the generating matrix of polar codes
	
	uint32_t i,j;
	
	for(i=0; i<N; i++) // Create the elements of d=u*G_N ...
    	{
        	d[i]=0;
        	for(j=0; j<N; j++) // ... looking at all the elements of u
        	{
			d[i]=d[i] ^ (!( (j-i)& i ))*u[j];
        	}
    	}


/*
 * It works, but there are too many moves from memory and it's slow. With AVX-512 it could be done faster
 *
	__m256i A,B,C,E,U, OUT;
	__m256i inc;
	uint32_t dTest[8];
	uint32_t uArray[8];
	uint32_t k;	
	uint32_t toCheck[8];

	uint32_t incArray[8];
	for(k=0; k<8; k++)
		incArray[k]=k; //0,1, ... 7

	inc=_mm256_loadu_si256((__m256i const*)incArray);
	
	for(i=0; i<N; i+=8)
        {

		B=_mm256_set1_epi32((int)i); // i, ... i
		B=_mm256_add_epi32(B, inc); //i, i+1, ... i+7
		
		OUT=_mm256_setzero_si256();
                
		for(j=0; j<N; j++)
		{
			//initialisation
			A=_mm256_set1_epi32((int)(j)); //j ...
			A=_mm256_sub_epi32(A, B); //(j-i), (j-(i+1)), ... (j-(i+7))  
			
			U=_mm256_set1_epi32((int)u[j]);
			_mm256_storeu_si256((__m256i*)uArray, U); //u(j) ... u(j) for the maskload

			C=_mm256_and_si256(A, B); //mask: if zero, then add

			_mm256_storeu_si256((__m256i*)toCheck, C);
			for(k=0; k<8; k++)
                        {
				toCheck[k]=!toCheck[k] << 31;
			}
			C=_mm256_loadu_si256((__m256i const*)toCheck); //mask: if 1, add

			E=_mm256_maskload_epi32((int const*)uArray, C);
			OUT=_mm256_xor_si256(OUT, E); //32 bit x 8

		}
		_mm256_storeu_si256((__m256i*)dTest, OUT);

		for(k=0; k<8; k++)
                {	
		        d[i+k]=(uint8_t)dTest[k]; //Conv from 32 to 8
                }

	}
*/
}
