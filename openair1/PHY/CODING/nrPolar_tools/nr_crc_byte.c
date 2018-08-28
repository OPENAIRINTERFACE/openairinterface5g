/*
 * Licensed to the OpenAirInterface (OAI) Software Alliance under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The OpenAirInterface Software Alliance licenses this file to You under
 * the OAI Public License, Version 1.1  (the "License"); you may not use this file
 * except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.openairinterface.org/?page_id=698
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *-------------------------------------------------------------------------------
 * For more information about the OpenAirInterface (OAI) Software Alliance:
 *      contact@openairinterface.org
 */

#include "PHY/CODING/nrPolar_tools/nr_polar_defs.h"

/*
// ----- New implementation ----
uint32_t poly6 = 0x84000000; // 10000100000... -> D^6+D^5+1
uint32_t poly11 = 0xc4200000; //11000100001000... -> D^11+D^10+D^9+D^5+1
uint32_t poly16 = 0x10210000; //00100000010000100... - > D^16+D^12+D^5+1
uint32_t poly24a = 0x864cfb00; //100001100100110011111011 -> D^24+D^23+D^18+D^17+D^14+D^11+D^10+D^7+D^6+D^5+D^4+D^3+D+1
uint32_t poly24b = 0x80006300; //100000000000000001100011 -> D^24+D^23+D^6+D^5+D+1
uint32_t poly24c = 0xB2B11700; //101100101011000100010111 -> D^24...

//static unsigned int crc256Table[256];

void nr_crc_computation(uint8_t* input, uint8_t* output, uint16_t payloadBits, uint16_t crcParityBits, uint32_t* crc256Table)
{
	//Create payload in bit
	uint8_t* input2 = (uint8_t*)malloc(payloadBits); //divided by 8 (in bits) 
        uint8_t mask = 128; // 10000000
	
        for(uint8_t ind=0; ind<(payloadBits/8); ind++)
        {
		input2[ind]=0;
                for(uint8_t ind2=0; ind2<8; ind2++)
                { 
                        if(input[8*ind+ind2])
                        {
                                input2[ind] = input2[ind] | mask;
                        }
                        mask= mask >> 1;
	        }
                mask=128;
        }

        //crcTable256Init(poly);

	unsigned int crcBits;	
	crcBits = crcPayload(input2, payloadBits, crc256Table);

	//create crc in byte
	unsigned int mask2=0x80000000; //100...
	output = (uint8_t*)malloc(sizeof(uint8_t)*crcParityBits);

	for(uint8_t ind=0; ind<crcParityBits; ind++)
        {
		if(crcBits & mask2)
			output[ind]=1;
		else
			output[ind]=0;

		mask2 = mask2 >> 1;
	}
      
}

unsigned int crcbit (unsigned char* inputptr, int octetlen, unsigned int poly)
{
	unsigned int i, crc = 0, c;

  	while (octetlen-- > 0) {
    		c = (*inputptr++) << 24;
	
    		for (i = 8; i != 0; i--) {
      			if ((1 << 31) & (c ^ crc))
        			crc = (crc << 1) ^ poly;
      			else
        			crc <<= 1;

      			c <<= 1;
    		}
  	}
	
  	return crc;
}

void crcTableInit (void)
{	
	unsigned char c = 0;

	do {
    	       	crc6Table[c] = crcbit(&c, 1, poly6);
        	crc11Table[c]= crcbit(&c, 1, poly11);
		crc16Table[c] =crcbit(&c, 1, poly16);
		crc24aTable[c]=crcbit(&c, 1, poly24a);
		crc24bTable[c]=crcbit(&c, 1, poly24b);
		crc24cTable[c]=crcbit(&c, 1, poly24c);

	} while (++c);
}

void crcTable256Init (uint32_t poly, uint32_t* crc256Table)
{
        unsigned char c = 0;
//	crc256Table = malloc(sizeof(uint32_t)*256);

        do {
		crc256Table[c] = crcbit(&c, 1, poly);

//                crc6Table[c] = crcbit(&c, 1, poly6);
  //              crc11Table[c]= crcbit(&c, 1, poly11);
//                crc16Table[c] =crcbit(&c, 1, poly16);
//                crc24aTable[c]=crcbit(&c, 1, poly24a);
//                crc24bTable[c]=crcbit(&c, 1, poly24b);
//                crc24cTable[c]=crcbit(&c, 1, poly24c);

        } while (++c);

	//return crc256Table;
}

unsigned int crcPayload(unsigned char * inptr, int bitlen, uint32_t* crc256Table)
{
	int octetlen, resbit;
        unsigned int crc = 0;
        octetlen = bitlen/8; // Change in bytes
        resbit = (bitlen % 8);

        while (octetlen-- > 0)
        {
                crc = (crc << 8) ^ crc256Table[(*inptr++) ^ (crc >> 24)];
        }

        if (resbit > 0)
        {
                crc = (crc << resbit) ^ crc256Table[((*inptr) >> (8 - resbit)) ^ (crc >> (32 - resbit))];
        }
        return crc;
}
*/
// ----- Old implementation ----
uint8_t **crc24c_generator_matrix(uint16_t payloadSizeBits){

	uint8_t crcPolynomialPattern[25] = {1,1,0,1,1,0,0,1,0,1,0,1,1,0,0,0,1,0,0,0,1,0,1,1,1};
	// 1011 0010 1011 0001 0001 0111 D^24 + D^23 + D^21 + D^20 + D^17 + D^15 + D^13 + D^12 + D^8 + D^4 + D^2 + D + 1
	uint8_t crcPolynomialSize = 24;// 24 because crc24c
	uint8_t temp1[crcPolynomialSize], temp2[crcPolynomialSize];

	uint8_t **crc_generator_matrix = malloc(payloadSizeBits * sizeof(uint8_t *));
	if (crc_generator_matrix)
	{
	  for (int i = 0; i < payloadSizeBits; i++)
	  {
		  crc_generator_matrix[i] = malloc(crcPolynomialSize * sizeof(uint8_t));
	  }
	}

	for (int i = 0; i < crcPolynomialSize; i++) crc_generator_matrix[payloadSizeBits-1][i]=crcPolynomialPattern[i+1];

	for (int i = payloadSizeBits-2; i >= 0; i--){

		for (int j = 0; j < crcPolynomialSize-1; j++) temp1[j]=crc_generator_matrix[i+1][j+1];
		temp1[crcPolynomialSize-1]=0;

		for (int j = 0; j < crcPolynomialSize; j++) temp2[j]=crc_generator_matrix[i+1][0]*crcPolynomialPattern[j+1];

		for (int j = 0; j < crcPolynomialSize; j++){
			if(temp1[j]+temp2[j] == 1){
				crc_generator_matrix[i][j]=1;
			} else {
				crc_generator_matrix[i][j]=0;
			}
		}

	}

	return crc_generator_matrix;
}

uint8_t **crc11_generator_matrix(uint16_t payloadSizeBits){

	uint8_t crcPolynomialPattern[12] = {1,1,1,0,0,0,1,0,0,0,0,1};
	// 1110 0010 0001 D^11 + D^10 + D^9 + D^5 + 1
	uint8_t crcPolynomialSize = 11;
	uint8_t temp1[crcPolynomialSize], temp2[crcPolynomialSize];

	uint8_t **crc_generator_matrix = malloc(payloadSizeBits * sizeof(uint8_t *));
	if (crc_generator_matrix)
	{
	  for (int i = 0; i < payloadSizeBits; i++)
	  {
		  crc_generator_matrix[i] = malloc(crcPolynomialSize * sizeof(uint8_t));
	  }
	}

	for (int i = 0; i < crcPolynomialSize; i++) crc_generator_matrix[payloadSizeBits-1][i]=crcPolynomialPattern[i+1];

	for (int i = payloadSizeBits-2; i >= 0; i--){

		for (int j = 0; j < crcPolynomialSize-1; j++) temp1[j]=crc_generator_matrix[i+1][j+1];
		temp1[crcPolynomialSize-1]=0;

		for (int j = 0; j < crcPolynomialSize; j++) temp2[j]=crc_generator_matrix[i+1][0]*crcPolynomialPattern[j+1];

		for (int j = 0; j < crcPolynomialSize; j++){
			if(temp1[j]+temp2[j] == 1){
				crc_generator_matrix[i][j]=1;
			} else {
				crc_generator_matrix[i][j]=0;
			}
		}

	}

	return crc_generator_matrix;
}

uint8_t **crc6_generator_matrix(uint16_t payloadSizeBits){

	uint8_t crcPolynomialPattern[7] = {1,1,0,0,0,0,1};
	// 0110 0001 D^6 + D^5 + 1
	uint8_t crcPolynomialSize = 6;
	uint8_t temp1[crcPolynomialSize], temp2[crcPolynomialSize];

	uint8_t **crc_generator_matrix = malloc(payloadSizeBits * sizeof(uint8_t *));
	if (crc_generator_matrix)
	{
	  for (int i = 0; i < payloadSizeBits; i++)
	  {
		  crc_generator_matrix[i] = malloc(crcPolynomialSize * sizeof(uint8_t));
	  }
	}

	for (int i = 0; i < crcPolynomialSize; i++) crc_generator_matrix[payloadSizeBits-1][i]=crcPolynomialPattern[i+1];

	for (int i = payloadSizeBits-2; i >= 0; i--){

		for (int j = 0; j < crcPolynomialSize-1; j++) temp1[j]=crc_generator_matrix[i+1][j+1];
		temp1[crcPolynomialSize-1]=0;

		for (int j = 0; j < crcPolynomialSize; j++) temp2[j]=crc_generator_matrix[i+1][0]*crcPolynomialPattern[j+1];

		for (int j = 0; j < crcPolynomialSize; j++){
			if(temp1[j]+temp2[j] == 1){
				crc_generator_matrix[i][j]=1;
			} else {
				crc_generator_matrix[i][j]=0;
			}
		}

	}

	return crc_generator_matrix;
}
