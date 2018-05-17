#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <stdint.h>

void nr_polar_bit_insertion_2(uint8_t* u, uint8_t* b, uint8_t* info_bit_pattern, uint8_t* PC_bit_pattern, uint16_t N)
{
        int PC_circular_buffer_length=5;

        int i, j;
        int k=0;

        int* y = (int*) malloc(sizeof(int)*PC_circular_buffer_length);
        if (y == NULL)
        {
            fprintf(stderr, "malloc failed\n");
            exit(-1);
        }
        for(i=0; i<PC_circular_buffer_length; i++)
        {
            y[i]=0;
        }

        int tempToShift;
        //for n=0:N-1
        printf("u = ");
        for(i=0; i<N; i++)
        {
            //y = [y(2:end),y(1)];
            tempToShift=y[0];
            for (j=0; j<PC_circular_buffer_length-1; j++)
            {
                y[j]=y[j+1];
            }
            y[PC_circular_buffer_length-1]=tempToShift;

            //if info_bit_pattern(n+1)
            //    if PC_bit_pattern(n+1)
            //        u(n+1) = y(1);
            //    else
            //        u(n+1) = b(k+1);
            //        k=k+1;
            //        y(1) = xor(y(1),u(n+1));
            if (info_bit_pattern[i])
            {
                if (PC_bit_pattern[i])
                {
                    u[i] = y[1];
                } else
                {
                    u[i] = b[k];
                    k++;
                    y[1] = ( y[1]!=u[i]);
                }
            } else
            {
                u[i]=0;
            }
            printf("%i ", u[i]);
        }

        free(y);
}
