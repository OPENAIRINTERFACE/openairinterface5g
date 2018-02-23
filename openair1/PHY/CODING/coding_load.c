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

/*! \file openair1/PHY/CODING
 * \brief: load library implementing coding/decoding algorithms
 * \author Francois TABURET
 * \date 2017
 * \version 0.1
 * \company NOKIA BellLabs France
 * \email: francois.taburet@nokia-bell-labs.com
 * \note
 * \warning
 */
#include <sys/types.h>
#include "PHY/defs.h"
#include "common/utils/load_module_shlib.h" 


loader_shlibfunc_t shlib_fdesc[DECODE_NUM_FPTR];

decoder_if_t    decoder16;
decoder_if_t    decoder8;
encoder_if_t    encoder;

extern int _may_i_use_cpu_feature(unsigned __int64);
uint8_t  nodecod(short *y,
    short *y2,
    unsigned char *decoded_bytes,
    unsigned char *decoded_bytes2,
    unsigned short n,
    unsigned short f1,
    unsigned short f2,
    unsigned char max_iterations,
    unsigned char crc_type,
    unsigned char F,
    time_stats_t *init_stats,
    time_stats_t *alpha_stats,
    time_stats_t *beta_stats,
    time_stats_t *gamma_stats,
    time_stats_t *ext_stats,
    time_stats_t *intl1_stats,
    time_stats_t *intl2_stats)
{
 printf(".");
 return max_iterations+1;
};

void decoding_setmode (int mode) {
   switch (mode) {
       case MODE_DECODE_NONE:
          decoder8=nodecod;
          decoder16=nodecod;
          encoder=(encoder_if_t)shlib_fdesc[ENCODE_C_FPTRIDX].fptr;
       break;
       case MODE_DECODE_SSE:
          decoder8=(decoder_if_t)shlib_fdesc[DECODE_TD8_SSE_FPTRIDX].fptr; 
          decoder16=(decoder_if_t)shlib_fdesc[DECODE_TD16_SSE_FPTRIDX].fptr;
          encoder=(encoder_if_t)shlib_fdesc[ENCODE_SSE_FPTRIDX].fptr;
       break;
       case MODE_DECODE_C:
          decoder16=(decoder_if_t)shlib_fdesc[DECODE_TD_C_FPTRIDX].fptr;
          decoder8=(decoder_if_t)shlib_fdesc[DECODE_TD_C_FPTRIDX].fptr;
          encoder=(encoder_if_t)shlib_fdesc[ENCODE_C_FPTRIDX].fptr;   
       break;
       case MODE_DECODE_AVX2:
          decoder16=(decoder_if_t)shlib_fdesc[DECODE_TD16_AVX2_FPTRIDX].fptr;
          decoder8=(decoder_if_t)shlib_fdesc[DECODE_TD8_SSE_FPTRIDX].fptr; 
          encoder=(encoder_if_t)shlib_fdesc[ENCODE_SSE_FPTRIDX].fptr;  
       break;
   }

}


int load_codinglib(void) {
 int ret;

 
     shlib_fdesc[DECODE_INITTD8_SSE_FPTRIDX].fname = "init_td8";
     shlib_fdesc[DECODE_INITTD16_SSE_FPTRIDX].fname= "init_td16";
     shlib_fdesc[DECODE_INITTD_AVX2_FPTRIDX].fname="init_td16avx2";

     shlib_fdesc[DECODE_TD8_SSE_FPTRIDX].fname=   "phy_threegpplte_turbo_decoder8";
     shlib_fdesc[DECODE_TD16_SSE_FPTRIDX].fname=  "phy_threegpplte_turbo_decoder16";
     shlib_fdesc[DECODE_TD_C_FPTRIDX].fname=      "phy_threegpplte_turbo_decoder_scalar";
     shlib_fdesc[DECODE_TD16_AVX2_FPTRIDX].fname= "phy_threegpplte_turbo_decoder16avx2";


     shlib_fdesc[DECODE_FREETD8_FPTRIDX].fname =    "free_td8";
     shlib_fdesc[DECODE_FREETD16_FPTRIDX].fname=    "free_td16";
     shlib_fdesc[DECODE_FREETD_AVX2_FPTRIDX].fname= "free_td16avx2";    

     shlib_fdesc[ENCODE_SSE_FPTRIDX].fname=    "threegpplte_turbo_encoder_sse";
     shlib_fdesc[ENCODE_C_FPTRIDX].fname=      "threegpplte_turbo_encoder";
     shlib_fdesc[ENCODE_INIT_SSE_FPTRIDX].fname=       "init_encoder_sse";
     ret=load_module_shlib("coding",shlib_fdesc,DECODE_NUM_FPTR);
     if (ret < 0) exit_fun("Error loading coding library");

/* execute encoder/decoder init functions */     
     shlib_fdesc[DECODE_INITTD8_SSE_FPTRIDX].fptr();
     shlib_fdesc[DECODE_INITTD16_SSE_FPTRIDX].fptr();
     shlib_fdesc[DECODE_INITTD_AVX2_FPTRIDX].fptr();
     shlib_fdesc[ENCODE_INIT_SSE_FPTRIDX].fptr();
     decoding_setmode(MODE_DECODE_SSE);
     
return 0;
}

void free_codinglib(void) {

     shlib_fdesc[DECODE_FREETD8_FPTRIDX].fptr();
     shlib_fdesc[DECODE_FREETD16_FPTRIDX].fptr();
     shlib_fdesc[DECODE_FREETD_AVX2_FPTRIDX].fptr();


}
