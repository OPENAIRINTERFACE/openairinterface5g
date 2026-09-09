/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */

/*
* @defgroup _PHY_MODULATION_
* @ingroup _physical_layer_ref_implementation_
* @{
\section _phy_modulation_ OFDM Modulation Blocks
This section deals with basic functions for OFDM Modulation */

#include "PHY/defs_eNB.h"
#include "PHY/impl_defs_top.h"
#include "common/utils/LOG/log.h"
#include "common/utils/LOG/vcd_signal_dumper.h"
#include "phy_ofdm_mod.h"
#include "modulation_common.h"
#include "PHY/LTE_TRANSPORT/transport_common_proto.h"

void normal_prefix_mod(int32_t *txdataF,int32_t *txdata,uint8_t nsymb,LTE_DL_FRAME_PARMS *frame_parms)
{
  PHY_ofdm_mod((int *)txdataF,        // input
               (int *)txdata,         // output
               frame_parms->ofdm_symbol_size,
               1,                 // number of symbols
               frame_parms->nb_prefix_samples0,               // number of prefix samples
               CYCLIC_PREFIX);
  PHY_ofdm_mod((int *)txdataF+frame_parms->ofdm_symbol_size,        // input
               (int *)txdata+OFDM_SYMBOL_SIZE_COMPLEX_SAMPLES0,         // output
               frame_parms->ofdm_symbol_size,
               nsymb-1,
               frame_parms->nb_prefix_samples,               // number of prefix samples
               CYCLIC_PREFIX);
}

void do_OFDM_mod(c16_t **txdataF, c16_t **txdata, uint32_t frame,uint16_t next_slot, LTE_DL_FRAME_PARMS *frame_parms)
{
  int aa, slot_offset, slot_offset_F;
  slot_offset_F = (next_slot)*(frame_parms->ofdm_symbol_size)*((frame_parms->Ncp==1) ? 6 : 7);
  slot_offset = (next_slot)*(frame_parms->samples_per_tti>>1);
  for (aa=0; aa<frame_parms->nb_antennas_tx; aa++) {
    if (is_pmch_subframe(frame,next_slot>>1,frame_parms)) {
      if ((next_slot%2)==0) {
        LOG_D(PHY,"Frame %d, subframe %d: Doing MBSFN modulation (slot_offset %d)\n",frame,next_slot>>1,slot_offset);
        PHY_ofdm_mod((int *)&txdataF[aa][slot_offset_F],        // input
                     (int *)&txdata[aa][slot_offset],         // output
                     frame_parms->ofdm_symbol_size,                
                     12,                 // number of symbols
                     frame_parms->ofdm_symbol_size>>2,               // number of prefix samples
                     CYCLIC_PREFIX);

        if (frame_parms->Ncp == EXTENDED)
          PHY_ofdm_mod((int *)&txdataF[aa][slot_offset_F],        // input
                       (int *)&txdata[aa][slot_offset],         // output
                       frame_parms->ofdm_symbol_size,                
                       2,                 // number of symbols
                       frame_parms->nb_prefix_samples,               // number of prefix samples
                       CYCLIC_PREFIX);
        else {
          LOG_D(PHY,"Frame %d, subframe %d: Doing PDCCH modulation\n",frame,next_slot>>1);
          normal_prefix_mod((int32_t *)&txdataF[aa][slot_offset_F],
                            (int32_t *)&txdata[aa][slot_offset],
                            2,
                            frame_parms);
        }
      }
    } else {
      if (frame_parms->Ncp == EXTENDED)
        PHY_ofdm_mod((int *)&txdataF[aa][slot_offset_F],        // input
                     (int *)&txdata[aa][slot_offset],         // output
                     frame_parms->ofdm_symbol_size,                
                     6,                 // number of symbols
                     frame_parms->nb_prefix_samples,               // number of prefix samples
                     CYCLIC_PREFIX);
      else {
        normal_prefix_mod((int32_t *)&txdataF[aa][slot_offset_F],
                          (int32_t *)&txdata[aa][slot_offset],
                          7,
                          frame_parms);
      }
    }
  }
}
