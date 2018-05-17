#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <stdint.h>

void get_PC_bit_pattern(uint16_t Q_N_length, uint16_t n_PC, uint8_t n_PC_wm, uint16_t* Q_N, uint8_t* info_bit_pattern, uint8_t** PC_bit_pattern)
{
//   GET_PC_BIT_PATTERN Obtain the Parity Check (PC) bit pattern,
//   according to Section 5.3.1.2 of 3GPP TS 38.212
//
//   info_bit_pattern should be a vector comprising N number of logical
//   elements, each having the value true or false. The number of elements
//   in info_bit_pattern having the value true should be I, where
//   I = A+P+n_PC. These elements having the value true identify the
//   positions of the information, CRC and PC bits within the input to the
//   polar encoder kernal.
//
//   Q_N should be a row vector comprising N number of unique integers in the
//   range 1 to N. Each successive element of Q_N provides the index of the
//   next most reliable input to the polar encoder kernal, where the first
//   element of Q_N gives the index of the least reliable bit and the last
//   element gives the index of the most reliable bit.
//
//   n_PC should be an integer scalar. It specifies the number of PC bits to
//   use, where n_PC should be no greater than I.
//
//   n_PC_wm should be an integer scalar. It specifies the number of PC bits
//   that occupy some of the most reliable positions at the input to the
//   polar encoder kernal. The remaining n_PC-n_PC_wm PC bits occupy some of
//   the least reliable positions at the input to the polar encoder kernal.
//   n_PC_wm should be no greater than n_PC.
//
//   PC_bit_pattern will be a vector comprising N number of logical
//   elements, each having the value true or false. The number of elements
//   in PC_bit_pattern having the value true will be n_PC.
//   These elements having the value true identify the positions of the
//   PC bits within the input to the polar encoder kernal.

        //N = length(info_bit_pattern); -> Q_N_length
        //I = sum(info_bit_pattern);
        int totInfoBit =0;
        int j,i;
        for (j=0; j<Q_N_length; j++)
        {
            if(info_bit_pattern[j])
                totInfoBit++;
        }

        if (n_PC > totInfoBit)
        {
            fprintf(stderr, "n_PC should be no greater than totInfoBit.");
            exit(-1);
        }

        if (n_PC_wm > n_PC)
        {
            fprintf(stderr, "n_PC_wm should be no greater than n_PC.");
            exit(-1);
        }
        //Q_I = 1:N;
        int* Q_I = (int*) malloc(sizeof(int)*Q_N_length);
        if (Q_I == NULL)
        {
            fprintf(stderr, "malloc failed\n");
            exit(-1);
        }
        for (j=0; j<Q_N_length; j++)
        {
            Q_I[j]=j+1;
        }

        //Q_N_I = intersect(Q_N, Q_I(info_bit_pattern), 'stable');
        int Q_N_I_length=0;
        int* Q_N_common = (int*) malloc(sizeof(int)*Q_N_length);
        if (Q_N_common == NULL)
        {
            fprintf(stderr, "malloc failed\n");
            exit(-1);
        }

        for (j=0; j<Q_N_length; j++) //init
        {
            Q_N_common[j]=0;
        }

        for (j=0; j<Q_N_length; j++) //look in Q_I
        {
            if(info_bit_pattern[j])
            {
                //Q_I(info_bit_pattern)
                for (i=0; i<Q_N_length; i++) //look in Q_N
                {
                    if(Q_N[i]+1==Q_I[j])
                    {
                        Q_N_common[i]=1;
                        Q_N_I_length++;
                        break;
                    }
                }
            }
        }
        free(Q_I);

        int* Q_N_I =  (int*) malloc(sizeof(int)*Q_N_I_length);
        if (Q_N_I == NULL)
        {
            fprintf(stderr, "malloc failed\n");
            exit(-1);
        }
        i=0;
        for(j=0; j<Q_N_length; j++)
        {
            if(Q_N_common[j])
            {
                Q_N_I[i]=Q_N[j]+1;
                i++;
            }
        }

        free(Q_N_common);

        //int G_N = get_G_N(N);
        //int w_g = sum(G_N,2);
        //useless, I do this
        int* w_g = (int*) malloc(sizeof(int)*Q_N_length);
        if (w_g == NULL)
        {
            fprintf(stderr, "malloc failed\n");
            exit(-1);
        }
        w_g[0]=1;
        w_g[1]=2;
        int counter=2;
        int n = log2(Q_N_length);

        for(i=0; i<n-1; i++) //n=log2(N)
        {
            for(j=0; j<counter; j++)
            {
                w_g[counter+j]=w_g[j]*2;
            }
            counter = counter*2;
        }

        //Q_tilde_N_I = Q_N_I(n_PC+1:end); % This is what it says in TS 38.212
        int* Q_tilde_N_I = (int*) malloc(sizeof(int)*(Q_N_I_length-n_PC));
        if (Q_tilde_N_I == NULL)
        {
            fprintf(stderr, "malloc failed\n");
            exit(-1);
        }
        int* Q_tilde_N_I_flip = (int*) malloc(sizeof(int)*(Q_N_I_length-n_PC));
        if (Q_tilde_N_I_flip == NULL)
        {
            fprintf(stderr, "malloc failed\n");
            exit(-1);
        }
        for (i=0; i<Q_N_I_length-n_PC; i++)
        {
            Q_tilde_N_I[i]=Q_N_I[n_PC+i];
            //Q_tilde_N_I_flip = fliplr(Q_tilde_N_I);
            Q_tilde_N_I_flip[Q_N_I_length-n_PC-i-1] = Q_tilde_N_I[i];
        }
        //%Q_tilde_N_I = Q_N_I(n_PC-n_PC_wm+1:end); % I think that this would be slightly more elegant

        //[w_g_sorted, indices] = sort(w_g(Q_tilde_N_I_flip));
        int* w_g_sorted =  (int*) malloc(sizeof(int)*(Q_N_I_length-n_PC));
        if (w_g_sorted == NULL)
        {
            fprintf(stderr, "malloc failed\n");
            exit(-1);
        }
        int* indices =  (int*) malloc(sizeof(int)*(Q_N_I_length-n_PC));
        if (indices == NULL)
        {
            fprintf(stderr, "malloc failed\n");
            exit(-1);
        }
        for (i=0; i<Q_N_I_length-n_PC; i++)
        {
            w_g_sorted[i]=w_g[Q_tilde_N_I_flip[i]-1]; // w_g(Q_tilde_N_I_flip), yet to sort
            indices[i] = i;
        }
        free(Q_tilde_N_I);
        free(w_g);

        //bubble sort
        int tempToSwap=0;
        for (i = 0; i < (Q_N_I_length-n_PC)-1; i++)
        {
            for (j = 0; j < (Q_N_I_length-n_PC)-i-1; j++)
            {
                 if (w_g_sorted[j] > w_g_sorted[j+1]) //then swap
                 {
                     tempToSwap = w_g_sorted[j];
                     w_g_sorted[j] = w_g_sorted[j+1];
                     w_g_sorted[j+1] = tempToSwap;

                     tempToSwap = indices[j];
                     indices[j] = indices[j+1];
                     indices[j+1] = tempToSwap;
                 }
            }
        }
        free(w_g_sorted);

        //Q_N_PC = [Q_N_I(1:n_PC-n_PC_wm), Q_tilde_N_I_flip(indices(1:n_PC_wm))];
        int* Q_N_PC = (int*) malloc(sizeof(int)*(n_PC));
        if (Q_N_PC == NULL)
        {
            fprintf(stderr, "malloc failed\n");
            exit(-1);
        }

        for (i=0; i<n_PC-n_PC_wm; i++)
        {
            Q_N_PC[i]=Q_N_I[i]; //Q_N_PC = [Q_N_I(1:n_PC-n_PC_wm), ...
        }
        free(Q_N_I);

        for (i=0; i<n_PC_wm; i++)
        {
            Q_N_PC[n_PC-n_PC_wm+i] = Q_tilde_N_I_flip[indices[i]];//... Q_tilde_N_I_flip(indices(1:n_PC_wm))];
        }
        free(Q_tilde_N_I_flip);
        free(indices);

        //PC_bit_pattern = false(1,N);
        //PC_bit_pattern(Q_N_PC) = true;
        *PC_bit_pattern =  (uint8_t*) malloc(sizeof(uint8_t)*(Q_N_length));
        if (*PC_bit_pattern == NULL)
        {
            fprintf(stderr, "malloc failed\n");
            exit(-1);
        }

        for (i=0; i<Q_N_length; i++)
        {
            (*PC_bit_pattern)[i]=0;
            for (j=0; j<n_PC; j++)
            {
                if (Q_N_PC[j]-1==i)
                {
                    (*PC_bit_pattern)[i]=1;
                    break;
                }
            }
        }

        //free(Q_I);
        //free(Q_N_common);
        //free(Q_N_I);
        //free(w_g);
        //free(Q_tilde_N_I);
        //free(Q_tilde_N_I_flip);
        //free(w_g_sorted);
        //free(indices);
        free(Q_N_PC);
}
