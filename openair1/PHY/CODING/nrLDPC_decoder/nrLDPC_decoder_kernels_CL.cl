/*
 * Licensed to the OpenAirInterface (OAI) Software Alliance under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The OpenAirInterface Software Alliance licenses this file to You under
 * the OAI Public License, Version 1.0  (the "License"); you may not use this file
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

/*! \file PHY/CODING/nrLDPC_decoder_kernels_CL.cl
* \brief kernel functions for ldpc decoder accelerated via openCL
* \author Francois TABURET
* \date 2021
* \version 1.0
* \company Nokia BellLabs France
* \email: francois.taburet@nokia-bell-labs.com
* \note initial implem - translation of cuda version
* \warning
*/
#define define MAX_ITERATION 2
#define MC	1

#define INT32_MAX 2147483647

typedef struct{
  char x;
  char y;
  short value;
} h_element;

//__global char dev_dt [46*68*384];
//__local char *dev_t;
//__global char dev_llr[68*384];
//__global unsigned char dev_tmp[68*384];

__constant h_element h_compact1 [46*19] = {};
__constant h_element h_compact2 [68*30] = {};

__constant h_element dev_h_compact1[46*19] = {};  // used in kernel 1
__constant h_element dev_h_compact2[68*30] = {};  // used in kernel 2

// __device__ __constantant__ h_element dev_h_compact1[46*19];  // used in kernel 1
// __device__ __constantant__ h_element dev_h_compact2[68*30];  // used in kernel 2

// row and col element count
__constant char h_ele_row_bg1_count[46] = {
	19, 19, 19, 19, 3,  8,  9,  7, 10,  9,
	7,  8,  7,  6,  7,  7,  6,  6,  6,  6,  
	6,  6,  5,  5,  6,  5,  5,  4,  5,  5,  
	5,  5,  5,  5,  5,  5,  5,  4,  5,  5,  
	4,  5,  4,  5,  5,  4};
__constant char h_ele_col_bg1_count[68] = {
	30, 28,  7, 11,  9,  4,  8, 12,  8,  7, 
	12, 10, 12, 11, 10,  7, 10, 10, 13,  7,  
	8,  11, 12,  5,  6,  6,  1,  1,  1,  1,  
	1,   1,  1,  1,  1,  1,  1,  1,  1,  1,  
	1,   1,  1,  1,  1,  1,  1,  1,  1,  1,  
	1,   1,  1,  1,  1,  1,  1,  1,  1,  1,  
	1,   1,  1,  1,  1,  1,  1,  1};
__constant char h_ele_row_bg2_count[42] = { 
	8, 10,  8, 10,  4,  6,  6,  6,  4,  5, 
	5,  5,  4,  5,  5,  4,  5,  5,  4,  4,
	4,  4,  3,  4,  4,  3,  5,  3,  4,  3,
	5,  3,  4,  4,  4,  4,  4,  3,  4,  4, 
	4,  4};
__constant char h_ele_col_bg2_count[52] = {
	22, 23, 10,  5,  5, 14,  7, 13,  6,  8, 
	9,  16,  9, 12,  1,  1,  1,  1,  1,  1,
	1,   1,  1,  1,  1,  1,  1,  1,  1,  1,
	1,   1,  1,  1,  1,  1,  1,  1,  1,  1,
	1,   1,  1,  1,  1,  1,  1,  1,  1,  1, 
	1,   1};



// Kernel 1
__kernel void ldpc_cnp_kernel_1st_iter( __local char * dev_llr, __local char * dev_dt, int BG, int row, int col, int Zc)
{
//	if(blockIdx.x == 0 && threadIdx.x == 1) printf("cnp %d\n", threadIdx.x);
	int iMCW = get_group_id(1);		// codeword id
	int iBlkRow = get_group_id(0);	// block row in h_base
	int iBlkCol;				// block col in h_base
	int iSubRow = get_local_id(0);;	// row index in sub_block of h_base
	int iCol;					// overall col index in h_base
	int offsetR;
	int shift_t;

//	For 2-min algorithm.
	int Q_sign = 0;
	int sq;
	int Q, Q_abs;
	int R_temp;

	int sign = 1;
	int rmin1 = INT32_MAX;
	int rmin2 = INT32_MAX;
	char idx_min = 0;

	h_element h_element_t;
	int s = (BG==1)? h_ele_row_bg1_count[iBlkRow]:h_ele_row_bg2_count[iBlkRow];
	offsetR = (iMCW * row*col*Zc) + iBlkRow * Zc + iSubRow;	// row*col*Zc = size of dev_dt
//	if(blockIdx.x == 0 && threadIdx.x == 1) printf("s: %d, offset %d\n", s, offsetR);
//	The 1st recursion
	for(int i = 0; i < s; i++) // loop through all the ZxZ sub-blocks in a row
	{
		h_element_t = dev_h_compact1[i*row+iBlkRow];	// compact_col == row

		iBlkCol = h_element_t.y;
		shift_t = h_element_t.value;

		shift_t = (iSubRow + shift_t) % Zc;
		iCol = (iMCW * col*Zc) + iBlkCol * Zc + shift_t;	// col*Zc = size of llr
		Q = dev_llr[iCol];
		Q_abs = (Q>0)? Q : -Q;
		sq = Q < 0;
//		if(blockIdx.x == 0 && threadIdx.x == 1) printf("i %d, icol %d, Q: %d\n", i, iCol, Q);
		// quick version
		sign = sign * (1 - sq * 2);
		Q_sign |= sq << i;

		if (Q_abs < rmin1){
			rmin2 = rmin1;
			rmin1 = Q_abs;
			idx_min = i;
		} else if (Q_abs < rmin2){
			rmin2 = Q_abs;
		}
	}

//	if(blockIdx.x == 0 && threadIdx.x == 1)printf("min1 %d, min2 %d, min1_idx %d\n", rmin1, rmin2, idx_min);

//	The 2nd recursion
	for(int i = 0; i < s; i++){
		// v0: Best performance so far. 0.75f is the value of alpha.
		sq = 1 - 2 * ((Q_sign >> i) & 0x01);
		R_temp = 0.75f * sign * sq * (i != idx_min ? rmin1 : rmin2);
		// write results to global memory
		h_element_t = dev_h_compact1[i*row+iBlkRow];
		int addr_temp = offsetR + h_element_t.y * row * Zc;
		dev_dt[addr_temp] = R_temp;
//		if(blockIdx.x == 0 && threadIdx.x == 1)printf("R_temp %d, temp_addr %d\n", R_temp, addr_temp);
	}	
}

// Kernel_1
__kernel void ldpc_cnp_kernel( __local char * dev_llr, __local char * dev_dt, int BG, int row, int col, int Zc)
{
//	if(blockIdx.x == 0 && threadIdx.x == 1) printf("cnp\n");
	int iMCW = get_group_id(1);
	int iBlkRow = get_group_id(0);	// block row in h_base
	int iBlkCol; 				// block col in h_base
	int iSubRow = get_local_id(0);;	// row index in sub_block of h_base
	int iCol; 					// overall col index in h_base
	int offsetR;
	int shift_t;

//	For 2-min algorithm.
	int Q_sign = 0;
	int sq;
	int Q, Q_abs;
	int R_temp;

	int sign = 1;
	int rmin1 = INT32_MAX;
	int rmin2 = INT32_MAX;
	char idx_min = 0;

	h_element h_element_t;
	int s = (BG==1)? h_ele_row_bg1_count[iBlkRow]: h_ele_row_bg2_count[iBlkRow];
	offsetR = (iMCW *row*col*Zc) + iBlkRow * Zc + iSubRow;	// row * col * Zc = size of dev_dt
//	if(blockIdx.x == 0 && threadIdx.x == 1) printf("s: %d, offset %d\n", s, offsetR);
//	The 1st recursion
	for(int i = 0; i < s; i++) // loop through all the ZxZ sub-blocks in a row
	{
		h_element_t = dev_h_compact1[i*row+iBlkRow];

		iBlkCol = h_element_t.y;
		shift_t = h_element_t.value;
		shift_t = (iSubRow + shift_t) % Zc;
		iCol = iBlkCol * Zc + shift_t;
		
		R_temp = dev_dt[offsetR + iBlkCol * row * Zc];

		Q = dev_llr[iMCW * (col*Zc) + iCol] - R_temp;
		Q_abs = (Q>0)? Q : -Q;
//		if(blockIdx.x == 0 && threadIdx.x == 1) printf("i %d, icol %d, Q: %d\n", i, iCol, Q);
		sq = Q < 0;
		sign = sign * (1 - sq * 2);
		Q_sign |= sq << i;

		if (Q_abs < rmin1){
			rmin2 = rmin1;
			rmin1 = Q_abs;
			idx_min = i;
		} else if (Q_abs < rmin2){
			rmin2 = Q_abs;
		}
		
	}

//	if(blockIdx.x == 0 && threadIdx.x == 1)printf("min1 %d, min2 %d, min1_idx %d\n", rmin1, rmin2, idx_min);
	
//	The 2nd recursion
	for(int i = 0; i < s; i ++){
		sq = 1 - 2 * ((Q_sign >> i) & 0x01);
		R_temp = 0.75f * sign * sq * (i != idx_min ? rmin1 : rmin2);
		

		// write results to global memory
		h_element_t = dev_h_compact1[i*row+iBlkRow];
		int addr_temp = h_element_t.y * row * Zc + offsetR;
		dev_dt[addr_temp] = R_temp;
//		if(blockIdx.x == 0 && threadIdx.x == 1)printf("R_temp %d, temp_addr %d\n", R_temp, addr_temp);
	}
}

// Kernel 2: VNP processing
__kernel void
ldpc_vnp_kernel_normal(__local char * dev_llr, __local char * dev_dt, /* char * dev_const_llr,*/ int BG, int row, int col, int Zc)
{	
	int iMCW = get_group_id(1);
	int iBlkCol = get_group_id(0);
	int iBlkRow;
	int iSubCol = get_local_id(0);
	int iRow;
	int iCol;

	int shift_t, sf;
	int APP;

	h_element h_element_t;

	// update all the llr values
	iCol = iBlkCol * Zc + iSubCol;
	APP = dev_const_llr[iMCW *col*Zc + iCol];
	int offsetDt = iMCW *row*col*Zc + iBlkCol * row * Zc;
	int s = (BG==1)? h_ele_col_bg1_count[iBlkCol]:h_ele_col_bg2_count[iBlkCol];
	
	for(int i = 0; i < s; i++)
	{
		h_element_t = dev_h_compact2[i*col+iBlkCol];

		shift_t = h_element_t.value%Zc;
		iBlkRow = h_element_t.x;

		sf = iSubCol - shift_t;
		sf = (sf + Zc) % Zc;

		iRow = iBlkRow * Zc + sf;
		APP = APP + dev_dt[offsetDt + iRow];
	}
	if(APP > SCHAR_MAX)	APP = SCHAR_MAX;
	if(APP < SCHAR_MIN)	APP = SCHAR_MIN;
	// write back to device global memory
	dev_llr[iMCW *col*Zc + iCol] = APP;
}


__kernel void pack_decoded_bit(__global unsigned char * dev_llr, __global unsigned char * dev_tmp, int col, int Zc)
{
	unsigned char tmp[128];
	int iMCW = get_group_id(1);
	int btid = get_local_id(0);
	int tid = iMCW * col*Zc + get_group_id(0)*128 + btid;	
	tmp[btid] = 0;
	
	if(dev_llr[tid] < 0){
		tmp[btid] = 1 << (7-(btid&7));
	}
//	__syncthreads();
	
	if(btid < 16){
		dev_tmp[iMCW * col*Zc + get_group_id(0)*16+btid] = 0;
		for(int i = 0; i < 8; i++){
			dev_tmp[iMCW * col*Zc + get_group_id(0)*16+btid] += tmp[btid*8+i];
		}
	}
}

