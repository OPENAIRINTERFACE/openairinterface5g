#include "PHY/CODING/nrPolar_tools/nr_polar_defs.h"

void nr_byte2bit(uint8_t *array, uint8_t arraySize, uint8_t *bitArray){//First 2 parameters are in bytes.

	for (int i=0; i<arraySize; i++){
		bitArray[(7+(i*8))] = ( array[i]>>0 & 0x01);
		bitArray[(6+(i*8))] = ( array[i]>>1 & 0x01);
		bitArray[(5+(i*8))] = ( array[i]>>2 & 0x01);
		bitArray[(4+(i*8))] = ( array[i]>>3 & 0x01);
		bitArray[(3+(i*8))] = ( array[i]>>4 & 0x01);
		bitArray[(2+(i*8))] = ( array[i]>>5 & 0x01);
		bitArray[(1+(i*8))] = ( array[i]>>6 & 0x01);
		bitArray[  (i*8)  ] = ( array[i]>>7 & 0x01);
	}

}
