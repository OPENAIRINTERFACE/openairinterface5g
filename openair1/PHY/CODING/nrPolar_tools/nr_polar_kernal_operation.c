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
	__m256i A,B,C,E, OUT;
	
	uint32_t dTest[8];
	uint32_t jiArray[8];
	uint32_t iArray[8];
	uint32_t uArray[8];
	uint32_t k;	
	uint32_t toCheck[8];

	for(i=0; i<N; i+=8)
        {
		iArray[0]=i;
		iArray[1]=i+1;
		iArray[2]=i+2;
                iArray[3]=i+3; 
		iArray[4]=i+4;
                iArray[5]=i+5; 
		iArray[6]=i+6;
                iArray[7]=i+7; 

		OUT=_mm256_setzero_si256();
                for(j=0; j<N; j++)
		{
			//initialisation
			jiArray[0]=j-i;
        	        jiArray[1]=j-(i+1);
                	jiArray[2]=j-(i+2);
                	jiArray[3]=j-(i+3);
                	jiArray[4]=j-(i+4);
                	jiArray[5]=j-(i+5);
                	jiArray[6]=j-(i+6);
                	jiArray[7]=j-(i+7);

			uArray[0]=(uint32_t)u[j];
                        uArray[1]=(uint32_t)u[j];
                        uArray[2]=(uint32_t)u[j];
                        uArray[3]=(uint32_t)u[j];
                        uArray[4]=(uint32_t)u[j];
                        uArray[5]=(uint32_t)u[j];
                        uArray[6]=(uint32_t)u[j];
                        uArray[7]=(uint32_t)u[j];
		
			A=_mm256_loadu_si256((__m256i const*)jiArray);
			B=_mm256_loadu_si256((__m256i const*)iArray);
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
		_mm256_storeu_si256((__m256i*)&dTest, OUT);

		for(k=0; k<8; k++)
                {	
		        d[i+k]=(uint8_t)dTest[k]; //Conv from 32 to 8
                }

	}
*/
}
