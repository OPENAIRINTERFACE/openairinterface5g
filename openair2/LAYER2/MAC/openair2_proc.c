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

/* \file openair2_proc.c
 \brief MAC layer online statistics
 \author Navid Nikaein
 \date 2013 - 2014
 \version 1.0
 @ingroup _mac
*/

#ifndef USER_MODE
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/proc_fs.h>

#endif
#include "LAYER2/MAC/defs.h"
#include "LAYER2/MAC/extern.h"
//#include "RRC/LITE/extern.h"
//#include "LAYER2/PDCP/pdcp.h"
#include "proto.h"
#ifndef USER_MODE
static int openair2_stats_read(char *buffer, char **my_buffer, off_t off, int length)
#else
int openair2_stats_read(char *buffer, char **my_buffer, off_t off, int length)
#endif
{

  int len = 0,fg,Overhead, Sign;
  unsigned int i,j,k,kk;
  unsigned int ue_id, eNB_id;
  unsigned int Mod_id = 0,CH_index;
  unsigned int stat_tx_pdcp_sdu;
  unsigned int stat_tx_pdcp_bytes;
  unsigned int stat_tx_pdcp_sdu_discarded;
  unsigned int stat_tx_pdcp_bytes_discarded;
  unsigned int stat_tx_data_pdu;
  unsigned int stat_tx_data_bytes;
  unsigned int stat_tx_retransmit_pdu_by_status;
  unsigned int stat_tx_retransmit_bytes_by_status;
  unsigned int stat_tx_retransmit_pdu;
  unsigned int stat_tx_retransmit_bytes;
  unsigned int stat_tx_control_pdu;
  unsigned int stat_tx_control_bytes;
  unsigned int stat_rx_pdcp_sdu;
  unsigned int stat_rx_pdcp_bytes;
  unsigned int stat_rx_data_pdus_duplicate;
  unsigned int stat_rx_data_bytes_duplicate;
  unsigned int stat_rx_data_pdu;
  unsigned int stat_rx_data_bytes;
  unsigned int stat_rx_data_pdu_dropped;
  unsigned int stat_rx_data_bytes_dropped;
  unsigned int stat_rx_data_pdu_out_of_window;
  unsigned int stat_rx_data_bytes_out_of_window;
  unsigned int stat_rx_control_pdu;
  unsigned int stat_rx_control_bytes;
  unsigned int stat_timer_reordering_timed_out;
  unsigned int stat_timer_poll_retransmit_timed_out;
  unsigned int stat_timer_status_prohibit_timed_out;

  // UE part
  for (ue_id=0; ue_id<NUM_UE_INST; ue_id++) {
    // mod_id used for PDCP and RLC
    Mod_id = NB_eNB_INST + ue_id ;

    len+=sprintf(&buffer[len],"UE RX TTI: %d\n",UE_mac_inst[ue_id].rxFrame);

    for (enb_id= 0; enb_id <NB_SIG_CNX_UE; enb_id++) {

      switch (mac_get_rrc_status(ue_id,0,enb_id) > RRC_CONNECTED) {
      case RRC_RECONFIGURED :
      case RRC_CONNECTED:
      case RRC_SI_RECEIVED:
      case RRC_IDLE:
        break;

        if (mac_get_rrc_status(ue_id,0,enb_id) > RRC_CONNECTED) {
          //  if (UE_mac_inst[ue_id].Dcch_lchan[CH_index].Active==1) {
          len+=sprintf(&buffer[len],"eNB %d: Wideband SINR %d dB---\n",
                       CH_index,UE_mac_inst[Mod_id].Def_meas[CH_index].Wideband_sinr);
          len+=sprintf(&buffer[len],"CH %d: Subband SINR (dB) :",
                       CH_index);

          for (fg=0; fg<NUMBER_OF_MEASUREMENT_SUBBANDS; fg++) {
            len+=sprintf(&buffer[len],"%d ",UE_mac_inst[Mod_id].Def_meas[CH_index].Sinr_meas[0][fg]);
          }

          len+=sprintf(&buffer[len],"\n");


          len+=sprintf(&buffer[len],"BCCH %d, NB_RX_MAC = %d (%d errors)\n",
                       UE_mac_inst[Mod_id].Bcch_lchan[CH_index].Lchan_info.Lchan_id.Index,
                       UE_mac_inst[Mod_id].Bcch_lchan[CH_index].Lchan_info.NB_RX,
                       UE_mac_inst[Mod_id].Bcch_lchan[CH_index].Lchan_info.NB_RX_ERRORS);



          len+=sprintf(&buffer[len],"CCCH %d, NB_RX_MAC = %d (%d errors)\n",
                       UE_mac_inst[Mod_id].Ccch_lchan[CH_index].Lchan_info.Lchan_id.Index,
                       UE_mac_inst[Mod_id].Ccch_lchan[CH_index].Lchan_info.NB_RX,
                       UE_mac_inst[Mod_id].Ccch_lchan[CH_index].Lchan_info.NB_RX_ERRORS);


          len+=sprintf(&buffer[len],"LCHAN %d (DCCH), NB_TX_MAC = %d (%d bits/TTI, %d kbits/sec), NB_RX_MAC = %d (%d errors)\n",
                       UE_mac_inst[Mod_id].Dcch_lchan[CH_index].Lchan_info.Lchan_id.Index,
                       UE_mac_inst[Mod_id].Dcch_lchan[CH_index].Lchan_info.NB_TX,
                       UE_mac_inst[Mod_id].Dcch_lchan[CH_index].Lchan_info.output_rate,
                       (10*UE_mac_inst[Mod_id].Dcch_lchan[CH_index].Lchan_info.output_rate)>>5,
                       UE_mac_inst[Mod_id].Dcch_lchan[CH_index].Lchan_info.NB_RX,
                       UE_mac_inst[Mod_id].Dcch_lchan[CH_index].Lchan_info.NB_RX_ERRORS);

          for(i=1; i<NB_RAB_MAX; i++) {
            if (UE_mac_inst[Mod_id].Dtch_lchan[i][CH_index].Active==1) {
              Overhead=UE_mac_inst[Mod_id].Dtch_lchan[i][CH_index].Lchan_info.output_rate - Pdcp_stats_tx_rate[k][CH_index][i];

              if(Overhead<0) {
                Overhead=-Overhead;
                Sign=-1;
              } else {
                Sign=1;
              }

              len+=sprintf(&buffer[len],"[PDCP]LCHAN %d: NB_TX = %d ,Tx_rate =(%d bits/TTI ,%d Kbits/s), NB_RX = %d ,Rx_rate =(%d bits/TTI ,%d Kbits/s) , LAYER2 TX OVERHEAD: %d Kbits/s\n",
                           UE_mac_inst[Mod_id].Dtch_lchan[i][CH_index].Lchan_info.Lchan_id.Index,
                           Pdcp_stats_tx[k][CH_index][i],
                           Pdcp_stats_tx_rate[k][CH_index][i],
                           (10*Pdcp_stats_tx_rate[k][CH_index][i])>>5,
                           Pdcp_stats_rx[k][CH_index][i],
                           Pdcp_stats_rx_rate[k][CH_index][i],
                           (10*Pdcp_stats_rx_rate[k][CH_index][i])>>5,
                           Sign*(10*Overhead)>>5);


              int status =  rlc_stat_req     (k,
                                              UE_mac_inst[Mod_id].Dtch_lchan[i][CH_index].Lchan_info.Lchan_id.Index,
                                              &stat_tx_pdcp_sdu,
                                              &stat_tx_pdcp_bytes,
                                              &stat_tx_pdcp_sdu_discarded,
                                              &stat_tx_pdcp_bytes_discarded,
                                              &stat_tx_data_pdu,
                                              &stat_tx_data_bytes,
                                              &stat_tx_retransmit_pdu_by_status,
                                              &stat_tx_retransmit_bytes_by_status,
                                              &stat_tx_retransmit_pdu,
                                              &stat_tx_retransmit_bytes,
                                              &stat_tx_control_pdu,
                                              &stat_tx_control_bytes,
                                              &stat_rx_pdcp_sdu,
                                              &stat_rx_pdcp_bytes,
                                              &stat_rx_data_pdus_duplicate,
                                              &stat_rx_data_bytes_duplicate,
                                              &stat_rx_data_pdu,
                                              &stat_rx_data_bytes,
                                              &stat_rx_data_pdu_dropped,
                                              &stat_rx_data_bytes_dropped,
                                              &stat_rx_data_pdu_out_of_window,
                                              &stat_rx_data_bytes_out_of_window,
                                              &stat_rx_control_pdu,
                                              &stat_rx_control_bytes,
                                              &stat_timer_reordering_timed_out,
                                              &stat_timer_poll_retransmit_timed_out,
                                              &stat_timer_status_prohibit_timed_out) ;

              if (status == RLC_OP_STATUS_OK) {
                len+=sprintf(&buffer[len],"RLC LCHAN %d, NB_SDU_TO_TX = %d\tNB_SDU_DISC %d\tNB_RX_SDU %d\n",
                             UE_mac_inst[Mod_id].Dtch_lchan[i][CH_index].Lchan_info.Lchan_id.Index,
                             tx_pdcp_sdu,
                             tx_pdcp_sdu_discarded,
                             rx_sdu);
                len+=sprintf(&buffer[len],"RLC LCHAN %d, NB_TB_TX_DATA = %d\tNB_TB_TX_CONTROL %d\tNB_TX_TB_RETRANS %d",
                             UE_mac_inst[Mod_id].Dtch_lchan[i][CH_index].Lchan_info.Lchan_id.Index,
                             tx_data_pdu,
                             tx_control_pdu,
                             tx_retransmit_pdu);
                len+=sprintf(&buffer[len],"\tRLC LCHAN %d, NB_TX_TB_RETRANS_BY_STATUS = %d\tNB_TX_TB_RETRANS_PADD %d\n",
                             UE_mac_inst[Mod_id].Dtch_lchan[i][CH_index].Lchan_info.Lchan_id.Index,
                             tx_retransmit_pdu_by_status,
                             tx_retransmit_pdu_unblock);
                len+=sprintf(&buffer[len],"RLC LCHAN %d, NB_RX_DATA = %d\tNB_RX_TB_OUT_WIN %d\tNB_RX_TB_CORRUPT %d\n",
                             UE_mac_inst[Mod_id].Dtch_lchan[i][CH_index].Lchan_info.Lchan_id.Index,
                             rx_data_pdu,
                             rx_data_pdu_out_of_window,
                             rx_error_pdu);
              }

              len+=sprintf(&buffer[len],"[MAC]: LCHAN %d, NB_TX_MAC = %d (%d bits/TTI, %d kbits/s), NB_RX_MAC = %d (%d errors)\n",
                           UE_mac_inst[Mod_id].Dtch_lchan[i][CH_index].Lchan_info.Lchan_id.Index,
                           UE_mac_inst[Mod_id].Dtch_lchan[i][CH_index].Lchan_info.NB_TX,
                           UE_mac_inst[Mod_id].Dtch_lchan[i][CH_index].Lchan_info.output_rate,
                           (10*UE_mac_inst[Mod_id].Dtch_lchan[i][CH_index].Lchan_info.output_rate)>>5,
                           UE_mac_inst[Mod_id].Dtch_lchan[i][CH_index].Lchan_info.NB_RX,
                           UE_mac_inst[Mod_id].Dtch_lchan[i][CH_index].Lchan_info.NB_RX_ERRORS);
              len+=sprintf(&buffer[len],"        TX per TB: ");

              for(kk=0; kk<MAX_NUMBER_TB_PER_LCHAN/2; kk++) {
                len+=sprintf(&buffer[len],"%d . ",UE_mac_inst[Mod_id].Dtch_lchan[i][CH_index].Lchan_info.NB_TX_TB[kk]);
              }

              len+=sprintf(&buffer[len],"\n");
              len+=sprintf(&buffer[len],"        RXerr per TB: ");

              for(kk=0; kk<MAX_NUMBER_TB_PER_LCHAN/2; kk++)
                len+=sprintf(&buffer[len],"%d/%d . ",UE_mac_inst[Mod_id].Dtch_lchan[i][CH_index].Lchan_info.NB_RX_ERRORS_TB[kk],
                             UE_mac_inst[Mod_id].Dtch_lchan[i][CH_index].Lchan_info.NB_RX_TB[kk]);

              len+=sprintf(&buffer[len],"\n");



            }

          }
        }
      }

#endif //PHY_EMUL_ONE_MACHINE
    }

    else if(Mac_rlc_xface->Is_cluster_head[k] ==1) {

      Mod_id=k;
      len+=sprintf(&buffer[len],
                   "-------------------------------------------------------------------CH %d: TTI: %d------------------------------------------------------------------\n",
                   NODE_ID[Mod_id],Mac_rlc_xface->frame);

      for(i=1; i<=NB_CNX_CH; i++) {
        if (CH_mac_inst[Mod_id].Dcch_lchan[i].Active==1) {
          len+=sprintf(&buffer[len],"\nMR index %d: DL SINR (feedback) %d dB, CQI: %s\n\n",
                       i,//CH_rrc_inst[Mod_id].Info.UE_list[i].L2_id[0],
                       CH_mac_inst[Mod_id].Def_meas[i].Wideband_sinr);
          //print_cqi(CH_mac_inst[Mod_id].Def_meas[i].cqi));

          len+=sprintf(&buffer[len],
                       "[MAC] LCHAN %d (DCCH), NB_TX_MAC= %d (%d bits/TTI, %d kbits/s), NB_RX_MAC= %d (errors %d, sacch errors %d, sach errors %d, sach_missing %d)\n\n",
                       CH_mac_inst[Mod_id].Dcch_lchan[i].Lchan_info.Lchan_id.Index,
                       CH_mac_inst[Mod_id].Dcch_lchan[i].Lchan_info.NB_TX,
                       CH_mac_inst[Mod_id].Dcch_lchan[i].Lchan_info.output_rate,
                       (10*CH_mac_inst[Mod_id].Dcch_lchan[i].Lchan_info.output_rate)>>5,
                       CH_mac_inst[Mod_id].Dcch_lchan[i].Lchan_info.NB_RX,
                       CH_mac_inst[Mod_id].Dcch_lchan[i].Lchan_info.NB_RX_ERRORS,
                       CH_mac_inst[Mod_id].Dcch_lchan[i].Lchan_info.NB_RX_SACCH_ERRORS,
                       CH_mac_inst[Mod_id].Dcch_lchan[i].Lchan_info.NB_RX_SACH_ERRORS,
                       CH_mac_inst[Mod_id].Dcch_lchan[i].Lchan_info.NB_RX_SACH_MISSING);

          for(j=0; j<NB_RAB_MAX; j++) {
            if (CH_mac_inst[Mod_id].Dtch_lchan[j][i].Active==1) {
              Overhead=CH_mac_inst[Mod_id].Dtch_lchan[j][i].Lchan_info.output_rate - Pdcp_stats_tx_rate[k][i][j];

              if(Overhead<0) {
                Overhead=-Overhead;
                Sign=-1;
              } else {
                Sign=1;
              }

              len+=sprintf(&buffer[len],
                           "[PDCP]LCHAN %d: NB_TX = %d ,Tx_rate =(%d bits/TTI ,%d Kbits/s), NB_RX = %d ,Rx_rate =(%d bits/TTI ,%d Kbits/s), LAYER2 TX OVERHEAD= %d Kbits/s\n",
                           CH_mac_inst[Mod_id].Dtch_lchan[j][i].Lchan_info.Lchan_id.Index,
                           Pdcp_stats_tx[k][i][j],
                           Pdcp_stats_tx_rate[k][i][j],
                           (10*Pdcp_stats_tx_rate[k][i][j])>>5,
                           Pdcp_stats_rx[k][i][j],
                           Pdcp_stats_rx_rate[k][i][j],
                           (10*Pdcp_stats_rx_rate[k][i][j])>>5,
                           Sign*(10*Overhead)>>5);
              int status =  rlc_stat_req     (k,
                                              CH_mac_inst[Mod_id].Dtch_lchan[j][i].Lchan_info.Lchan_id.Index,
                                              &tx_pdcp_sdu,
                                              &tx_pdcp_sdu_discarded,
                                              &tx_retransmit_pdu_unblock,
                                              &tx_retransmit_pdu_by_status,
                                              &tx_retransmit_pdu,
                                              &tx_data_pdu,
                                              &tx_control_pdu,
                                              &rx_sdu,
                                              &rx_error_pdu,
                                              &rx_data_pdu,
                                              &rx_data_pdu_out_of_window,
                                              &rx_control_pdu) ;
              /*
              if (status == RLC_OP_STATUS_OK) {
                len+=sprintf(&buffer[len],"\t[RLC] LCHAN %d, NB_SDU_TO_TX = %d\tNB_SDU_DISC %d\tNB_RX_SDU %d\n",
                      CH_mac_inst[Mod_id].Dtch_lchan[j][i].Lchan_info.Lchan_id.Index,
                   tx_pdcp_sdu,
                   tx_pdcp_sdu_discarded,
                   rx_sdu);
                len+=sprintf(&buffer[len],"\t[RLC] LCHAN %d, NB_TB_TX_DATA = %d\tNB_TB_TX_CONTROL %d\tNB_TX_TB_RETRANS %\n",
                      CH_mac_inst[Mod_id].Dtch_lchan[j][i].Lchan_info.Lchan_id.Index,
                   tx_data_pdu,
                   tx_control_pdu,
                   tx_retransmit_pdu);
                len+=sprintf(&buffer[len],"\t[RLC] LCHAN %d, NB_TX_TB_RETRANS_BY_STATUS = %d\tNB_TX_TB_RETRANS_PADD %d\n",
                      CH_mac_inst[Mod_id].Dtch_lchan[j][i].Lchan_info.Lchan_id.Index,
                   tx_retransmit_pdu_by_status,
                   tx_retransmit_pdu_unblock);
                len+=sprintf(&buffer[len],"\t[RLC] LCHAN %d, NB_RX_DATA = %d\tNB_RX_TB_OUT_WIN %d\tNB_RX_TB_CORRUPT %d\n",
                      CH_mac_inst[Mod_id].Dtch_lchan[j][i].Lchan_info.Lchan_id.Index,
                   rx_data_pdu,
                   rx_data_pdu_out_of_window,
                   rx_error_pdu);
              }
              */
              len+=sprintf(&buffer[len],
                           "[MAC]LCHAN %d (CNX %d,RAB %d), NB_TX_MAC= %d (%d bits/TTI, %d kbit/s), NB_RX_MAC= %d (errors %d, sacch_errors %d, sach_errors %d, sach_missing %d)\n",
                           CH_mac_inst[Mod_id].Dtch_lchan[j][i].Lchan_info.Lchan_id.Index,
                           i,j,
                           CH_mac_inst[Mod_id].Dtch_lchan[j][i].Lchan_info.NB_TX,
                           CH_mac_inst[Mod_id].Dtch_lchan[j][i].Lchan_info.output_rate,
                           (10*CH_mac_inst[Mod_id].Dtch_lchan[j][i].Lchan_info.output_rate)>>5,
                           CH_mac_inst[Mod_id].Dtch_lchan[j][i].Lchan_info.NB_RX,
                           CH_mac_inst[Mod_id].Dtch_lchan[j][i].Lchan_info.NB_RX_ERRORS,
                           CH_mac_inst[Mod_id].Dtch_lchan[j][i].Lchan_info.NB_RX_SACCH_ERRORS,
                           CH_mac_inst[Mod_id].Dtch_lchan[j][i].Lchan_info.NB_RX_SACH_ERRORS,
                           CH_mac_inst[Mod_id].Dtch_lchan[j][i].Lchan_info.NB_RX_SACH_MISSING);
              len+=sprintf(&buffer[len],"[MAC][SCHEDULER] TX Arrival Rate %d, TX Service Rate %d, RX Arrival rate %d, RX Service rate %d, NB_BW_REQ_RX %d\n\n",
                           CH_mac_inst[Mod_id].Dtch_lchan[j][i].Lchan_info.Arrival_rate,
                           CH_mac_inst[Mod_id].Dtch_lchan[j][i].Lchan_info.Tx_rate,
                           CH_mac_inst[Mod_id].Dtch_lchan[j][i].Lchan_info.Req_rate,
                           CH_mac_inst[Mod_id].Dtch_lchan[j][i].Lchan_info.Rx_rate,
                           CH_mac_inst[Mod_id].Dtch_lchan[j][i].Lchan_info.NB_BW_REQ_RX);

              /*
                      len+=sprintf(&buffer[len],"        TX per TB: ");
                      for(kk=0;kk<MAX_NUMBER_TB_PER_LCHAN/2;kk++)
                  len+=sprintf(&buffer[len],"%d.",CH_mac_inst[Mod_id].Dtch_lchan[j][i].Lchan_info.NB_TX_TB[kk]);
                      len+=sprintf(&buffer[len],"\n");
                      len+=sprintf(&buffer[len],"        RXerr per TB: ");
                      for(kk=0;kk<MAX_NUMBER_TB_PER_LCHAN/2;kk++)
                  len+=sprintf(&buffer[len],"%d/%d . ",CH_mac_inst[Mod_id].Dtch_lchan[j][i].Lchan_info.NB_RX_ERRORS_TB[kk],
                         CH_mac_inst[Mod_id].Dtch_lchan[j][i].Lchan_info.NB_RX_TB[kk]);
                      len+=sprintf(&buffer[len],"\n");
              */
            }
          }
        }
      }

    }
  }

  return len;
}

#ifndef USER_MODE
static struct proc_dir_entry *proc_openair2_root;
/*
 * Initialize the module and add the /proc file.
 */
int add_openair2_stats()
{
  //#ifdef  KERNEL_VERSION_GREATER_THAN_2629
  struct proc_dir_entry *pde;
  //#endif

  proc_openair2_root = proc_mkdir("openair2",0);
  //#ifdef  KERNEL_VERSION_GREATER_THAN_2629
  // pde = proc_create_entry("lchan_stats", S_IFREG | S_IRUGO, proc_openair2_root);
  pde = create_proc_read_entry("lchan_stats", S_IFREG | S_IRUGO, proc_openair2_root, (read_proc_t*)&openair2_stats_read, NULL);

  if (!pde) {
    printk("[OPENAIR][ERROR] can't create proc entry !\n");
  }

  //#else
  //create_proc_info_entry("lchan_stats", S_IFREG | S_IRUGO, proc_openair2_root, openair2_stats_read);
  //#endif

  return 0;
}
/*
 * Unregister the file when the module is closed.
 */
void remove_openair2_stats()
{

  if (proc_openair2_root) {
    printk("[OPENAIR][CLEANUP] Removing openair proc entry\n");
    remove_proc_entry("lchan_stats", proc_openair2_root);
    //#ifdef  KERNEL_VERSION_GREATER_THAN_2629

    //#else
    remove_proc_entry("openair2",NULL);
    //#endif;
  }
}
#endif
