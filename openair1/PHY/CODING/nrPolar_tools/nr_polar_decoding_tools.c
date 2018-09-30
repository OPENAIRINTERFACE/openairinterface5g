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


#ifdef SHOWCOMP
	printf("computeLLR (%d,%d,%d)\n",row,col,offset);
#endif
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
	printf("computeLLR_int8 (%d,%d,%d)\n",row,col,offset);
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
#ifdef SHOWCOMP
      printf("updatingLLR (%d,%d,%d) (bit %d,%d,%d, llr0 %d,%d,%d, llr1 %d,%d,%d \n",row,col,i,
	     row-offset,col,i,row-offset,col+1,i,row,col+1,i);
#endif
      dlist[i]->llr[col][row] = (pow((-1),dlist[i]->bit[col][row-offset])*dlist[i]->llr[col+1][row-offset]) + dlist[i]->llr[col+1][row];
      //      printf("updating dlist[%d]->llr[%d][%d] => %f (%f,%f) offset %d\n",i,col,row,32*dlist[i]->llr[col][row],
      //	     (pow((-1),dlist[i]->bit[col][row-offset])*32*dlist[i]->llr[col+1][row-offset]),32*dlist[i]->llr[col+1][row],offset);
    }
  } else {
    if (llrU[row][col+1]==0) updateLLR(dlist, llrU, bitU, listSize, row, (col+1), xlen, ylen, approximation);
    if (llrU[row+offset][col+1]==0) updateLLR(dlist, llrU, bitU, listSize, (row+offset), (col+1), xlen, ylen, approximation);
    for (int i=0;i<listSize;i++) computeLLR(dlist[i]->llr, row, col, offset, approximation);
  }
  
  llrU[row][col]=1;
}

void updateLLR_int8(decoder_list_int8_t **dlist,uint8_t **llrU, uint8_t **bitU,
		    uint8_t listSize, uint16_t row, uint16_t col, uint16_t xlen, uint8_t ylen) {
  uint16_t offset = (xlen/(1<<(ylen-col-1)));
  if (( (row) % (2*offset) ) >= offset ) {
    if (bitU[row-offset][col]==0) updateBit_int8(dlist, bitU, listSize, (row-offset), col, xlen, ylen);
    if (llrU[row-offset][col+1]==0) updateLLR_int8(dlist, llrU, bitU, listSize, (row-offset), (col+1), xlen, ylen);
    if (llrU[row][col+1]==0) updateLLR_int8(dlist, llrU, bitU, listSize, row, (col+1), xlen, ylen);
    for (uint8_t i=0; i<listSize; i++) {
#ifdef SHOWCOMP
      printf("updatingLLR_int8 (%d,%d,%d) (bit %d,%d,%d, llr0 %d,%d,%d, llr1 %d,%d,%d \n",row,col,i,
	     row-offset,col,i,row-offset,col+1,i,row,col+1,i);
#endif
      if (dlist[i]->bit[col][row-offset]==0) 
	dlist[i]->llr[col][row] =  dlist[i]->llr[col+1][row-offset] + dlist[i]->llr[col+1][row];
      else
	dlist[i]->llr[col][row] = -dlist[i]->llr[col+1][row-offset] + dlist[i]->llr[col+1][row];
      //      printf("updating dlist[%d]->llr[%d][%d] => %d (%d,%d) offset %d\n",i,col,row,dlist[i]->llr[col][row],
      //	(dlist[i]->bit[col][row-offset]==0 ? 1 : -1)*dlist[i]->llr[col+1][row-offset],dlist[i]->llr[col+1][row], offset);
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
#ifdef SHOWCOMP
      printf("updating bit (%d,%d,%d) from (%d,%d,%d)\n",
	     row,col,i,row,col-1,i);
#endif
    } else {
      if (bitU[row][col-1]==0) updateBit(dlist, bitU, listSize, row, (col-1), xlen, ylen);
      if (bitU[row+offset][col-1]==0) updateBit(dlist, bitU, listSize, (row+offset), (col-1), xlen, ylen);
      dlist[i]->bit[col][row] = ( (dlist[i]->bit[col-1][row]+dlist[i]->bit[col-1][row+offset]) % 2);
      //      printf("updating dlist[%d]->bit[%d][%d] => %d\n",i,col,row,dlist[i]->bit[col][row]);
#ifdef SHOWCOMP
      printf("updating bit (%d,%d,%d) from (%d,%d,%d)+(%d,%d,%d)\n",
	     row,col,i,row,col-1,i,row+offset,col-1,i);
#endif
    }
  }
  
  bitU[row][col]=1;
}

void updateBit_int8(decoder_list_int8_t **dlist, uint8_t **bitU, uint8_t listSize, uint16_t row,
		    uint16_t col, uint16_t xlen, uint8_t ylen) {
  uint16_t offset = ( xlen/(pow(2,(ylen-col))) );
  
  for (uint8_t i=0; i<listSize; i++) {
    if (( (row) % (2*offset) ) >= offset ) {
      if (bitU[row][col-1]==0) updateBit_int8(dlist, bitU, listSize, row, (col-1), xlen, ylen);
      dlist[i]->bit[col][row] = dlist[i]->bit[col-1][row];
#ifdef SHOWCOMP
      printf("updating bit (%d,%d,%d) from (%d,%d,%d)\n",
	     row,col,i,row,col-1,i);
#endif
    } else {
      if (bitU[row][col-1]==0) updateBit_int8(dlist, bitU, listSize, row, (col-1), xlen, ylen);
      if (bitU[row+offset][col-1]==0) updateBit_int8(dlist, bitU, listSize, (row+offset), (col-1), xlen, ylen);
      dlist[i]->bit[col][row] = dlist[i]->bit[col-1][row]^dlist[i]->bit[col-1][row+offset];
      //      printf("updating dlist[%d]->bit[%d][%d] => %d\n",i,col,row,dlist[i]->bit[col][row]);
#ifdef SHOWCOMP
      printf("updating bit (%d,%d,%d) from (%d,%d,%d)+(%d,%d,%d)\n",
	     row,col,i,row,col-1,i,row+offset,col-1,i);
#endif
    }
  }
  
  bitU[row][col]=1;
}

void updatePathMetric(decoder_list_t **dlist,uint8_t listSize, uint8_t bitValue,
		uint16_t row, uint8_t approximation) {

#ifdef SHOWCOMP
  printf("updating path_metric from Frozen bit (%d,%d) \n",
	 row,0);
#endif

	if (approximation) { //eq. (12)
		for (uint8_t i=0; i<listSize; i++) {
			if ((2*bitValue) != ( 1 - copysign(1.0,dlist[i]->llr[0][row]) )) dlist[i]->pathMetric += fabs(dlist[i]->llr[0][row]);
			//			printf("updatepathmetric : llr %f pathMetric %f (bitValue %d)\n",32*dlist[i]->llr[0][row],32*dlist[i]->pathMetric,bitValue);
		}
	} else { //eq. (11b)
      int8_t multiplier = (2*bitValue) - 1;
      for (uint8_t i=0; i<listSize; i++) {
      dlist[i]->pathMetric += log ( 1 + exp(multiplier*dlist[i]->llr[0][row]) ) ;
      //      printf("updatepathmetric : llr %f pathMetric %f\n",32*dlist[i]->llr[0][row],32*dlist[i]->pathMetric);
    }  
	}

}


void updatePathMetric0_int8(decoder_list_int8_t **dlist,uint8_t listSize, uint16_t row) {

#ifdef SHOWCOMP
  printf("updating path_metric from Frozen bit (%d,%d) \n",
	 row,0);
#endif
  int16_t mask;
  for (uint8_t i=0; i<listSize; i++) {
    //    if ((2*bitValue) != ( 1 - copysign(1.0,dlist[i]->llr[0][row]) )) dlist[i]->pathMetric += fabs(dlist[i]->llr[0][row]);
    // equiv: if ((llr>0 && bitValue==1) || (llr<0 && bitValue==0) ...
    // equiv: (llr>>7 + bitValue) != 0, in opposite case (llr>8 + bitValue) = -1 or 1
    

      mask = dlist[i]->llr[0][row]>>15;
      
      if (mask != 0) {
        int16_t absllr = (dlist[i]->llr[0][row]+mask)^mask; 
        dlist[i]->pathMetric += absllr;
      }
      //      printf("updatepathmetric : llr %d, pathMetric %d (bitValue %d)\n",dlist[i]->llr[0][row],dlist[i]->pathMetric);  
    }

}

void updatePathMetric2(decoder_list_t **dlist, uint8_t listSize, uint16_t row, uint8_t appr) {

  int i;

  for (i=0;i<listSize;i++) dlist[i+listSize]->pathMetric = dlist[i]->pathMetric;
  decoder_list_t **dlist2 = &dlist[listSize];

#ifdef SHOWCOMP
  printf("updating path_metric from information bit (%d,%d) \n",
	 row,0);
#endif  
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

void updatePathMetric2_int8(decoder_list_int8_t **dlist, uint8_t listSize, uint16_t row) {

  int i;

  for (i=0;i<listSize;i++) dlist[i+listSize]->pathMetric = dlist[i]->pathMetric;
  decoder_list_int8_t **dlist2 = &dlist[listSize];

#ifdef SHOWCOMP
  printf("updating path_metric from information bit (%d,%d) \n",
	 row,0);
#endif  
  for (i = 0; i < listSize; i++) {
    // bitValue=0
    if (dlist[i]->llr[0][row]<0) dlist[i]->pathMetric  -= dlist[i]->llr[0][row];
    // bitValue=1
    else                         dlist2[i]->pathMetric += dlist[i]->llr[0][row];
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

void updateCrcChecksum_int8(decoder_list_int8_t **dlist, uint8_t **crcGen,
		       uint8_t listSize, uint32_t i2, uint8_t len) {
  for (uint8_t i = 0; i < listSize; i++) {
    for (uint8_t j = 0; j < len; j++) {
      dlist[i]->crcChecksum[j] = ( (dlist[i]->crcChecksum[j] + crcGen[i2][j]) % 2 );
    }
  }
}

void updateCrcChecksum2_int8(decoder_list_int8_t **dlist, uint8_t **crcGen,
			uint8_t listSize, uint32_t i2, uint8_t len) {
  for (uint8_t i = 0; i < listSize; i++) {
    for (uint8_t j = 0; j < len; j++) {
      dlist[i+listSize]->crcChecksum[j] = ( (dlist[i]->crcChecksum[j] + crcGen[i2][j]) % 2 );
    }
  }
}
