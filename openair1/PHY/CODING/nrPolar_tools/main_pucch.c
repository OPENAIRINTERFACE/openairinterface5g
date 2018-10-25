#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "nr_polar_defs.h"
#include "nr_polar_pucch_defs.h"

void PUCCH_encoder(uint8_t a[], uint8_t A, uint8_t G, uint16_t* E_r, uint8_t** f);

int main()
{
    uint16_t E_r;
    uint8_t* f;
    uint8_t A=32; //16, 32, 64, 128, 256, 512, 1024 Payload length
    uint8_t G = 54; //, 108, 216, 432, 864, 1728, 3456, 6912, 13824}; encoded block length

    uint8_t j;

    uint8_t* a = (uint8_t *)malloc(sizeof(uint8_t)*A);
    if (a == NULL)
    {
        fprintf(stderr, "malloc failed\n");
        exit(-1);
    }

    printf("DEBUG: Send ");
    for (j=0; j<A; j++) //create the message to encode
    {
        //a[j]=rand()%2;
        a[j]=1; // ONLY ONES FOR TEST, otherwise random
        printf("%i", a[j]);
    }
    printf("\n");

    PUCCH_encoder(a, A, G, &E_r, &f);


    return 0;
}
