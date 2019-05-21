#include<stdio.h>
#include <string.h>
#include <math.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>

#include "PHY/impl_defs_nr.h"
#include "PHY/defs_nr_common.h"
#include "PHY/defs_nr_UE.h"
#include "PHY/NR_UE_TRANSPORT/pucch_nr.h"
#include "PHY/NR_UE_TRANSPORT/nr_transport_proto_ue.h"

#include "common/utils/LOG/log.h"
#include "common/utils/LOG/vcd_signal_dumper.h"

#include "T.h"


void nr_decode_pucch0( int32_t **rxdataF,
			pucch_GroupHopping_t pucch_GroupHopping,
			uint32_t n_id,       // hoppingID higher layer parameter                                        
                        uint8_t *payload,
                        NR_DL_FRAME_PARMS *frame_parms,
                        int16_t amp,
                        int nr_tti_tx,
                        uint8_t m0,                                          // should come from resource set
                        uint8_t nrofSymbols,				    // should come from resource set	
                        uint8_t startingSymbolIndex,			    // should come from resource set
                        uint16_t startingPRB,				   // should come from resource set
			uint8_t nr_bit) {                                 // is number of UCI bits to be decoded
  int nr_sequences;
  uint8_t *mcs;
  if(nr_bit==1){
    mcs=table1_mcs;
    nr_sequences=4;
  }
  else if(nr_bit==2){
    mcs=table2_mcs;
    nr_sequences=8;
  }
  /*
   * Implement TS 38.211 Subclause 6.3.2.3.1 Sequence generation
   *
   */
  /*
   * Defining cyclic shift hopping TS 38.211 Subclause 6.3.2.2.2
   */
  // alpha is cyclic shift
  double alpha;
  // lnormal is the OFDM symbol number in the PUCCH transmission where l=0 corresponds to the first OFDM symbol of the PUCCH transmission
  //uint8_t lnormal;
  // lprime is the index of the OFDM symbol in the slot that corresponds to the first OFDM symbol of the PUCCH transmission in the slot given by [5, TS 38.213]
  //uint8_t lprime;
  // mcs is provided by TC 38.213 subclauses 9.2.3, 9.2.4, 9.2.5 FIXME!
  //uint8_t mcs;

  /*
   * in TS 38.213 Subclause 9.2.1 it is said that:
   * for PUCCH format 0 or PUCCH format 1, the index of the cyclic shift
   * is indicated by higher layer parameter PUCCH-F0-F1-initial-cyclic-shift
   */

  /*
   * Implementing TS 38.211 Subclause 6.3.2.3.1, the sequence x(n) shall be generated according to:
   * x(l*12+n) = r_u_v_alpha_delta(n)
   */
  // the value of u,v (delta always 0 for PUCCH) has to be calculated according to TS 38.211 Subclause 6.3.2.2.1
  uint8_t u=0,v=0;//,delta=0;
  // if frequency hopping is disabled by the higher-layer parameter PUCCH-frequency-hopping
  //              n_hop = 0
  // if frequency hopping is enabled by the higher-layer parameter PUCCH-frequency-hopping
  //              n_hop = 0 for first hop
  //              n_hop = 1 for second hop
  uint8_t n_hop = 0;
  //uint8_t PUCCH_Frequency_Hopping; // from higher layers FIXME!!

  // x_n contains the sequence r_u_v_alpha_delta(n)
  int16_t x_n_re[nr_sequences][24],x_n_im[nr_sequences][24];
  int n,i,l;
  for(i=0;i<nr_sequences;i++){ 
  // we proceed to calculate alpha according to TS 38.211 Subclause 6.3.2.2.2
    for (l=0; l<nrofSymbols; l++){
    // if frequency hopping is enabled n_hop = 1 for second hop. Not sure frequency hopping concerns format 0. FIXME!!!
    // if ((PUCCH_Frequency_Hopping == 1)&&(l == (nrofSymbols-1))) n_hop = 1;
      nr_group_sequence_hopping(pucch_GroupHopping,n_id,n_hop,nr_tti_tx,&u,&v); // calculating u and v value
      alpha = nr_cyclic_shift_hopping(n_id,m0,mcs[i],l,startingSymbolIndex,nr_tti_tx);
      #ifdef DEBUG_NR_PUCCH_TX
        printf("\t [nr_generate_pucch0] sequence generation \tu=%d \tv=%d \talpha=%lf \t(for symbol l=%d)\n",u,v,alpha,l);
      #endif
      for (n=0; n<12; n++){
        x_n_re[i][(12*l)+n] = (int16_t)((int32_t)(amp)*(int16_t)(((((int32_t)(round(32767*cos(alpha*n))) * table_5_2_2_2_2_Re[u][n])>>15)
                                  - (((int32_t)(round(32767*sin(alpha*n))) * table_5_2_2_2_2_Im[u][n])>>15)))>>15); // Re part of base sequence shifted by alpha
        x_n_im[i][(12*l)+n] =(int16_t)((int32_t)(amp)* (int16_t)(((((int32_t)(round(32767*cos(alpha*n))) * table_5_2_2_2_2_Im[u][n])>>15)
                                  + (((int32_t)(round(32767*sin(alpha*n))) * table_5_2_2_2_2_Re[u][n])>>15)))>>15); // Im part of base sequence shifted by alpha
        #ifdef DEBUG_NR_PUCCH_TX
          printf("\t [nr_generate_pucch0] sequence generation \tu=%d \tv=%d \talpha=%lf \tx_n(l=%d,n=%d)=(%d,%d)\n",
                u,v,alpha,l,n,x_n_re[(12*l)+n],x_n_im[(12*l)+n]);
        #endif
      }
    }
  }
  int16_t r_re[24],r_im[24];
  /*
   * Implementing TS 38.211 Subclause 6.3.2.3.2 Mapping to physical resources FIXME!
   */
  uint32_t re_offset=0;
  for (l=0; l<nrofSymbols; l++) {
    if ((startingPRB <  (frame_parms->N_RB_DL>>1)) && ((frame_parms->N_RB_DL & 1) == 0)) { // if number RBs in bandwidth is even and current PRB is lower band
      re_offset = ((l+startingSymbolIndex)*frame_parms->ofdm_symbol_size) + (12*startingPRB) + frame_parms->first_carrier_offset;
    }
    if ((startingPRB >= (frame_parms->N_RB_DL>>1)) && ((frame_parms->N_RB_DL & 1) == 0)) { // if number RBs in bandwidth is even and current PRB is upper band
      re_offset = ((l+startingSymbolIndex)*frame_parms->ofdm_symbol_size) + (12*(startingPRB-(frame_parms->N_RB_DL>>1)));
    }
    if ((startingPRB <  (frame_parms->N_RB_DL>>1)) && ((frame_parms->N_RB_DL & 1) == 1)) { // if number RBs in bandwidth is odd  and current PRB is lower band
      re_offset = ((l+startingSymbolIndex)*frame_parms->ofdm_symbol_size) + (12*startingPRB) + frame_parms->first_carrier_offset;
    }
    if ((startingPRB >  (frame_parms->N_RB_DL>>1)) && ((frame_parms->N_RB_DL & 1) == 1)) { // if number RBs in bandwidth is odd  and current PRB is upper band
      re_offset = ((l+startingSymbolIndex)*frame_parms->ofdm_symbol_size) + (12*(startingPRB-(frame_parms->N_RB_DL>>1))) + 6;
    }
    if ((startingPRB == (frame_parms->N_RB_DL>>1)) && ((frame_parms->N_RB_DL & 1) == 1)) { // if number RBs in bandwidth is odd  and current PRB contains DC
      re_offset = ((l+startingSymbolIndex)*frame_parms->ofdm_symbol_size) + (12*startingPRB) + frame_parms->first_carrier_offset;
    }
    for (n=0; n<12; n++){
      if ((n==6) && (startingPRB == (frame_parms->N_RB_DL>>1)) && ((frame_parms->N_RB_DL & 1) == 1)) {
        // if number RBs in bandwidth is odd  and current PRB contains DC, we need to recalculate the offset when n=6 (for second half PRB)
        re_offset = ((l+startingSymbolIndex)*frame_parms->ofdm_symbol_size);
      }
      r_re[(12*l)+n]=((int16_t *)&rxdataF[0][re_offset])[0];
      r_im[(12*l)+n]=((int16_t *)&rxdataF[0][re_offset])[1];
      #ifdef DEBUG_NR_PUCCH_TX
        printf("\t [nr_generate_pucch0] mapping to RE \t amp=%d \tofdm_symbol_size=%d \tN_RB_DL=%d \tfirst_carrier_offset=%d \ttxptr(%d)=(x_n(l=%d,n=%d)=(%d,%d))\n",
                amp,frame_parms->ofdm_symbol_size,frame_parms->N_RB_DL,frame_parms->first_carrier_offset,re_offset,
                l,n,((int16_t *)&rxdataF[0][re_offset])[0],((int16_t *)&rxdataF[0][re_offset])[1]);
      #endif
      re_offset++;
    }
  }   
  double corr[nr_sequences],corr_re[nr_sequences],corr_im[nr_sequences];
  memset(corr,0,nr_sequences*sizeof(double));
  memset(corr_re,0,nr_sequences*sizeof(double));
  memset(corr_im,0,nr_sequences*sizeof(double));
  for(i=0;i<nr_sequences;i++){
    for(l=0;l<nrofSymbols;l++){
      for(n=0;n<12;n++){
        corr_re[i]+= (double)(r_re[12*l+n])/32767*(double)(x_n_re[i][12*l+n])/32767+(double)(r_im[12*l+n])/32767*(double)(x_n_im[i][12*l+n])/32767;
	corr_im[i]+= (double)(r_re[12*l+n])/32767*(double)(x_n_im[i][12*l+n])/32767-(double)(r_im[12*l+n])/32767*(double)(x_n_re[i][12*l+n])/32767;
      }
    }
    corr[i]=corr_re[i]*corr_re[i]+corr_im[i]*corr_im[i];
  }
  float max_corr=corr[0];
  int index=0;
  for(i=1;i<nr_sequences;i++){
    if(corr[i]>max_corr){
      index= i;
      max_corr=corr[i];
    }
  }
  *payload=(uint8_t)index;  // payload bits 00..b3b2b0, b0 is the SR bit and b3b2 are HARQ bits
}

