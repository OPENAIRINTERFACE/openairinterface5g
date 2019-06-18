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

/* file: lte_sync_time.c
   purpose: coarse timing synchronization for LTE (using PSS)
   author: florian.kaltenberger@eurecom.fr, oscar.tonelli@yahoo.it
   date: 22.10.2009
*/

//#include <string.h>
#include "PHY/defs_UE.h"
#include "PHY/phy_extern_ue.h"
#include <math.h>


#include "LAYER2/MAC/mac.h"
#include "RRC/LTE/rrc_extern.h"
#include "PHY_INTERFACE/phy_interface.h"
#include "PHY/LTE_REFSIG/lte_refsig.h"

// Note: this is for prototype of generate_drs_pusch (OTA synchronization of RRUs)
#include "PHY/LTE_UE_TRANSPORT/transport_proto_ue.h"

int* sync_corr_ue0 = NULL;
int* sync_corr_ue1 = NULL;
int* sync_corr_ue2 = NULL;
int sync_tmp[2048*4] __attribute__((aligned(32)));
short syncF_tmp[2048*2] __attribute__((aligned(32)));



int lte_sync_time_init(LTE_DL_FRAME_PARMS *frame_parms )   // LTE_UE_COMMON *common_vars
{

  int i,k;

  sync_corr_ue0 = (int *)malloc16(LTE_NUMBER_OF_SUBFRAMES_PER_FRAME*sizeof(int)*frame_parms->samples_per_tti);
  sync_corr_ue1 = (int *)malloc16(LTE_NUMBER_OF_SUBFRAMES_PER_FRAME*sizeof(int)*frame_parms->samples_per_tti);
  sync_corr_ue2 = (int *)malloc16(LTE_NUMBER_OF_SUBFRAMES_PER_FRAME*sizeof(int)*frame_parms->samples_per_tti);

  if (sync_corr_ue0) {
#ifdef DEBUG_PHY
    LOG_D(PHY,"[openair][LTE_PHY][SYNC] sync_corr_ue allocated at %p\n", sync_corr_ue0);
#endif
    //common_vars->sync_corr = sync_corr;
  } else {
    LOG_E(PHY,"[openair][LTE_PHY][SYNC] sync_corr_ue0 not allocated\n");
    return(-1);
  }

  if (sync_corr_ue1) {
#ifdef DEBUG_PHY
    LOG_D(PHY,"[openair][LTE_PHY][SYNC] sync_corr_ue allocated at %p\n", sync_corr_ue1);
#endif
    //common_vars->sync_corr = sync_corr;
  } else {
    LOG_E(PHY,"[openair][LTE_PHY][SYNC] sync_corr_ue1 not allocated\n");
    return(-1);
  }

  if (sync_corr_ue2) {
#ifdef DEBUG_PHY
    LOG_D(PHY,"[openair][LTE_PHY][SYNC] sync_corr_ue allocated at %p\n", sync_corr_ue2);
#endif
    //common_vars->sync_corr = sync_corr;
  } else {
    LOG_E(PHY,"[openair][LTE_PHY][SYNC] sync_corr_ue2 not allocated\n");
    return(-1);
  }

  //  primary_synch0_time = (int *)malloc16((frame_parms->ofdm_symbol_size+frame_parms->nb_prefix_samples)*sizeof(int));
  primary_synch0_time = (int16_t *)malloc16((frame_parms->ofdm_symbol_size)*sizeof(int16_t)*2);

  if (primary_synch0_time) {
    //    bzero(primary_synch0_time,(frame_parms->ofdm_symbol_size+frame_parms->nb_prefix_samples)*sizeof(int));
    bzero(primary_synch0_time,(frame_parms->ofdm_symbol_size)*sizeof(int16_t)*2);
#ifdef DEBUG_PHY
    LOG_D(PHY,"[openair][LTE_PHY][SYNC] primary_synch0_time allocated at %p\n", primary_synch0_time);
#endif
  } else {
    LOG_E(PHY,"[openair][LTE_PHY][SYNC] primary_synch0_time not allocated\n");
    return(-1);
  }

  //  primary_synch1_time = (int *)malloc16((frame_parms->ofdm_symbol_size+frame_parms->nb_prefix_samples)*sizeof(int));
  primary_synch1_time = (int16_t *)malloc16((frame_parms->ofdm_symbol_size)*sizeof(int16_t)*2);

  if (primary_synch1_time) {
    //    bzero(primary_synch1_time,(frame_parms->ofdm_symbol_size+frame_parms->nb_prefix_samples)*sizeof(int));
    bzero(primary_synch1_time,(frame_parms->ofdm_symbol_size)*sizeof(int16_t)*2);
#ifdef DEBUG_PHY
    LOG_D(PHY,"[openair][LTE_PHY][SYNC] primary_synch1_time allocated at %p\n", primary_synch1_time);
#endif
  } else {
    LOG_E(PHY,"[openair][LTE_PHY][SYNC] primary_synch1_time not allocated\n");
    return(-1);
  }

  //  primary_synch2_time = (int *)malloc16((frame_parms->ofdm_symbol_size+frame_parms->nb_prefix_samples)*sizeof(int));
  primary_synch2_time = (int16_t *)malloc16((frame_parms->ofdm_symbol_size)*sizeof(int16_t)*2);

  if (primary_synch2_time) {
    //    bzero(primary_synch2_time,(frame_parms->ofdm_symbol_size+frame_parms->nb_prefix_samples)*sizeof(int));
    bzero(primary_synch2_time,(frame_parms->ofdm_symbol_size)*sizeof(int16_t)*2);
#ifdef DEBUG_PHY
    LOG_D(PHY,"[openair][LTE_PHY][SYNC] primary_synch2_time allocated at %p\n", primary_synch2_time);
#endif
  } else {
    LOG_E(PHY,"[openair][LTE_PHY][SYNC] primary_synch2_time not allocated\n");
    return(-1);
  }


  // generate oversampled sync_time sequences
  k=frame_parms->ofdm_symbol_size-36;

  for (i=0; i<72; i++) {
    syncF_tmp[2*k] = primary_synch0[2*i]>>2;  //we need to shift input to avoid overflow in fft
    syncF_tmp[2*k+1] = primary_synch0[2*i+1]>>2;
    k++;

    if (k >= frame_parms->ofdm_symbol_size) {
      k++;  // skip DC carrier
      k-=frame_parms->ofdm_symbol_size;
    }
  }

  switch (frame_parms->N_RB_DL) {
  case 6:
    idft128((short*)syncF_tmp,          /// complex input
	   (short*)sync_tmp, /// complex output
	   1);
    break;
  case 25:
    idft512((short*)syncF_tmp,          /// complex input
	   (short*)sync_tmp, /// complex output
	   1);
    break;
  case 50:
    idft1024((short*)syncF_tmp,          /// complex input
	    (short*)sync_tmp, /// complex output
	    1);
    break;
    
  case 75:
    idft1536((short*)syncF_tmp,          /// complex input
	     (short*)sync_tmp,
	     1); /// complex output
    break;
  case 100:
    idft2048((short*)syncF_tmp,          /// complex input
	     (short*)sync_tmp, /// complex output
	     1);
    break;
  default:
    LOG_E(PHY,"Unsupported N_RB_DL %d\n",frame_parms->N_RB_DL);
    break;
  }

  for (i=0; i<frame_parms->ofdm_symbol_size; i++)
    ((int32_t*)primary_synch0_time)[i] = sync_tmp[i];

  k=frame_parms->ofdm_symbol_size-36;

  for (i=0; i<72; i++) {
    syncF_tmp[2*k] = primary_synch1[2*i]>>2;  //we need to shift input to avoid overflow in fft
    syncF_tmp[2*k+1] = primary_synch1[2*i+1]>>2;
    k++;

    if (k >= frame_parms->ofdm_symbol_size) {
      k++;  // skip DC carrier
      k-=frame_parms->ofdm_symbol_size;
    }
  }

  switch (frame_parms->N_RB_DL) {
  case 6:
    idft128((short*)syncF_tmp,          /// complex input
	   (short*)sync_tmp, /// complex output
	   1);
    break;
  case 25:
    idft512((short*)syncF_tmp,          /// complex input
	   (short*)sync_tmp, /// complex output
	   1);
    break;
  case 50:
    idft1024((short*)syncF_tmp,          /// complex input
	    (short*)sync_tmp, /// complex output
	    1);
    break;
    
  case 75:
    idft1536((short*)syncF_tmp,          /// complex input
	     (short*)sync_tmp, /// complex output
	     1);
    break;
  case 100:
    idft2048((short*)syncF_tmp,          /// complex input
	    (short*)sync_tmp, /// complex output
	    1);
    break;
  default:
    LOG_E(PHY,"Unsupported N_RB_DL %d\n",frame_parms->N_RB_DL);
    break;
  }

  for (i=0; i<frame_parms->ofdm_symbol_size; i++)
    ((int32_t*)primary_synch1_time)[i] = sync_tmp[i];

  k=frame_parms->ofdm_symbol_size-36;

  for (i=0; i<72; i++) {
    syncF_tmp[2*k] = primary_synch2[2*i]>>2;  //we need to shift input to avoid overflow in fft
    syncF_tmp[2*k+1] = primary_synch2[2*i+1]>>2;
    k++;

    if (k >= frame_parms->ofdm_symbol_size) {
      k++;  // skip DC carrier
      k-=frame_parms->ofdm_symbol_size;
    }
  }

  switch (frame_parms->N_RB_DL) {
  case 6:
    idft128((short*)syncF_tmp,          /// complex input
	   (short*)sync_tmp, /// complex output
	   1);
    break;
  case 25:
    idft512((short*)syncF_tmp,          /// complex input
	   (short*)sync_tmp, /// complex output
	   1);
    break;
  case 50:
    idft1024((short*)syncF_tmp,          /// complex input
	    (short*)sync_tmp, /// complex output
	    1);
    break;
    
  case 75:
    idft1536((short*)syncF_tmp,          /// complex input
	     (short*)sync_tmp, /// complex output
	     1);
    break;
  case 100:
    idft2048((short*)syncF_tmp,          /// complex input
	    (short*)sync_tmp, /// complex output
	    1);
    break;
  default:
    LOG_E(PHY,"Unsupported N_RB_DL %d\n",frame_parms->N_RB_DL);
    break;
  }

  for (i=0; i<frame_parms->ofdm_symbol_size; i++)
    ((int32_t*)primary_synch2_time)[i] = sync_tmp[i];




  if ( LOG_DUMPFLAG(DEBUG_LTEESTIM)){
    LOG_M("primary_sync0.m","psync0",primary_synch0_time,frame_parms->ofdm_symbol_size,1,1);
    LOG_M("primary_sync1.m","psync1",primary_synch1_time,frame_parms->ofdm_symbol_size,1,1);
    LOG_M("primary_sync2.m","psync2",primary_synch2_time,frame_parms->ofdm_symbol_size,1,1);
  }
  return (1);
}


void lte_sync_time_free(void)
{


  if (sync_corr_ue0) {
    LOG_D(PHY,"Freeing sync_corr_ue (%p)...\n",sync_corr_ue0);
    free(sync_corr_ue0);
  }

  if (sync_corr_ue1) {
    LOG_D(PHY,"Freeing sync_corr_ue (%p)...\n",sync_corr_ue1);
    free(sync_corr_ue1);
  }

  if (sync_corr_ue2) {
    LOG_D(PHY,"Freeing sync_corr_ue (%p)...\n",sync_corr_ue2);
    free(sync_corr_ue2);
  }

  if (primary_synch0_time) {
    LOG_D(PHY,"Freeing primary_sync0_time ...\n");
    free(primary_synch0_time);
  }

  if (primary_synch1_time) {
    LOG_D(PHY,"Freeing primary_sync1_time ...\n");
    free(primary_synch1_time);
  }

  if (primary_synch2_time) {
    LOG_D(PHY,"Freeing primary_sync2_time ...\n");
    free(primary_synch2_time);
  }

  sync_corr_ue0 = NULL;
  sync_corr_ue1 = NULL;
  sync_corr_ue2 = NULL;
  primary_synch0_time = NULL;
  primary_synch1_time = NULL;
  primary_synch2_time = NULL;
}

static inline int abs32(int x)
{
  return (((int)((short*)&x)[0])*((int)((short*)&x)[0]) + ((int)((short*)&x)[1])*((int)((short*)&x)[1]));
}

#define SHIFT 17

int lte_sync_time(int **rxdata, ///rx data in time domain
                  LTE_DL_FRAME_PARMS *frame_parms,
                  int *eNB_id)
{


  // perform a time domain correlation using the oversampled sync sequence

  unsigned int n, ar, s, peak_pos, peak_val, sync_source;
  int result,result2;
  int sync_out[3] = {0,0,0},sync_out2[3] = {0,0,0};
  int tmp[3] = {0,0,0};
  int length =   LTE_NUMBER_OF_SUBFRAMES_PER_FRAME*frame_parms->samples_per_tti>>1;

  //LOG_D(PHY,"[SYNC TIME] Calling sync_time.\n");
  AssertFatal(sync_corr_ue0 != NULL,
             "sync_corr_ue0 not yet allocated! Exiting.\n");
  AssertFatal(sync_corr_ue1 != NULL,
             "sync_corr_ue0 not yet allocated! Exiting.\n");
  AssertFatal(sync_corr_ue2 != NULL,
             "sync_corr_ue0 not yet allocated! Exiting.\n");

  peak_val = 0;
  peak_pos = 0;
  sync_source = 0;


  for (n=0; n<length; n+=4) {

    sync_corr_ue0[n] = 0;
    sync_corr_ue0[n+length] = 0;
    sync_corr_ue1[n] = 0;
    sync_corr_ue1[n+length] = 0;
    sync_corr_ue2[n] = 0;
    sync_corr_ue2[n+length] = 0;

    for (s=0; s<3; s++) {
      sync_out[s]=0;
      sync_out2[s]=0;
    }

    //    if (n<(length-frame_parms->ofdm_symbol_size-frame_parms->nb_prefix_samples)) {
    if (n<(length-frame_parms->ofdm_symbol_size)) {

      //calculate dot product of primary_synch0_time and rxdata[ar][n] (ar=0..nb_ant_rx) and store the sum in temp[n];
      for (ar=0; ar<frame_parms->nb_antennas_rx; ar++) {

        result  = dot_product((short*)primary_synch0_time, (short*) &(rxdata[ar][n]), frame_parms->ofdm_symbol_size, SHIFT);
        result2 = dot_product((short*)primary_synch0_time, (short*) &(rxdata[ar][n+length]), frame_parms->ofdm_symbol_size, SHIFT);

        ((short*)sync_corr_ue0)[2*n] += ((short*) &result)[0];
        ((short*)sync_corr_ue0)[2*n+1] += ((short*) &result)[1];
        ((short*)sync_corr_ue0)[2*(length+n)] += ((short*) &result2)[0];
        ((short*)sync_corr_ue0)[(2*(length+n))+1] += ((short*) &result2)[1];
        ((short*)sync_out)[0] += ((short*) &result)[0];
        ((short*)sync_out)[1] += ((short*) &result)[1];
        ((short*)sync_out2)[0] += ((short*) &result2)[0];
        ((short*)sync_out2)[1] += ((short*) &result2)[1];
      }

      for (ar=0; ar<frame_parms->nb_antennas_rx; ar++) {
        result = dot_product((short*)primary_synch1_time, (short*) &(rxdata[ar][n]), frame_parms->ofdm_symbol_size, SHIFT);
        result2 = dot_product((short*)primary_synch1_time, (short*) &(rxdata[ar][n+length]), frame_parms->ofdm_symbol_size, SHIFT);
        ((short*)sync_corr_ue1)[2*n] += ((short*) &result)[0];
        ((short*)sync_corr_ue1)[2*n+1] += ((short*) &result)[1];
        ((short*)sync_corr_ue1)[2*(length+n)] += ((short*) &result2)[0];
        ((short*)sync_corr_ue1)[(2*(length+n))+1] += ((short*) &result2)[1];

        ((short*)sync_out)[2] += ((short*) &result)[0];
        ((short*)sync_out)[3] += ((short*) &result)[1];
        ((short*)sync_out2)[2] += ((short*) &result2)[0];
        ((short*)sync_out2)[3] += ((short*) &result2)[1];
      }

      for (ar=0; ar<frame_parms->nb_antennas_rx; ar++) {

        result = dot_product((short*)primary_synch2_time, (short*) &(rxdata[ar][n]), frame_parms->ofdm_symbol_size, SHIFT);
        result2 = dot_product((short*)primary_synch2_time, (short*) &(rxdata[ar][n+length]), frame_parms->ofdm_symbol_size, SHIFT);
        ((short*)sync_corr_ue2)[2*n] += ((short*) &result)[0];
        ((short*)sync_corr_ue2)[2*n+1] += ((short*) &result)[1];
        ((short*)sync_corr_ue2)[2*(length+n)] += ((short*) &result2)[0];
        ((short*)sync_corr_ue2)[(2*(length+n))+1] += ((short*) &result2)[1];
        ((short*)sync_out)[4] += ((short*) &result)[0];
        ((short*)sync_out)[5] += ((short*) &result)[1];
        ((short*)sync_out2)[4] += ((short*) &result2)[0];
        ((short*)sync_out2)[5] += ((short*) &result2)[1];
      }

    }

    // calculate the absolute value of sync_corr[n]

    sync_corr_ue0[n] = abs32(sync_corr_ue0[n]);
    sync_corr_ue0[n+length] = abs32(sync_corr_ue0[n+length]);
    sync_corr_ue1[n] = abs32(sync_corr_ue1[n]);
    sync_corr_ue1[n+length] = abs32(sync_corr_ue1[n+length]);
    sync_corr_ue2[n] = abs32(sync_corr_ue2[n]);
    sync_corr_ue2[n+length] = abs32(sync_corr_ue2[n+length]);

    for (s=0; s<3; s++) {
      tmp[s] = (abs32(sync_out[s])>>1) + (abs32(sync_out2[s])>>1);

      if (tmp[s]>peak_val) {
        peak_val = tmp[s];
        peak_pos = n;
        sync_source = s;
        /*
        printf("s %d: n %d sync_out %d, sync_out2  %d (sync_corr %d,%d), (%d,%d) (%d,%d)\n",s,n,abs32(sync_out[s]),abs32(sync_out2[s]),sync_corr_ue0[n],
               sync_corr_ue0[n+length],((int16_t*)&sync_out[s])[0],((int16_t*)&sync_out[s])[1],((int16_t*)&sync_out2[s])[0],((int16_t*)&sync_out2[s])[1]);
        */
      }
    }
  }

  *eNB_id = sync_source;

  LOG_I(PHY,"[UE] lte_sync_time: Sync source = %d, Peak found at pos %d, val = %d (%d dB)\n",sync_source,peak_pos,peak_val,dB_fixed(peak_val)/2);


  if ( LOG_DUMPFLAG(DEBUG_LTEESTIM)){
    static int debug_cnt;
    if (debug_cnt == 0) {
      LOG_M("sync_corr0_ue.m","synccorr0",sync_corr_ue0,2*length,1,2);
      LOG_M("sync_corr1_ue.m","synccorr1",sync_corr_ue1,2*length,1,2);
      LOG_M("sync_corr2_ue.m","synccorr2",sync_corr_ue2,2*length,1,2);
      LOG_M("rxdata0.m","rxd0",rxdata[0],length<<1,1,1);
      //    exit(-1);
    } else {
    debug_cnt++;
  }
} 


  return(peak_pos);

}



int ru_sync_time_init(RU_t *ru)   // LTE_UE_COMMON *common_vars
{

  /*
  int16_t dmrs[2048];
  int16_t *dmrsp[2] = {dmrs,NULL};
  */

  int32_t dmrs[ru->frame_parms.ofdm_symbol_size*14] __attribute__((aligned(32)));
//  int32_t *dmrsp[2] = {&dmrs[(3-ru->frame_parms.Ncp)*ru->frame_parms.ofdm_symbol_size],NULL};
  int32_t *dmrsp[2] = {&dmrs[0],NULL};

  generate_ul_ref_sigs();
 
  ru->dmrssync = (int16_t*)malloc16_clear(ru->frame_parms.ofdm_symbol_size*2*sizeof(int16_t)); 
  ru->dmrs_corr = (uint64_t*)malloc16_clear(ru->frame_parms.samples_per_tti*10*sizeof(uint64_t));

  generate_drs_pusch(NULL,NULL,
		     &ru->frame_parms,
		     dmrsp,
		     0,
		     AMP,
		     0,
		     0,
		     ru->frame_parms.N_RB_DL,
		     0);

  switch (ru->frame_parms.N_RB_DL) {
  case 6:
    idft128((int16_t*)(&dmrsp[0][3*ru->frame_parms.ofdm_symbol_size]),
	    ru->dmrssync, /// complex output
	    1);
    break;
  case 25:
    idft512((int16_t*)(&dmrsp[0][3*ru->frame_parms.ofdm_symbol_size]),
	    ru->dmrssync, /// complex output
	    1);
    break;
  case 50:
    idft1024((int16_t*)(&dmrsp[0][3*ru->frame_parms.ofdm_symbol_size]),
	    ru->dmrssync, /// complex output
	    1);
    break;
     
  case 75:
    idft1536((int16_t*)(&dmrsp[0][3*ru->frame_parms.ofdm_symbol_size]),
	     ru->dmrssync,
	     1); /// complex output
    break;
  case 100:
    idft2048((int16_t*)(&dmrsp[0][3*ru->frame_parms.ofdm_symbol_size]),
	     ru->dmrssync, /// complex output
	     1);
    break;
  default:
    AssertFatal(1==0,"Unsupported N_RB_DL %d\n",ru->frame_parms.N_RB_DL);
    break;
  }

  return(0);
}

void ru_sync_time_free(RU_t *ru) {

  AssertFatal(ru->dmrssync!=NULL,"ru->dmrssync is NULL\n");
  free(ru->dmrssync);
  if (ru->dmrs_corr) free(ru->dmrs_corr);
}

//#define DEBUG_PHY

int lte_sync_time_eNB(int32_t **rxdata, ///rx data in time domain
                      LTE_DL_FRAME_PARMS *frame_parms,
                      uint32_t length,
                      uint32_t *peak_val_out,
                      uint32_t *sync_corr_eNB)
{

  // perform a time domain correlation using the oversampled sync sequence

  unsigned int n, ar, peak_val, peak_pos;
  uint64_t mean_val;
  int result;
  short *primary_synch_time;
  int eNB_id = frame_parms->Nid_cell%3;

  // LOG_E(PHY,"[SYNC TIME] Calling sync_time_eNB(%p,%p,%d,%d)\n",rxdata,frame_parms,eNB_id,length);
  if (sync_corr_eNB == NULL) {
    LOG_E(PHY,"[SYNC TIME] sync_corr_eNB not yet allocated! Exiting.\n");
    return(-1);
  }

  switch (eNB_id) {
  case 0:
    primary_synch_time = (short*)primary_synch0_time;
    break;

  case 1:
    primary_synch_time = (short*)primary_synch1_time;
    break;

  case 2:
    primary_synch_time = (short*)primary_synch2_time;
    break;

  default:
    LOG_E(PHY,"[SYNC TIME] Illegal eNB_id!\n");
    return (-1);
  }

  peak_val = 0;
  peak_pos = 0;
  mean_val = 0;

  for (n=0; n<length; n+=4) {

    sync_corr_eNB[n] = 0;

    if (n<(length-frame_parms->ofdm_symbol_size-frame_parms->nb_prefix_samples)) {

      //calculate dot product of primary_synch0_time and rxdata[ar][n] (ar=0..nb_ant_rx) and store the sum in temp[n];
      for (ar=0; ar<frame_parms->nb_antennas_rx; ar++)  {

        result = dot_product((short*)primary_synch_time, (short*) &(rxdata[ar][n]), frame_parms->ofdm_symbol_size, SHIFT);
        //((short*)sync_corr)[2*n]   += ((short*) &result)[0];
        //((short*)sync_corr)[2*n+1] += ((short*) &result)[1];
        sync_corr_eNB[n] += abs32(result);

      }

    }

    /*
    if (eNB_id == 2) {
      printf("sync_time_eNB %d : %d,%d (%d)\n",n,sync_corr_eNB[n],mean_val,
       peak_val);
    }
    */
    mean_val += sync_corr_eNB[n];

    if (sync_corr_eNB[n]>peak_val) {
      peak_val = sync_corr_eNB[n];
      peak_pos = n;
    }
  }

  mean_val/=length;

  *peak_val_out = peak_val;

  if (peak_val <= (40*(uint32_t)mean_val)) {
    LOG_I(PHY,"[SYNC TIME] No peak found (%u,%u,%"PRIu64",%"PRIu64")\n",peak_pos,peak_val,mean_val,40*mean_val);
    return(-1);
  } else {
    LOG_I(PHY,"[SYNC TIME] Peak found at pos %u, val = %u, mean_val = %"PRIu64"\n",peak_pos,peak_val,mean_val);
    return(peak_pos);
  }

}

static inline int64_t abs64(int64_t x)
{
  return (((int64_t)((int32_t*)&x)[0])*((int64_t)((int32_t*)&x)[0]) + ((int64_t)
((int32_t*)&x)[1])*((int64_t)((int32_t*)&x)[1]));
}

int ru_sync_time(RU_t *ru,
		 int64_t *lev,
		 int64_t *avg)
{


  LTE_DL_FRAME_PARMS *frame_parms = &ru->frame_parms;
  RU_CALIBRATION *calibration = &ru->calibration;
		      
  // perform a time domain correlation using the oversampled sync sequence

  int length =   LTE_NUMBER_OF_SUBFRAMES_PER_FRAME*frame_parms->samples_per_tti;
  

  // circular copy of beginning to end of rxdata buffer. Note: buffer should be big enough upon calling this function
  for (int ar=0;ar<ru->nb_rx;ar++) memcpy((void*)&ru->common.rxdata[ar][2*length],
					  (void*)&ru->common.rxdata[ar][0],
					  frame_parms->ofdm_symbol_size);
  
  int32_t maxlev0=0;
  int     maxpos0=0;
  int64_t avg0=0;
  int64_t result;
  int64_t dmrs_corr;

  int maxval=0;
  for (int i=0;i<2*(frame_parms->ofdm_symbol_size);i++) {
    maxval = max(maxval,ru->dmrssync[i]);
    maxval = max(maxval,-ru->dmrssync[i]);
  }

  if (ru->state == RU_CHECK_SYNC) {
  	for (int i=0;i<2*(frame_parms->ofdm_symbol_size);i++) {
    		maxval = max(maxval,calibration->drs_ch_estimates_time[0][i]);
	        maxval = max(maxval,-calibration->drs_ch_estimates_time[0][i]);
        }
  }

  int shift = log2_approx(maxval);

  for (int n=0; n<length; n+=4) {

    dmrs_corr = 0;

    //calculate dot product of primary_synch0_time and rxdata[ar][n] (ar=0..nb_ant_rx) and store the sum in temp[n];
    for (int ar=0; ar<ru->nb_rx; ar++) {
      
      result  = dot_product64(ru->dmrssync,
			      (int16_t*) &ru->common.rxdata[ar][n],
			      frame_parms->ofdm_symbol_size,
			      shift);     

      if (ru->state == RU_CHECK_SYNC) {
      	result  = dot_product64((int16_t*) &calibration->drs_ch_estimates_time[ar],
                              (int16_t*) &ru->common.rxdata[ar][n],
                              frame_parms->ofdm_symbol_size,
                              shift);
      }
      dmrs_corr += abs64(result);
    }
    if (ru->dmrs_corr != NULL) ru->dmrs_corr[n] = dmrs_corr;

    // tmpi holds <synchi,rx0>+<synci,rx1>+...+<synchi,rx_{nbrx-1}>


    if (dmrs_corr>maxlev0) { maxlev0 = dmrs_corr; maxpos0 = n; }
    avg0 += dmrs_corr;
  }
  avg0/=(length/4);

  int dmrsoffset = frame_parms->samples_per_tti + (3*frame_parms->ofdm_symbol_size)+(3*frame_parms->nb_prefix_samples) + frame_parms->nb_prefix_samples0;
  
  if ((int64_t)maxlev0 > (10*avg0)) {*lev = maxlev0; *avg=avg0; return((length+maxpos0-dmrsoffset)%length);}

  return(-1);


}


