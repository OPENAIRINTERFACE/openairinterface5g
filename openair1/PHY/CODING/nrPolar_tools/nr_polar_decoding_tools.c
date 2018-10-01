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
//#define SHOWCOMP 1

inline void computeLLR(double llr[1+nmax][Nmax], uint16_t row, uint16_t col, 
		       uint16_t offset, uint8_t approximation) __attribute__((always_inline));
inline void computeLLR(double llr[1+nmax][Nmax], uint16_t row, uint16_t col, 
		       uint16_t offset, uint8_t approximation) {

        double a;
        double b;
	double absA,absB;


	a = llr[col + 1][row];   
	b = llr[col+1][row + offset];
	
	if (approximation) { //eq. (9)
	  absA = fabs(a);
	  absB = fabs(b);
	  llr[col][row] = copysign(1.0, a) * copysign(1.0, b) * fmin(absA, absB);
	} else { //eq. (8a)
	  llr[col][row] = log((exp(a + b) + 1) / (exp(a) + exp(b)));
	}
	//	printf("LLR (a %f, b %f): llr[%d][%d] %f\n",32*a,32*b,col,row,32*llr[col][row]);
}

inline void computeLLR_int8(int16_t llr[1+nmax][Nmax], uint16_t row, uint16_t col, 
		       uint16_t offset) __attribute__((always_inline));
inline void computeLLR_int8(int16_t llr[1+nmax][Nmax], uint16_t row, uint16_t col, 
		       uint16_t offset) {

        int16_t a;
        int16_t b;
	int16_t absA,absB;
	int16_t maska,maskb;
	int16_t minabs;

#ifdef SHOWCOMP
	printf("computeLLR_int8(llr,%d,%d,%d);\n",row,col,offset);
#endif
	a = llr[col + 1][row];   
	b = llr[col+1][row + offset];
	//	printf("LLR: a %d, b %d\n",a,b);
	maska = a>>15;
	maskb = b>>15;
	absA = (a+maska)^maska;
	absB = (b+maskb)^maskb;
	//	printf("LLR: absA %d, absB %d\n",absA,absB);
	minabs = absA<absB ? absA : absB;
	if ((maska^maskb) == 0)
	  llr[col][row] = minabs;
	else 
	  llr[col][row] = -minabs;
	//	printf("LLR (a %d, b %d): llr[%d][%d] %d\n",a,b,col,row,llr[col][row]);
}


void updateLLR(decoder_list_t **dlist,uint8_t **llrU, uint8_t **bitU,
	       uint8_t listSize, uint16_t row, uint16_t col, uint16_t xlen, uint8_t ylen, uint8_t approximation) {
  uint16_t offset = (xlen/(1<<(ylen-col-1)));
  if (( (row) % (2*offset) ) >= offset ) {
    if (bitU[row-offset][col]==0) updateBit(dlist, bitU, listSize, (row-offset), col, xlen, ylen);
    if (llrU[row-offset][col+1]==0) updateLLR(dlist, llrU, bitU, listSize, (row-offset), (col+1), xlen, ylen, approximation);
    if (llrU[row][col+1]==0) updateLLR(dlist, llrU, bitU, listSize, row, (col+1), xlen, ylen, approximation);
    for (uint8_t i=0; i<listSize; i++) {
      dlist[i]->llr[col][row] = (pow((-1),dlist[i]->bit[col][row-offset])*dlist[i]->llr[col+1][row-offset]) + dlist[i]->llr[col+1][row];
    }
  } else {
    if (llrU[row][col+1]==0) updateLLR(dlist, llrU, bitU, listSize, row, (col+1), xlen, ylen, approximation);
    if (llrU[row+offset][col+1]==0) updateLLR(dlist, llrU, bitU, listSize, (row+offset), (col+1), xlen, ylen, approximation);
    for (int i=0;i<listSize;i++) computeLLR(dlist[i]->llr, row, col, offset, approximation);
  }
  
  llrU[row][col]=1;
}

#define updateLLR_int8_A(dlist,i,col,row,offset) if (dlist[(i)]->bit[(col)][(row)-(offset)]==0) dlist[(i)]->llr[(col)][(row)] =  dlist[(i)]->llr[(col)+1][(row)-(offset)] + dlist[(i)]->llr[(col)+1][(row)]; else dlist[(i)]->llr[(col)][(row)] = -dlist[(i)]->llr[(col)+1][(row)-(offset)] + dlist[(i)]->llr[(col)+1][(row)];


void updateLLR_int8(decoder_list_int8_t **dlist,uint8_t **llrU, uint8_t **bitU,
		    uint8_t listSize, uint16_t row, uint16_t col, uint16_t xlen, uint8_t ylen) {
  uint16_t offset = (xlen/(1<<(ylen-col-1)));
  if (( (row) % (2*offset) ) >= offset ) {
    if (bitU[row-offset][col]==0) updateBit_int8(dlist, bitU, listSize, (row-offset), col, xlen, ylen);
    if (llrU[row-offset][col+1]==0) updateLLR_int8(dlist, llrU, bitU, listSize, (row-offset), (col+1), xlen, ylen);
    if (llrU[row][col+1]==0) updateLLR_int8(dlist, llrU, bitU, listSize, row, (col+1), xlen, ylen);

      
    for (uint8_t i=0; i<listSize; i++) {
#ifdef SHOWCOMP
      printf("updateLLR_int8_A(dlist,%d,%d,%d,%d);\n",i,row,col,offset);
#endif
      updateLLR_int8_A(dlist,i,col,row,offset);
      /*
      if (dlist[i]->bit[col][row-offset]==0) 
	dlist[i]->llr[col][row] =  dlist[i]->llr[col+1][row-offset] + dlist[i]->llr[col+1][row];
      else
	dlist[i]->llr[col][row] = -dlist[i]->llr[col+1][row-offset] + dlist[i]->llr[col+1][row];*/
      }
  } else {
    if (llrU[row][col+1]==0) updateLLR_int8(dlist, llrU, bitU, listSize, row, (col+1), xlen, ylen);
    if (llrU[row+offset][col+1]==0) updateLLR_int8(dlist, llrU, bitU, listSize, (row+offset), (col+1), xlen, ylen);
    for (int i=0;i<listSize;i++) computeLLR_int8(dlist[i]->llr, row, col, offset);
  }
  
  llrU[row][col]=1;
}

void updateBit(decoder_list_t **dlist, uint8_t **bitU, uint8_t listSize, uint16_t row,
	       uint16_t col, uint16_t xlen, uint8_t ylen) {
  uint16_t offset = ( xlen/(pow(2,(ylen-col))) );
  
  for (uint8_t i=0; i<listSize; i++) {
    if (( (row) % (2*offset) ) >= offset ) {
      if (bitU[row][col-1]==0) updateBit(dlist, bitU, listSize, row, (col-1), xlen, ylen);
      dlist[i]->bit[col][row] = dlist[i]->bit[col-1][row];
    } else {
      if (bitU[row][col-1]==0) updateBit(dlist, bitU, listSize, row, (col-1), xlen, ylen);
      if (bitU[row+offset][col-1]==0) updateBit(dlist, bitU, listSize, (row+offset), (col-1), xlen, ylen);
      dlist[i]->bit[col][row] = ( (dlist[i]->bit[col-1][row]+dlist[i]->bit[col-1][row+offset]) % 2);
    }
  }
  
  bitU[row][col]=1;
}

#define updateBit_int8_A(dlist,i,col,row) dlist[(i)]->bit[(col)][(row)] = dlist[(i)]->bit[(col)-1][(row)]
#define updateBit_int8_B(dlist,i,col,row,offset) dlist[(i)]->bit[(col)][(row)] = dlist[(i)]->bit[(col)-1][(row)]^dlist[(i)]->bit[(col)-1][(row)+(offset)]

void updateBit_int8(decoder_list_int8_t **dlist, uint8_t **bitU, uint8_t listSize, uint16_t row,
		    uint16_t col, uint16_t xlen, uint8_t ylen) {
  uint16_t offset = ( xlen/(pow(2,(ylen-col))) );
  
  for (uint8_t i=0; i<listSize; i++) {
    if (( (row) % (2*offset) ) >= offset ) {
      if (bitU[row][col-1]==0) updateBit_int8(dlist, bitU, listSize, row, (col-1), xlen, ylen);
      //      dlist[i]->bit[col][row] = dlist[i]->bit[col-1][row];
#ifdef SHOWCOMP
      printf("updateBit_int8_A(dlist,%d,%d,%d);\n",i,col,row);
#endif
      updateBit_int8_A(dlist,i,col,row);

    } else {
      if (bitU[row][col-1]==0) updateBit_int8(dlist, bitU, listSize, row, (col-1), xlen, ylen);
      if (bitU[row+offset][col-1]==0) updateBit_int8(dlist, bitU, listSize, (row+offset), (col-1), xlen, ylen);
      //      dlist[i]->bit[col][row] = dlist[i]->bit[col-1][row]^dlist[i]->bit[col-1][row+offset];
      //      printf("updating dlist[%d]->bit[%d][%d] => %d\n",i,col,row,dlist[i]->bit[col][row]);
#ifdef SHOWCOMP
      printf("updateBit_int8_B(dlist,%d,%d,%d,%d);\n",i,col,row,offset);
#endif
      updateBit_int8_B(dlist,i,col,row,offset);
    }
  }
  
  bitU[row][col]=1;
}
 
void updatePathMetric(decoder_list_t **dlist,uint8_t listSize, uint8_t bitValue,
		       uint16_t row, uint8_t approximation) {
   
  if (approximation) { //eq. (12)
    for (uint8_t i=0; i<listSize; i++) {
      if ((2*bitValue) != ( 1 - copysign(1.0,dlist[i]->llr[0][row]) )) dlist[i]->pathMetric += fabs(dlist[i]->llr[0][row]);
     }
  } else { //eq. (11b)
    int8_t multiplier = (2*bitValue) - 1;
    for (uint8_t i=0; i<listSize; i++) {
      dlist[i]->pathMetric += log ( 1 + exp(multiplier*dlist[i]->llr[0][row]) ) ;
    }  
  }
  
}
 
#define updatePathMetric0_int8_A(dlist,i,row,mask,absllr) { mask=dlist[i]->llr[0][row]>>15;if(mask!=0){absllr=(dlist[i]->llr[0][row]+mask)^mask;dlist[i]->pathMetric+=absllr;}}

void updatePathMetric0_int8(decoder_list_int8_t **dlist,uint8_t listSize, uint16_t row) {

  int16_t mask,absllr;
  for (uint8_t i=0; i<listSize; i++) {

    updatePathMetric0_int8_A(dlist,i,row,mask,absllr);
#ifdef SHOWCOMP
    printf("updatePathMetric0_int8_A(dlist,i,%d,%d);\n",listSize,row);
#endif


    /*
      mask = dlist[i]->llr[0][row]>>15;
      
      if (mask != 0) {
        int16_t absllr = (dlist[i]->llr[0][row]+mask)^mask; 
        dlist[i]->pathMetric += absllr;
	}*/


  }

}

void updatePathMetric2(decoder_list_t **dlist, uint8_t listSize, uint16_t row, uint8_t appr) {

  int i;

  for (i=0;i<listSize;i++) dlist[i+listSize]->pathMetric = dlist[i]->pathMetric;
  decoder_list_t **dlist2 = &dlist[listSize];

  if (appr) { //eq. (12)
    for (i = 0; i < listSize; i++) {
      // bitValue=0
      if (dlist[i]->llr[0][row]<0) dlist[i]->pathMetric  -= dlist[i]->llr[0][row];
       // bitValue=1
      else                         dlist2[i]->pathMetric += dlist[i]->llr[0][row];
    }
  } else { //eq. (11b)
    for (i = 0; i < listSize; i++) {
      // bitValue=0
       dlist[i]->pathMetric += log(1 + exp(-dlist[i]->llr[0][row]));
      // bitValue=1
       dlist2[i]->pathMetric += log(1 + exp(dlist[i]->llr[0][row]));

    }
  }
}

#define updatePathMetric2_int8_A(dlist,i,listSize,row)     {dlist[i+listSize]->pathMetric = dlist[i]->pathMetric;if (dlist[i]->llr[0][row]<0) dlist[i]->pathMetric-=dlist[i]->llr[0][row];else dlist[i+listSize]->pathMetric += dlist[i]->llr[0][row];}


void updatePathMetric2_int8(decoder_list_int8_t **dlist, uint8_t listSize, uint16_t row) {

  int i;


  for (i = 0; i < listSize; i++) {
#ifdef SHOWCOMP
    printf("updatePathMetric2_int8_A(dlist,%d,%d,%d);\n",
	   i,listSize,row);
#endif  
    updatePathMetric2_int8_A(dlist,i,listSize,row);
    //    dlist[i+listSize]->pathMetric = dlist[i]->pathMetric;
    //if (dlist[i]->llr[0][row]<0) dlist[i]->pathMetric  -= dlist[i]->llr[0][row];
    //else                         dlist[i+listSize]->pathMetric += dlist[i]->llr[0][row];
  }
}


void updateCrcChecksum(decoder_list_t **dlist, uint8_t **crcGen,
		       uint8_t listSize, uint32_t i2, uint8_t len) {
  for (uint8_t i = 0; i < listSize; i++) {
    for (uint8_t j = 0; j < len; j++) {
      dlist[i]->crcChecksum[j] = ( (dlist[i]->crcChecksum[j] + crcGen[i2][j]) % 2 );
    }
  }
}

void updateCrcChecksum2(decoder_list_t **dlist, uint8_t **crcGen,
			uint8_t listSize, uint32_t i2, uint8_t len) {
  for (uint8_t i = 0; i < listSize; i++) {
    for (uint8_t j = 0; j < len; j++) {
      dlist[i+listSize]->crcChecksum[j] = ( (dlist[i]->crcChecksum[j] + crcGen[i2][j]) % 2 );
    }
  }
}

#define updateCrcChecksum_int8_A(dlist,i,crcGen,i2,len) {for (uint8_t j = 0; j < len; j++) dlist[i]->crcChecksum[j] = (dlist[i]->crcChecksum[j]^crcGen[i2][j]);}
    
void updateCrcChecksum_int8(decoder_list_int8_t **dlist, uint8_t **crcGen,
		       uint8_t listSize, uint32_t i2, uint8_t len) {
  for (uint8_t i = 0; i < listSize; i++) {
#ifdef SHOWCOMP
    printf("updateCrcChecksum_int8_A(dlist,%d,crcGen,%d,%d);\n",i,i2,len);
#endif
    updateCrcChecksum_int8_A(dlist,i,crcGen,i2,len);
    //    for (uint8_t j = 0; j < len; j++) {
    //      dlist[i]->crcChecksum[j] = ( (dlist[i]->crcChecksum[j] + crcGen[i2][j]) % 2 );
    //    }
  }
}

#define updateCrcChecksum2_int8_A(dlist,i,listSize,crcGen,i2,len) {for (uint8_t j = 0; j < len; j++) dlist[i+listSize]->crcChecksum[j]=dlist[i]->crcChecksum[j]^crcGen[i2][j];}

void updateCrcChecksum2_int8(decoder_list_int8_t **dlist, uint8_t **crcGen,
			uint8_t listSize, uint32_t i2, uint8_t len) {
  for (uint8_t i = 0; i < listSize; i++) {
#ifdef SHOWCOMP
    printf("updateCrcChecksum2_int8_A(dlist,%d,%d,crcGen,%d,%d);\n",i,listSize,i2,len);
#endif
    updateCrcChecksum2_int8_A(dlist,i,listSize,crcGen,i2,len);
    //    for (uint8_t j = 0; j < len; j++) {
    //      dlist[i+listSize]->crcChecksum[j] = ( (dlist[i]->crcChecksum[j] + crcGen[i2][j]) % 2 );
    //    }
  }
}
