#ifndef __UTIL_H__
#define __UTIL_H__

// cuda check macro
#define cudaCheck(ans) { cudaAssert((ans), __FILE__, __LINE__); }
inline void cudaAssert(cudaError_t code, const char *file, int line)
{
   if (code != cudaSuccess){
      fprintf(stderr,"GPUassert: %s %s %d\n", cudaGetErrorString(code), file, line);
      exit(code);
   }
}

// struct
typedef struct
{
	FILE* fptr;
	char filename[64];
	char tmp[64];
}file_t;

// utility
void ReadDataFromFile(const char*, unsigned int*, int*, int, int, int);
__global__ void BNProcess(int flag, int *BN, int *CN, int *CNbuf, const int *const_llr);
__global__ void CNProcess(int flag, int *BN, int *CN, int *CNbuf);
__global__ void BitDetermination(int *BN, unsigned int *decode_d);

/*
int32_t nrLDPC_decoder(t_nrLDPC_dec_params* p_decParams, 
					   int8_t* p_llr, int8_t* p_out, 
					   t_nrLDPC_procBuf* p_procBuf, 
					   t_nrLDPC_time_stats* p_profiler){

	return iter;
}

static inline uint32_t nrLDPC_decoder_core(int8_t* p_llr, int8_t* p_out, 
										   t_nrLDPC_procBuf* p_procBuf, 
										   uint32_t numLLR, t_nrLDPC_lut* p_lut, 
										   t_nrLDPC_dec_params* p_decParams, 
										   t_nrLDPC_time_stats* p_profiler){

	return iter;							
}

*/
#endif
