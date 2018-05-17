#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <stdint.h>

void get_crc_generator_matrix(uint8_t A, uint8_t P, uint8_t* crc_polynomial_pattern, uint8_t*** G_P)
{
//   GET_CRC_GENERATOR_MATRIX Obtain a Cyclic Redudancy Check (CRC) generator
//   matrix.
//
//   A should be an integer scalar. It specifies the number of bits in the
//   information bit sequence.
//
//   crc_polynomial_pattern should be a binary row vector comprising P+1
//   number of bits, each having the value 0 or 1. These bits parameterise a
//   Cyclic Redundancy Check (CRC) comprising P bits. Each bit provides the
//   coefficient of the corresponding element in the CRC generator
//   polynomial. From left to right, the bits provide the coefficients for
//   the elements D^P, D^P-1, D^P-2, ..., D^2, D, 1.
//
//   G_P will be a K by P binary matrix. The CRC bits can be generated
//   according to mod(a*G_P,2).

        *G_P = (uint8_t **)malloc(A * sizeof(uint8_t *));
        if (*G_P == NULL)
        {
            fprintf(stderr, "malloc failed\n");
            exit(-1);
        }
        int i,j;
        for (i=0; i<A; i++)
        {
            (*G_P)[i] = (uint8_t *)malloc(P * sizeof(uint8_t));
            if ((*G_P)[i] == NULL)
            {
                fprintf(stderr, "malloc failed\n");
                exit(-1);
            }
        }

        if(A>0)
        {
            //G_P(end,:) = crc_polynomial_pattern(2:end);
            for(i=0; i<P; i++)
            {
                (*G_P)[A-1][i] = crc_polynomial_pattern[i+1];
            }

            //for k = A-1:-1:1
            //    G_P(k,:) = xor([G_P(k+1,2:end),0],G_P(k+1,1)*crc_polynomial_pattern(2:end));
            //end
            for(j=A-2; j>-1; j--)
            {
                for(i=0; i<P; i++)
                {
                    (*G_P)[j][i]=0; //init with zeros
                }

                for(i=1; i<P; i++)
                {
                    if( (*G_P)[j+1][i] != ( (*G_P)[j+1][0])*crc_polynomial_pattern[i]) //xor
                        (*G_P)[j][i-1]=1;
                }

                if(0 != ( (*G_P)[j+1][0])*crc_polynomial_pattern[P]) //xor
                        (*G_P)[j][P-1]=1;
            }
        }
printf("G_P=\n");
        for(i=0; i<A; i++)
        {
            for(j=0; j<P; j++)
            {
                printf("%i ", (int)(*G_P)[i][j]);
            }
            printf("\n");
        }

}
