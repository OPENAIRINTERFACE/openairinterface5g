/*! \file PHY/CODING/nrLDPC_decoder_LYC/nrLDPC_decoder_LYC.cu
 * \brief LDPC cuda support BG1 all length
 * \author NCTU OpinConnect Terng-Yin Hsu,WEI-YING,LIN
 * \email tyhsu@cs.nctu.edu.tw
 * \date 13-05-2020
 * \version 
 * \note
 * \warning
 */

#include <stdio.h>
#include <unistd.h>
#include <cuda_runtime.h>
#include <CL/opencl.h>
#include "PHY/CODING/nrLDPC_decoder/nrLDPC_types.h"
#include "PHY/CODING/nrLDPC_decoder/nrLDPCdecoder_defs.h"
#include "assertions.h"
#include "common/utils/LOG/log.h"

#define MAX_ITERATION 2
#define MC	1



typedef struct{
  char x;
  char y;
  short value;
} h_element;









void init_LLR_DMA(t_nrLDPC_dec_params* p_decParams, int8_t* p_llr, int8_t* p_out){
	
	uint16_t Zc          = p_decParams->Z;
    uint8_t  BG         = p_decParams->BG;
    int block_length     = p_decParams->block_length;
	uint8_t row,col;
	if(BG == 1){
		row = 46;
		col = 68;
	}
	else{
		row = 42;
		col = 52;
	}
	unsigned char *hard_decision = (unsigned char*)p_out;
	int memorySize_llr_cuda = col * Zc * sizeof(char) * MC;
//	cudaCheck( cudaMemcpyToSymbol(dev_const_llr, p_llr, memorySize_llr_cuda) );
//	cudaCheck( cudaMemcpyToSymbol(dev_llr, p_llr, memorySize_llr_cuda) );
//	cudaDeviceSynchronize();
	
}


/* from here: entry points in decoder shared lib */
int ldpc_autoinit(void) {   // called by the library loader 
  cl_platform_id platforms[10];
  cl_int         num_platforms_found;
  
  cl_int rt = clGetPlatformIDs( sizeof(platforms)/sizeof(cl_platform_id), platforms, &num_platforms_found );
  AssertFatal(rt == CL_SUCCESS, "clGetPlatformIDs error %d\n" , (int)rt);
  AssertFatal( num_platforms_found>0 , "clGetPlatformIDs: no cl compatible platform found\n");
  for (int i=0 ; i<(int)num_platforms_found ; i++) {
	  cl_device_id devices[20];
	  cl_int       num_devices_found;
	  rt = clGetDeviceIDs(platforms[i],CL_DEVICE_TYPE_ALL, sizeof(devices)/sizeof(cl_device_id),devices,&num_devices_found);
	  AssertFatal(rt == CL_SUCCESS, "clGetDeviceIDs error %d\n" , (int)rt);
	  for (int j=0; j<num_devices_found; j++) {
		cl_device_type devtype;
		rt = clGetDeviceInfo(devices[j],CL_DEVICE_TYPE, sizeof(cl_device_type),&devtype,NULL);
		AssertFatal(rt == CL_SUCCESS, "clGetDeviceInfo error %d\n" , (int)rt); 
		LOG_I(HW,"Device %i, type %d\n", j,(int)devtype);
      }
  }
  return 0;  
}


void nrLDPC_initcall(t_nrLDPC_dec_params* p_decParams, int8_t* p_llr, int8_t* p_out) {

}


int32_t nrLDPC_decod(t_nrLDPC_dec_params* p_decParams, int8_t* p_llr, int8_t* p_out,t_nrLDPC_procBuf* p_procBuf, t_nrLDPC_time_stats *time_decoder)
{
    uint16_t Zc          = p_decParams->Z;
    uint8_t  BG         = p_decParams->BG;
    uint8_t  numMaxIter = p_decParams->numMaxIter;
    int block_length    = p_decParams->block_length;
    e_nrLDPC_outMode outMode = p_decParams->outMode;
	cudaError_t cudaStatus;
	uint8_t row,col;
	if(BG == 1){
		row = 46;
		col = 68;
	}
	else{
		row = 42;
		col = 52;
	}

//	alloc memory
	unsigned char *hard_decision = (unsigned char*)p_out;
//	gpu
	int memorySize_llr_cuda = col * Zc * sizeof(char) * MC;
//	cudaCheck( cudaMemcpyToSymbol(dev_const_llr, p_llr, memorySize_llr_cuda) );
//	cudaCheck( cudaMemcpyToSymbol(dev_llr, p_llr, memorySize_llr_cuda) );
	
// Define CUDA kernel dimension
	int blockSizeX = Zc;
//	dim3 dimGridKernel1(row, MC, 1); 	// dim of the thread blocks
//	dim3 dimBlockKernel1(blockSizeX, 1, 1);

//    dim3 dimGridKernel2(col, MC, 1);
//    dim3 dimBlockKernel2(blockSizeX, 1, 1);	
//	cudaDeviceSynchronize();

// lauch kernel 
/*
	for(int ii = 0; ii < MAX_ITERATION; ii++){
		// first kernel	
		if(ii == 0){
			ldpc_cnp_kernel_1st_iter 
			<<<dimGridKernel1, dimBlockKernel1>>>
			( BG, row, col, Zc);
		}else{
			ldpc_cnp_kernel
			<<<dimGridKernel1, dimBlockKernel1>>>
			( BG, row, col, Zc);
		}
		// second kernel
		ldpc_vnp_kernel_normal
		<<<dimGridKernel2, dimBlockKernel2>>>
		// (dev_llr, dev_const_llr,BG, row, col, Zc);
		(BG, row, col, Zc);
	}
	
	int pack = (block_length/128)+1;
	dim3 pack_block(pack, MC, 1);
	pack_decoded_bit<<<pack_block,128>>>( col, Zc);
	
	cudaCheck( cudaMemcpyFromSymbol((void*)hard_decision, (const void*)dev_tmp, (block_length/8)*sizeof(unsigned char)) );
	cudaDeviceSynchronize();
*/	

	return MAX_ITERATION;
	
}
