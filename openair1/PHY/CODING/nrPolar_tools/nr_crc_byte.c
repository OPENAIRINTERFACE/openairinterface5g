#include "PHY/CODING/nrPolar_tools/nr_polar_defs.h"

/*ref 38-212 v15.0.0, pp 8-9 */
/* the highest degree is set by default */
unsigned int poly24a = 0x864cfb00; // 1000 0110 0100 1100 1111 1011 D^24 + D^23 + D^18 + D^17 + D^14 + D^11 + D^10 + D^7 + D^6 + D^5 + D^4 + D^3 + D + 1
unsigned int poly24b = 0x80006300; // 1000 0000 0000 0000 0110 0011 D^24 + D^23 + D^6 + D^5 + D + 1
unsigned int poly24c = 0xb2b11700; // 1011 0010 1011 0001 0001 0111 D^24 + D^23 + D^21 + D^20 + D^17 + D^15 + D^13 + D^12 + D^8 + D^4 + D^2 + D + 1
unsigned int poly16 = 0x10210000;  // 0001 0000 0010 0001           D^16 + D^12 + D^5 + 1
unsigned int poly11 = 0xe2100000;  // 1110 0010 0001 				D^11 + D^10 + D^9 + D^5 + 1
unsigned int poly6 = 0x61000000;   // 0110 0001 					D^6 + D^5 + 1
/*********************************************************

For initialization && verification purposes,
   bit by bit implementation with any polynomial

The first bit is in the MSB of each byte

*********************************************************/
unsigned int crcbit (unsigned char * inputptr, int octetlen, unsigned int poly)
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

/*********************************************************

crc table initialization

*********************************************************/
static unsigned int      crc24aTable[256];
static unsigned int      crc24bTable[256];
static unsigned int      crc24cTable[256];
static unsigned short    crc16Table[256];
static unsigned short    crc11Table[256];
static unsigned char     crc6Table[256];

void crcTableInit (void)
{
  unsigned char c = 0;

  do {
    crc24aTable[c] = crcbit (&c, 1, poly24a);
    crc24bTable[c] = crcbit (&c, 1, poly24b);
    crc24cTable[c] = crcbit (&c, 1, poly24c);
    crc16Table[c] = (unsigned short) (crcbit (&c, 1, poly16) >> 16);
    crc11Table[c] = (unsigned short) (crcbit (&c, 1, poly11) >> 16);
    crc6Table[c] = (unsigned char) (crcbit (&c, 1, poly6) >> 24);
  } while (++c);
}
/*********************************************************

Byte by byte implementations,
assuming initial byte is 0 padded (in MSB) if necessary

*********************************************************/
unsigned int crc24a (unsigned char * inptr, int bitlen)
{

  int octetlen, resbit;
  unsigned int crc = 0;
  octetlen = bitlen / 8;        /* Change in octets */
  resbit = (bitlen % 8);

  while (octetlen-- > 0) {
    //    printf("in %x => crc %x\n",crc,*inptr);
    crc = (crc << 8) ^ crc24aTable[(*inptr++) ^ (crc >> 24)];
  }

  if (resbit > 0)
    crc = (crc << resbit) ^ crc24aTable[((*inptr) >> (8 - resbit)) ^ (crc >> (32 - resbit))];

  return crc;
}

unsigned int crc24b (unsigned char * inptr, int bitlen)
{

  int octetlen, resbit;
  unsigned int crc = 0;
  octetlen = bitlen / 8;
  resbit = (bitlen % 8);

  while (octetlen-- > 0) {
    crc = (crc << 8) ^ crc24bTable[(*inptr++) ^ (crc >> 24)];
  }

  if (resbit > 0)
    crc = (crc << resbit) ^ crc24bTable[((*inptr) >> (8 - resbit)) ^ (crc >> (32 - resbit))];

  return crc;
}

unsigned int crc24c (unsigned char * inptr, int bitlen)
{

  int octetlen, resbit;
  unsigned int  crc = 0;
  octetlen = bitlen / 8;
  resbit = (bitlen % 8);

  while (octetlen-- > 0) {
    crc = (crc << 8) ^ crc24cTable[(*inptr++) ^ (crc >> 24)];
  }

  if (resbit > 0)
    crc = (crc << resbit) ^ crc24cTable[((*inptr) >> (8 - resbit)) ^ (crc >> (32 - resbit))];

  return crc;
}

unsigned int crc16 (unsigned char * inptr, int bitlen)
{
  int octetlen, resbit;
  unsigned int crc = 0;
  octetlen = bitlen / 8;        /* Change in octets */
  resbit = (bitlen % 8);

  while (octetlen-- > 0) {

    crc = (crc << 8) ^ (crc16Table[(*inptr++) ^ (crc >> 24)] << 16);
  }

  if (resbit > 0)
    crc = (crc << resbit) ^ (crc16Table[((*inptr) >> (8 - resbit)) ^ (crc >> (32 - resbit))] << 16);

  return crc;
}

unsigned int crc11 (unsigned char * inptr, int bitlen)
{
  int octetlen, resbit;
  unsigned int             crc = 0;
  octetlen = bitlen / 8;        /* Change in octets */
  resbit = (bitlen % 8);

  while (octetlen-- > 0) {
    crc = (crc << 8) ^ (crc11Table[(*inptr++) ^ (crc >> 24)] << 16);
  }

  if (resbit > 0)
    crc = (crc << resbit) ^ (crc11Table[((*inptr) >> (8 - resbit)) ^ (crc >> (32 - resbit))] << 16);

  return crc;
}

unsigned int crc6 (unsigned char * inptr, int bitlen)
{
  int octetlen, resbit;
  unsigned int crc = 0;
  octetlen = bitlen / 8;        /* Change in octets */
  resbit = (bitlen % 8);

  while (octetlen-- > 0) {
    crc = crc6Table[(*inptr++) ^ (crc >> 24)] << 24;
  }

  if (resbit > 0)
    crc = (crc << resbit) ^ (crc6Table[((*inptr) >> (8 - resbit)) ^ (crc >> (32 - resbit))] << 24);

  return crc;
}

uint8_t check_crc(uint8_t *decoded_bytes, uint16_t len, uint8_t crc_type)
{
  uint8_t crc_len,temp;
  uint16_t oldcrc,crc;

  switch (crc_type) {

  case 0: //CRC24_A:
  case 1: //CRC24_B:
    crc_len=3;
    break;

  case 2: //CRC16:
    crc_len=2;
    break;

  case 3: //CRC8:
    crc_len=1;
    break;

  default:
    crc_len=3;
  }

  // check the CRC
  oldcrc= *((unsigned int *)(&decoded_bytes[(len>>3)-crc_len]));

      switch (crc_type) {

      case 0: //CRC24_A:
        oldcrc&=0x00ffffff;
        crc = crc24a(decoded_bytes,len-24)>>8;
        temp=((uint8_t *)&crc)[2];
        ((uint8_t *)&crc)[2] = ((uint8_t *)&crc)[0];
        ((uint8_t *)&crc)[0] = temp;
        break;

      case 1: //CRC24_B:
        oldcrc&=0x00ffffff;
        crc = crc24b(decoded_bytes,len-24)>>8;
        temp=((uint8_t *)&crc)[2];
        ((uint8_t *)&crc)[2] = ((uint8_t *)&crc)[0];
        ((uint8_t *)&crc)[0] = temp;
        break;

      case 2: //CRC16:
        oldcrc&=0x0000ffff;
        crc = crc16(decoded_bytes,len-16)>>16;
        break;

      case 3: //CRC8:
        oldcrc&=0x000000ff;
        crc = crc6(decoded_bytes,len-8)>>24;
        break;

      default:
        printf("FATAL: Unknown CRC\n");
        return(255);
        break;
      }

      printf("old CRC %x, CRC %x \n",oldcrc,crc);
      return (crc == oldcrc); 
}

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


#ifdef DEBUG_CRC
/*******************************************************************/
/**   Test code
/*******************************************************************/

#include <stdio.h>

main()
{
  unsigned char test[] = "Thebigredfox";
  crcTableInit();
  printf("%x\n", crcbit(test, sizeof(test) - 1, poly24a));
  printf("%x\n", crc24a(test, (sizeof(test) - 1)*8));
  printf("%x\n", crcbit(test, sizeof(test) - 1, poly6));
  printf("%x\n", crc6(test, (sizeof(test) - 1)*8));
}
#endif

