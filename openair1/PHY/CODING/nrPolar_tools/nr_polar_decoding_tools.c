#include "PHY/CODING/nrPolar_tools/nr_polar_defs.h"

void updateLLR(double ***llr, uint8_t **llrU, uint8_t ***bit, uint8_t **bitU,
		uint8_t listSize, uint16_t row, uint16_t col, uint16_t xlen, uint8_t ylen, uint8_t approximation) {
	uint16_t offset = (xlen/(pow(2,(ylen-col-1))));
	for (uint8_t i=0; i<listSize; i++) {
		if (( (row) % (2*offset) ) >= offset ) {
			if(bitU[row-offset][col]==0) updateBit(bit, bitU, listSize, (row-offset), col, xlen, ylen);
			if(llrU[row-offset][col+1]==0) updateLLR(llr, llrU, bit, bitU, listSize, (row-offset), (col+1), xlen, ylen, approximation);
			if(llrU[row][col+1]==0) updateLLR(llr, llrU, bit, bitU, listSize, row, (col+1), xlen, ylen, approximation);
			llr[row][col][i] = (pow((-1),bit[row-offset][col][i])*llr[row-offset][col+1][i]) + llr[row][col+1][i];
		} else {
			if(llrU[row][col+1]==0) updateLLR(llr, llrU, bit, bitU, listSize, row, (col+1), xlen, ylen, approximation);
			if(llrU[row+offset][col+1]==0) updateLLR(llr, llrU, bit, bitU, listSize, (row+offset), (col+1), xlen, ylen, approximation);
			computeLLR(llr, row, col, i, offset, approximation);
		}
	}

	llrU[row][col]=1;
}

void updateBit(uint8_t ***bit, uint8_t **bitU, uint8_t listSize, uint16_t row,
		uint16_t col, uint16_t xlen, uint8_t ylen) {
	uint16_t offset = ( xlen/(pow(2,(ylen-col))) );

	for (uint8_t i=0; i<listSize; i++) {
		if (( (row) % (2*offset) ) >= offset ) {
			if (bitU[row][col-1]==0) updateBit(bit, bitU, listSize, row, (col-1), xlen, ylen);
			bit[row][col][i] = bit[row][col-1][i];
		} else {
			if (bitU[row][col-1]==0) updateBit(bit, bitU, listSize, row, (col-1), xlen, ylen);
			if (bitU[row+offset][col-1]==0) updateBit(bit, bitU, listSize, (row+offset), (col-1), xlen, ylen);
			bit[row][col][i] = ( (bit[row][col-1][i]+bit[row+offset][col-1][i]) % 2);
		}
	}

	bitU[row][col]=1;
}

void updatePathMetric(double *pathMetric, double ***llr, uint8_t listSize, uint8_t bitValue,
		uint16_t row, uint8_t approximation) {

	if (approximation) { //eq. (12)
		for (uint8_t i=0; i<listSize; i++) {
			if ((2*bitValue) != ( 1 - copysign(1.0,llr[row][0][i]) )) pathMetric[i] += fabs(llr[row][0][i]);
		}
	} else { //eq. (11b)
		int8_t multiplier = (2*bitValue) - 1;
		for (uint8_t i=0; i<listSize; i++) pathMetric[i] += log ( 1 + exp(multiplier*llr[row][0][i]) ) ;
	}

}

void updatePathMetric2(double *pathMetric, double ***llr, uint8_t listSize, uint16_t row, uint8_t appr) {

	double *tempPM = malloc(sizeof(double) * listSize);
	for (int i=0; i < listSize; i++) tempPM[i]=pathMetric[i];

	uint8_t bitValue = 0;
	if (appr) { //eq. (12)
		for (uint8_t i = 0; i < listSize; i++) {
			if ((2 * bitValue) != (1 - copysign(1.0, llr[row][0][i]))) pathMetric[i] += fabs(llr[row][0][i]);
		}
	} else { //eq. (11b)
		int8_t multiplier = (2 * bitValue) - 1;
		for (uint8_t i = 0; i < listSize; i++) pathMetric[i] += log(1 + exp(multiplier * llr[row][0][i]));
	}

	bitValue = 1;
	if (appr) { //eq. (12)
		for (uint8_t i = listSize; i < 2*listSize; i++) {
			if ((2 * bitValue) != (1 - copysign(1.0, llr[row][0][(i-listSize)]))) pathMetric[i] = tempPM[(i-listSize)] + fabs(llr[row][0][(i-listSize)]);
		}
	} else { //eq. (11b)
		int8_t multiplier = (2 * bitValue) - 1;
		for (uint8_t i = listSize; i < 2*listSize; i++) pathMetric[i] = tempPM[(i-listSize)] + log(1 + exp(multiplier * llr[row][0][(i-listSize)]));
	}

	free(tempPM);

}

void computeLLR(double ***llr, uint16_t row, uint16_t col, uint8_t i,
		uint16_t offset, uint8_t approximation) {

	double a = llr[row][col + 1][i];
	double absA = fabs(a);
	double b = llr[row + offset][col + 1][i];
	double absB = fabs(b);

	if (approximation || isinf(absA) || isinf(absB)) { //eq. (9)
		llr[row][col][i] = copysign(1.0, a) * copysign(1.0, b) * fmin(absA, absB);
	} else { //eq. (8a)
		llr[row][col][i] = log((exp(a + b) + 1) / (exp(a) + exp(b)));
	}

}

void updateCrcChecksum(uint8_t **crcChecksum, uint8_t **crcGen,
		uint8_t listSize, uint32_t i2, uint8_t len) {
	for (uint8_t i = 0; i < listSize; i++) {
		for (uint8_t j = 0; j < len; j++) {
			crcChecksum[j][i] = ( (crcChecksum[j][i] + crcGen[i2][j]) % 2 );
		}
	}
}

void updateCrcChecksum2(uint8_t **crcChecksum, uint8_t **crcGen,
		uint8_t listSize, uint32_t i2, uint8_t len) {
	for (uint8_t i = 0; i < listSize; i++) {
		for (uint8_t j = 0; j < len; j++) {
			crcChecksum[j][i+listSize] = ( (crcChecksum[j][i] + crcGen[i2][j]) % 2 );
		}
	}
}
