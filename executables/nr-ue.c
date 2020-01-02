/*
 * Licensed to the OpenAirInterface (OAI) Software Alliance under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The OpenAirInterface Software Alliance licenses this file to You under
 * the OAI Public License, Version 1.0  (the "License"); you may not use this file
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

#include "executables/nr-uesoftmodem.h"

#include "LAYER2/NR_MAC_UE/mac.h"
//#include "RRC/LTE/rrc_extern.h"
#include "PHY_INTERFACE/phy_interface_extern.h"

#undef MALLOC //there are two conflicting definitions, so we better make sure we don't use it at all
//#undef FRAME_LENGTH_COMPLEX_SAMPLES //there are two conflicting definitions, so we better make sure we don't use it at all

#include "fapi_nr_ue_l1.h"
#include "PHY/phy_extern_nr_ue.h"
#include "PHY/INIT/phy_init.h"
#include "PHY/MODULATION/modulation_UE.h"
#include "LAYER2/NR_MAC_UE/mac_proto.h"
#include "RRC/NR_UE/rrc_proto.h"

//#ifndef NO_RAT_NR
#include "SCHED_NR/phy_frame_config_nr.h"
//#endif
#include "SCHED_NR_UE/defs.h"

#include "PHY/NR_UE_TRANSPORT/nr_transport_proto_ue.h"

#include "common/utils/LOG/log.h"
#include "common/utils/system.h"
#include "common/utils/LOG/vcd_signal_dumper.h"
#include "executables/nr-softmodem.h"

#include "T.h"

#ifdef XFORMS
  #include "PHY/TOOLS/nr_phy_scope.h"

  extern char do_forms;
#endif


extern double cpuf;
//static  nfapi_nr_config_request_t config_t;
//static  nfapi_nr_config_request_t* config =&config_t;

/*
 *  NR SLOT PROCESSING SEQUENCE
 *
 *  Processing occurs with following steps for connected mode:
 *
 *  - Rx samples for a slot are received,
 *  - PDCCH processing (including DCI extraction for downlink and uplink),
 *  - PDSCH processing (including transport blocks decoding),
 *  - PUCCH/PUSCH (transmission of acknowledgements, CSI, ... or data).
 *
 *  Time between reception of the slot and related transmission depends on UE processing performance.
 *  It is defined by the value NR_UE_CAPABILITY_SLOT_RX_TO_TX.
 *
 *  In NR, network gives the duration between Rx slot and Tx slot in the DCI:
 *  - for reception of a PDSCH and its associated acknowledgment slot (with a PUCCH or a PUSCH),
 *  - for reception of an uplink grant and its associated PUSCH slot.
 *
 *  So duration between reception and it associated transmission depends on its transmission slot given in the DCI.
 *  NR_UE_CAPABILITY_SLOT_RX_TO_TX means the minimum duration but higher duration can be given by the network because UE can support it.
 *
 *                                                                                                    Slot k
 *                                                                                  -------+------------+--------
 *                Frame                                                                    | Tx samples |
 *                Subframe                                                                 |   buffer   |
 *                Slot n                                                            -------+------------+--------
 *       ------ +------------+--------                                                     |
 *              | Rx samples |                                                             |
 *              |   buffer   |                                                             |
 *       -------+------------+--------                                                     |
 *                           |                                                             |
 *                           V                                                             |
 *                           +------------+                                                |
 *                           |   PDCCH    |                                                |
 *                           | processing |                                                |
 *                           +------------+                                                |
 *                           |            |                                                |
 *                           |            v                                                |
 *                           |            +------------+                                   |
 *                           |            |   PDSCH    |                                   |
 *                           |            | processing | decoding result                   |
 *                           |            +------------+    -> ACK/NACK of PDSCH           |
 *                           |                         |                                   |
 *                           |                         v                                   |
 *                           |                         +-------------+------------+        |
 *                           |                         | PUCCH/PUSCH | Tx samples |        |
 *                           |                         |  processing | transfer   |        |
 *                           |                         +-------------+------------+        |
 *                           |                                                             |
 *                           |/___________________________________________________________\|
 *                            \  duration between reception and associated transmission   /
 *
 * Remark: processing is done slot by slot, it can be distribute on different threads which are executed in parallel.
 * This is an architecture optimization in order to cope with real time constraints.
 * By example, for LTE, subframe processing is spread over 4 different threads.
 *
 */

#ifndef NO_RAT_NR
  #define DURATION_RX_TO_TX           (NR_UE_CAPABILITY_SLOT_RX_TO_TX)  /* for NR this will certainly depends to such UE capability which is not yet defined */
#else
  #define DURATION_RX_TO_TX           (4)   /* For LTE, this duration is fixed to 4 and it is linked to LTE standard for both modes FDD/TDD */
#endif

#define FRAME_PERIOD    100000000ULL
#define DAQ_PERIOD      66667ULL

typedef enum {
  pss=0,
  pbch=1,
  si=2
} sync_mode_t;


PHY_VARS_NR_UE *init_nr_ue_vars(NR_DL_FRAME_PARMS *frame_parms,
                                uint8_t UE_id,
                                uint8_t abstraction_flag)

{
  PHY_VARS_NR_UE *ue;
  ue = (PHY_VARS_NR_UE *)malloc(sizeof(PHY_VARS_NR_UE));
  memset(ue,0,sizeof(PHY_VARS_NR_UE));
  memcpy(&(ue->frame_parms), frame_parms, sizeof(NR_DL_FRAME_PARMS));

  ue->Mod_id      = UE_id;
  ue->mac_enabled = 1;

  // initialize all signal buffers
  init_nr_ue_signal(ue,1,abstraction_flag);
  // intialize transport
  init_nr_ue_transport(ue,abstraction_flag);
  return(ue);
}

/*!
 * It performs band scanning and synchonization.
 * \param arg is a pointer to a \ref PHY_VARS_NR_UE structure.
 */

typedef struct syncData_s {
  UE_nr_rxtx_proc_t proc;
  PHY_VARS_NR_UE *UE;
} syncData_t;

static void UE_synch(void *arg) {
  syncData_t *syncD=(syncData_t *) arg;
  int i, hw_slot_offset;
  PHY_VARS_NR_UE *UE = syncD->UE;
  sync_mode_t sync_mode = pbch;
  int CC_id = UE->CC_id;
  int freq_offset=0;
  UE->is_synchronized = 0;


  if (UE->UE_scan == 0) {
    get_band(downlink_frequency[CC_id][0], &UE->frame_parms.eutra_band,   &uplink_frequency_offset[CC_id][0], &UE->frame_parms.frame_type);
    LOG_I( PHY, "[SCHED][UE] Check absolute frequency DL %"PRIu32", UL %"PRIu32" (oai_exit %d, rx_num_channels %d)\n",
           downlink_frequency[0][0], downlink_frequency[0][0]+uplink_frequency_offset[0][0],
           oai_exit, openair0_cfg[0].rx_num_channels);

    for (i=0; i<openair0_cfg[UE->rf_map.card].rx_num_channels; i++) {
      openair0_cfg[UE->rf_map.card].rx_freq[UE->rf_map.chain+i] = downlink_frequency[CC_id][i];
      openair0_cfg[UE->rf_map.card].tx_freq[UE->rf_map.chain+i] =
        downlink_frequency[CC_id][i]+uplink_frequency_offset[CC_id][i];
      openair0_cfg[UE->rf_map.card].autocal[UE->rf_map.chain+i] = 1;

      if (uplink_frequency_offset[CC_id][i] != 0) //
        openair0_cfg[UE->rf_map.card].duplex_mode = duplex_mode_FDD;
      else //FDD
        openair0_cfg[UE->rf_map.card].duplex_mode = duplex_mode_TDD;
    }

    sync_mode = pbch;
  } else {
    LOG_E(PHY,"Fixme!\n");
    /*
    for (i=0; i<openair0_cfg[UE->rf_map.card].rx_num_channels; i++) {
      downlink_frequency[UE->rf_map.card][UE->rf_map.chain+i] = bands_to_scan.band_info[CC_id].dl_min;
      uplink_frequency_offset[UE->rf_map.card][UE->rf_map.chain+i] =
        bands_to_scan.band_info[CC_id].ul_min-bands_to_scan.band_info[CC_id].dl_min;
      openair0_cfg[UE->rf_map.card].rx_freq[UE->rf_map.chain+i] = downlink_frequency[CC_id][i];
      openair0_cfg[UE->rf_map.card].tx_freq[UE->rf_map.chain+i] =
        downlink_frequency[CC_id][i]+uplink_frequency_offset[CC_id][i];
      openair0_cfg[UE->rf_map.card].rx_gain[UE->rf_map.chain+i] = UE->rx_total_gain_dB;
    }
    */
  }

  LOG_W(PHY, "Starting sync detection\n");

  switch (sync_mode) {
    /*
    case pss:
      LOG_I(PHY,"[SCHED][UE] Scanning band %d (%d), freq %u\n",bands_to_scan.band_info[current_band].band, current_band,bands_to_scan.band_info[current_band].dl_min+current_offset);
      //lte_sync_timefreq(UE,current_band,bands_to_scan.band_info[current_band].dl_min+current_offset);
      current_offset += 20000000; // increase by 20 MHz

      if (current_offset > bands_to_scan.band_info[current_band].dl_max-bands_to_scan.band_info[current_band].dl_min) {
        current_band++;
        current_offset=0;
      }

      if (current_band==bands_to_scan.nbands) {
        current_band=0;
        oai_exit=1;
      }

      for (i=0; i<openair0_cfg[UE->rf_map.card].rx_num_channels; i++) {
        downlink_frequency[UE->rf_map.card][UE->rf_map.chain+i] = bands_to_scan.band_info[current_band].dl_min+current_offset;
        uplink_frequency_offset[UE->rf_map.card][UE->rf_map.chain+i] = bands_to_scan.band_info[current_band].ul_min-bands_to_scan.band_info[0].dl_min + current_offset;
        openair0_cfg[UE->rf_map.card].rx_freq[UE->rf_map.chain+i] = downlink_frequency[CC_id][i];
        openair0_cfg[UE->rf_map.card].tx_freq[UE->rf_map.chain+i] = downlink_frequency[CC_id][i]+uplink_frequency_offset[CC_id][i];
        openair0_cfg[UE->rf_map.card].rx_gain[UE->rf_map.chain+i] = UE->rx_total_gain_dB;

        if (UE->UE_scan_carrier) {
          openair0_cfg[UE->rf_map.card].autocal[UE->rf_map.chain+i] = 1;
        }
      }

      break;
    */
    case pbch:
      LOG_I(PHY, "[UE thread Synch] Running Initial Synch (mode %d)\n",UE->mode);

      if (nr_initial_sync( &syncD->proc, UE, UE->mode,2) == 0) {
        freq_offset = UE->common_vars.freq_offset; // frequency offset computed with pss in initial sync
        hw_slot_offset = (UE->rx_offset<<1) / UE->frame_parms.samples_per_slot;
        LOG_I(PHY,"Got synch: hw_slot_offset %d, carrier off %d Hz, rxgain %d (DL %u, UL %u), UE_scan_carrier %d\n",
              hw_slot_offset,
              freq_offset,
              UE->rx_total_gain_dB,
              downlink_frequency[0][0]+freq_offset,
              downlink_frequency[0][0]+uplink_frequency_offset[0][0]+freq_offset,
              UE->UE_scan_carrier );

        // rerun with new cell parameters and frequency-offset
        for (i=0; i<openair0_cfg[UE->rf_map.card].rx_num_channels; i++) {
          openair0_cfg[UE->rf_map.card].rx_gain[UE->rf_map.chain+i] = UE->rx_total_gain_dB;//-USRP_GAIN_OFFSET;

          if (freq_offset >= 0)
            openair0_cfg[UE->rf_map.card].rx_freq[UE->rf_map.chain+i] += abs(freq_offset);
          else
            openair0_cfg[UE->rf_map.card].rx_freq[UE->rf_map.chain+i] -= abs(freq_offset);

          openair0_cfg[UE->rf_map.card].tx_freq[UE->rf_map.chain+i] =
            openair0_cfg[UE->rf_map.card].rx_freq[UE->rf_map.chain+i]+uplink_frequency_offset[CC_id][i];
          downlink_frequency[CC_id][i] = openair0_cfg[CC_id].rx_freq[i];
        }

        // reconfigure for potentially different bandwidth
        switch(UE->frame_parms.N_RB_DL) {
          case 6:
            openair0_cfg[UE->rf_map.card].sample_rate =1.92e6;
            openair0_cfg[UE->rf_map.card].rx_bw          =.96e6;
            openair0_cfg[UE->rf_map.card].tx_bw          =.96e6;
            //            openair0_cfg[0].rx_gain[0] -= 12;
            break;

          case 25:
            openair0_cfg[UE->rf_map.card].sample_rate =7.68e6;
            openair0_cfg[UE->rf_map.card].rx_bw          =2.5e6;
            openair0_cfg[UE->rf_map.card].tx_bw          =2.5e6;
            //            openair0_cfg[0].rx_gain[0] -= 6;
            break;

          case 50:
            openair0_cfg[UE->rf_map.card].sample_rate =15.36e6;
            openair0_cfg[UE->rf_map.card].rx_bw          =5.0e6;
            openair0_cfg[UE->rf_map.card].tx_bw          =5.0e6;
            //            openair0_cfg[0].rx_gain[0] -= 3;
            break;

          case 100:
            openair0_cfg[UE->rf_map.card].sample_rate=30.72e6;
            openair0_cfg[UE->rf_map.card].rx_bw=10.0e6;
            openair0_cfg[UE->rf_map.card].tx_bw=10.0e6;
            //            openair0_cfg[0].rx_gain[0] -= 0;
            break;
        }

        if (UE->mode != loop_through_memory) {
          UE->rfdevice.trx_set_freq_func(&UE->rfdevice,&openair0_cfg[0],0);
          //UE->rfdevice.trx_set_gains_func(&openair0,&openair0_cfg[0]);
          //UE->rfdevice.trx_stop_func(&UE->rfdevice);
          // sleep(1);
          //nr_init_frame_parms_ue(&UE->frame_parms);
          /*if (UE->rfdevice.trx_start_func(&UE->rfdevice) != 0 ) {
            LOG_E(HW,"Could not start the device\n");
            oai_exit=1;
            }*/
        }

        if (UE->UE_scan_carrier == 1) {
          UE->UE_scan_carrier = 0;
        } else {
          UE->is_synchronized = 1;
        }
      } else {
        // initial sync failed
        // calculate new offset and try again
        if (UE->UE_scan_carrier == 1) {
          if (freq_offset >= 0)
            freq_offset += 100;

          freq_offset *= -1;
          LOG_I(PHY, "[initial_sync] trying carrier off %d Hz, rxgain %d (DL %u, UL %u)\n",
                freq_offset,
                UE->rx_total_gain_dB,
                downlink_frequency[0][0]+freq_offset,
                downlink_frequency[0][0]+uplink_frequency_offset[0][0]+freq_offset );

          for (i=0; i<openair0_cfg[UE->rf_map.card].rx_num_channels; i++) {
            openair0_cfg[UE->rf_map.card].rx_freq[UE->rf_map.chain+i] = downlink_frequency[CC_id][i]+freq_offset;
            openair0_cfg[UE->rf_map.card].tx_freq[UE->rf_map.chain+i] = downlink_frequency[CC_id][i]+uplink_frequency_offset[CC_id][i]+freq_offset;
            openair0_cfg[UE->rf_map.card].rx_gain[UE->rf_map.chain+i] = UE->rx_total_gain_dB;//-USRP_GAIN_OFFSET;

            if (UE->UE_scan_carrier==1)
              openair0_cfg[UE->rf_map.card].autocal[UE->rf_map.chain+i] = 1;
          }

          if (UE->mode != loop_through_memory)
            UE->rfdevice.trx_set_freq_func(&UE->rfdevice,&openair0_cfg[0],0);
        }// initial_sync=0

        break;

      case si:
      default:
        break;
      }
  }
}

void processSlotRX( PHY_VARS_NR_UE *UE, UE_nr_rxtx_proc_t *proc) {

  nr_dcireq_t dcireq;
  nr_scheduled_response_t scheduled_response;
  uint32_t nb_rb, start_rb;
  uint8_t nb_symb_sch, start_symbol, mcs, precod_nbr_layers, harq_pid, rvidx;
  uint16_t n_rnti;

  // Process Rx data for one sub-frame
  if (slot_select_nr(&UE->frame_parms, proc->frame_tx, proc->nr_tti_tx) & NR_DOWNLINK_SLOT) {
    //TODO: all of this has to be moved to the MAC!!!
    dcireq.module_id = UE->Mod_id;
    dcireq.gNB_index = 0;
    dcireq.cc_id     = 0;
    dcireq.frame     = proc->frame_rx;
    dcireq.slot      = proc->nr_tti_rx;
    nr_ue_dcireq(&dcireq); //to be replaced with function pointer later

    scheduled_response.dl_config = &dcireq.dl_config_req;
    scheduled_response.ul_config = &dcireq.ul_config_req;
    scheduled_response.tx_request = NULL;
    scheduled_response.module_id = UE->Mod_id;
    scheduled_response.CC_id     = 0;
    scheduled_response.frame = proc->frame_rx;
    scheduled_response.slot  = proc->nr_tti_rx;

    //--------------------------Temporary configuration-----------------------------//
    n_rnti = 0x1234;
    nb_rb = 50;
    start_rb = 0;
    nb_symb_sch = 12;
    start_symbol = 2;
    precod_nbr_layers = 1;
    mcs = 9;
    harq_pid = 0;
    rvidx = 0;
  //------------------------------------------------------------------------------//

    scheduled_response.ul_config->sfn_slot = NR_UPLINK_SLOT;
    scheduled_response.ul_config->number_pdus = 1;
    scheduled_response.ul_config->ul_config_list[0].pdu_type = FAPI_NR_UL_CONFIG_TYPE_PUSCH;
    scheduled_response.ul_config->ul_config_list[0].ulsch_config_pdu.rnti = n_rnti;
    scheduled_response.ul_config->ul_config_list[0].ulsch_config_pdu.ulsch_pdu_rel15.number_rbs = nb_rb;
    scheduled_response.ul_config->ul_config_list[0].ulsch_config_pdu.ulsch_pdu_rel15.start_rb = start_rb;
    scheduled_response.ul_config->ul_config_list[0].ulsch_config_pdu.ulsch_pdu_rel15.number_symbols = nb_symb_sch;
    scheduled_response.ul_config->ul_config_list[0].ulsch_config_pdu.ulsch_pdu_rel15.start_symbol = start_symbol;
    scheduled_response.ul_config->ul_config_list[0].ulsch_config_pdu.ulsch_pdu_rel15.mcs = mcs;
    scheduled_response.ul_config->ul_config_list[0].ulsch_config_pdu.ulsch_pdu_rel15.ndi = 0;
    scheduled_response.ul_config->ul_config_list[0].ulsch_config_pdu.ulsch_pdu_rel15.rv = rvidx;
    scheduled_response.ul_config->ul_config_list[0].ulsch_config_pdu.ulsch_pdu_rel15.n_layers = precod_nbr_layers;
    scheduled_response.ul_config->ul_config_list[0].ulsch_config_pdu.ulsch_pdu_rel15.harq_process_nbr = harq_pid;
    nr_ue_scheduled_response(&scheduled_response);

#ifdef UE_SLOT_PARALLELISATION
    phy_procedures_slot_parallelization_nrUE_RX( UE, proc, 0, 0, 1, UE->mode, no_relay, NULL );
#else
    uint64_t a=rdtsc();
    phy_procedures_nrUE_RX( UE, proc, 0, 1, UE->mode);
    LOG_D(PHY,"phy_procedures_nrUE_RX: slot:%d, time %lu\n", proc->nr_tti_rx, (rdtsc()-a)/3500);
    //printf(">>> nr_ue_pdcch_procedures ended\n");
#endif
  if(IS_SOFTMODEM_NOS1){ //&& proc->nr_tti_rx==1
	  //Hardcoded rnti value
	  protocol_ctxt_t ctxt;
	  PROTOCOL_CTXT_SET_BY_MODULE_ID(&ctxt, UE->Mod_id, ENB_FLAG_NO,
                                   0x1234, proc->frame_rx,
                                   proc->nr_tti_rx, 0);
	  pdcp_run(&ctxt);
          pdcp_fifo_flush_sdus(&ctxt);
  }
  }

  
  // no UL for now
  /*
  if (UE->mac_enabled==1) {
    //  trigger L2 to run ue_scheduler thru IF module
    //  [TODO] mapping right after NR initial sync
    if(UE->if_inst != NULL && UE->if_inst->ul_indication != NULL) {
      UE->ul_indication.module_id = 0;
      UE->ul_indication.gNB_index = 0;
      UE->ul_indication.cc_id = 0;
      UE->ul_indication.frame = proc->frame_rx;
      UE->ul_indication.slot = proc->nr_tti_rx;
      UE->if_inst->ul_indication(&UE->ul_indication);
    }
  }
  */
}

/*!
 * \brief This is the UE thread for RX subframe n and TX subframe n+4.
 * This thread performs the phy_procedures_UE_RX() on every received slot.
 * then, if TX is enabled it performs TX for n+4.
 * \param arg is a pointer to a \ref PHY_VARS_NR_UE structure.
 * \returns a pointer to an int. The storage is not on the heap and must not be freed.
 */

typedef struct processingData_s {
  UE_nr_rxtx_proc_t proc;
  PHY_VARS_NR_UE    *UE;
}  processingData_t;

void UE_processing(void *arg) {
  uint8_t thread_id;
  processingData_t *rxtxD=(processingData_t *) arg;
  UE_nr_rxtx_proc_t *proc = &rxtxD->proc;
  PHY_VARS_NR_UE    *UE   = rxtxD->UE;
  processSlotRX(UE, proc);
  //printf(">>> mac ended\n");
  // Prepare the future Tx data
/*
#ifndef NO_RAT_NR

  if (slot_select_nr(&UE->frame_parms, proc->frame_tx, proc->nr_tti_tx) & NR_UPLINK_SLOT)
#else
  if ((subframe_select( &UE->frame_parms, proc->subframe_tx) == SF_UL) ||
      (UE->frame_parms.frame_type == FDD) )
#endif
*/


  if (proc->nr_tti_tx == NR_UPLINK_SLOT || UE->frame_parms.frame_type == FDD){

    thread_id = PHY_vars_UE_g[UE->Mod_id][0]->current_thread_id[proc->nr_tti_tx];


    if (UE->mode != loop_through_memory)
      phy_procedures_nrUE_TX(UE,proc,0,thread_id);

  }

  //phy_procedures_UE_TX(UE,proc,0,0,UE->mode,no_relay);

#if 0

  if ((subframe_select( &UE->frame_parms, proc->subframe_tx) == SF_S) &&
      (UE->frame_parms.frame_type == TDD))
    if (UE->mode != loop_through_memory)
      //phy_procedures_UE_S_TX(UE,0,0,no_relay);
      updateTimes(current, &t3, 10000, timing_proc_name);

#endif
}

void readFrame(PHY_VARS_NR_UE *UE,  openair0_timestamp *timestamp) {
  void *rxp[NB_ANTENNAS_RX];
  void *dummy_tx[UE->frame_parms.nb_antennas_tx];

  for (int i=0; i<UE->frame_parms.nb_antennas_tx; i++)
    dummy_tx[i]=malloc16_clear(UE->frame_parms.samples_per_subframe*4);

  for(int x=0; x<20; x++) {  // two frames for initial sync
    for (int i=0; i<UE->frame_parms.nb_antennas_rx; i++)
      rxp[i] = ((void *)&UE->common_vars.rxdata[i][0]) + 4*x*UE->frame_parms.samples_per_subframe;

    AssertFatal( UE->frame_parms.samples_per_subframe ==
                 UE->rfdevice.trx_read_func(&UE->rfdevice,
                                            timestamp,
                                            rxp,
                                            UE->frame_parms.samples_per_subframe,
                                            UE->frame_parms.nb_antennas_rx), "");
  }

  for (int i=0; i<UE->frame_parms.nb_antennas_tx; i++)
    free(dummy_tx[i]);
}

void trashFrame(PHY_VARS_NR_UE *UE, openair0_timestamp *timestamp) {
  void *dummy_tx[UE->frame_parms.nb_antennas_tx];

  for (int i=0; i<UE->frame_parms.nb_antennas_tx; i++)
    dummy_tx[i]=malloc16_clear(UE->frame_parms.samples_per_subframe*4);

  void *dummy_rx[UE->frame_parms.nb_antennas_rx];

  for (int i=0; i<UE->frame_parms.nb_antennas_rx; i++)
    dummy_rx[i]=malloc16(UE->frame_parms.samples_per_subframe*4);

  for (int sf=0; sf<NR_NUMBER_OF_SUBFRAMES_PER_FRAME; sf++) {
    //      printf("Reading dummy sf %d\n",sf);
    UE->rfdevice.trx_read_func(&UE->rfdevice,
                               timestamp,
                               dummy_rx,
                               UE->frame_parms.samples_per_subframe,
                               UE->frame_parms.nb_antennas_rx);
    if (IS_SOFTMODEM_RFSIM ) {
	 usleep(1000); // slow down, as would do actuall rf to let cpu for the synchro thread
    }
  }

  for (int i=0; i<UE->frame_parms.nb_antennas_tx; i++)
    free(dummy_tx[i]);

  for (int i=0; i<UE->frame_parms.nb_antennas_rx; i++)
    free(dummy_rx[i]);
}

void syncInFrame(PHY_VARS_NR_UE *UE, openair0_timestamp *timestamp) {

    LOG_I(PHY,"Resynchronizing RX by %d samples (mode = %d)\n",UE->rx_offset,UE->mode);
    void *dummy_tx[UE->frame_parms.nb_antennas_tx];

    for (int i=0; i<UE->frame_parms.nb_antennas_tx; i++)
      dummy_tx[i]=malloc16_clear(UE->frame_parms.samples_per_subframe*4);

    for ( int size=UE->rx_offset ; size > 0 ; size -= UE->frame_parms.samples_per_subframe ) {
      int unitTransfer=size>UE->frame_parms.samples_per_subframe ? UE->frame_parms.samples_per_subframe : size ;
      AssertFatal(unitTransfer ==
                  UE->rfdevice.trx_read_func(&UE->rfdevice,
                                             timestamp,
                                             (void **)UE->common_vars.rxdata,
                                             unitTransfer,
                                             UE->frame_parms.nb_antennas_rx),"");
    }

    for (int i=0; i<UE->frame_parms.nb_antennas_tx; i++)
      free(dummy_tx[i]);

}

int computeSamplesShift(PHY_VARS_NR_UE *UE) {
  if ( getenv("RFSIMULATOR") != 0) {
    LOG_E(PHY,"SET rx_offset %d \n",UE->rx_offset);
    //UE->rx_offset_diff=0;
    return 0;
  }

  // compute TO compensation that should be applied for this frame
  if ( UE->rx_offset < UE->frame_parms.samples_per_frame/2  &&
       UE->rx_offset > 0 ) {
    //LOG_I(PHY,"!!!adjusting -1 samples!!!\n");
    return -1 ;
  }

  if ( UE->rx_offset > UE->frame_parms.samples_per_frame/2 &&
       UE->rx_offset < UE->frame_parms.samples_per_frame ) {
    //LOG_I(PHY,"!!!adjusting +1 samples!!!\n");
    return 1;
  }

  return 0;
}

void *UE_thread(void *arg) {
  //this thread should be over the processing thread to keep in real time
  PHY_VARS_NR_UE *UE = (PHY_VARS_NR_UE *) arg;
  //  int tx_enabled = 0;
  openair0_timestamp timestamp;
  void *rxp[NB_ANTENNAS_RX], *txp[NB_ANTENNAS_TX];
  int start_rx_stream = 0;
  const uint16_t table_sf_slot[20] = {0,0,1,1,2,2,3,3,4,4,5,5,6,6,7,7,8,8,9,9};
  AssertFatal(0== openair0_device_load(&(UE->rfdevice), &openair0_cfg[0]), "");
  UE->rfdevice.host_type = RAU_HOST;
  AssertFatal(UE->rfdevice.trx_start_func(&UE->rfdevice) == 0, "Could not start the device\n");
  notifiedFIFO_t nf;
  initNotifiedFIFO(&nf);
  int nbSlotProcessing=0;
  int thread_idx=0;
  notifiedFIFO_t freeBlocks;
  initNotifiedFIFO_nothreadSafe(&freeBlocks);

  for (int i=0; i<RX_NB_TH+1; i++)  // RX_NB_TH working + 1 we are making to be pushed
    pushNotifiedFIFO_nothreadSafe(&freeBlocks,
                                  newNotifiedFIFO_elt(sizeof(processingData_t), 0,&nf,UE_processing));

  bool syncRunning=false;
  const int nb_slot_frame = 10*UE->frame_parms.slots_per_subframe;
  int absolute_slot=0, decoded_frame_rx=INT_MAX, trashed_frames=0;

  while (!oai_exit) {
    if (syncRunning) {
      notifiedFIFO_elt_t *res=tryPullTpool(&nf, Tpool);

      if (res) {
        syncRunning=false;
        syncData_t *tmp=(syncData_t *)NotifiedFifoData(res);
        // shift the frame index with all the frames we trashed meanwhile we perform the synch search
        decoded_frame_rx=(tmp->proc.decoded_frame_rx+trashed_frames) % MAX_FRAME_NUMBER;
        delNotifiedFIFO_elt(res);
      } else {
        trashFrame(UE, &timestamp);
        trashed_frames++;
        continue;
      }
    }

    AssertFatal( !syncRunning, "At this point synchronisation can't be running\n");

    if (!UE->is_synchronized) {
      readFrame(UE, &timestamp);
      notifiedFIFO_elt_t *Msg=newNotifiedFIFO_elt(sizeof(syncData_t),0,&nf,UE_synch);
      syncData_t *syncMsg=(syncData_t *)NotifiedFifoData(Msg);
      syncMsg->UE=UE;
      memset(&syncMsg->proc, 0, sizeof(syncMsg->proc));
      pushTpool(Tpool, Msg);
      trashed_frames=0;
      syncRunning=true;
      continue;
    }

    if (start_rx_stream==0) {
      start_rx_stream=1;
      syncInFrame(UE, &timestamp);
      UE->rx_offset=0;
      UE->time_sync_cell=0;
      // read in first symbol
      AssertFatal (UE->frame_parms.ofdm_symbol_size+UE->frame_parms.nb_prefix_samples0 ==
                   UE->rfdevice.trx_read_func(&UE->rfdevice,
                                              &timestamp,
                                              (void **)UE->common_vars.rxdata,
                                              UE->frame_parms.ofdm_symbol_size+UE->frame_parms.nb_prefix_samples0,
                                              UE->frame_parms.nb_antennas_rx),"");
      // we have the decoded frame index in the return of the synch process
      // and we shifted above to the first slot of next frame
      decoded_frame_rx++;
      // we do ++ first in the regular processing, so it will be beging of frame;
      absolute_slot=decoded_frame_rx*nb_slot_frame + nb_slot_frame -1;
      continue;
    }


    absolute_slot++;
    // whatever means thread_idx
    // Fix me: will be wrong when slot 1 is slow, as slot 2 finishes
    // Slot 3 will overlap if RX_NB_TH is 2
    // this is general failure in UE !!!
    thread_idx = absolute_slot % RX_NB_TH;
    int slot_nr = absolute_slot % nb_slot_frame;
    notifiedFIFO_elt_t *msgToPush;
    AssertFatal((msgToPush=pullNotifiedFIFO_nothreadSafe(&freeBlocks)) != NULL,"chained list failure");
    processingData_t *curMsg=(processingData_t *)NotifiedFifoData(msgToPush);
    curMsg->UE=UE;
    // update thread index for received subframe
    curMsg->UE->current_thread_id[slot_nr] = thread_idx;
    curMsg->proc.CC_id = 0;
    curMsg->proc.nr_tti_rx= slot_nr;
    curMsg->proc.subframe_rx=table_sf_slot[slot_nr];
    curMsg->proc.nr_tti_tx = (absolute_slot + DURATION_RX_TO_TX) % nb_slot_frame;
    curMsg->proc.subframe_tx=curMsg->proc.nr_tti_rx;
    curMsg->proc.frame_rx = ( absolute_slot/nb_slot_frame ) % MAX_FRAME_NUMBER;
    curMsg->proc.frame_tx = ( (absolute_slot + DURATION_RX_TO_TX) /nb_slot_frame ) % MAX_FRAME_NUMBER;
    curMsg->proc.decoded_frame_rx=-1;
    //LOG_I(PHY,"Process slot %d thread Idx %d total gain %d\n", slot_nr, thread_idx, UE->rx_total_gain_dB);

#ifdef OAI_ADRV9371_ZC706
    /*uint32_t total_gain_dB_prev = 0;
    if (total_gain_dB_prev != UE->rx_total_gain_dB) {
		total_gain_dB_prev = UE->rx_total_gain_dB;
        openair0_cfg[0].rx_gain[0] = UE->rx_total_gain_dB;
        UE->rfdevice.trx_set_gains_func(&UE->rfdevice,&openair0_cfg[0]);
    }*/
#endif

    for (int i=0; i<UE->frame_parms.nb_antennas_rx; i++)
      rxp[i] = (void *)&UE->common_vars.rxdata[i][UE->frame_parms.ofdm_symbol_size+
               UE->frame_parms.nb_prefix_samples0+
               slot_nr*UE->frame_parms.samples_per_slot];

    for (int i=0; i<UE->frame_parms.nb_antennas_tx; i++)
      txp[i] = (void *)&UE->common_vars.txdata[i][((curMsg->proc.nr_tti_rx + DURATION_RX_TO_TX)%nb_slot_frame)*UE->frame_parms.samples_per_slot];

    int readBlockSize, writeBlockSize;

    if (slot_nr<(nb_slot_frame - 1)) {
      readBlockSize=UE->frame_parms.samples_per_slot;
      writeBlockSize=UE->frame_parms.samples_per_slot;
    } else {
      UE->rx_offset_diff = computeSamplesShift(UE);
      readBlockSize=UE->frame_parms.samples_per_slot -
                    UE->frame_parms.ofdm_symbol_size -
                    UE->frame_parms.nb_prefix_samples0 -
                    UE->rx_offset_diff;
      writeBlockSize=UE->frame_parms.samples_per_slot -
                     UE->rx_offset_diff;
    }

    AssertFatal(readBlockSize ==
                UE->rfdevice.trx_read_func(&UE->rfdevice,
                                           &timestamp,
                                           rxp,
                                           readBlockSize,
                                           UE->frame_parms.nb_antennas_rx),"");

if (slot_nr==18)
    AssertFatal( writeBlockSize ==
                 UE->rfdevice.trx_write_func(&UE->rfdevice,
                     timestamp+
                     (DURATION_RX_TO_TX*UE->frame_parms.samples_per_slot) -
                     UE->frame_parms.ofdm_symbol_size-UE->frame_parms.nb_prefix_samples0 -
                     openair0_cfg[0].tx_sample_advance,
                     txp,
                     writeBlockSize,
                     UE->frame_parms.nb_antennas_tx,
                     4),"");

    if( slot_nr==(nb_slot_frame-1)) {
      // read in first symbol of next frame and adjust for timing drift
      int first_symbols=writeBlockSize-readBlockSize;

      if ( first_symbols > 0 )
        AssertFatal(first_symbols ==
                    UE->rfdevice.trx_read_func(&UE->rfdevice,
                                               &timestamp,
                                               (void **)UE->common_vars.rxdata,
                                               first_symbols,
                                               UE->frame_parms.nb_antennas_rx),"");
      else
        LOG_E(PHY,"can't compensate: diff =%d\n", first_symbols);
    }

    curMsg->proc.timestamp_tx = timestamp+
                                (DURATION_RX_TO_TX*UE->frame_parms.samples_per_slot)-
                                UE->frame_parms.ofdm_symbol_size-UE->frame_parms.nb_prefix_samples0;
    notifiedFIFO_elt_t *res;

    while (nbSlotProcessing >= RX_NB_TH) {
      if ( (res=tryPullTpool(&nf, Tpool)) != NULL ) {
        nbSlotProcessing--;
        processingData_t *tmp=(processingData_t *)res->msgData;

        if (tmp->proc.decoded_frame_rx != -1)
          decoded_frame_rx=tmp->proc.decoded_frame_rx;

        pushNotifiedFIFO_nothreadSafe(&freeBlocks,res);
      }

      usleep(200);
    }

    if (  decoded_frame_rx != curMsg->proc.frame_rx &&
          ((decoded_frame_rx+1) % MAX_FRAME_NUMBER) != curMsg->proc.frame_rx )
      LOG_D(PHY,"Decoded frame index (%d) is not compatible with current context (%d), UE should go back to synch mode\n",
            decoded_frame_rx, curMsg->proc.frame_rx  );

    nbSlotProcessing++;
    msgToPush->key=slot_nr;
    pushTpool(Tpool, msgToPush);

    if (getenv("RFSIMULATOR") || IS_SOFTMODEM_NOS1) {  //getenv("RFSIMULATOR")
      // FixMe: Wait previous thread is done, because race conditions seems too bad
      // in case of actual RF board, the overlap between threads mitigate the issue
      // We must receive one message, that proves the slot processing is done
      res=pullTpool(&nf, Tpool);
      nbSlotProcessing--;
      processingData_t *tmp=(processingData_t *)res->msgData;

      if (tmp->proc.decoded_frame_rx != -1)
        decoded_frame_rx=tmp->proc.decoded_frame_rx;

      pushNotifiedFIFO_nothreadSafe(&freeBlocks,res);
    }
  } // while !oai_exit

  return NULL;
}

void init_NR_UE(int nb_inst) {
  int inst;
  NR_UE_MAC_INST_t *mac_inst;
  pthread_t threads[nb_inst];

  for (inst=0; inst < nb_inst; inst++) {
    PHY_VARS_NR_UE *UE = PHY_vars_UE_g[inst][0];
    AssertFatal((UE->if_inst = nr_ue_if_module_init(inst)) != NULL, "can not initial IF module\n");
    nr_l3_init_ue();
    nr_l2_init_ue();
    mac_inst = get_mac_inst(inst);
    mac_inst->if_module = UE->if_inst;
    // Initial bandwidth part configuration -- full carrier bandwidth
    mac_inst->initial_bwp_dl.bwp_id = 0;
    mac_inst->initial_bwp_dl.location = 0;
    mac_inst->initial_bwp_dl.scs = UE->frame_parms.subcarrier_spacing;
    mac_inst->initial_bwp_dl.N_RB = UE->frame_parms.N_RB_DL;
    mac_inst->initial_bwp_dl.cyclic_prefix = UE->frame_parms.Ncp;
    mac_inst->initial_bwp_ul.bwp_id = 0;
    mac_inst->initial_bwp_ul.location = 0;
    mac_inst->initial_bwp_ul.scs = UE->frame_parms.subcarrier_spacing;
    mac_inst->initial_bwp_ul.N_RB = UE->frame_parms.N_RB_UL;
    mac_inst->initial_bwp_ul.cyclic_prefix = UE->frame_parms.Ncp;
    LOG_I(PHY,"Intializing UE Threads for instance %d (%p,%p)...\n",inst,PHY_vars_UE_g[inst],PHY_vars_UE_g[inst][0]);
    threadCreate(&threads[inst], UE_thread, (void *)UE, "UEthread", -1, OAI_PRIORITY_RT_MAX);

#ifdef UE_DLSCH_PARALLELISATION
    pthread_t dlsch0_threads;
    threadCreate(&dlsch0_threads, dlsch_thread, (void *)UE, "DLthread", -1, OAI_PRIORITY_RT_MAX-1);
#endif

  }

  printf("UE threads created by %ld\n", gettid());
}

