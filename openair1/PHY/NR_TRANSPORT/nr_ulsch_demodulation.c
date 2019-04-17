// This should have nr_rx_pdsch, nr_ulsch_extract_rbs_single, and nr_ulsch_compute_llr
#include "PHY/defs_gNB.h"
#include "nr_transport_proto.h"
#include "PHY/impl_defs_top.h"

//==============================================================================================
// Extraction functions
//==============================================================================================

unsigned short nr_ulsch_extract_rbs_single(int **rxdataF,
                                           int **rxdataF_ext,
                                           uint32_t rxdataF_ext_offset,
                                           // unsigned int *rb_alloc, [hna] Resource Allocation Type 1 is assumed only for the moment
                                           unsigned char symbol,
                                           unsigned short start_rb,
                                           unsigned short nb_rb_pdsch,
                                           NR_DL_FRAME_PARMS *frame_parms) 
{
  unsigned short start_re,re;
  unsigned char aarx, is_dmrs_symbol = 0;
  uint32_t rxF_ext_index = 0, nb_re_pdsch = 0;

  int16_t *rxF,*rxF_ext;
    
  is_dmrs_symbol = (symbol == 2) ? 1 : 0; //to be updated from config
  
  start_re = frame_parms->first_carrier_offset + (start_rb * NR_NB_SC_PER_RB);
  
  nb_re_pdsch = NR_NB_SC_PER_RB * nb_rb_pdsch;

  for (aarx = 0; aarx < frame_parms->nb_antennas_rx; aarx++) {
    
    rxF       = (int16_t *)&rxdataF[aarx][symbol * frame_parms->ofdm_symbol_size];
    rxF_ext   = (int16_t *)&rxdataF_ext[aarx][rxdataF_ext_offset];

    for (re = 0; re < nb_re_pdsch; re++) {

    	if ( (is_dmrs_symbol && ((re&1) != frame_parms->nushift))    ||    (is_dmrs_symbol == 0) ) { // [hna] (re&1) != frame_parms->nushift) assuming only dmrs type 1 and mapping type A

    	  rxF_ext[rxF_ext_index]     = (rxF[ ((start_re + re)*2)      % (frame_parms->ofdm_symbol_size*2)] << 15) >> AMP_SHIFT;
    	  rxF_ext[rxF_ext_index + 1] = (rxF[(((start_re + re)*2) + 1) % (frame_parms->ofdm_symbol_size*2)] << 15) >> AMP_SHIFT;
        
        rxF_ext_index = rxF_ext_index + 2;
    	}
    }
  }
  
  return(nb_rb_pdsch/frame_parms->nb_antennas_rx);
}