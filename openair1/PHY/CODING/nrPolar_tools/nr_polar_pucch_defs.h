/*
 * Defines the constant variables for polar coding of the PUCCH.
 */

#ifndef __NR_POLAR_PUCCH_DEFS__H__
#define __NR_POLAR_PUCCH_DEFS__H__

#define NR_POLAR_PUCCH_PAYLOAD_BITS 16

//Ref. 38-212 v15.0.1
#define NR_POLAR_PUCCH_N_MAX 10   //uint8_t <------
#define NR_POLAR_PUCCH_I_IL 1    //uint8_t <--- interleaving: if 0 no interleaving
#define NR_POLAR_PUCCH_N_PC 3    //uint8_t <-- or zero ??
#define NR_POLAR_PUCCH_N_PC_WM 0 //uint8_t
//#define NR_POLAR_PUCCH_N 512     //uint16_t

//Ref. 38-212 v15.0.1, Section 7.1.5: Rate Matching
#define NR_POLAR_PUCCH_I_BIL 0 //uint8_t
#define NR_POLAR_PUCCH_E 864   //uint16_t


/*
 * TEST CODE
 */
//unsigned int testPayload0=0x00000000, testPayload1=0xffffffff; //payload1=~payload0;
//unsigned int testPayload2=0xa5a5a5a5, testPayload3=0xb3f02c82;

#endif
