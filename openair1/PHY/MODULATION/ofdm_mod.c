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

#include "PHY/defs_eNB.h"
#include "PHY/defs_gNB.h"
#include "PHY/impl_defs_top.h"
#include "common/utils/LOG/log.h"
#include "common/utils/LOG/vcd_signal_dumper.h"
#include "modulation_common.h"
#include "PHY/LTE_TRANSPORT/transport_common_proto.h"
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

void nr_normal_prefix_mod(int32_t *txdataF,int32_t *txdata,uint8_t nsymb,NR_DL_FRAME_PARMS *frame_parms)
{
  PHY_ofdm_mod(txdataF,        // input
	       txdata,         // output
	       frame_parms->ofdm_symbol_size,                
	       1,                 // number of symbols
	       frame_parms->nb_prefix_samples0,               // number of prefix samples
	       CYCLIC_PREFIX);
  PHY_ofdm_mod(txdataF+frame_parms->ofdm_symbol_size,        // input
	       txdata + frame_parms->ofdm_symbol_size + frame_parms->nb_prefix_samples0,         // output
	       frame_parms->ofdm_symbol_size,                
	       nsymb - 1,
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

  if(nb_symbols == 0) return;

  int16_t temp[2*2*6144*4] __attribute__((aligned(32)));
  int i,j;

  volatile int *output_ptr=(int*)0;

  int *temp_ptr=(int*)0;
  idft_size_idx_t idftsize;

  switch (fftsize) {
  case 128:
    idftsize = IDFT_128;
    break;

  case 256:
    idftsize = IDFT_256;
    break;

  case 512:
    idftsize = IDFT_512;
    break;

  case 1024:
    idftsize = IDFT_1024;
    break;

  case 1536:
    idftsize = IDFT_1536;
    break;

  case 2048:
    idftsize = IDFT_2048;
    break;

  case 3072:
    idftsize = IDFT_3072;
    break;

  case 4096:
    idftsize = IDFT_4096;
    break;

  case 6144:
    idftsize= IDFT_6144;
    break;

 case 12288:
    idftsize= IDFT_12288;
    break;

 case 24576:
    idftsize= IDFT_24576;
    break;

  default:
    idftsize = IDFT_512;
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
    idft(idftsize,(int16_t *)&input[i*fftsize],
         (fftsize==128) ? (int16_t *)temp : (int16_t *)&output[(i*fftsize) + ((1+i)*nb_prefix_samples)],
         1);
#else
    // on AVX2 need 256-bit alignment
    idft(idftsize,(int16_t *)&input[i*fftsize],
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
        memcpy((void*)output_ptr,(void*)temp_ptr,fftsize<<2);
      }
      memcpy((void*)&output_ptr[-nb_prefix_samples],(void*)&output_ptr[fftsize-nb_prefix_samples],nb_prefix_samples<<2);
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

void apply_nr_rotation(NR_DL_FRAME_PARMS *fp,
		       int16_t* trxdata,
		       int slot,
		       int first_symbol,
		       int nsymb,
		       int length) {
  int symb_offset = (slot%fp->slots_per_subframe)*fp->symbols_per_slot;

  int16_t *symbol_rotation = fp->symbol_rotation[0];

  for (int sidx=0;sidx<nsymb;sidx++) {

    LOG_D(PHY,"Rotating symbol %d, slot %d, symbol_subframe_index %d, length %d (%d,%d)\n",
      first_symbol + sidx,
      slot,
      sidx + first_symbol + symb_offset,
      length,
      symbol_rotation[2 * (sidx + first_symbol + symb_offset)],
      symbol_rotation[1 + 2 * (sidx + first_symbol + symb_offset)]);

    rotate_cpx_vector(trxdata + (sidx * length * 2),
                      &symbol_rotation[2 * (sidx + first_symbol + symb_offset)],
                      trxdata + (sidx * length * 2),
                      length,
                      15);
  }
}
		       
