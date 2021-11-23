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
* \author Francois TABURET
* \date 2021
* \version 1.0
* \note initial implem - translation of cuda version
*/


#define MAX_ITERATION 2
#define MC	1

#define MAX_OCLDEV   10
#define MAX_OCLRUNTIME 5

typedef struct{
  char x;
  char y;
  short value;
} h_element;

#ifdef NRLDPC_KERNEL_SOURCE
#include "nrLDPC_decoder_kernels_CL.c"
#else
/* uses HW  component id for log messages ( --log_config.hw_log_level <warning| info|debug|trace>) */
#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>
#include <CL/opencl.h>
#include "PHY/CODING/nrLDPC_decoder/nrLDPC_types.h"
#include "PHY/CODING/nrLDPC_decoder/nrLDPCdecoder_defs.h"
#include "assertions.h"
#include "common/utils/LOG/log.h"

#define CLSETKERNELARG(A,B,C,D) \
rt=clSetKernelArg(A,B,C,D) ;\
AssertFatal(rt == CL_SUCCESS, "Error %d setting kernel argument index %d\n" , (int)rt, B);

#include "../nrLDPC_decoder_LYC/bgs/BG1_compact_in_C.h"

typedef struct{
  cl_uint max_CU;
  cl_uint max_WID;
  size_t  *max_WIS;
} ocldev_t;

typedef struct {
  cl_kernel cnp_kernel_1st;
  cl_kernel cnp_kernel;
  cl_kernel vnp_kernel_normal;
  cl_kernel pack_decoded_bit;
} oclkernels_t;

typedef struct{
  cl_uint num_devices;
  cl_device_id devices[MAX_OCLDEV];
  ocldev_t ocldev[MAX_OCLDEV];
  cl_context context;
  cl_program program;
  cl_mem dev_h_compact1;
  cl_mem dev_h_compact2;
  cl_mem dev_const_llr;
  cl_mem dev_llr;
  cl_mem dev_dt;
  cl_mem dev_tmp;  
  oclkernels_t kernels[MAX_OCLDEV];
  cl_command_queue queue[MAX_OCLDEV];
} oclruntime_t;

typedef struct{
  oclruntime_t runtime[MAX_OCLRUNTIME];
} ocl_t;



ocl_t ocl;


void set_compact_BG(int Zc,short BG){
	cl_uint rt;
	int row,col;
	if(BG == 1){
		row = 46;
		col = 68;
	}
	else{
		row = 42;
		col = 52;
	}
	int compact_row = 30; 
	int compact_col = 19;
	if(BG==2){compact_row = 10, compact_col = 23;}
	int memorySize_h_compact1 = row * compact_col * sizeof(h_element);
	int memorySize_h_compact2 = compact_row * col * sizeof(h_element);
	int lift_index = 0;
	short lift_set[][9] = {
		{2,4,8,16,32,64,128,256},
		{3,6,12,24,48,96,192,384},
		{5,10,20,40,80,160,320},
		{7,14,28,56,112,224},
		{9,18,36,72,144,288},
		{11,22,44,88,176,352},
		{13,26,52,104,208},
		{15,30,60,120,240},
		{0}
	};
	
	for(int i = 0; lift_set[i][0] != 0; i++){
		for(int j = 0; lift_set[i][j] != 0; j++){
			if(Zc == lift_set[i][j]){
				lift_index = i;
				break;
			}
		}
	}
	printf("\nZc = %d BG = %d\n",Zc,BG);
    ocl.runtime[0].dev_h_compact1 = clCreateBuffer(ocl.runtime[0].context, CL_MEM_READ_ONLY|CL_MEM_HOST_WRITE_ONLY, memorySize_h_compact1, NULL, (cl_int *)&rt);
    AssertFatal(rt == CL_SUCCESS, "Error %d creating buffer dev_h_compact1 for platform %i \n" , (int)rt, 0);	
    ocl.runtime[0].dev_h_compact2 = clCreateBuffer(ocl.runtime[0].context, CL_MEM_READ_ONLY|CL_MEM_HOST_WRITE_ONLY, memorySize_h_compact2, NULL, (cl_int *)&rt);
    AssertFatal(rt == CL_SUCCESS, "Error %d creating buffer dev_h_compact2 for platform %i \n" , (int)rt, 0);  
    h_element *h1;
    h_element *h2; 
	switch(lift_index){
			case 0:
				h1 = host_h_compact1_I0;
				h2 = host_h_compact2_I0;
				break;
			case 1:
				h1 = host_h_compact1_I1;
				h2 = host_h_compact2_I1;
				break;
			case 2:
				h1 = host_h_compact1_I2;
				h2 = host_h_compact2_I2;
				break;
			case 3:
				h1 = host_h_compact1_I3;
				h2 = host_h_compact2_I3;
				break;
			case 4:
				h1 = host_h_compact1_I4;
				h2 = host_h_compact2_I4;
				break;
			case 5:
				h1 = host_h_compact1_I5;
				h2 = host_h_compact2_I5;
				break;
			case 6:
				h1 = host_h_compact1_I6;
				h2 = host_h_compact2_I6;
				break;
			case 7:
				h1 = host_h_compact1_I7;
				h2 = host_h_compact2_I7;
				break;
			default:
				AssertFatal(0, "Invalid lift_index value %i\n" , lift_index); 
				break;
		}
     rt = clEnqueueWriteBuffer(ocl.runtime[0].queue[0], ocl.runtime[0].dev_h_compact1, CL_TRUE, 0,memorySize_h_compact1, h1, 0, NULL, NULL);
	 AssertFatal(rt == CL_SUCCESS, "Error %d moving  h_compact1 memory to pltf %i dev %i\n" , (int)rt, 0,0); 
     rt = clEnqueueWriteBuffer(ocl.runtime[0].queue[0], ocl.runtime[0].dev_h_compact2, CL_TRUE, 0,memorySize_h_compact2, h2, 0, NULL, NULL);
	 AssertFatal(rt == CL_SUCCESS, "Error %d moving  h_compact2 memory to pltf %i dev %i\n" , (int)rt, 0,0); 	 	
	// return 0;
}
void cl_error_callback(const char* errinfo, const void* private_info, size_t cb, void* user_data) {
  oclruntime_t *runtime = (oclruntime_t *)user_data;
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

void get_CompilErr(cl_program program, int pltf) {

    // Determine the size of the log
    size_t log_size;
    for(int i=0; i<ocl.runtime[pltf].num_devices;i++) {
      clGetProgramBuildInfo(program, ocl.runtime[pltf].devices[i], CL_PROGRAM_BUILD_LOG, 0, NULL, &log_size);
    // Allocate memory for the log
      char *log = (char *) malloc(log_size);
    // Get the log
      clGetProgramBuildInfo(program, ocl.runtime[pltf].devices[i], CL_PROGRAM_BUILD_LOG, log_size, log, NULL);
    // Print the log
      printf("%s\n", log);
      free(log);
    }

}

size_t load_source(char **source_str, char *filename) {
    FILE *fp;
    struct stat st ;
    size_t source_size;
    char *src= NULL;
    
  if (filename == NULL) {
	src = "nrLDPC_decoder_kernels_CL.clc";
  } else {
	src = filename;
  }
  fp = fopen(src, "r");
  AssertFatal(fp,"failed to open cl source %s: %s\n",src,strerror(errno));

  fstat(fileno(fp), &st);
  source_size = st.st_size;    
  *source_str = (char*)malloc(source_size);
  source_size = fread( *source_str, 1, source_size, fp);
  fclose( fp );
  LOG_I(HW,"Loaded kernel sources from %s %u bytes\n", (filename==NULL)?"embedded cl code":src,(unsigned int)source_size );
  return source_size;
}



/* from here: entry points in decoder shared lib */
int ldpc_autoinit(void) {   // called by the library loader 
  cl_platform_id platforms[10];
  cl_uint         num_platforms_found;
  int             context_ok=0;
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
	  rt = clGetDeviceIDs(platforms[i],CL_DEVICE_TYPE_ALL, sizeof(ocl.runtime[i].devices)/sizeof(cl_device_id),ocl.runtime[i].devices,&(ocl.runtime[i].num_devices));
	  AssertFatal(rt == CL_SUCCESS, "clGetDeviceIDs error %d\n" , (int)rt);
	  int devok=0;
	  for (int j=0; j<ocl.runtime[i].num_devices; j++) {
		cl_bool abool;
		rt = clGetDeviceInfo(ocl.runtime[i].devices[j],CL_DEVICE_AVAILABLE, sizeof(abool),&abool,NULL);
		AssertFatal(rt == CL_SUCCESS, "clGetDeviceInfo DEVICE_AVAILABLE error %d\n" , (int)rt); 
		LOG_I(HW,"Device %i is %s available\n", j, (abool==CL_TRUE?"":"not"));
		cl_device_type devtype;		
		rt = clGetDeviceInfo(ocl.runtime[i].devices[j],CL_DEVICE_TYPE, sizeof(cl_device_type),&devtype,NULL);
		AssertFatal(rt == CL_SUCCESS, "clGetDeviceInfo DEVICE_TYPE error %d\n" , (int)rt); 
		LOG_I(HW,"Device %i, type %d = %s\n", j,(int)devtype, clutil_getstrdev(devtype));
		rt = clGetDeviceInfo(ocl.runtime[i].devices[j],CL_DEVICE_MAX_COMPUTE_UNITS,sizeof(ocl.runtime[i].ocldev[j].max_CU),&(ocl.runtime[i].ocldev[j].max_CU),NULL);
		AssertFatal(rt == CL_SUCCESS, "clGetDeviceInfo MAX_COMPUTE_UNITS error %d\n" , (int)rt);
		LOG_I(HW,"Device %i, number of Compute Units: %d\n", j,ocl.runtime[i].ocldev[j].max_CU);
		rt = clGetDeviceInfo(ocl.runtime[i].devices[j],CL_DEVICE_MAX_WORK_ITEM_DIMENSIONS,sizeof(ocl.runtime[i].ocldev[j].max_WID),&(ocl.runtime[i].ocldev[j].max_WID),NULL);
		AssertFatal(rt == CL_SUCCESS, "clGetDeviceInfo MAX_WORK_ITEM_DIMENSIONS error %d\n" , (int)rt);
		LOG_I(HW,"Device %i, max Work Items dimension: %d\n", j,ocl.runtime[i].ocldev[j].max_WID);	
		ocl.runtime[i].ocldev[j].max_WIS = (size_t *)malloc(ocl.runtime[i].ocldev[j].max_WID * sizeof(size_t));
		rt = clGetDeviceInfo(ocl.runtime[i].devices[j],CL_DEVICE_MAX_WORK_ITEM_SIZES,sizeof(ocl.runtime[i].ocldev[j].max_WID)*sizeof(size_t),ocl.runtime[i].ocldev[j].max_WIS,NULL);
		AssertFatal(rt == CL_SUCCESS, "clGetDeviceInfo MAX_WORK_ITEM_SIZES error %d\n" , (int)rt);
		for(int k=0; k<ocl.runtime[i].ocldev[j].max_WID;k++)
		  LOG_I(HW,"Device %i, max Work Items size for dimension: %d %u\n", j,k,(uint32_t)ocl.runtime[i].ocldev[j].max_WIS[k]); 
		devok++;
      }
      if (devok >0) {
        ocl.runtime[i].context = clCreateContext(NULL, ocl.runtime[i].num_devices, ocl.runtime[i].devices, cl_error_callback, &(ocl.runtime[i]), (cl_int *)&rt); 
        AssertFatal(rt == CL_SUCCESS, "Error %d creating context for platform %i\n" , (int)rt, i);
        for(int dev=0; dev<ocl.runtime[i].num_devices; dev++) {
          ocl.runtime[i].queue[dev] = clCreateCommandQueueWithProperties(ocl.runtime[i].context,ocl.runtime[i].devices[dev] , 0, (cl_int *)&rt);
          AssertFatal(rt == CL_SUCCESS, "Error %d creating command queue for platform %i device %i\n" , (int)rt, i,dev);
        }
        ocl.runtime[i].dev_const_llr = clCreateBuffer(ocl.runtime[i].context, CL_MEM_READ_ONLY|CL_MEM_HOST_WRITE_ONLY, 68*384, NULL, (cl_int *)&rt);
        AssertFatal(rt == CL_SUCCESS, "Error %d creating buffer dev_const_llr for platform %i \n" , (int)rt, i);
        ocl.runtime[i].dev_llr = clCreateBuffer(ocl.runtime[i].context, CL_MEM_READ_WRITE|CL_MEM_HOST_WRITE_ONLY, 68*384, NULL, (cl_int *)&rt);
        AssertFatal(rt == CL_SUCCESS, "Error %d creating buffer dev_llr for platform %i \n" , (int)rt, i); 
        ocl.runtime[i].dev_dt = clCreateBuffer(ocl.runtime[i].context, CL_MEM_READ_WRITE|CL_MEM_HOST_NO_ACCESS, 46*68*384, NULL, (cl_int *)&rt);
        AssertFatal(rt == CL_SUCCESS, "Error %d creating buffer dev_dt for platform %i \n" , (int)rt, i);                
        ocl.runtime[i].dev_tmp = clCreateBuffer(ocl.runtime[i].context, CL_MEM_READ_ONLY|CL_MEM_HOST_WRITE_ONLY, 68*384, NULL, (cl_int *)&rt);
        AssertFatal(rt == CL_SUCCESS, "Error %d creating buffer dev_tmp for platform %i \n" , (int)rt, i);      
        char *source_str;
        size_t source_size=load_source(&source_str,"nrLDPC_decoder_kernels_CL.clc");      
        cl_program program = clCreateProgramWithSource(ocl.runtime[i].context, 1, 
                                                       (const char **)&source_str, (const size_t *)&source_size,  (cl_int *)&rt);
        AssertFatal(rt == CL_SUCCESS, "Error %d creating program for platform %i \n" , (int)rt, i); 
        rt = clBuildProgram(program, ocl.runtime[i].num_devices,ocl.runtime[i].devices, NULL /* compile options */, NULL, NULL);  
        if (rt == CL_BUILD_PROGRAM_FAILURE) {
          get_CompilErr(program,i);
	    } 
        AssertFatal(rt == CL_SUCCESS, "Error %d buildding program for platform %i \n" , rt, i); 
        
	    for(int dev=0; dev<ocl.runtime[i].num_devices; dev++) {
		  ocl.runtime[i].kernels[dev].cnp_kernel_1st = clCreateKernel(program, "ldpc_cnp_kernel_1st_iter", (cl_int *)&rt);
		  AssertFatal(rt == CL_SUCCESS, "Error %d creating kernel %s platform %i, dev %i\n" , (int)rt,"ldpc_cnp_kernel_1st_iter", i,dev); 
		  ocl.runtime[i].kernels[dev].cnp_kernel = clCreateKernel(program, "ldpc_cnp_kernel", (cl_int *)&rt);
		  AssertFatal(rt == CL_SUCCESS, "Error %d creating kernel %s platform %i, dev %i\n" , (int)rt,"ldpc_cnp_kernel", i,dev); 
		  ocl.runtime[i].kernels[dev].vnp_kernel_normal = clCreateKernel(program, "ldpc_vnp_kernel_normal", (cl_int *)&rt);
		  AssertFatal(rt == CL_SUCCESS, "Error %d creating kernel %s platform %i, dev %i\n" , (int)rt,"ldpc_vnp_kernel_normal", i,dev); 
		  ocl.runtime[i].kernels[dev].pack_decoded_bit = clCreateKernel(program, "pack_decoded_bit", (cl_int *)&rt);
		  AssertFatal(rt == CL_SUCCESS, "Error %d creating kernel %s platform %i, dev %i\n" , (int)rt,"pack_decoded_bit", i,dev); 		  		  		  
        }
        context_ok++;
      }
      devok=0;
  }
  AssertFatal(context_ok>0, "No openCL device available to accelerate ldpc\n"); 
  return 0;  
}


void nrLDPC_initcall(t_nrLDPC_dec_params* p_decParams, int8_t* p_llr, int8_t* p_out) {
	set_compact_BG(p_decParams->Z,p_decParams->BG);
//	init_LLR_DMA(p_decParams, p_llr,  p_out);
}


int32_t nrLDPC_decod(t_nrLDPC_dec_params* p_decParams, int8_t* p_llr, int8_t* p_out,t_nrLDPC_procBuf* p_procBuf, t_nrLDPC_time_stats *time_decoder)
{
    uint16_t Zc          = p_decParams->Z;
    uint8_t  BG         = p_decParams->BG;
//    uint8_t  numMaxIter = p_decParams->numMaxIter;
    int block_length    = p_decParams->block_length;
//    e_nrLDPC_outMode outMode = p_decParams->outMode;
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
//	unsigned char *hard_decision = (unsigned char*)p_out;
//	gpu
	int memorySize_llr = col * Zc * sizeof(char) * MC;
//	cudaCheck( cudaMemcpyToSymbol(dev_const_llr, p_llr, memorySize_llr_cuda) );
//	cudaCheck( cudaMemcpyToSymbol(dev_llr, p_llr, memorySize_llr_cuda) );
    int rt = clEnqueueWriteBuffer(ocl.runtime[0].queue[0], ocl.runtime[0].dev_const_llr, CL_TRUE, 0,
                               memorySize_llr, p_llr, 0, NULL, NULL);
	AssertFatal(rt == CL_SUCCESS, "Error %d moving p_llr data to  read only memory in pltf %i dev %i\n" , (int)rt, 0,0); 
    rt = clEnqueueWriteBuffer(ocl.runtime[0].queue[0], ocl.runtime[0].dev_llr, CL_TRUE, 0,
                               memorySize_llr, p_llr, 0, NULL, NULL);
	AssertFatal(rt == CL_SUCCESS, "Error %d moving p_llr data to  read-write memory in pltf %i dev %i\n" , (int)rt, 0,0); 	
// Define CUDA kernel dimension
//	int blockSizeX = Zc;
//	dim3 dimGridKernel1(row, MC, 1); 	// dim of the thread blocks
//	dim3 dimBlockKernel1(blockSizeX, 1, 1);

//    dim3 dimGridKernel2(col, MC, 1);
//    dim3 dimBlockKernel2(blockSizeX, 1, 1);	
//	cudaDeviceSynchronize();

// lauch kernel 
    size_t global_item_sizek0[2] = {row*Zc,MC}; // Process the entire lists
    size_t global_item_sizek1[2] = {col*Zc,MC}; // Process the entire lists
    size_t local_item_sizek[2] = {128,1}; // Divide work items into groups of 128
    
	for(int ii = 0; ii < MAX_ITERATION; ii++){
		// first kernel	
		if(ii == 0){
//			ldpc_cnp_kernel_1st_iter 
//			<<<dimGridKernel1, dimBlockKernel1>>>
//			( BG, row, col, Zc);
	      CLSETKERNELARG(ocl.runtime[0].kernels[0].cnp_kernel_1st, 0, sizeof(cl_mem), (void *)&(ocl.runtime[0].dev_llr));
	      CLSETKERNELARG(ocl.runtime[0].kernels[0].cnp_kernel_1st, 1, sizeof(cl_mem), (void *)&(ocl.runtime[0].dev_dt));
	      CLSETKERNELARG(ocl.runtime[0].kernels[0].cnp_kernel_1st, 2, sizeof(cl_mem), (void *)&(ocl.runtime[0].dev_h_compact1));
	      CLSETKERNELARG(ocl.runtime[0].kernels[0].cnp_kernel_1st, 3, sizeof(int), (void *)&(BG));
	      CLSETKERNELARG(ocl.runtime[0].kernels[0].cnp_kernel_1st, 4, sizeof(int), (void *)&(row));	 	      
	      CLSETKERNELARG(ocl.runtime[0].kernels[0].cnp_kernel_1st, 5, sizeof(int), (void *)&(col));
	      CLSETKERNELARG(ocl.runtime[0].kernels[0].cnp_kernel_1st, 6, sizeof(int), (void *)&(Zc));	      
          rt = clEnqueueNDRangeKernel(ocl.runtime[0].queue[0], ocl.runtime[0].kernels[0].cnp_kernel_1st, 2, NULL, 
                                      global_item_sizek0, local_item_sizek, 0, NULL, NULL);
	      AssertFatal(rt == CL_SUCCESS, "Error %d enqueing cnp_kernel_1st \n" , (int)rt);  
		}else{
//		 	ldpc_cnp_kernel
//			<<<dimGridKernel1, dimBlockKernel1>>>
//			( BG, row, col, Zc);
	      CLSETKERNELARG(ocl.runtime[0].kernels[0].cnp_kernel, 0, sizeof(cl_mem), (void *)&(ocl.runtime[0].dev_llr));
	      CLSETKERNELARG(ocl.runtime[0].kernels[0].cnp_kernel, 1, sizeof(cl_mem), (void *)&(ocl.runtime[0].dev_dt));
	      CLSETKERNELARG(ocl.runtime[0].kernels[0].cnp_kernel, 2, sizeof(cl_mem), (void *)&(ocl.runtime[0].dev_h_compact1));
	      CLSETKERNELARG(ocl.runtime[0].kernels[0].cnp_kernel, 3, sizeof(int), (void *)&(BG));
	      CLSETKERNELARG(ocl.runtime[0].kernels[0].cnp_kernel, 4, sizeof(int), (void *)&(row));	 	      
	      CLSETKERNELARG(ocl.runtime[0].kernels[0].cnp_kernel, 5, sizeof(int), (void *)&(col));
	      CLSETKERNELARG(ocl.runtime[0].kernels[0].cnp_kernel, 6, sizeof(int), (void *)&(Zc));	  
          rt = clEnqueueNDRangeKernel(ocl.runtime[0].queue[0], ocl.runtime[0].kernels[0].cnp_kernel, 2, NULL, 
                                      global_item_sizek0, local_item_sizek, 0, NULL, NULL);
	      AssertFatal(rt == CL_SUCCESS, "Error %d enqueing  cnp_kernel\n" , (int)rt);  
		}
		// second kernel
//		ldpc_vnp_kernel_normal
//		<<<dimGridKernel2, dimBlockKernel2>>>
//		// (dev_llr, dev_const_llr,BG, row, col, Zc);
//		(BG, row, col, Zc);
	  CLSETKERNELARG(ocl.runtime[0].kernels[0].vnp_kernel_normal, 0, sizeof(cl_mem), (void *)&(ocl.runtime[0].dev_llr));
	  CLSETKERNELARG(ocl.runtime[0].kernels[0].vnp_kernel_normal, 1, sizeof(cl_mem), (void *)&(ocl.runtime[0].dev_dt));
	  CLSETKERNELARG(ocl.runtime[0].kernels[0].vnp_kernel_normal, 2, sizeof(cl_mem), (void *)&(ocl.runtime[0].dev_const_llr));
	  CLSETKERNELARG(ocl.runtime[0].kernels[0].vnp_kernel_normal, 3, sizeof(cl_mem), (void *)&(ocl.runtime[0].dev_h_compact2));
	  CLSETKERNELARG(ocl.runtime[0].kernels[0].vnp_kernel_normal, 4, sizeof(int), (void *)&(BG));
	  CLSETKERNELARG(ocl.runtime[0].kernels[0].vnp_kernel_normal, 5, sizeof(int), (void *)&(row));	 	      
	  CLSETKERNELARG(ocl.runtime[0].kernels[0].vnp_kernel_normal, 6, sizeof(int), (void *)&(col));
	  CLSETKERNELARG(ocl.runtime[0].kernels[0].vnp_kernel_normal, 7, sizeof(int), (void *)&(Zc));	 
      rt = clEnqueueNDRangeKernel(ocl.runtime[0].queue[0], ocl.runtime[0].kernels[0].vnp_kernel_normal, 2, NULL, 
                                      global_item_sizek1, local_item_sizek, 0, NULL, NULL);
	  AssertFatal(rt == CL_SUCCESS, "Error %d enqueing vnp_kernel_normal \n" , (int)rt);  
	}
	
//   int pack = (block_length/128)+1;
//	dim3 pack_block(pack, MC, 1);
//	pack_decoded_bit<<<pack_block,128>>>( col, Zc);
	CLSETKERNELARG(ocl.runtime[0].kernels[0].pack_decoded_bit, 0, sizeof(cl_mem), (void *)&(ocl.runtime[0].dev_llr));
	CLSETKERNELARG(ocl.runtime[0].kernels[0].pack_decoded_bit, 1, sizeof(cl_mem), (void *)&(ocl.runtime[0].dev_tmp));
	CLSETKERNELARG(ocl.runtime[0].kernels[0].pack_decoded_bit, 2, sizeof(int), (void *)&(col));
	CLSETKERNELARG(ocl.runtime[0].kernels[0].pack_decoded_bit, 3, sizeof(int), (void *)&(Zc));
 
    // Execute the OpenCL kernel on the list
    size_t global_item_size[2] = {block_length,MC}; // Process the entire lists
    size_t local_item_size[2] = {128,1}; // Divide work items into groups of 128
    rt = clEnqueueNDRangeKernel(ocl.runtime[0].queue[0], ocl.runtime[0].kernels[0].pack_decoded_bit, 2, NULL, 
            global_item_size, local_item_size, 0, NULL, NULL);
	AssertFatal(rt == CL_SUCCESS, "Error %d enqueing pack_decoded_bit \n" , (int)rt);             
//	cudaCheck( cudaMemcpyFromSymbol((void*)hard_decision, (const void*)dev_tmp, (block_length/8)*sizeof(unsigned char)) );
//	cudaDeviceSynchronize();
	
    rt = clEnqueueReadBuffer(ocl.runtime[0].queue[0], ocl.runtime[0].dev_tmp, CL_TRUE, 0,
                              (block_length/8)*sizeof(unsigned char) , p_llr, 0, NULL, NULL);
	AssertFatal(rt == CL_SUCCESS, "Error %d moving p_llr data to  pltf %i dev %i\n" , (int)rt, 0,0); 
	return MAX_ITERATION;
	
}
#endif //NRLDPC_KERNEL_SOURCE
