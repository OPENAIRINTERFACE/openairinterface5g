#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "nr_polar_defs.h"
#include "nr_polar_pucch_defs.h"

void PUCCH_encoder(uint8_t a[], uint8_t A, uint8_t G, uint16_t* E_r, uint8_t** f)
{
// PUCCH_ENCODER Polar encoder for the Physical Uplink Control Channel (PUCCH) and the
// Physical Uplink Shared Channel (PUSCH) of 3GPP New Radio, as defined in
// Section 6.3 of TS38.212. Implements the code block segmentation and
// Cyclic Redudancy Check (CRC) attachment of Sections 6.3.1.2.1 and 6.3.2.2.1,
// the channel coding of Sections 6.3.1.3.1 and 6.3.2.3.2, the rate matching of
// Sections 6.3.1.4.1 and 6.3.2.4.1, as well as the code block concatenation of
// Sections 6.3.1.5.1 and 6.3.2.5.1. Note that this code does not implement the
// UCI bit sequence generation of Sections 6.3.1.1 and 6.3.2.1, the
// determination of the encoded block length E_UCI of Sections 6.3.1.4.1 and
// 6.3.2.4.1, or the multiplexing of Sections 6.3.1.6 and 6.3.2.6. Also, this
// code does not implement the small block lengths, which are detailed in
// Sections 6.3.1.2.2, 6.3.1.3.2, 6.3.1.4.2, 6.3.2.2.2, 6.3.2.3.2 and 6.3.2.4.2.

    uint8_t C=0;
    uint8_t i;
    uint8_t crc_polynomial_pattern_length;
    uint8_t P[32] = {0, 1, 2, 4, 3, 5, 6, 7, 8, 16, 9, 17, 10, 18, 11, 19, 12, 20, 13, 21, 14, 22, 15, 23, 24, 25, 26, 28, 27, 29, 30, 31};
    uint8_t* crc_polynomial_pattern;

    if (A < 12) {
        fprintf(stderr, "A should be no less than 12.");
        exit(-1);
    } else if (A > 1706) {
        fprintf(stderr, "A should be no greater than 1706.");
        exit(-1);
    }
    else if (A <= 19) //Use PCCA-polar
    {
        // The CRC polynomial used with PCCA-polar in 3GPP PUCCH channel is D^6 + D^5 + 1
        crc_polynomial_pattern_length = 7;
        crc_polynomial_pattern = (uint8_t *)malloc(sizeof(uint8_t)*crc_polynomial_pattern_length);
        crc_polynomial_pattern[0] = 1;
        crc_polynomial_pattern[1] = 1;
        crc_polynomial_pattern[2] = 0;
        crc_polynomial_pattern[3] = 0;
        crc_polynomial_pattern[4] = 0;
        crc_polynomial_pattern[5] = 0;
        crc_polynomial_pattern[6] = 1;

        C = 1; // Use one segment
    }
    else // Use CA-polar
    {
        crc_polynomial_pattern_length = 12;
        crc_polynomial_pattern = (uint8_t *)malloc(sizeof(uint8_t)*crc_polynomial_pattern_length);
        crc_polynomial_pattern[0] = 1;
        crc_polynomial_pattern[1] = 1;
        crc_polynomial_pattern[2] = 1;
        crc_polynomial_pattern[3] = 0;
        crc_polynomial_pattern[4] = 0;
        crc_polynomial_pattern[5] = 0;
        crc_polynomial_pattern[6] = 1;
        crc_polynomial_pattern[7] = 0;
        crc_polynomial_pattern[8] = 0;
        crc_polynomial_pattern[9] = 0;
        crc_polynomial_pattern[10] = 0;
        crc_polynomial_pattern[11] = 1;

        if (A >= 360 && G >= 1088 )
            C = 2; // Use two segments
        else
            C = 1; // Use one segment
    }

    // Determine the number of information and CRC bits.
    uint16_t K = ceil(A/C)+crc_polynomial_pattern_length-1;

    *E_r = floor((double)G/C);

    if(*E_r > 8192) {
        fprintf(stderr, "G is too long.");
        exit(-1);
    }

    //Determine the number of bits used at the input and output of the polar encoder kernal.
    uint16_t N = nr_polar_coding_output_length(K, *E_r, NR_POLAR_PUCCH_N_MAX);
	uint8_t n = log2(N);

    //int N = get_3GPP_N(K,*E_r,10); //n_max = 10 is used in PUCCH channels
    printf("DEBUG: The number of bits used at I/O of encoder kernal is %i\n", (int)N);

    // Get a rate matching pattern
	uint16_t* rate_matching_pattern = (uint16_t*)malloc(sizeof(uint16_t) * (*E_r));
	uint16_t *J = malloc(sizeof(uint16_t) * N);
	nr_polar_coding_rate_matching_pattern(rate_matching_pattern, J, P, K, N, *E_r);

    printf("DEBUG: The rate matching pattern is ");
    for (i=0; i<*E_r; i++) {
        printf("%i ", (int)rate_matching_pattern[i]);
    }
    printf("\n");

    // Get a sequence pattern
    uint16_t *Q_0_Nminus1 = nr_polar_coding_sequence_pattern(n);
    printf("DEBUG: The sequence pattern is ");
    for (i=0; i<N; i++) {
        printf("%i ", Q_0_Nminus1[i]);
    }
    printf("\n");

    // Get the channel interleaving pattern
    uint16_t *channel_interleaver_pattern = malloc(sizeof(uint16_t) * *E_r);
	nr_polar_coding_channel_interleaver_pattern(channel_interleaver_pattern, 1, *E_r);
	printf("DEBUG: The channel interleaver pattern is " );
    for (i=0; i<*E_r; i++) {
        printf("%i ", (int)channel_interleaver_pattern[i]);
    }
    printf("\n");

    uint16_t n_PC; //PC bits
    uint8_t* info_bit_pattern;
    uint8_t** G_P;
    uint8_t* b;
    uint8_t* u;
    uint8_t* d;
    uint8_t* e;

    if (A <= 19) // Use PCCA-polar
    {
        n_PC = 3; // 3 PC bits

        // Get an information bit pattern.
        get_3GPP_info_bit_pattern(K, n_PC, N, *E_r, P, Q_0_Nminus1, &info_bit_pattern); // -> info_bit_pattern
        printf("DEBUG: The information bit pattern is " );
        for (i=0; i<N; i++) {
            printf("%i", (int)info_bit_pattern[i] );
        }
        printf("\n");

        //Get a PC bit pattern.
        uint8_t* PC_bit_pattern;
        if (G-K+3 > 192)
            get_PC_bit_pattern(N, n_PC, 1, Q_0_Nminus1, info_bit_pattern, &PC_bit_pattern); // -> PC_bit_pattern
        else
            get_PC_bit_pattern(N, n_PC, 0, Q_0_Nminus1, info_bit_pattern, &PC_bit_pattern); // -> PC_bit_pattern

        printf("DEBUG: The PC bit pattern is " );
        for (i=0; i<N; i++) {
            printf("%i", PC_bit_pattern[i] );
        }
        printf("\n");

/// ------------------

        //Attach CRC to the Transport Block. (a to b)
        get_crc_generator_matrix(A, crc_polynomial_pattern_length-1, crc_polynomial_pattern, &G_P); // -> G_P
        nr_polar_crc(a, A, crc_polynomial_pattern_length-1, G_P, &b); //-> b

        //Bit insertion
        u = (uint8_t*) malloc(sizeof(uint8_t)*N);
        if (u == NULL) {
            fprintf(stderr, "malloc failed\n");
            exit(-1);
        }
        nr_polar_bit_insertion_2(u, b, info_bit_pattern, PC_bit_pattern, N); // -> u
        free(b);

        //encoding
        d = malloc(sizeof(uint8_t) * N);
        if (d == NULL) {
            fprintf(stderr, "malloc failed\n");
            exit(-1);
        }
        nr_polar_kernel_operation(u, d, N); // -> d
        free(u);

        //rate matching
        e = malloc(sizeof(uint8_t) * N);
        if (e == NULL) {
            fprintf(stderr, "malloc failed\n");
            exit(-1);
        }
        nr_polar_coding_rate_matcher(d, e, rate_matching_pattern, (*E_r));
        free(d);
        printf("\ne = ");
        for(i=0;i<(*E_r);i++)
        {
            printf("%i ", e[i]);
        }

        // Perform channel interleaving.
        *f = (uint8_t*) malloc(sizeof(uint8_t)*(*E_r));
        nr_polar_coding_interleaver(e, *f, channel_interleaver_pattern, (uint16_t)(*E_r));
        free(e);
        printf("\nf = ");
        for(i=0;i<(*E_r);i++)
        {
            printf("%i ", (*f)[i]);
        }
        printf("\n\n\n ");
    } else //Use CA-polar
    {
        n_PC = 0;

        // Get an information bit pattern.
        get_3GPP_info_bit_pattern(K, n_PC, N, *E_r, P, Q_0_Nminus1, &info_bit_pattern); // -> info_bit_pattern
        printf("DEBUG: The information bit pattern is " );
        for (i=0; i<N; i++) {
            printf("%i", (int)info_bit_pattern[i] );
        }
        printf("\n");

        if (C == 2)
        {
            //info_bit_pattern2 = info_bit_pattern;
            uint8_t* info_bit_pattern2;
            if(A%2) // Prepend a zero to the first segment during encoding (Treat the first information bit as a frozen bit)
            {
                info_bit_pattern2 = (uint8_t*)malloc(sizeof(uint8_t)*(1+N));
                info_bit_pattern2[0] =0;

                for (i=0; i<1+N; i++)
                {
                    info_bit_pattern2[i+1] = info_bit_pattern[i];
                }
            } else
            {
                info_bit_pattern2 = (uint8_t*)malloc(sizeof(uint8_t)*N);

                for (i=0; i<N; i++)
                {
                    info_bit_pattern2[i] = info_bit_pattern[i];
                }
            }

            //Attach CRC to the Transport Block. (a to b)
            get_crc_generator_matrix(A/C, crc_polynomial_pattern_length-1, crc_polynomial_pattern, &G_P); // -> G_P
            nr_polar_crc(a, A/C, crc_polynomial_pattern_length-1, G_P, &b); //-> b

        } else
        {
            //Attach CRC to the Transport Block. (a to b)
            get_crc_generator_matrix(A, crc_polynomial_pattern_length-1, crc_polynomial_pattern, &G_P); // -> G_P
            nr_polar_crc(a, A, crc_polynomial_pattern_length-1, G_P, &b); //-> b
        }

            //Bit insertion
            u = (uint8_t*) malloc(sizeof(uint8_t)*N);
            if (u == NULL) {
                fprintf(stderr, "malloc failed\n");
                exit(-1);
            }
            int j=0;
            printf("\nu = ");
            for(i=0; i<N; i++)
            {
                if(info_bit_pattern[i])
                {
                    u[i]=b[j];
                    j++;
                }
                else
                    u[i]=0;

                printf("%i ", u[i]);
            }
            free(b);

            //encoding
            d = malloc(sizeof(uint8_t) * N);
            if (d == NULL) {
                fprintf(stderr, "malloc failed\n");
                exit(-1);
            }
            nr_polar_kernel_operation(u, d, N); // -> d
            free(u);

            //rate matching
            e = malloc(sizeof(uint8_t) * N);
            if (e == NULL) {
                fprintf(stderr, "malloc failed\n");
                exit(-1);
            }
            nr_polar_coding_rate_matcher(d, e, rate_matching_pattern, (*E_r));
            free(d);
            printf("\ne = ");
            for(i=0;i<(*E_r);i++)
            {
                printf("%i ", e[i]);
            }

            // Perform channel interleaving.
            *f = (uint8_t*) malloc(sizeof(uint8_t)*(*E_r));
            nr_polar_coding_interleaver(e, *f, channel_interleaver_pattern, (uint16_t)(*E_r));
            free(e);
            printf("\nf = ");
            for(i=0;i<(*E_r);i++)
            {
                printf("%i ", (*f)[i]);
            }
            printf("\n\n\n ");




    }
//
//    free(crc_polynomial_pattern);
//    //free others...

}
