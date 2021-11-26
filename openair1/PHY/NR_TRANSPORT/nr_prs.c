
#include "PHY/defs_gNB.h"
#include "PHY/NR_TRANSPORT/nr_transport_proto.h"
#include "PHY/LTE_REFSIG/lte_refsig.h"
#include "PHY/sse_intrin.h"

#define DEBUG_PRS

extern short nr_qpsk_mod_table[8];

int nr_generate_prs(uint32_t *nr_gold_prs,
                          int32_t *txdataF,
                          int16_t amp,
                          uint8_t ssb_start_symbol,
                          nfapi_nr_config_request_scf_t *config,
                          NR_DL_FRAME_PARMS *frame_parms) {
  //int k,l;
  //int16_t a;
  int16_t mod_prs[NR_MAX_PRS_INIT_LENGTH_DWORD<<1];
  uint8_t idx=0;
  uint8_t nushift = config->cell_config.phy_cell_id.value &3;
  LOG_D(PHY, "PRS mapping started at symbol %d shift %d\n", ssb_start_symbol+1, nushift);

  /// QPSK modulation
  for (int m=0; m<NR_MAX_PRS_LENGTH; m++) {
    idx = (((nr_gold_prs[(m<<1)>>5])>>((m<<1)&0x1f))&3);
    mod_prs[m<<1] = nr_qpsk_mod_table[idx<<1];
    mod_prs[(m<<1)+1] = nr_qpsk_mod_table[(idx<<1) + 1];
    
#ifdef DEBUG_PRS
    printf("m %d idx %d gold seq %d b0-b1 %d-%d mod_prs %d %d\n", m, idx, nr_gold_prs[(m<<1)>>5], (((nr_gold_prs[(m<<1)>>5])>>((m<<1)&0x1f))&1),
           (((nr_gold_prs[((m<<1)+1)>>5])>>(((m<<1)+1)&0x1f))&1), mod_prs[(m<<1)], mod_prs[(m<<1)+1]);
#endif
  }
  return 0;
}
 


