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

/*
* @defgroup _PHY_MODULATION_
* @ingroup _physical_layer_ref_implementation_
* @{
\section _phy_modulation_ OFDM Modulation Blocks
This section deals with basic functions for OFDM Modulation.


*/

#include "PHY/defs.h"
#include "UTIL/LOG/log.h"
#include "UTIL/LOG/vcd_signal_dumper.h"

//#define DEBUG_OFDM_MOD


void normal_prefix_mod(int32_t *txdataF,int32_t *txdata,uint8_t nsymb,LTE_DL_FRAME_PARMS *frame_parms)
{


  
  PHY_ofdm_mod(txdataF,        // input
	       txdata,         // output
	       frame_parms->ofdm_symbol_size,                
	       1,                 // number of symbols
	       frame_parms->nb_prefix_samples0,               // number of prefix samples
	       CYCLIC_PREFIX);
  PHY_ofdm_mod(txdataF+frame_parms->ofdm_symbol_size,        // input
	       txdata+OFDM_SYMBOL_SIZE_COMPLEX_SAMPLES0,         // output
	       frame_parms->ofdm_symbol_size,                
	       nsymb-1,
	       frame_parms->nb_prefix_samples,               // number of prefix samples
	       CYCLIC_PREFIX);
  

  
}

void PHY_ofdm_mod(int *input,                       /// pointer to complex input
                  int *output,                      /// pointer to complex output
                  int fftsize,            /// FFT_SIZE
                  unsigned char nb_symbols,         /// number of OFDM symbols
                  unsigned short nb_prefix_samples,  /// cyclic prefix length
                  Extension_t etype                /// type of extension
                 )
{

  short temp[2048*4] __attribute__((aligned(32)));
  unsigned short i,j;
  short k;

  volatile int *output_ptr=(int*)0;

  int *temp_ptr=(int*)0;
  void (*idft)(int16_t *,int16_t *, int);

  switch (fftsize) {
  case 128:
    idft = idft128;
    break;

  case 256:
    idft = idft256;
    break;

  case 512:
    idft = idft512;
    break;

  case 1024:
    idft = idft1024;
    break;

  case 1536:
    idft = idft1536;
    break;

  case 2048:
    idft = idft2048;
    break;

  default:
    idft = idft512;
    break;
  }

#ifdef DEBUG_OFDM_MOD
  printf("[PHY] OFDM mod (size %d,prefix %d) Symbols %d, input %p, output %p\n",
      fftsize,nb_prefix_samples,nb_symbols,input,output);
#endif



  for (i=0; i<nb_symbols; i++) {

#ifdef DEBUG_OFDM_MOD
    printf("[PHY] symbol %d/%d offset %d (%p,%p -> %p)\n",i,nb_symbols,i*fftsize+(i*nb_prefix_samples),input,&input[i*fftsize],&output[(i*fftsize) + ((i)*nb_prefix_samples)]);
#endif

#ifndef __AVX2__
    // handle 128-bit alignment for 128-bit SIMD (SSE4,NEON,AltiVEC)
    idft((int16_t *)&input[i*fftsize],
         (fftsize==128) ? (int16_t *)temp : (int16_t *)&output[(i*fftsize) + ((1+i)*nb_prefix_samples)],
         1);
#else
    // on AVX2 need 256-bit alignment
    idft((int16_t *)&input[i*fftsize],
         (int16_t *)temp,
         1);

#endif

    // Copy to frame buffer with Cyclic Extension
    // Note:  will have to adjust for synchronization offset!

    switch (etype) {
    case CYCLIC_PREFIX:
      output_ptr = &output[(i*fftsize) + ((1+i)*nb_prefix_samples)];
      temp_ptr = (int *)temp;


      //      msg("Doing cyclic prefix method\n");

#ifndef __AVX2__
      if (fftsize==128) 
#endif
      {
        /*for (j=0; j<fftsize ; j++) {
          output_ptr[j] = temp_ptr[j];
        }*/
        memcpy((void*)output_ptr,(void*)temp_ptr,fftsize<<2);
      }

      j=fftsize;

      for (k=-1; k>=-nb_prefix_samples; k--) {
        output_ptr[k] = output_ptr[--j];
      }

      break;

    case CYCLIC_SUFFIX:


      output_ptr = &output[(i*fftsize)+ (i*nb_prefix_samples)];

      temp_ptr = (int *)temp;

      //      msg("Doing cyclic suffix method\n");

      for (j=0; j<fftsize ; j++) {
        output_ptr[j] = temp_ptr[2*j];
      }


      for (j=0; j<nb_prefix_samples; j++)
        output_ptr[fftsize+j] = output_ptr[j];

      break;

    case ZEROS:

      break;

    case NONE:

      //      msg("NO EXTENSION!\n");
      output_ptr = &output[fftsize];

      temp_ptr = (int *)temp;

      for (j=0; j<fftsize ; j++) {
        output_ptr[j] = temp_ptr[2*j];


      }

      break;

    default:
      break;

    }



  }


}


void do_OFDM_mod(int32_t **txdataF, int32_t **txdata, uint32_t frame,uint16_t next_slot, LTE_DL_FRAME_PARMS *frame_parms)
{

  int aa, slot_offset, slot_offset_F;

  slot_offset_F = (next_slot)*(frame_parms->ofdm_symbol_size)*((frame_parms->Ncp==1) ? 6 : 7);
  slot_offset = (next_slot)*(frame_parms->samples_per_tti>>1);

  for (aa=0; aa<frame_parms->nb_antennas_tx; aa++) {
    if (is_pmch_subframe(frame,next_slot>>1,frame_parms)) {
      if ((next_slot%2)==0) {
        LOG_D(PHY,"Frame %d, subframe %d: Doing MBSFN modulation (slot_offset %d)\n",frame,next_slot>>1,slot_offset);
        PHY_ofdm_mod(&txdataF[aa][slot_offset_F],        // input
                     &txdata[aa][slot_offset],         // output
                     frame_parms->ofdm_symbol_size,                
                     12,                 // number of symbols
                     frame_parms->ofdm_symbol_size>>2,               // number of prefix samples
                     CYCLIC_PREFIX);

        if (frame_parms->Ncp == EXTENDED)
          PHY_ofdm_mod(&txdataF[aa][slot_offset_F],        // input
                       &txdata[aa][slot_offset],         // output
                       frame_parms->ofdm_symbol_size,                
                       2,                 // number of symbols
                       frame_parms->nb_prefix_samples,               // number of prefix samples
                       CYCLIC_PREFIX);
        else {
          LOG_D(PHY,"Frame %d, subframe %d: Doing PDCCH modulation\n",frame,next_slot>>1);
          normal_prefix_mod(&txdataF[aa][slot_offset_F],
                            &txdata[aa][slot_offset],
                            2,
                            frame_parms);
        }
      }
    } else {
      if (frame_parms->Ncp == EXTENDED)
        PHY_ofdm_mod(&txdataF[aa][slot_offset_F],        // input
                     &txdata[aa][slot_offset],         // output
                     frame_parms->ofdm_symbol_size,                
                     6,                 // number of symbols
                     frame_parms->nb_prefix_samples,               // number of prefix samples
                     CYCLIC_PREFIX);
      else {
        normal_prefix_mod(&txdataF[aa][slot_offset_F],
                          &txdata[aa][slot_offset],
                          7,
                          frame_parms);
      }
    }
  }

}

/*
// OFDM modulation for each symbol
void do_OFDM_mod_symbol(LTE_eNB_COMMON *eNB_common_vars, RU_COMMON *common, uint16_t next_slot, LTE_DL_FRAME_PARMS *frame_parms,int do_precoding)
{

  int aa, l, slot_offset, slot_offsetF;
  int32_t **txdataF    = eNB_common_vars->txdataF;
  int32_t **txdataF_BF = common->txdataF_BF;
  int32_t **txdata     = common->txdata;

  slot_offset  = (next_slot)*(frame_parms->samples_per_tti>>1);
  slot_offsetF = (next_slot)*(frame_parms->ofdm_symbol_size)*((frame_parms->Ncp==EXTENDED) ? 6 : 7);
  //printf("Thread %d starting ... aa %d (%llu)\n",omp_get_thread_num(),aa,rdtsc());
  for (l=0; l<frame_parms->symbols_per_tti>>1; l++) {
  
    for (aa=0; aa<frame_parms->nb_antennas_tx; aa++) {

      //printf("do_OFDM_mod_l, slot=%d, l=%d, NUMBER_OF_OFDM_CARRIERS=%d,OFDM_SYMBOL_SIZE_COMPLEX_SAMPLES=%d\n",next_slot, l,NUMBER_OF_OFDM_CARRIERS,OFDM_SYMBOL_SIZE_COMPLEX_SAMPLES);
      VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_ENB_BEAM_PRECODING,1);
      if (do_precoding==1) beam_precoding(txdataF,txdataF_BF,frame_parms,eNB_common_vars->beam_weights,next_slot,l,aa);
      VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_ENB_BEAM_PRECODING,0);

      //PMCH case not implemented... 

      if (frame_parms->Ncp == EXTENDED)
        PHY_ofdm_mod((do_precoding == 1)?txdataF_BF[aa]:&txdataF[aa][slot_offsetF+l*frame_parms->ofdm_symbol_size],         // input
                     &txdata[aa][slot_offset+l*OFDM_SYMBOL_SIZE_COMPLEX_SAMPLES],            // output
                     frame_parms->ofdm_symbol_size,       
                     1,                                   // number of symbols
                     frame_parms->nb_prefix_samples,      // number of prefix samples
                     CYCLIC_PREFIX);
      else {
        if (l==0) {
          PHY_ofdm_mod((do_precoding==1)?txdataF_BF[aa]:&txdataF[aa][slot_offsetF+l*frame_parms->ofdm_symbol_size],        // input
                       &txdata[aa][slot_offset],           // output
                       frame_parms->ofdm_symbol_size,      
                       1,                                  // number of symbols
                       frame_parms->nb_prefix_samples0,    // number of prefix samples
                       CYCLIC_PREFIX);
           
        } else {
	  PHY_ofdm_mod((do_precoding==1)?txdataF_BF[aa]:&txdataF[aa][slot_offsetF+l*frame_parms->ofdm_symbol_size],        // input
                       &txdata[aa][slot_offset+OFDM_SYMBOL_SIZE_COMPLEX_SAMPLES0+(l-1)*OFDM_SYMBOL_SIZE_COMPLEX_SAMPLES],           // output
                       frame_parms->ofdm_symbol_size,      
                       1,                                  // number of symbols
                       frame_parms->nb_prefix_samples,     // number of prefix samples
                       CYCLIC_PREFIX);

          //printf("txdata[%d][%d]=%d\n",aa,slot_offset+OFDM_SYMBOL_SIZE_COMPLEX_SAMPLES0+(l-1)*OFDM_SYMBOL_SIZE_COMPLEX_SAMPLES,txdata[aa][slot_offset+OFDM_SYMBOL_SIZE_COMPLEX_SAMPLES0+(l-1)*OFDM_SYMBOL_SIZE_COMPLEX_SAMPLES]);
 
        }
      }
    }
  }

}
*/
