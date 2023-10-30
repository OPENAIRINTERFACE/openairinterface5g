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
#include <math.h>
#include "PHY/defs_UE.h"
#include "PHY/phy_extern.h"
#include "PHY/LTE_REFSIG/lte_refsig.h"

// Note: this is for prototype of generate_drs_pusch (OTA synchronization of RRUs)
#include "PHY/LTE_UE_TRANSPORT/transport_proto_ue.h"

static c16_t *primary_synch_time[3] __attribute__((aligned(32)));

static void doIdft(int size, short *in, short *out) {
  switch (size) {
  case 6:
      idft(IDFT_128,in,out,1);
    break;

  case 25:
      idft(IDFT_512,in,out,1);
    break;

  case 50:
      idft(IDFT_1024,in,out,1);
    break;
    
  case 75:
      idft(IDFT_1536,in,out,1);
    break;

  case 100:
      idft(IDFT_2048,in,out,1);
    break;

  default:
      LOG_E(PHY,"Unsupported N_RB_DL %d\n",size);
      abort();
    break;
    }
  }

static void copyPrimary( c16_t *out, struct complex16 *in, int ofdmSize) {
  int k=ofdmSize-36;
    
  for (int i=0; i<72; i++) {
    out[k].r = in[i].r>>1;  //we need to shift input to avoid overflow in fft
    out[k].i = in[i].i>>1;
    k++;

    if (k >= ofdmSize) {
      k++;  // skip DC carrier
      k-=ofdmSize;
    }
  }
  }

int lte_sync_time_init(LTE_DL_FRAME_PARMS *frame_parms ) { // LTE_UE_COMMON *common_vars
  c16_t syncF_tmp[2048]__attribute__((aligned(32)))= {{0}};
  int sz = frame_parms->ofdm_symbol_size * sizeof(**primary_synch_time);
  for (int i = 0; i < sizeofArray(primary_synch_time); i++) {
    primary_synch_time[i] = (c16_t *)malloc16_clear(sz);
    AssertFatal(primary_synch_time[i],"no memory\n");
  }
  // generate oversampled sync_time sequences
  copyPrimary( syncF_tmp, (c16_t *) primary_synch0, frame_parms->ofdm_symbol_size);
  doIdft(frame_parms->N_RB_DL, (short *)syncF_tmp, (short *)primary_synch_time[0]);
  copyPrimary( syncF_tmp, (c16_t *) primary_synch1, frame_parms->ofdm_symbol_size);
  doIdft(frame_parms->N_RB_DL, (short *)syncF_tmp, (short *)primary_synch_time[1]);
  copyPrimary( syncF_tmp, (c16_t *) primary_synch2, frame_parms->ofdm_symbol_size);
  doIdft(frame_parms->N_RB_DL, (short *)syncF_tmp, (short *)primary_synch_time[2]);

  if ( LOG_DUMPFLAG(DEBUG_LTEESTIM)){
    LOG_M("primary_sync0.m", "psync0", primary_synch_time[0], frame_parms->ofdm_symbol_size, 1, 1);
    LOG_M("primary_sync1.m", "psync1", primary_synch_time[1], frame_parms->ofdm_symbol_size, 1, 1);
    LOG_M("primary_sync2.m", "psync2", primary_synch_time[2], frame_parms->ofdm_symbol_size, 1, 1);
  }

  return (1);
}


void lte_sync_time_free(void) {
  for (int i = 0; i < sizeofArray(primary_synch_time); i++)
    if (primary_synch_time[i]) {
      LOG_D(PHY, "Freeing primary_sync0_time ...\n");
      free_and_zero(primary_synch_time[i]);
    }
}


#define complexNull(c) bzero((void*) &(c), sizeof(c))

#define SHIFT 17

int lte_sync_time(int **rxdata, LTE_DL_FRAME_PARMS *frame_parms, int *eNB_id)
{
  // perform a time domain correlation using the oversampled sync sequence
  unsigned int n, ar, s, peak_pos, peak_val, sync_source;
  struct complexd sync_out[3]= {{0}}, sync_out2[3]= {{0}};
  int length =   LTE_NUMBER_OF_SUBFRAMES_PER_FRAME*frame_parms->samples_per_tti>>1;
  peak_val = 0;
  peak_pos = 0;
  sync_source = 0;

  for (n=0; n<length; n+=4) {
    for (s=0; s<3; s++) {
      complexNull(sync_out[s]);
      complexNull(sync_out2[s]);
    }

    //    if (n<(length-frame_parms->ofdm_symbol_size-frame_parms->nb_prefix_samples)) {
    if (n<(length-frame_parms->ofdm_symbol_size)) {
      //calculate dot product of primary_synch0_time and rxdata[ar][n] (ar=0..nb_ant_rx) and store the sum in temp[n];
      for (int i = 0; i < sizeofArray(primary_synch_time); i++)
        for (ar = 0; ar < frame_parms->nb_antennas_rx; ar++) {
          c32_t result = dot_product(primary_synch_time[i], (c16_t *)&(rxdata[ar][n]), frame_parms->ofdm_symbol_size, SHIFT);
          c32_t result2 = dot_product(primary_synch_time[i], (c16_t *)&(rxdata[ar][n + length]), frame_parms->ofdm_symbol_size, SHIFT);
          sync_out[i].r += result.r;
          sync_out[i].i += result.i;
          sync_out2[i].r += result2.r;
          sync_out2[i].i += result2.i;
        }
    }

    // calculate the absolute value of sync_corr[n]

    for (s=0; s<3; s++) {
      double tmp = squaredMod(sync_out[s]) + squaredMod(sync_out2[s]);

      if (tmp>peak_val) {
        peak_val = tmp;
        peak_pos = n;
        sync_source = s;
      }
    }
  }

  *eNB_id = sync_source;
  LOG_I(PHY,"[UE] lte_sync_time: Sync source = %d, Peak found at pos %d, val = %d (%d dB)\n",sync_source,peak_pos,peak_val/2,dB_fixed(peak_val/2)/2);
  return(peak_pos);
}

int ru_sync_time_init(RU_t *ru) { // LTE_UE_COMMON *common_vars
  /*
  int16_t dmrs[2048];
  int16_t *dmrsp[2] = {dmrs,NULL};
  */
  int32_t dmrs[ru->frame_parms->ofdm_symbol_size*14] __attribute__((aligned(32)));
  //int32_t *dmrsp[2] = {&dmrs[(3-ru->frame_parms->Ncp)*ru->frame_parms->ofdm_symbol_size],NULL};
  int32_t *dmrsp[2] = {&dmrs[0],NULL};
  generate_ul_ref_sigs();
  ru->dmrssync = (int16_t *)malloc16_clear(ru->frame_parms->ofdm_symbol_size*2*sizeof(int16_t));
  ru->dmrs_corr = (uint64_t *)malloc16_clear(ru->frame_parms->samples_per_tti*10*sizeof(uint64_t));
  generate_drs_pusch(NULL,
                     NULL,
                     ru->frame_parms,
                     dmrsp,
                     0,
                     AMP,
                     0,
                     0,
                     ru->frame_parms->N_RB_DL,
                     0);

  switch (ru->frame_parms->N_RB_DL) {
    case 6:
      idft(IDFT_128,(int16_t *)(&dmrsp[0][3*ru->frame_parms->ofdm_symbol_size]),
           ru->dmrssync, /// complex output
           1);
      break;

    case 25:
      idft(IDFT_512,(int16_t *)(&dmrsp[0][3*ru->frame_parms->ofdm_symbol_size]),
           ru->dmrssync, /// complex output
           1);
      break;

    case 50:
      idft(IDFT_1024,(int16_t *)(&dmrsp[0][3*ru->frame_parms->ofdm_symbol_size]),
           ru->dmrssync, /// complex output
           1);
      break;

    case 75:
      idft(IDFT_1536,(int16_t *)(&dmrsp[0][3*ru->frame_parms->ofdm_symbol_size]),
           ru->dmrssync,
           1); /// complex output
      break;

    case 100:
      idft(IDFT_2048,(int16_t *)(&dmrsp[0][3*ru->frame_parms->ofdm_symbol_size]),
           ru->dmrssync, /// complex output
           1);
      break;

    default:
      AssertFatal(1==0,"Unsupported N_RB_DL %d\n",ru->frame_parms->N_RB_DL);
      break;
  }

  return(0);
}


void ru_sync_time_free(RU_t *ru) {
  AssertFatal(ru->dmrssync!=NULL,"ru->dmrssync is NULL\n");
  free(ru->dmrssync);

  if (ru->dmrs_corr)
    free(ru->dmrs_corr);
}

int ru_sync_time(RU_t *ru,
                 int64_t *lev,
                 int64_t *avg) {
  LTE_DL_FRAME_PARMS *frame_parms = ru->frame_parms;
  RU_CALIBRATION *calibration = &ru->calibration;
  // perform a time domain correlation using the oversampled sync sequence
  int length =   LTE_NUMBER_OF_SUBFRAMES_PER_FRAME*frame_parms->samples_per_tti;

  // circular copy of beginning to end of rxdata buffer. Note: buffer should be big enough upon calling this function
  for (int ar=0; ar<ru->nb_rx; ar++)
    memcpy((void *)&ru->common.rxdata[ar][2*length],
           (void *)&ru->common.rxdata[ar][0],
           frame_parms->ofdm_symbol_size);

  int32_t maxlev0=0;
  int     maxpos0=0;
  int64_t avg0 = 0;
  int64_t dmrs_corr;
  int maxval=0;

  for (int i=0; i<2*(frame_parms->ofdm_symbol_size); i++) {
    maxval = max(maxval,ru->dmrssync[i]);
    maxval = max(maxval,-ru->dmrssync[i]);
  }

  if (ru->state == RU_CHECK_SYNC) {
    for (int i=0; i<2*(frame_parms->ofdm_symbol_size); i++) {
      maxval = max(maxval,calibration->drs_ch_estimates_time[0][i]);
      maxval = max(maxval,-calibration->drs_ch_estimates_time[0][i]);
    }
  }

  int shift = log2_approx(maxval);

  for (int n=0; n<length; n+=4) {
    dmrs_corr = 0;

    //calculate dot product of primary_synch0_time and rxdata[ar][n] (ar=0..nb_ant_rx) and store the sum in temp[n];
    for (int ar=0; ar<ru->nb_rx; ar++) {
      const c16_t *input = ru->state == RU_CHECK_SYNC ?
        (c16_t *)&calibration->drs_ch_estimates_time[ar] :
        (c16_t *)ru->dmrssync;
      c32_t result = dot_product(input, (c16_t *)&ru->common.rxdata[ar][n], frame_parms->ofdm_symbol_size, shift);
      const c64_t tmp = {result.r, result.i};
      dmrs_corr += squaredMod(tmp);
    }

    if (ru->dmrs_corr != NULL)
      ru->dmrs_corr[n] = dmrs_corr;

    // tmpi holds <synchi,rx0>+<synci,rx1>+...+<synchi,rx_{nbrx-1}>

    if (dmrs_corr>maxlev0) {
      maxlev0 = dmrs_corr;
      maxpos0 = n;
    }

    avg0 += dmrs_corr;
  }

  avg0/=(length/4);
  int dmrsoffset = frame_parms->samples_per_tti + (3*frame_parms->ofdm_symbol_size)+(3*frame_parms->nb_prefix_samples) + frame_parms->nb_prefix_samples0;

  if ((int64_t)maxlev0 > (10*avg0)) {
    *lev = maxlev0;
    *avg=avg0;
    return((length+maxpos0-dmrsoffset)%length);
  }

  return(-1);
}
