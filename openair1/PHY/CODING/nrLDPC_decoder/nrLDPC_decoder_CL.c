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
 /*! \file nrLDPC_decoder_CL.c
* \brief ldpc decoder, openCL implementaion
* \author 
* \date 2021
* \version 1.0
* @ingroup 

*/
/* uses HW  component id for log messages ( --log_config.hw_log_level <warning| info|debug|trace>) */
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

#define MAX_OCLDEV 10

typedef struct{
  char x;
  char y;
  short value;
} h_element;

typedef struct{
  cl_uint max_CU;
  cl_uint max_WID;
  size_t  *max_WIS;
  cl_context context;
  cl_program program;
  cl_kernel kernel;
  cl_command_queue queue;
} ocldev_t;

typedef struct{
  ocldev_t ocldev[MAX_OCLDEV];
} ocl_t;



ocl_t ocl;

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
cl_error_callback(const char* errinfo, const void* private_info, size_t cb, void* user_data) {
  LOG_E(HW,"OpenCL accelerator error  %s\n", errinfo );
}

char *clutil_getstrdev(int intdev) {
  static char retstring[255]="";
  char *retptr=retstring;
  retptr+=sprintf(retptr,"0x%08x: ",(uint32_t)intdev);
  if (intdev & CL_DEVICE_TYPE_CPU)
	  retptr+=sprintf(retptr,"%s","cpu ");
  if (intdev & CL_DEVICE_TYPE_GPU)
	  retptr+=sprintf(retptr,"%s","gpu ");
  if (intdev & CL_DEVICE_TYPE_ACCELERATOR)
	  retptr+=sprintf(retptr,"%s","acc ");  
  return retstring;
}

/* from here: entry points in decoder shared lib */
int ldpc_autoinit(void) {   // called by the library loader 
  cl_platform_id platforms[10];
  cl_uint         num_platforms_found;
  
  cl_uint rt = clGetPlatformIDs( sizeof(platforms)/sizeof(cl_platform_id), platforms, &num_platforms_found );
  AssertFatal(rt == CL_SUCCESS, "clGetPlatformIDs error %d\n" , (int)rt);
  AssertFatal( num_platforms_found>0 , "clGetPlatformIDs: no cl compatible platform found\n");
  for (int i=0 ; i<(int)num_platforms_found ; i++) {
	  char stringval[255];
	  rt = clGetPlatformInfo(platforms[i],CL_PLATFORM_PROFILE, sizeof(stringval),stringval,NULL);	
	  AssertFatal(rt == CL_SUCCESS, "clGetPlatformInfo PROFILE error %d\n" , (int)rt);  
	  LOG_I(HW,"Platform %i, OpenCL profile %s\n", i,stringval );
	  rt = clGetPlatformInfo(platforms[i],CL_PLATFORM_VERSION, sizeof(stringval),stringval,NULL);	
	  AssertFatal(rt == CL_SUCCESS, "clGetPlatformInfo VERSION error %d\n" , (int)rt);  
	  LOG_I(HW,"Platform %i, OpenCL version %s\n", i,stringval );	  
	  cl_device_id devices[20];
	  cl_uint       num_devices_found;
	  rt = clGetDeviceIDs(platforms[i],CL_DEVICE_TYPE_ALL, sizeof(devices)/sizeof(cl_device_id),devices,&num_devices_found);
	  AssertFatal(rt == CL_SUCCESS, "clGetDeviceIDs error %d\n" , (int)rt);
	  for (int j=0; j<num_devices_found; j++) {
		cl_bool abool;
		rt = clGetDeviceInfo(devices[j],CL_DEVICE_AVAILABLE, sizeof(abool),&abool,NULL);
		AssertFatal(rt == CL_SUCCESS, "clGetDeviceInfo DEVICE_AVAILABLE error %d\n" , (int)rt); 
		LOG_I(HW,"Device %i is %s available\n", j, (abool==CL_TRUE?"":"not"));
		cl_device_type devtype;		
		rt = clGetDeviceInfo(devices[j],CL_DEVICE_TYPE, sizeof(cl_device_type),&devtype,NULL);
		AssertFatal(rt == CL_SUCCESS, "clGetDeviceInfo DEVICE_TYPE error %d\n" , (int)rt); 
		LOG_I(HW,"Device %i, type %d = %s\n", j,(int)devtype, clutil_getstrdev(devtype));
		rt = clGetDeviceInfo(devices[j],CL_DEVICE_MAX_COMPUTE_UNITS,sizeof(ocl.ocldev[j].max_CU),&(ocl.ocldev[j].max_CU),NULL);
		AssertFatal(rt == CL_SUCCESS, "clGetDeviceInfo MAX_COMPUTE_UNITS error %d\n" , (int)rt);
		LOG_I(HW,"Device %i, number of Compute Units: %d\n", j,ocl.ocldev[j].max_CU);
		rt = clGetDeviceInfo(devices[j],CL_DEVICE_MAX_WORK_ITEM_DIMENSIONS,sizeof(ocl.ocldev[j].max_WID),&(ocl.ocldev[j].max_WID),NULL);
		AssertFatal(rt == CL_SUCCESS, "clGetDeviceInfo MAX_WORK_ITEM_DIMENSIONS error %d\n" , (int)rt);
		LOG_I(HW,"Device %i, max Work Items dimension: %d\n", j,ocl.ocldev[j].max_WID);	
		ocl.ocldev[j].max_WIS = (size_t *)malloc(ocl.ocldev[j].max_WID * sizeof(size_t));
		rt = clGetDeviceInfo(devices[j],CL_DEVICE_MAX_WORK_ITEM_SIZES,sizeof(ocl.ocldev[j].max_WID)*sizeof(size_t),ocl.ocldev[j].max_WIS,NULL);
		AssertFatal(rt == CL_SUCCESS, "clGetDeviceInfo MAX_WORK_ITEM_SIZES error %d\n" , (int)rt);
		for(int k=0; k<ocl.ocldev[j].max_WID;k++)
		  LOG_I(HW,"Device %i, max Work Items size for dimension: %d %u\n", j,k,(uint32_t)ocl.ocldev[j].max_WIS[k]);
		
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
