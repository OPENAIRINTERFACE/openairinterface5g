#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <stdint.h>

#include <immintrin.h>

void nr_polar_kernal_operation(uint8_t *u, uint8_t *d, uint16_t N)
{
	// Martino's algorithm to avoid multiplication for the generating matrix of polar codes
	
	uint16_t i,j;
 
	for(i=0; i<N; i++) // Create the elements of d=u*G_N ...
    	{
        	d[i]=0;
        	for(j=0; j<N; j++) // ... looking at all the elements of u
        	{
            		d[i]=d[i] || ( (!(j-i)) | (!i) )*u[j];
        	}

		//d[i]=d[i]%2; // modulo 2
    	}

/*
	__m256i maddReg, uReg, orReg;
	__m512i maddRegConv;
	__m256i bitJIReg, bitIReg;
	uint8_t bitJI[32];
	uint8_t bitI[32];
	int sumPartial;
	uint8_t indToInit;

        for(i=0; i<N; i++) // Create the elements of d=u*G_N ...
        {
                d[i]=0;
                for(j=0; j<N; j+=32) // ... looking at all the elements of u 32 at a time
                {
                        //d[i]=d[i]+( (!(j-i)) | (!i) )*u[j];  <--- THIS IN INTRINSIC
			// Products between ( (!(j-i)) | (!i) ) and u[j] and sum all with a reduce add

			uReg = _mm256_maskz_loadu_epi8 (0xFFFFFFFF, (void const*)&u[j]); // load 32 8-bit from u
			
			//init arrays for (!(i-j)) and for (!i)
			for(indToInit=0; indToInit<32; indToInit++)
			{
				// j = j*32+indToInit
				bitJI[j*32+indToInit] = !((j*32+indToInit)-i); // (!(j-i))
				bitI[j*32+indToInit] = !i; // (!i)
			}
	 	
			bitJIReg = _mm256_maskz_loadu_epi8(0xFFFFFFFF, (void const*)bitJI); // 32x8-bit
			bitIReg = _mm256_maskz_loadu_epi8(0xFFFFFFFF, (void const*)bitI);   // 32x8-bit
			orReg=_mm256_or_si256(bitWise1, bitWise2); // (!(j-i)) | (!i)   32x8-bit
			maddReg=_mm256_maddubs_epi16(uReg, orReg); //a1*b1+a2*b2 from 32x8 to 16x16-bit
			maddRegConv= _mm512_cvtepi16_epi32(maddReg); //convert to 16x32-bit
			sumPartial = _mm512_reduce_add_epi32(maddRegConv); //sum all 16 values

			d[i] = d[i] + sumPartial; //store in the final variable
                }

                d[i]=d[i]%2; // modulo 2
        }
*/

/*
 __m128 num1, num2, num3, num4;

        for (uint16_t i = 0; i < col; i++) {
        num4=_mm_setzero_ps(); //sets sum to zero
                for (uint16_t j = 0; j < row; j+=4) {
                        //output[i] += matrix1[j] * matrix2[j][i];
                        num1=_mm_load_ps((float*)&matrix1[j]); // 1[3], 1[2], 1[1], 1[0] -> num1
                        num2=_mm_load_ps((float*)&matrix2[j][i]); // 2[3], 2[2], 2[1], 2[0] -> num2
                        num3=_mm_mul_ps(num1, num2); // 1[3]*2[3],...1[0]*2[0] -> num3
                        num3=_mm_hadd_ps(num3, num3); //1[3]*2[3]+1[2]*2[2] ... 
                        num4 = _mm_add_ps(num4, num3);
                }
                num4= _mm_hadd_ps(num4, num4);
                _mm_store_ss(&output[i], num4); // Stores only the lower SP FP that contain the sum
        }
*/
}
