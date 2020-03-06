#include <stdio.h>
#include <cuda_runtime.h>
#include <cuda.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include "../BGs/isip_ldpc_bg1_i0.h"
#include "../BGs/isip_ldpc_bg1_i1.h"
#include "../BGs/isip_ldpc_bg1_i2.h"
#include "../BGs/isip_ldpc_bg1_i3.h"
#include "../BGs/isip_ldpc_bg1_i4.h"
#include "../BGs/isip_ldpc_bg1_i5.h"
#include "../BGs/isip_ldpc_bg1_i6.h"
#include "../BGs/isip_ldpc_bg1_i7.h"
#include "../BGs/isip_ldpc_bg2_i0.h"
#include "../BGs/isip_ldpc_bg2_i1.h"
#include "../BGs/isip_ldpc_bg2_i2.h"
#include "../BGs/isip_ldpc_bg2_i3.h"
#include "../BGs/isip_ldpc_bg2_i4.h"
#include "../BGs/isip_ldpc_bg2_i5.h"
#include "../BGs/isip_ldpc_bg2_i6.h"
#include "../BGs/isip_ldpc_bg2_i7.h"
#include "util.h"

#define TNPB 35
#define BNPG 1024
#define ITER 5

__constant__ int BG_GPU[46*68];



__global__ 
void BNProcess(int flag, int *BN, int *CN, int *CNbuf, const int *const_llr, int BG_col, int BG_row, int Zc)
{
	int *CNG = (flag)? CN : CNbuf;
	int id = blockIdx.x*blockDim.x + threadIdx.x;

	for(int col = id; col < BG_col*Zc; col += (TNPB*BNPG))
	{
		int tmp = const_llr[col];
		for(int row = 0; row < BG_row; row++)
		{
			int up_shift = (BG_GPU[col/Zc + row*BG_col] - 1)%Zc;
			if(up_shift != -1)
			{
				int row_idx = col%Zc;
				row_idx = row_idx - up_shift;
				if(row_idx < 0)	row_idx = Zc + row_idx;

				row_idx = row*Zc + row_idx;
				tmp = tmp + CNG[row_idx*BG_col*Zc + col];
			}
		}
		BN[col] = tmp;
	}
	__syncthreads();
}



__global__ void CNProcess(int flag, int *BN, int *CN, int *CNbuf, int BG_col, int BG_row, int Zc)
{
	int *CNG	= (flag)? CN : CNbuf;
	int *SCNG 	= (flag)? CNbuf : CN;
	int id = blockIdx.x*blockDim.x + threadIdx.x;
	
	for(int row = id; row < BG_row*Zc; row += (TNPB*BNPG))
	{
		for(int col = 0; col < BG_col; col++)
		{
			int right_shift = BG_GPU[(row/Zc)*BG_col + col] -1;
			if(right_shift != -1)
			{
				int row_idx = row;
//				int col_idx = ((row%384) + right_shift%384) %384 + col*384;
                int col_idx = (row + right_shift) %Zc + col*Zc;
				int sgn_cnt = 0, min = INT32_MAX;
				for(int comp = 0; comp < BG_col; comp++)
				{
					if(comp == col)	continue;
					int comp_right_shift = BG_GPU[(row/Zc)*BG_col + comp] -1;
					if(comp_right_shift != -1)
					{
						int comp_row_idx = row;
//						int comp_col_idx = ((row%384) + (comp_right_shift%384)) %384 + comp*384;
                        int comp_col_idx = (row + comp_right_shift) %Zc + comp*Zc;
						int tmp = BN[comp_col_idx] - CNG[comp_row_idx*BG_col*Zc + comp_col_idx];
						if(tmp < 0)
						{
							tmp = -tmp;
							sgn_cnt++;
						}
						if(tmp < min)	min = tmp;
					}
				}
				SCNG[row_idx*BG_col*Zc + col_idx] = (sgn_cnt%2 == 0)? min: -min;
			}
		}
	}
	__syncthreads();
}

__global__ void BitDetermination(int *BN, unsigned int *decode_d)
{
	__shared__ int tmp[256];
	int tid = blockIdx.x*256 + threadIdx.x;
	int bid = threadIdx.x;
	tmp[bid] = 0;
	
	
	if(BN[tid] < 0)
	{
		tmp[bid] = 1 << (bid&7);
	}

	__syncthreads();
	
	if(threadIdx.x < 32)
	{
		decode_d[blockIdx.x*32 + threadIdx.x] = 0;
		for(int i = 0; i < 8; i++)
		{
			decode_d[blockIdx.x*32 + threadIdx.x] += tmp[threadIdx.x*8+i];
		}
	}
}


// helper function 
void printllr(const char *name, int *src, int *des, int count, int type_size)
{
	cudaCheck( cudaMemcpy((void *)des, (const void *)src, count*type_size, cudaMemcpyDeviceToHost) );

	FILE *fp = fopen(name, "w");
	if(!fp)	printf("[error]: open file %s failed\n", name);

	for(int i = 0; i < count; i++){
		fprintf(fp, "llr[%d]= %d\n", i, des[i]);
	}
}


int main(int argc, char* argv[])
{
	int opt = 0, block_length = 0, BG = 0, Kb = 0, Zc = 0, BG_row = 0, BG_col = 0, lift_index = 0;
	char file[50] = {};
	short lift_size[51] = {2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,18,20,22,24,26,28,30,32,36,40,44,48,52,56,60,64,72,80,88,96,104,112,120,128,144,160,176,192,208,224,240,256,288,320,352,384};
	short lift_set[][9] = {
		{2,4,8,16,32,64,128,256},
		{3,6,12,24,48,96,192,384},
		{5,10,20,40,80,160,320},
		{7,14,28,56,112,224},
		{9,18,36,72,144,288},
		{11,22,44,88,176,352},
		{13,26,52,104,208},
		{15,30,60,120,240},
		{0}};

	while( (opt = getopt(argc, argv, "l:f:")) != -1){
		switch(opt){
			case 'l':
				block_length = atoi(optarg);
				break;
			case 'f':
				strncpy(file, optarg, strlen(optarg));
				break;
			default:
				fprintf(stderr, "Usage: %s [-l code block length] <-f input file>\n", argv[0]);
				exit(1);
		}
	}
	
	if(block_length == 0 || file[0] == ' '){
		fprintf(stderr, "no input file specified or code block length == 0");
	}

	if(block_length > 3840){
		BG = 1;
		Kb = 22;
		BG_row = 46;
		BG_col = 68;
	}else if(block_length <= 3840){
		BG = 2;
		BG_row = 42;
		BG_col = 52;
		if(block_length > 640)
			Kb = 10;
		else if(block_length > 560)
			Kb = 9;
		else if(block_length > 192)
			Kb = 8;
		else
			Kb = 6;
	}

	for(int i = 0; i < 51; i++){
		if(lift_size[i] >= (double)block_length/Kb){
			Zc = lift_size[i];
			break;
		}
	}
	
	for(int i = 0; lift_set[i][0] != 0; i++){
		for(int j = 0; lift_set[i][j] != 0; j++){
			if(Zc == lift_set[i][j]){
				lift_index = i;
				break;
			}
		}
	}

	int *BG_CPU = NULL;
	switch(lift_index){
		case 0:
			BG_CPU = (BG == 1)? BG1_I0:BG2_I0;
			break;
		case 1:
			BG_CPU = (BG == 1)? BG1_I1:BG2_I1;
			break;
		case 2:
			BG_CPU = (BG == 1)? BG1_I2:BG2_I2;
			break;
		case 3:
			BG_CPU = (BG == 1)? BG1_I3:BG2_I3;
			break;
		case 4:
			BG_CPU = (BG == 1)? BG1_I4:BG2_I4;
			break;
		case 5:
			BG_CPU = (BG == 1)? BG1_I5:BG2_I5;
			break;
		case 6:
			BG_CPU = (BG == 1)? BG1_I6:BG2_I6;
			break;
		case 7:
			BG_CPU = (BG == 1)? BG1_I7:BG2_I7;
			break;
	}
    
//    printf("BG %d lift_index %d Zc %d BG_row %d BG_col %d\n", BG, lift_index, Zc, BG_row, BG_col);
    
	// alloc cpu memory
 	unsigned int *input = (unsigned int*)malloc(sizeof(unsigned int)*8448/8), *decode_output_d, *decode_output_h;
	int *BN, *CN, *CNbuf, *channel_output, *const_llr;
//	debug
//	int *p_BN = (int*)calloc(68*384, sizeof(int));
//	int *p_CN = (int*)calloc(68*384*46*384, sizeof(int));
	int *debug_llr = (int*)calloc(68*384, sizeof(int));
	cudaCheck( cudaHostAlloc((void**)&channel_output, 68*384*sizeof(int), cudaHostAllocDefault) );
	cudaCheck( cudaHostAlloc((void**)&decode_output_h, (8448/8)*sizeof(unsigned int), cudaHostAllocMapped) );
	// | cudaHostAllocPortable | cudaHostAllocMapped | cudaHostAllocWriteCombined);
	
	// read data from input file
	ReadDataFromFile(file, input, channel_output, block_length, BG_col, Zc);
	
	// alloc gpu memory 
	// BG
	cudaCheck( cudaMemcpyToSymbol(BG_GPU, BG_CPU, BG_col*BG_row*sizeof(int)) );
	// LLR CN BN BUF
	size_t p_llr;
	cudaCheck( cudaMallocPitch((void**)&const_llr, &p_llr, 68*384*sizeof(int), 1) );
	cudaCheck( cudaMallocPitch((void**)&BN, &p_llr, 68*384*sizeof(int), 1) );
	cudaCheck( cudaMallocPitch((void**)&CN, &p_llr, 68*384*sizeof(int), 46*384) );
	cudaCheck( cudaMallocPitch((void**)&CNbuf, &p_llr, 68*384*sizeof(int), 46*384) );

	cudaCheck( cudaMemcpyAsync((void*)const_llr, (const void*)channel_output, 68*384*sizeof(int), cudaMemcpyHostToDevice) );
	cudaCheck( cudaMemcpyAsync((void*)BN, (const void*)channel_output, 68*384*sizeof(int), cudaMemcpyHostToDevice) );
	cudaCheck( cudaHostGetDevicePointer((void**)&decode_output_d, (void*)decode_output_h, 0) );
	cudaDeviceSynchronize();
	
	cudaEvent_t start, end;
	float time;
	
	cudaEventCreate(&start);
	cudaEventCreate(&end);
	
	cudaEventRecord(start,0);

	dim3 grid(BNPG, 1, 1);
	dim3 block(TNPB, 1, 1);
	int flag = 0;
	char str[20] = {};
	for(int it = 0; it < ITER; it++){
		CNProcess<<<grid, block>>>(flag, BN, CN, CNbuf, BG_col, BG_row, Zc);
		flag = (flag+1)&1;
		BNProcess<<<grid, block>>>(flag, BN, CN, CNbuf, const_llr, BG_col, BG_row, Zc);
#ifdef debug 
		snprintf(str, 20,  "%s_%d", "llr", it);
		printllr(str, BN, debug_llr, 68*384, sizeof(int));
#endif
	}
	BitDetermination<<<33, 256>>>(BN, decode_output_d);
	
	cudaDeviceSynchronize();
	cudaEventRecord(end,0);
	cudaEventSynchronize(end);
	cudaEventElapsedTime(&time, start, end);

	
	int err_num = 0;
	for(int i = 0; i < block_length/8; i++){
		if(input[i] != decode_output_h[i]){
			printf("input[%d] = %d, decode_output[%d] = %d\n", i, input[i], i, decode_output_h[i]);
			err_num++;
		}
	}
	printf("err_num == %d\n", err_num);
	printf("decode time:%f ms\n",time);
	
	// free resource 
	free(input);
//	free(p_BN);
//	free(p_CN);
	cudaFreeHost(channel_output);
	cudaFreeHost(decode_output_h);
	cudaFree(const_llr);
	cudaFree(BN);
	cudaFree(CN);
	cudaFree(CNbuf);
	
	return 0;
}
