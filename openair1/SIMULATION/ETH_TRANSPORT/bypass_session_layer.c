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

/*! \file bypass_session_layer.h
 *  \brief implementation of emultor tx and rx
 *  \author Navid Nikaein and Raymond Knopp
 *  \date 2011 - 2014
 *  \version 1.0
 *  \company Eurecom
 *  \email: navid.nikaein@eurecom.fr
 */

#include "PHY/defs.h"
#include "defs.h"
#include "proto.h"
#include "extern.h"

#include "assertions.h"
#include "UTIL/OCG/OCG.h"
#include "UTIL/OCG/OCG_extern.h"
#include "UTIL/LOG/log.h"

#include "multicast_link.h"

char rx_bufferP[BYPASS_RX_BUFFER_SIZE];
unsigned int num_bytesP = 0;
int      N_P = 0, N_R = 0;
char         bypass_tx_buffer[BYPASS_TX_BUFFER_SIZE];
static unsigned int byte_tx_count;
unsigned int Master_list_rx;
static uint64_t seq_num_tx = 0;

mapping transport_names[] = {
  {"WAIT PM TRANSPORT INFO", EMU_TRANSPORT_INFO_WAIT_PM},
  {"WAIT SM TRANSPORT INFO", EMU_TRANSPORT_INFO_WAIT_SM},
  {"SYNC TRANSPORT INFO", EMU_TRANSPORT_INFO_SYNC},
  {"ENB_TRANSPORT INFO", EMU_TRANSPORT_INFO_ENB},
  {"UE TRANSPORT INFO", EMU_TRANSPORT_INFO_UE},
  {"RELEASE TRANSPORT INFO", EMU_TRANSPORT_INFO_RELEASE},
  {NULL, -1}
};

void init_bypass (void)
{
  LOG_I(EMU, "[PHYSIM] INIT BYPASS\n");

#if !defined(ENABLE_NEW_MULTICAST)
  pthread_mutex_init (&Tx_mutex, NULL);
  pthread_cond_init (&Tx_cond, NULL);
  Tx_mutex_var = 1;
  pthread_mutex_init (&emul_low_mutex, NULL);
  pthread_cond_init (&emul_low_cond, NULL);
  emul_low_mutex_var = 1;
#endif
  bypass_init (emul_tx_handler, emul_rx_handler);
}

/***************************************************************************/
void bypass_init (tx_handler_t tx_handlerP, rx_handler_t rx_handlerP)
{
  /***************************************************************************/
  multicast_link_start (bypass_rx_handler, oai_emulation.info.multicast_group,
                        oai_emulation.info.multicast_ifname);
  tx_handler = tx_handlerP;
  rx_handler = rx_handlerP;
  Master_list_rx=0;
  emu_tx_status = WAIT_SYNC_TRANSPORT;
  emu_rx_status = WAIT_SYNC_TRANSPORT;
}

int emu_transport_handle_sync(bypass_msg_header_t *messg)
{
  int m_id;

  DevAssert(messg != NULL);

  // determite the total number of remote enb & ue
  oai_emulation.info.nb_enb_remote += messg->nb_enb;
  oai_emulation.info.nb_ue_remote += messg->nb_ue;

  // determine the index of local enb and ue wrt the remote ones
  if (messg->master_id < oai_emulation.info.master_id) {
    oai_emulation.info.first_enb_local +=messg->nb_enb;
    oai_emulation.info.first_ue_local +=messg->nb_ue;
  }

  // store param for enb per master
  if ((oai_emulation.info.master[messg->master_id].nb_enb = messg->nb_enb) > 0) {
    for (m_id=0; m_id < messg->master_id; m_id++ ) {
      oai_emulation.info.master[messg->master_id].first_enb+=oai_emulation.info.master[m_id].nb_enb;
    }

    LOG_I(EMU,
          "[ENB] WAIT_SYNC_TRANSPORT state:  for master %d (first enb %d, totan enb %d)\n",
          messg->master_id,
          oai_emulation.info.master[messg->master_id].first_enb,
          oai_emulation.info.master[messg->master_id].nb_enb);
  }

  // store param for ue per master
  if ((oai_emulation.info.master[messg->master_id].nb_ue  = messg->nb_ue) > 0) {
    for (m_id=0; m_id < messg->master_id; m_id++ ) {
      oai_emulation.info.master[messg->master_id].first_ue+=oai_emulation.info.master[m_id].nb_ue;
    }

    LOG_I(EMU,
          "[UE] WAIT_SYNC_TRANSPORT state: for master %d (first ue %d, total ue%d)\n",
          messg->master_id,
          oai_emulation.info.master[messg->master_id].first_ue,
          oai_emulation.info.master[messg->master_id].nb_ue );
  }

  Master_list_rx=((Master_list_rx) |(1<< messg->master_id));

  if (Master_list_rx == oai_emulation.info.master_list) {
    emu_rx_status = SYNCED_TRANSPORT;
  }

  LOG_I(EMU,
        "WAIT_SYNC_TRANSPORT state: m_id %d total enb remote %d total ue remote %d \n",
        messg->master_id,oai_emulation.info.nb_enb_remote,
        oai_emulation.info.nb_ue_remote );
  return 0;
}

int emu_transport_handle_wait_sm(bypass_msg_header_t *messg)
{
  DevAssert(messg != NULL);
  Master_list_rx = ((Master_list_rx) | (1 << messg->master_id));

  return 0;
}

int emu_transport_handle_wait_pm(bypass_msg_header_t *messg)
{
  DevAssert(messg != NULL);

  if (messg->master_id == 0) {
    Master_list_rx = ((Master_list_rx) | (1 << messg->master_id));
  }

  return 0;
}

static
int emu_transport_handle_enb_info(bypass_msg_header_t *messg,
                                  unsigned int next_slot,
                                  int bytes_read)
{
  eNB_transport_info_t *eNB_info;
  int total_header = 0, total_tbs = 0;
  int n_dci, n_enb, enb_info_ix = 0,CC_id;

  DevAssert(bytes_read >= 0);
  DevAssert(messg != NULL);

#ifdef DEBUG_EMU
  LOG_D(EMU," RX ENB_TRANSPORT INFO from master %d \n",messg->master_id);
#endif
  clear_eNB_transport_info(oai_emulation.info.nb_enb_local+
                           oai_emulation.info.nb_enb_remote);

  if (oai_emulation.info.master[messg->master_id].nb_enb > 0 ) {
    total_header += sizeof(eNB_transport_info_t)-MAX_TRANSPORT_BLOCKS_BUFFER_SIZE;

    eNB_info = (eNB_transport_info_t *) (&rx_bufferP[bytes_read]);

    for (n_enb = oai_emulation.info.master[messg->master_id].first_enb;
         n_enb < oai_emulation.info.master[messg->master_id].first_enb+
         oai_emulation.info.master[messg->master_id].nb_enb;
         n_enb ++) {
      for (n_dci = 0; n_dci < (eNB_info[enb_info_ix].num_ue_spec_dci + eNB_info[enb_info_ix].num_common_dci); n_dci ++) {
        total_tbs += eNB_info[enb_info_ix].tbs[n_dci];
      }

      enb_info_ix++;

      if ((total_tbs + total_header) > MAX_TRANSPORT_BLOCKS_BUFFER_SIZE ) {
        LOG_W(EMU,"RX eNB Transport buffer total size %d (header%d,tbs %d) \n",
              total_header+total_tbs, total_header,total_tbs);
      }

      memcpy(&eNB_transport_info[n_enb], eNB_info, total_header + total_tbs);

      /* Go to the next eNB info */
      eNB_info = (eNB_transport_info_t *)((uintptr_t)eNB_info + total_header+
                                          total_tbs);
      bytes_read += (total_header + total_tbs);
    }

    for (n_enb = oai_emulation.info.master[messg->master_id].first_enb;
         n_enb < oai_emulation.info.master[messg->master_id].first_enb +
         oai_emulation.info.master[messg->master_id].nb_enb; n_enb ++)
      for (CC_id=0; CC_id<MAX_NUM_CCs; CC_id++) {
        fill_phy_enb_vars(n_enb, CC_id,next_slot);
      }
  } else {
    LOG_T(EMU,"WAIT_ENB_TRANSPORT state: no enb transport info from master %d \n",
          messg->master_id);
  }

  Master_list_rx=((Master_list_rx) |(1<< messg->master_id));

  if (Master_list_rx == oai_emulation.info.master_list) {
    emu_rx_status = SYNCED_TRANSPORT;
  }

  return 0;
}

static
int emu_transport_handle_ue_info(bypass_msg_header_t *messg,
                                 unsigned int last_slot,
                                 int bytes_read)
{
  UE_transport_info_t *UE_info;
  int n_ue, n_enb,CC_id=0;
  int total_tbs = 0, total_header = 0, ue_info_ix =0;

  DevAssert(bytes_read >= 0);
  DevAssert(messg != NULL);

#ifdef DEBUG_EMU
  LOG_D(EMU," RX UE TRANSPORT INFO from master %d\n",messg->master_id);
#endif
  clear_UE_transport_info(oai_emulation.info.nb_ue_local+
                          oai_emulation.info.nb_ue_remote);

  if (oai_emulation.info.master[messg->master_id].nb_ue > 0 ) {
    total_header += sizeof(UE_transport_info_t)-MAX_TRANSPORT_BLOCKS_BUFFER_SIZE;

    UE_info = (UE_transport_info_t *) (&rx_bufferP[bytes_read]);

    // get the total size of the transport blocks
    for (n_ue = oai_emulation.info.master[messg->master_id].first_ue;
         n_ue < oai_emulation.info.master[messg->master_id].first_ue +
         oai_emulation.info.master[messg->master_id].nb_ue; n_ue ++) {
      total_tbs = 0;

      for (n_enb = 0; n_enb < UE_info[ue_info_ix].num_eNB; n_enb ++) {
        total_tbs += UE_info[ue_info_ix].tbs[n_enb];
      }

      ue_info_ix++;

      if (total_tbs + total_header > MAX_TRANSPORT_BLOCKS_BUFFER_SIZE ) {
        LOG_W(EMU,"RX [UE %d] Total size of buffer is %d (header%d,tbs %d) \n",
              n_ue, total_header+total_tbs,total_header,total_tbs);
      }

//#warning "CC id should be adjusted, set to zero for the moment"
      memcpy(&UE_transport_info[n_ue][CC_id], UE_info, total_header + total_tbs);

      /* Go to the next UE info */
      UE_info = (UE_transport_info_t *)((uintptr_t)UE_info + total_header+
                                        total_tbs);
      bytes_read += (total_header + total_tbs);
    }

    for (n_ue = oai_emulation.info.master[messg->master_id].first_ue;
         n_ue < oai_emulation.info.master[messg->master_id].first_ue +
         oai_emulation.info.master[messg->master_id].nb_ue;
         n_ue ++)
      for (CC_id=0; CC_id<MAX_NUM_CCs; CC_id++) {
        fill_phy_ue_vars(n_ue, CC_id,last_slot);
      }
  } else {
    LOG_T(EMU,"WAIT_UE_TRANSPORT state: no UE transport info from master %d\n",
          messg->master_id);
  }

  Master_list_rx=((Master_list_rx) |(1<< messg->master_id));

  if (Master_list_rx == oai_emulation.info.master_list) {
    emu_rx_status = SYNCED_TRANSPORT;
  }

  return 0;
}

int bypass_rx_data(unsigned int frame, unsigned int last_slot,
                   unsigned int next_slot, uint8_t is_master)
{
  bypass_msg_header_t *messg;
  bypass_proto2multicast_header_t *bypass_read_header;
  int bytes_read = 0;

  LOG_D(EMU, "Entering bypass_rx for frame %d next_slot %d is_master %u\n",
        frame, next_slot, is_master);

#if defined(ENABLE_NEW_MULTICAST)

  if (multicast_link_read_data_from_sock(is_master) == 1) {
    /* We got a timeout */
    return -1;
  }

#else
  pthread_mutex_lock(&emul_low_mutex);

  if(emul_low_mutex_var) {
    pthread_cond_wait(&emul_low_cond, &emul_low_mutex);
  }

  if(num_bytesP==0) {
    pthread_mutex_unlock(&emul_low_mutex);
  } else {
#endif
  bypass_read_header = (bypass_proto2multicast_header_t *) (
                         &rx_bufferP[bytes_read]);
  bytes_read += sizeof (bypass_proto2multicast_header_t);

  if (num_bytesP != bytes_read + bypass_read_header->size) {
    LOG_W(EMU, "WARNINIG BYTES2READ # DELIVERED BYTES!!! (%d != %d)\n",
          num_bytesP, bytes_read + bypass_read_header->size);
    exit(EXIT_FAILURE);
  } else {
    messg = (bypass_msg_header_t *) (&rx_bufferP[bytes_read]);
    bytes_read += sizeof (bypass_msg_header_t);
#if defined(ENABLE_NEW_MULTICAST)
    LOG_D(EMU, "Received %d bytes [%s] from master_id %d with seq %"PRIuMAX"\n",
          num_bytesP, map_int_to_str(transport_names, messg->Message_type),
          messg->master_id,
          messg->seq_num);
      DevCheck4((messg->frame == frame) && (messg->subframe == (next_slot>>1)),
                messg->frame, frame, messg->subframe, next_slot>>1);

#else

    if ((messg->frame != frame) || (messg->subframe != next_slot>>1))
      LOG_W(EMU,
            "Received %s from master %d for (frame %d,subframe %d) "
            "currently (frame %d,subframe %d)\n",
            map_int_to_str(transport_names,messg->Message_type),
            messg->master_id,
            messg->frame, messg->subframe,
            frame, next_slot>>1);

#endif

    //chek if MASTER in my List
    switch(messg->Message_type) {
    case EMU_TRANSPORT_INFO_WAIT_PM:
      emu_transport_handle_wait_pm(messg);
      break;

    case EMU_TRANSPORT_INFO_WAIT_SM:
      emu_transport_handle_wait_sm(messg);
      break;

    case EMU_TRANSPORT_INFO_SYNC:
      emu_transport_handle_sync(messg);
      break;

    case EMU_TRANSPORT_INFO_ENB:
      emu_transport_handle_enb_info(messg, next_slot, bytes_read);
      break;

    case EMU_TRANSPORT_INFO_UE:
      emu_transport_handle_ue_info(messg, last_slot, bytes_read);
      break;

    case EMU_TRANSPORT_INFO_RELEASE :
      Master_list_rx = oai_emulation.info.master_list;
      LOG_E(EMU, "RX EMU_TRANSPORT_INFO_RELEASE\n");
      break;

    default:
      LOG_E(EMU, "[MAC][BYPASS] ERROR RX UNKNOWN MESSAGE\n");
      //mac_xface->macphy_exit("");
      break;
    }
  }

  num_bytesP=0;

#if !defined(ENABLE_NEW_MULTICAST)
  emul_low_mutex_var=1;

  pthread_cond_signal(&emul_low_cond);
  pthread_mutex_unlock(&emul_low_mutex);
#endif
  bypass_signal_mac_phy(frame, last_slot, next_slot, is_master);
#if !defined(ENABLE_NEW_MULTICAST)
}

#endif

return bytes_read;
}

/******************************************************************************************************/
void bypass_rx_handler(unsigned int Num_bytes,char *Rx_buffer)
{
  /******************************************************************************************************/
  if(Num_bytes >0) {
#if !defined(ENABLE_NEW_MULTICAST)
    pthread_mutex_lock(&emul_low_mutex);

    while(!emul_low_mutex_var) {
      pthread_cond_wait(&emul_low_cond, &emul_low_mutex);
    }

#endif
    num_bytesP=Num_bytes;
    memcpy(rx_bufferP, Rx_buffer, Num_bytes);
#if !defined(ENABLE_NEW_MULTICAST)
    emul_low_mutex_var=0;

    /* on ne peut que signaler depuis un context linux
     * (rtf_handler); pas de wait, jamais!!!!!!
     */
    pthread_cond_signal(&emul_low_cond);
    pthread_mutex_unlock(&emul_low_mutex);
#endif
  }
}

/******************************************************************************************************/
void  bypass_signal_mac_phy(unsigned int frame, unsigned int last_slot,
                            unsigned int next_slot, uint8_t is_master)
{
  /******************************************************************************************************/
  if (Master_list_rx != oai_emulation.info.master_list) {
    bypass_rx_data(frame, last_slot, next_slot, is_master);
  } else {
    Master_list_rx = 0;
  }
}

/***************************************************************************/
void bypass_tx_data(emu_transport_info_t Type, unsigned int frame, unsigned int next_slot)
{
  /***************************************************************************/
  unsigned int         num_flows;
  bypass_msg_header_t *messg;

  LOG_D(EMU, "Entering bypass_tx [%s] for frame %d next_slot %d\n",
        map_int_to_str(transport_names, Type), frame, next_slot);

  int n_enb,n_ue, CC_id,n_dci,total_tbs=0,total_size=0;
  messg = (bypass_msg_header_t *) (
            &bypass_tx_buffer[sizeof (bypass_proto2multicast_header_t)]);
  num_flows = 0;
  messg->master_id       = oai_emulation.info.master_id; //Master_id;

  messg->nb_enb          = oai_emulation.info.nb_enb_local; //Master_id;
  messg->nb_ue           = oai_emulation.info.nb_ue_local; //Master_id;
  messg->nb_flow         = num_flows;
  messg->frame           = frame;
  messg->subframe        = next_slot>>1;
  messg->seq_num         = seq_num_tx;
  messg->failing_master_id = 0;

  seq_num_tx++;

  byte_tx_count = sizeof (bypass_msg_header_t) + sizeof (
                    bypass_proto2multicast_header_t);

    if (Type == WAIT_PM_TRANSPORT) {
      messg->Message_type = EMU_TRANSPORT_INFO_WAIT_PM;
      LOG_T(EMU,"[TX_DATA] WAIT SYNC PM TRANSPORT\n");
    } else if (Type == WAIT_SM_TRANSPORT) {
      messg->Message_type = EMU_TRANSPORT_INFO_WAIT_SM;
      LOG_T(EMU,"[TX_DATA] WAIT SYNC SM TRANSPORT\n");
    } else if (Type == SYNC_TRANSPORT) {
      messg->Message_type = EMU_TRANSPORT_INFO_SYNC;
      /* make sure that sync messages from the masters are received in
       * increasing order of master id
       */
      sleep(oai_emulation.info.master_id+1);
      LOG_T(EMU,"[TX_DATA] SYNC TRANSPORT\n");
    } else if(Type==ENB_TRANSPORT) {

      messg->Message_type = EMU_TRANSPORT_INFO_ENB;
      total_size=0;
      total_tbs=0;

      for (n_enb=oai_emulation.info.first_enb_local;
           n_enb<(oai_emulation.info.first_enb_local+oai_emulation.info.nb_enb_local);
           n_enb++)
        for (CC_id=0; CC_id<MAX_NUM_CCs; CC_id++) {
          total_tbs=0;
          LOG_D(EMU,"[TX_DATA] Frame %d subframe %d CC id %d : ENB TRANSPORT: num dci %d \n",
                frame, next_slot>>1, CC_id,
                eNB_transport_info[n_enb][CC_id].num_pmch +
                eNB_transport_info[n_enb][CC_id].num_ue_spec_dci +
                eNB_transport_info[n_enb][CC_id].num_common_dci   );

          for (n_dci = 0; n_dci < (eNB_transport_info[n_enb][CC_id].num_pmch +
                                   eNB_transport_info[n_enb][CC_id].num_ue_spec_dci +
                                   eNB_transport_info[n_enb][CC_id].num_common_dci);
               n_dci++) {
            total_tbs +=eNB_transport_info[n_enb][CC_id].tbs[n_dci];
          }

          if (total_tbs <= MAX_TRANSPORT_BLOCKS_BUFFER_SIZE) {
            total_size = sizeof(eNB_transport_info_t) + total_tbs -
                         MAX_TRANSPORT_BLOCKS_BUFFER_SIZE;
          } else {
            LOG_E(EMU,
                  "[eNB]running out of memory for the eNB emulation transport buffer of size %d\n",
                  MAX_TRANSPORT_BLOCKS_BUFFER_SIZE);
          }

          memcpy(&bypass_tx_buffer[byte_tx_count], (char *)&eNB_transport_info[n_enb][CC_id],
                 total_size);
          byte_tx_count += total_size;
        }
    } else if (Type == UE_TRANSPORT) {
      LOG_D(EMU,"[TX_DATA] UE TRANSPORT\n");
      messg->Message_type = EMU_TRANSPORT_INFO_UE;
      total_size=0;
      total_tbs=0; // compute the actual size of transport_blocks

      for (n_ue = oai_emulation.info.first_ue_local;
           n_ue < (oai_emulation.info.first_ue_local+oai_emulation.info.nb_ue_local);
           n_ue++) {
        for (CC_id=0; CC_id<MAX_NUM_CCs; CC_id++) {
          for (n_enb=0; n_enb<UE_transport_info[n_ue][CC_id].num_eNB; n_enb++) {
            total_tbs+=UE_transport_info[n_ue][CC_id].tbs[n_enb];
          }

          if (total_tbs <= MAX_TRANSPORT_BLOCKS_BUFFER_SIZE) {
            total_size = sizeof(UE_transport_info_t)+total_tbs-
                         MAX_TRANSPORT_BLOCKS_BUFFER_SIZE;
          } else {
            LOG_E(EMU,
                  "[UE]running out of memory for the UE emulation transport buffer of size %d\n",
                  MAX_TRANSPORT_BLOCKS_BUFFER_SIZE);
          }

#ifdef DEBUG_EMU
          LOG_D(EMU,"[UE %d][eNB %d][CC %d] total size %d\n",
                n_ue, n_enb, CC_id,total_size);
#endif
          memcpy(&bypass_tx_buffer[byte_tx_count], (char *)&UE_transport_info[n_ue][CC_id],
                 total_size);
          byte_tx_count += total_size;
        }
      }
    } else if (Type == RELEASE_TRANSPORT) {
      messg->Message_type = EMU_TRANSPORT_INFO_RELEASE;
    } else {
      LOG_E(EMU,"[TX_DATA] UNKNOWN MSG  \n");
    }

  ((bypass_proto2multicast_header_t *) bypass_tx_buffer)->size = byte_tx_count -
      sizeof (bypass_proto2multicast_header_t);

  multicast_link_write_sock(oai_emulation.info.multicast_group,
                            bypass_tx_buffer, byte_tx_count);

  LOG_D(EMU, "Frame %d, subframe %d (%d): Sent %d bytes [%s] with master_id %d and seq %"PRIuMAX"\n",
        frame, next_slot>>1, next_slot,byte_tx_count, map_int_to_str(transport_names, Type),
        messg->master_id, messg->seq_num);
}

