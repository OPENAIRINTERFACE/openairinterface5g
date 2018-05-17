#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <stdint.h>

void get_3GPP_info_bit_pattern(uint16_t K, uint16_t n_PC, uint16_t Q_N_length, uint8_t E_r, uint8_t* P, uint16_t* Q_N, uint8_t** info_bit_pattern)
{
//   GET_3GPP_INFO_BIT_PATTERN Obtain the 3GPP information bit pattern,
//   according to Section 5.4.1.1 of 3GPP TS 38.212
//
//   I should be an integer scalar. It specifies the number of bits in the
//   information, CRC and PC bit sequence. It should be no greater than N or E.
//
//   Q_N should be a row vector comprising N number of unique integers in the
//   range 1 to N. Each successive element of Q_N provides the index of the
//   next most reliable input to the polar encoder kernal, where the first
//   element of Q_N gives the index of the least reliable bit and the last
//   element gives the index of the most reliable bit.
//
//   rate_matching_pattern should be a vector comprising E_r number of
//   integers, each having a value in the range 1 to N. Each integer
//   identifies which one of the N outputs from the polar encoder kernal
//   provides the corresponding bit in the encoded bit sequence e.
//
//   mode should have the value 'repetition', 'puncturing' or 'shortening'.
//   This specifies how the rate matching has been achieved. 'repetition'
//   indicates that some outputs of the polar encoder kernal are repeated in
//   the encoded bit sequence two or more times. 'puncturing' and
//   'shortening' indicate that some outputs of the polar encoder kernal
//   have been excluded from the encoded bit sequence. In the case of
//   'puncturing' these excluded bits could have values of 0 or 1. In the
//   case of 'shortening' these excluded bits are guaranteed to have values
//   of 0.
//
//   info_bit_pattern will be a vector comprising N number of logical
//   elements, each having the value true or false. The number of elements
//   in info_bit_pattern having the value true will be I. These elements
//   having the value true identify the positions of the information and
//   CRC bits within the input to the polar encoder kernal. The
//   information bit arrangement can be achieved according to
//   u(info_bit_pattern) = a.

int I=K+n_PC;

        if (I > Q_N_length) //I=K+n_PC
        {
            fprintf(stderr, "I=K+n_PC should be no greater than N.");
            exit(-1);
        }
        if (I > E_r)
        {
            fprintf(stderr, "I=K+n_PC should be no greater than E.");
            exit(-1);
        }

        //This is how the rate matching is described in TS 38.212
        int J[Q_N_length];
        int i,j;
        for (j=0; j<Q_N_length; j++)
        {
            i=floor(32*(double)j/Q_N_length);
            J[j] = P[i]*(Q_N_length/32)+(j%(Q_N_length/32));
        }

        //Q_Ftmp_N = [];
        int Q_Ftmp_N_length= Q_N_length-E_r;
        if (E_r < Q_N_length)
        {
            if ((double)(I)/E_r <= (double)7/16) // puncturing
            {
                //Q_Ftmp_N_length = Q_Ftmp_N_length + N-E;
                if (E_r >= (double)3*Q_N_length/4)
                {
                    //Q_Ftmp_N = [Q_Ftmp_N,0:ceil(3*N/4-E/2)-1];
                    Q_Ftmp_N_length = Q_Ftmp_N_length + ceil(3*Q_N_length/4-(double)E_r/2);
                } else
                {
                    //Q_Ftmp_N = [Q_Ftmp_N,0:ceil(9*N/16-E/4)-1];
                    Q_Ftmp_N_length = Q_Ftmp_N_length + ceil(9*Q_N_length/16-(double)E_r/4);
                }
            }
        }

        int* Q_Ftmp_N = (int *)malloc(sizeof(int)*Q_Ftmp_N_length);
        if (Q_Ftmp_N == NULL)
        {
            fprintf(stderr, "malloc failed\n");
            exit(-1);
        }
        if (E_r < Q_N_length)
        {
            if ((double)I/E_r <= 7/16) // puncturing
            {
                for (j=0; j<Q_N_length-E_r; j++)
                {
                    Q_Ftmp_N[j] = J[j];
                }

                if (E_r >= 3*Q_N_length/4)
                {
                    for (j=0; j<ceil(3*Q_N_length/4-(double)E_r/2); j++)
                    {
                        Q_Ftmp_N[Q_N_length-E_r+1+j] = j;
                    }
                } else
                {
                    for (j=0; j<ceil(9*Q_N_length/16-(double)E_r/4); j++)
                    {
                        Q_Ftmp_N[Q_N_length-E_r+1+j] = j;
                    }
                }
            } else // shortening
            {
                for (j=E_r; j<Q_N_length; j++)
                {
                    Q_Ftmp_N[j-E_r] = J[j];
                }

            }
        }

        //Q_Itmp_N = setdiff(Q_N-1,Q_Ftmp_N,'stable'); // -1 because TS 38.212 assumes that indices start at 0, not 1 like in Matlab
        int Q_Itmp_N_length = Q_N_length;
        int Q_Itmp_N_common[Q_N_length];

        for(i=0; i<Q_N_length; i++)
        {
            Q_Itmp_N_common[i]=0; //1 if in common, otherwise 0

            for (j=0; j<Q_Ftmp_N_length; j++)
            {
                if((int)Q_N[i]==Q_Ftmp_N[j])
                {
                    Q_Itmp_N_common[i]=1;
                    Q_Itmp_N_length--;
                    break;
                }
            }
        }

        free(Q_Ftmp_N);

        if (Q_Itmp_N_length < I)
        {
            fprintf(stderr, "Too many pre-frozen bits.");
            exit(-1);
        }

        int* Q_Itmp_N = (int *)malloc(sizeof(int)*Q_Itmp_N_length);
        if (Q_Itmp_N == NULL)
        {
            fprintf(stderr, "malloc failed\n");
            exit(-1);
        }
        j=0;
        for(i=0; i<Q_N_length; i++)
        {
            if(!Q_Itmp_N_common[i]) //if not commonc
            {
                Q_Itmp_N[j]=(int)Q_N[i];
                j++;
            }
        }

        int* Q_I_N = (int *)malloc(sizeof(int)*(I));
        if (Q_I_N == NULL)
        {
            fprintf(stderr, "malloc failed\n");
            exit(-1);
        }
        //Q_I_N=Q_Itmp_N(end-I+1:end);
        for (j=Q_Itmp_N_length-(I); j<Q_Itmp_N_length; j++)
        {
            Q_I_N[j-(Q_Itmp_N_length-(I))]=Q_Itmp_N[j];
        }

        free(Q_Itmp_N);

        //info_bit_pattern(Q_I_N+1) = true;
        *info_bit_pattern = (uint8_t *)malloc(sizeof(uint8_t)*Q_N_length);
        if (*info_bit_pattern == NULL)
        {
            fprintf(stderr, "malloc failed\n");
            exit(-1);
        }
        for(j=0; j<Q_N_length; j++)
        {
            (*info_bit_pattern)[j]=0;
            for(i=0; i<I; i++)
            {
                if(Q_I_N[i]==j)
                {
                    (*info_bit_pattern)[j]=1;
                    break;
                }
            }
        }

        free(Q_I_N);
}
