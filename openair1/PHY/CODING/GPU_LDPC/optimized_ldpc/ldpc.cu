#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <cuda_runtime.h>
#include <cuda.h>
#include "bg1_i1_index_array.h"

void print_arr_cpu(const char *file, int *arr, int size)
{
	FILE *fp = fopen(file, "w");
	for(int i = 0; i < size; i++){
		fprintf(fp, "%s[%d]: %d\n", file, i, arr[i]);
	}
	fclose(fp);
}

void print_arr(const char *file, int *arr, int size)
{
	int *tmp = (int*)malloc(sizeof(int)*size);
	FILE *fp = fopen(file, "w");
	cudaMemcpy((void*)tmp, (const void*)arr, size*sizeof(int), cudaMemcpyDeviceToHost);
	cudaDeviceSynchronize();
	for(int i = 0; i < size; i++){
		fprintf(fp, "%s[%d]: %d\n", file, i, tmp[i]);
	}
	free(tmp);
	fclose(fp);
}

__global__ void llr2CN(float *llr, float *cnbuf, int *l2c_idx)
{
	int tid = blockIdx.x*blockDim.x + threadIdx.x;

	cnbuf[tid] = llr[l2c_idx[tid]];
	__syncthreads();
}

__global__ void llr2BN(float *llr, float *const_llr, int *l2b_idx)
{
	int tid = blockIdx.x*blockDim.x + threadIdx.x;

	const_llr[tid] = llr[l2b_idx[tid]];
	__syncthreads();
}

__global__ void CNProcess(float *cnbuf, float *bnbuf, int *b2c_idx, int *cnproc_start_idx, int *cnproc_end_idx)
{
	int tid = blockIdx.x*blockDim.x + threadIdx.x;

	int start = cnproc_start_idx[tid];
	int end = cnproc_end_idx[tid];
	
	
	int sgn = 1, val = INT32_MAX;
	for(int i = start; i < end; i++){
		if(i == tid)	continue;

		int tmp = cnbuf[i];
		if(tmp < 0){
			tmp = -tmp;
			sgn = -sgn;
		}
		if(val > tmp){
			val = tmp;
		}
	}
	bnbuf[b2c_idx[tid]] = sgn*val;// + const_llr[tid];
	__syncthreads();
}

__global__ void add(int *bnbuf, int start, int pid, int *buf)
{
	__shared__ int sdata[25];
	int tid = threadIdx.x;
	int num = blockDim.x;
	sdata[tid] = bnbuf[start+tid];
	for(int s = num/2; s > 0; s>>=1){
		if(tid < s){
			sdata[tid] += sdata[tid+s];
		}
	}
	if(tid == 0){
		buf[pid] = sdata[tid];
	}
}

__global__ void BNProcess(float *const_llr, float *bnbuf, float *cnbuf, int *c2b_idx, int *bnproc_start_idx, int *bnproc_end_idx, float *resbuf)
{
	int tid = blockIdx.x*blockDim.x + threadIdx.x;
	float val = 0.0;
	
	int start = bnproc_start_idx[tid];
	int end = bnproc_end_idx[tid];
	for(int i = start; i < end; i++){
		if(i == tid)	continue;
		val += bnbuf[i];
	}
	
//	cnbuf[c2b_idx[tid]] = resbuf[tid] + const_llr[tid];
	cnbuf[c2b_idx[tid]] = val + const_llr[tid];
	__syncthreads();
}


__global__ void BN2llr(float *const_llr, float *bnbuf, float *llrbuf, int *llr_idx)
{
	int tid = blockIdx.x*blockDim.x + threadIdx.x;

	int start = llr_idx[tid];
	int end = llr_idx[tid+1];

	int res = 0.0;
	for(int i = start; i < end; i++){
		res += bnbuf[i];
	}
	llrbuf[tid] = res + const_llr[tid];
	__syncthreads();
}

__global__ void BitDetermination(float *BN, unsigned int *decode_d)
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

void Read_Data(char *filename, int *data_sent, float *data_received)
{
	FILE *fp = fopen(filename, "r");
	fscanf(fp, "%*s");
	for(int i = 0; i < 1056; i++){
		fscanf(fp, "%d", &data_sent[i]);
	}
	fscanf(fp, "%*s");
	fscanf(fp, "%*s");
	fscanf(fp, "%*s");
	for(int i = 0; i < 26112; i++){
		fscanf(fp, "%f", &data_received[i]);
	}
	fclose(fp);
}

int main(int argc, char **argv)
{
	int code_length = 8448, BG = 1;
	int *input = (int*)malloc(1056*sizeof(int));
	float *llr = (float*)malloc(26112*sizeof(float));

	float *llr_d, *llrbuf_d, *const_llr_d, *cnbuf_d, *bnbuf_d, *resbuf_d;
	unsigned int *decode_output_h, *decode_output_d;

	int *l2c_idx_d, *cnproc_start_idx_d, *cnproc_end_idx_d, *c2b_idx_d, *bnproc_start_idx_d, *bnproc_end_idx_d, *b2c_idx_d, *llr_idx_d, *l2b_idx_d;

	char *file = argv[1];
	
	
	int blockNum = 237, threadNum = 512;
	//int blockNum = 33, threadNum = 256;
	//int blockNum = 17, threadNum = 512;

	int rounds = 5, Zc = 384;

	Read_Data(file, input, llr);



	size_t p_llr;
	cudaHostAlloc((void**)&decode_output_h, 1056*sizeof(unsigned int), cudaHostAllocMapped);

	cudaMallocPitch((void**)&llr_d, &p_llr, 26112*sizeof(float), 1);
	cudaMallocPitch((void**)&llrbuf_d, &p_llr, 26112*sizeof(float), 1);
	cudaMallocPitch((void**)&const_llr_d, &p_llr, 316*384*sizeof(float), 1);
	cudaMallocPitch((void**)&cnbuf_d, &p_llr, 316*384*sizeof(float), 1);
	cudaMallocPitch((void**)&bnbuf_d, &p_llr, 316*384*sizeof(float), 1);
	cudaMallocPitch((void**)&l2c_idx_d, &p_llr, 316*384*sizeof(int), 1);
	cudaMallocPitch((void**)&l2b_idx_d, &p_llr, 316*384*sizeof(int), 1);
	cudaMallocPitch((void**)&cnproc_start_idx_d, &p_llr, 316*384*sizeof(int), 1);
	cudaMallocPitch((void**)&cnproc_end_idx_d, &p_llr, 316*384*sizeof(int), 1);
	cudaMallocPitch((void**)&c2b_idx_d, &p_llr, 316*384*sizeof(int), 1);
	cudaMallocPitch((void**)&bnproc_start_idx_d, &p_llr, 316*384*sizeof(int), 1);
	cudaMallocPitch((void**)&bnproc_end_idx_d, &p_llr, 316*384*sizeof(int), 1);
	cudaMallocPitch((void**)&b2c_idx_d, &p_llr, 316*384*sizeof(int), 1);
	cudaMallocPitch((void**)&resbuf_d, &p_llr, 316*384*sizeof(int), 1);
	cudaMallocPitch((void**)&llr_idx_d, &p_llr, 26113*sizeof(int), 1);

	cudaMemcpyAsync((void*)llr_d, (const void*)llr, 68*384*sizeof(float), cudaMemcpyHostToDevice);	
	cudaMemcpyAsync((void*)l2c_idx_d, (const void*)l2c_idx, 316*384*sizeof(int), cudaMemcpyHostToDevice);	
	cudaMemcpyAsync((void*)l2b_idx_d, (const void*)l2b_idx, 316*384*sizeof(int), cudaMemcpyHostToDevice);	
	cudaMemcpyAsync((void*)cnproc_start_idx_d, (const void*)cnproc_start_idx, 316*384*sizeof(int), cudaMemcpyHostToDevice);	
	cudaMemcpyAsync((void*)cnproc_end_idx_d, (const void*)cnproc_end_idx, 316*384*sizeof(int), cudaMemcpyHostToDevice);	
	cudaMemcpyAsync((void*)c2b_idx_d, (const void*)c2b_idx, 316*384*sizeof(int), cudaMemcpyHostToDevice);	
	cudaMemcpyAsync((void*)bnproc_start_idx_d, (const void*)bnproc_start_idx, 316*384*sizeof(int), cudaMemcpyHostToDevice);	
	cudaMemcpyAsync((void*)bnproc_end_idx_d, (const void*)bnproc_end_idx, 316*384*sizeof(int), cudaMemcpyHostToDevice);	
	cudaMemcpyAsync((void*)b2c_idx_d, (const void*)b2c_idx, 316*384*sizeof(int), cudaMemcpyHostToDevice);	
	cudaMemcpyAsync((void*)llr_idx_d, (const void*)llr_idx, 26113*sizeof(int), cudaMemcpyHostToDevice);	

	cudaHostGetDevicePointer((void**)&decode_output_d, (void*)decode_output_h, 0);
	cudaDeviceSynchronize();


	printf("BG %d, Zc %d, code_length %d\n", BG, Zc, code_length);


	cudaEvent_t start, end;
	float time;

	cudaEventCreate(&start);
	cudaEventCreate(&end);
	cudaEventRecord(start, 0);


	llr2CN<<<blockNum, threadNum>>>(llr_d, cnbuf_d, l2c_idx_d);
	llr2BN<<<blockNum, threadNum>>>(llr_d, const_llr_d, l2b_idx_d);

/*
	print_arr("debug/const_llr_d", const_llr_d, 26112);
	print_arr("debug/cnbuf_d", cnbuf_d, 316*384);
	print_arr("debug/const_llrbuf_d", const_llrbuf_d, 316*384);
*/

	char dir[] = "debug/", cn[] = "cnbuf", bn[] = "bnbuf", llrstr[] = "llrbuf_d";
	char str[100] = {};
	for(int i = 0; i < rounds; i++){
		CNProcess<<<blockNum, threadNum>>>(cnbuf_d, bnbuf_d, b2c_idx_d, cnproc_start_idx_d, cnproc_end_idx_d);
#ifdef debug
		snprintf(str, 20, "%s%s_%d", dir, bn, i+1);
		print_arr(str, bnbuf_d, 316*384);
#endif
		BNProcess<<<blockNum, threadNum>>>(const_llr_d, bnbuf_d, cnbuf_d, c2b_idx_d, bnproc_start_idx_d, bnproc_end_idx_d, resbuf_d);
#ifdef debug
		snprintf(str, 20, "%s%s_%d", dir, cn, i+1);
		print_arr(str, cnbuf_d, 316*384);
#endif
		BN2llr<<<51, 512>>>(llr_d, bnbuf_d, llrbuf_d, llr_idx_d);
#ifdef debug
		snprintf(str, 20, "%s%s_%d", dir, llrstr, i+1);
		print_arr(str, llrbuf_d, 26112);
#endif
	}

	BitDetermination<<<33, 256>>>(llrbuf_d, decode_output_d);
	cudaDeviceSynchronize();


	cudaEventRecord(end, 0);
	cudaEventSynchronize(end);
	cudaEventElapsedTime(&time, start, end);
	printf("time: %.6f ms\n", time);


	int err = 0;
	for(int i = 0; i < 8448/8; i++){
		if(input[i] != decode_output_h[i]){
//			printf("input[%d] :%d, decode_output[%d]: %d\n", i, input[i], i, decode_output_h[i]);
			err++;
		}
	}
	printf("err: %d\n", err);

	free(input);
	free(llr);
	cudaFree(llr_d);
	cudaFree(llrbuf_d);
	cudaFree(bnbuf_d);
	cudaFree(cnbuf_d);
	cudaFree(l2c_idx_d);
	cudaFree(cnproc_start_idx_d);
	cudaFree(cnproc_end_idx_d);
	cudaFree(c2b_idx_d);
	cudaFree(bnproc_start_idx_d);
	cudaFree(bnproc_end_idx_d);
	cudaFree(b2c_idx_d);
	cudaFree(const_llr_d);
	cudaFree(llr_idx_d);
	cudaFree(resbuf_d);

	cudaFreeHost(decode_output_h);
	return 0;
}
