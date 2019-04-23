#include "PHY/defs_gNB.h"
#include "nr_transport_proto.h"
#include "PHY/impl_defs_top.h"
#include "PHY/NR_TRANSPORT/nr_sch_dmrs.h"



void nr_ulsch_extract_rbs_single(int **rxdataF,
                                 int **rxdataF_ext,
                                 uint32_t rxdataF_ext_offset,
                                 // unsigned int *rb_alloc, [hna] Resource Allocation Type 1 is assumed only for the moment
                                 unsigned char symbol,
                                 unsigned short start_rb,
                                 unsigned short nb_rb_pusch,
                                 NR_DL_FRAME_PARMS *frame_parms) 
{
  unsigned short start_re,re;
  unsigned char aarx, is_dmrs_symbol = 0;
  uint32_t rxF_ext_index = 0, nb_re_pusch = 0;

  int16_t *rxF,*rxF_ext;

  is_dmrs_symbol = (symbol == 2) ? 1 : 0; //to be updated from config
  
  start_re = frame_parms->first_carrier_offset + (start_rb * NR_NB_SC_PER_RB);
  
  nb_re_pusch = NR_NB_SC_PER_RB * nb_rb_pusch;

  for (aarx = 0; aarx < frame_parms->nb_antennas_rx; aarx++) {
    
    rxF       = (int16_t *)&rxdataF[aarx][symbol * frame_parms->ofdm_symbol_size];
    rxF_ext   = (int16_t *)&rxdataF_ext[aarx][rxdataF_ext_offset];

    for (re = 0; re < nb_re_pusch; re++) {

      if ( (is_dmrs_symbol && ((re&1) != frame_parms->nushift))    ||    (is_dmrs_symbol == 0) ) { // [hna] (re&1) != frame_parms->nushift) assuming only dmrs type 1 and mapping type A

        rxF_ext[rxF_ext_index]     = (rxF[ ((start_re + re)*2)      % (frame_parms->ofdm_symbol_size*2)] << 15) >> AMP_SHIFT;
        rxF_ext[rxF_ext_index + 1] = (rxF[(((start_re + re)*2) + 1) % (frame_parms->ofdm_symbol_size*2)] << 15) >> AMP_SHIFT;
        
        rxF_ext_index = rxF_ext_index + 2;
    	}
    }
  }
}



void nr_rx_pusch(PHY_VARS_gNB *gNB,
                 uint8_t UE_id,
                 uint32_t frame,
                 uint8_t nr_tti_rx,
                 unsigned char symbol,
                 unsigned char harq_pid)
{
  
  NR_DL_FRAME_PARMS *frame_parms = &gNB->frame_parms;
  nfapi_nr_ul_config_ulsch_pdu_rel15_t *rel15_ul = &gNB->ulsch[UE_id+1][0]->harq_processes[harq_pid]->ulsch_pdu.ulsch_pdu_rel15;
  uint32_t nb_re;

  if(symbol == rel15_ul->start_symbol)
    gNB->pusch_vars[UE_id]->rxdataF_ext_offset = 0;

  if (symbol == 2)  // [hna] here it is assumed that symbol 2 carries 6 DMRS REs (dmrs-type 1)
    nb_re = rel15_ul->number_rbs * 6;
  else
    nb_re = rel15_ul->number_rbs * 12;

  //----------------------------------------------------------
  //--------------------- RBs extraction ---------------------
  //----------------------------------------------------------
  
  nr_ulsch_extract_rbs_single(gNB->common_vars.rxdataF,
                              gNB->pusch_vars[UE_id]->rxdataF_ext,
                              gNB->pusch_vars[UE_id]->rxdataF_ext_offset,
                              // rb_alloc, [hna] Resource Allocation Type 1 is assumed only for the moment
                              symbol,
                              rel15_ul->start_rb,
                              nb_re,
                              frame_parms);
    
  
  //----------------------------------------------------------
  //-------------------- LLRs computation --------------------
  //----------------------------------------------------------
  
  nr_ulsch_compute_llr(&gNB->pusch_vars[UE_id]->rxdataF_ext[0][gNB->pusch_vars[UE_id]->rxdataF_ext_offset],
                       gNB->pusch_vars[UE_id]->ul_ch_mag,
                       gNB->pusch_vars[UE_id]->ul_ch_magb,
                       &gNB->pusch_vars[UE_id]->llr[gNB->pusch_vars[UE_id]->rxdataF_ext_offset * rel15_ul->Qm],
                       nb_re,
                       symbol,
                       rel15_ul->Qm);
  
  gNB->pusch_vars[UE_id]->rxdataF_ext_offset = gNB->pusch_vars[UE_id]->rxdataF_ext_offset +  nb_re;

}