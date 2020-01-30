#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <stdint.h>

void nr_polar_crc(uint8_t* a, uint8_t A, uint8_t P, uint8_t** G_P, uint8_t** b)
{
    int i, j;
    int* temp_b = (int*) malloc(sizeof(int)*P);
    if (temp_b == NULL)
    {
        fprintf(stderr, "malloc failed\n");
        exit(-1);
    }
    printf("temp = ");
    for(i=0; i<P; i++)
    {
        temp_b[i]=0;
        for(j=0; j<A; j++)
        {
            temp_b[i]=temp_b[i] + a[j]*G_P[j][i];
        }
        temp_b[i]=temp_b[i]%2;
        printf("%i ", temp_b[i]);
    }

    *b = (uint8_t*) malloc(sizeof(uint8_t)*(A+P));
    if (*b == NULL)
    {
        fprintf(stderr, "malloc failed\n");
        exit(-1);
    }
    printf("\nb = ");
    for(i=0; i<A; i++)
    {
        (*b)[i]=a[i];
        printf("%i", (*b)[i]);
    }
    for(i=A; i<A+P; i++)
    {
        (*b)[i]=temp_b[i-A];
        printf("%i", (*b)[i]);
    }

    free(temp_b);
}
