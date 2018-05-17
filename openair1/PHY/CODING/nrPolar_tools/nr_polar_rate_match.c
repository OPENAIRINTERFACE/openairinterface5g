#include <math.h>
#include "PHY/CODING/nrPolar_tools/nr_polar_defs.h"

void nr_polar_rate_matching_pattern(uint16_t *rmp, uint16_t *J, uint8_t *P_i_, uint16_t K, uint16_t N, uint16_t E){
	
	uint8_t i;
	uint16_t *d, *y, ind;
	d = (uint16_t *)malloc(sizeof(uint16_t) * N);
	y = (uint16_t *)malloc(sizeof(uint16_t) * N);
	
	for (int m=0; m<=N-1; m++) d[m]=m;
	
	for (int m=0; m<=N-1; m++){
		i=floor((32*m)/N);
		J[m] = (P_i_[i]*(N/32)) + (m%(N/32));
		y[m] = d[J[m]];
	}
	
	if (E>=N) { //repetition
		for (int k=0; k<=E-1; k++) {
			ind = (k%N);
			rmp[k]=y[ind];
		}
	} else {
		if ( (K/(double)E) <= (7.0/16) ) { //puncturing
			for (int k=0; k<=E-1; k++) {
				rmp[k]=y[k+N-E];
			}
		} else { //shortening
			for (int k=0; k<=E-1; k++) {
				rmp[k]=y[k];
			}
		}	
	}	
	
	free(d);
	free(y);
}


void nr_polar_rate_matching(double *input, double *output, uint16_t *rmp, uint16_t K, uint16_t N, uint16_t E){

	if (E>=N) { //repetition
		for (int i=0; i<=N-1; i++) output[i]=0;
		for (int i=0; i<=E-1; i++){
			output[rmp[i]]+=input[i];
		}
	} else {
		if ( (K/(double)E) <= (7.0/16) ) { //puncturing
			for (int i=0; i<=N-1; i++) output[i]=0;
		} else { //shortening
			for (int i=0; i<=N-1; i++) output[i]=INFINITY;
		}

		for (int i=0; i<=E-1; i++){
			output[rmp[i]]=input[i];
		}
	}

}

void nr_polar_rate_matcher(uint8_t *input, unsigned char *output, uint16_t *pattern, uint16_t size) {
	for (int i=0; i<size; i++) output[i]=input[pattern[i]];
}
