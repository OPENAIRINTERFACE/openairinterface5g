/*! \file PHY/LTE_TRANSPORT/pilots_mbsfn.c
* \brief Top-level routines for generating DL mbsfn reference signals
* \authors S. Paranche, R. Knopp
* \date 2012
* \version 0.1
* \company Eurecom
* \email: knopp@eurecom.fr
* \note
* \warning
*/
//#include "defs.h"
#include "PHY/defs.h"


int generate_mbsfn_pilot(PHY_VARS_eNB *eNB,
			 eNB_rxtx_proc_t *proc,
                         int32_t **txdataF,
                         int16_t amp)

{

  LTE_DL_FRAME_PARMS *frame_parms = &eNB->frame_parms;
  uint32_t subframe_offset,Nsymb,samples_per_symbol;
  int subframe = proc->subframe_tx;


  if (subframe<0 || subframe>= 10) {
    msg("generate_mbsfn_pilots_subframe: subframe not in range (%d)\n",subframe);
    return(-1);
  }

  Nsymb = (frame_parms->Ncp==NORMAL) ? 7 : 6;

  subframe_offset = subframe*frame_parms->ofdm_symbol_size*Nsymb<<1;
  samples_per_symbol = frame_parms->ofdm_symbol_size;

  //    printf("tti %d : offset %d (slot %d)\n",tti,tti_offset,slot_offset);
  //Generate Pilots

  //antenna 4 symbol 2 Slot 0
  lte_dl_mbsfn(eNB,
               &txdataF[0][subframe_offset+(2*samples_per_symbol)],
               amp,
               subframe,
               0);



  //antenna 4 symbol 0 slot 1
  lte_dl_mbsfn(eNB,
               &txdataF[0][subframe_offset+(6*samples_per_symbol)],
               amp,
               subframe,
               1);

  //antenna 4 symbol 4 slot 1
  lte_dl_mbsfn(eNB,
               &txdataF[0][subframe_offset+(10*samples_per_symbol)],
               amp,
               subframe,
               2);

  return(0);
}


