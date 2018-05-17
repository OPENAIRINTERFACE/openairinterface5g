#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <stdint.h>

void nr_polar_kernel_operation(uint8_t *u, uint8_t *d, uint16_t N)
{
    // Martino's algorithm to avoid multiplication for the generating matrix

    int i,j;
    printf("\nd = ");
	for(i=0; i<N; i++)
    {
        d[i]=0;
        for(j=0; j<N; j++)
        {
            d[i]=d[i]+(( (j-i)& i )==0)*u[j];
        }
        d[i]=d[i]%2;

        printf("%i", d[i]);
    }
}
