#include "PHY/CODING/nrPolar_tools/nr_polar_defs.h"

void nr_polar_bit_insertion(uint8_t *input, uint8_t *output, uint16_t N, uint16_t K,
		uint16_t *Q_I_N, uint16_t *Q_PC_N, uint8_t n_PC){

	uint16_t k=0;
	uint8_t flag;

	if (n_PC>0) {
		/*
		 *
		 */
	} else {
		for (int n=0; n<=N-1; n++) {
			flag=0;
			for (int m=0; m<=(K+n_PC)-1; m++) {
				if ( n == Q_I_N[m]) {
					flag=1;
					break;
				}
			}
			if (flag) { // n ϵ Q_I_N
				output[n]=input[k];
				k++;
			} else {
				output[n] = 0;
			}
		}
	}

}
