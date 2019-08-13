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

#define PDCP_C
#include <string.h>
#include <math.h>
#include <unistd.h>
#include "SIMULATION/TOOLS/defs.h"
#include "SIMULATION/RF/defs.h"
#include "PHY/types.h"
#include "PHY/defs.h"
#include "PHY/vars.h"
#include "MAC_INTERFACE/vars.h"

#include "LAYER2/MAC/defs.h"
#include "LAYER2/MAC/vars.h"
#include "UTIL/LOG/log_if.h"
#include "UTIL/LOG/log_extern.h"
#include "RRC/LTE/vars.h"
#include "PHY_INTERFACE/vars.h"
#include "UTIL/OCG/OCG.h"
#include "LAYER2/PDCP_v10.1.0/pdcp.h"
#include "OCG_vars.h"
#include "rlc.h"
#include "pdcp.h"


#define TEST_MAX_SEND_SDU 8192
#define TARGET_MAX_RX_ERROR_RATE 10
#define TARGET_MAX_TX_ERROR_RATE 10
static int  g_error_on_phy = 0;
static int  g_random_sdu;
static int  g_random_nb_frames;
static int  g_random_tx_pdu_size;
static int  g_random_rx_pdu_size;
static int  g_target_tx_error_rate;
static int  g_target_rx_error_rate;
static int  g_tx_packets = 0;
static int  g_dropped_tx_packets = 0;
static int  g_rx_packets = 0;
static int  g_dropped_rx_packets = 0;
static int  g_drop_rx = 0;
static int  g_drop_tx = 0;
static int  g_send_sdu_ids[TEST_MAX_SEND_SDU][2];
static int  g_send_id_write_index[2];
static int  g_send_id_read_index[2];
static s8_t *g_sdus[] = {"En dépit de son volontarisme affiché, le premier ministre est de plus en plus décrié pour son incompétence. La tension politique et dans l'opinion publique est encore montée d'un cran au Japon, sur fond d'inquiétantes nouvelles, avec du plutonium détecté dans le sol autour de la centrale de Fukushima. Le premier ministre Naoto Kan a solennellement déclaré que son gouvernement était «en état d'alerte maximum». Tout en reconnaissant que la situation restait «imprévisible». Ce volontarisme affiché par le premier ministre - que Nicolas Sarkozy rencontrera demain lors d'une visite au Japon - ne l'a pas empêché d'être la cible de violentes critiques de la part de parlementaires sur sa gestion de la crise. Attaqué sur le manque de transparence, il a assuré qu'il rendait publiques toutes les informations en sa possession. Un député de l'opposition, Yosuke Isozaki, a aussi reproché à Naoto Kan de ne pas avoir ordonné l'évacuation des populations dans la zone comprise entre 20 et 30 km autour de la centrale. «Peut-il y avoir quelque chose de plus irresponsable que cela ?», a-t-il lancé. Pour l'heure, la zone d'évacuation est limitée à un rayon de 20 km, seul le confinement étant recommandé pour les 10 km suivants. Sur ce sujet, les autorités japonaises ont été fragilisées mardi par les déclarations de Greenpeace, affirmant que ses experts avaient détecté une radioactivité dangereuse à 40 km de la centrale. L'organisation écologiste a appelé à une extension de la zone d'évacuation, exhortant Tokyo à «cesser de privilégier la politique aux dépens de la science». L'Agence japonaise de sûreté nucléaire a balayé ces critiques.",

                         "La pâquerette (Bellis perennis) est une plante vivace des prés, des pelouses, des bords de chemins et des prairies, haute de dix à vingt centimètres, de la famille des Astéracées, dont les fleurs naissent sur des inflorescences appelées capitules : celles du pourtour, que l'on croit à tort être des pétales, appelées fleurs ligulées, parce qu'elles ont la forme d'une languette, ou demi-fleurons, sont des fleurs femelles, dont la couleur varie du blanc au rose plus ou moins prononcé ; celles du centre, jaunes, appelées fleurs tubuleuses, parce que leur corolle forme un tube, ou fleurons, sont hermaphrodites. Ainsi, contrairement à l'opinion populaire, ce qu'on appelle une « fleur » de pâquerette n'est en réalité pas « une » fleur mais un capitule portant des fleurs très nombreuses.Leurs fruits s'envolent grâce au vent et dégagent des odeurs qui attirent les insectes.Une variété muricole peut pousser sur des murs humides verticaux.Les pâquerettes sont des fleurs rustiques et très communes en Europe, sur les gazons, les prairies, les chemins et les zones d'herbe rase.Elles ont la particularité, comme certaines autres fleurs de plantes herbacées, de se fermer la nuit et de s'ouvrir le matin pour s'épanouir au soleil ; elles peuvent aussi se fermer pendant les averses, voire un peu avant, ce qui permet dans les campagnes de prédire la pluie légèrement à l'avance.",

                         "La pâquerette",
                         " (Bellis perennis)",
                         " est une plante vivace des prés,",
                         " des pelouses,",
                         " des bords de chemins et des prairies,",
                         "haute de dix à",
                         " vingt centimètres",
                         ", de la",
                         " famille des",
                         " Astéracées",
                         ", dont",
                         " les",
                         " fleurs",
                         " naissent",
                         " sur",
                         " des",
                         " inflorescences",
                         " appelées",
                         " capitules",
                         " : celles",
                         " du pourtour",
                         ", que l'on",
                         " croit à",
                         " tort",
                         " être",
                         " des pétales",
                         ", appelées",
                         " fleurs ligulées",
                         ", parce qu'elles",
                         " ont la forme d'une languette, ou demi-fleurons, sont",
                         " des fleurs femelles,",
                         " dont la couleur varie du blanc au rose plus ou moins prononcé ; celles du centre, jaunes,",
                         " appelées ",
                         "fleurs tubuleuses",
                         ", parce que leur corolle forme un tube, ou fleurons, sont hermaphrodites."
                        };



int pdcp_fifo_flush_sdus () {}
int pdcp_fifo_read_input_sdus_remaining_bytes () {}
int pdcp_fifo_read_input_sdus () {}

BOOL init_pdcp_entity(pdcp_t *pdcp_entity);

//-----------------------------------------------------------------------------
void pdcp_rlc_test_mac_rlc_loop (struct mac_data_ind *data_indP,  struct mac_data_req *data_requestP, int* drop_countP, int *tx_packetsP, int* dropped_tx_packetsP) //-----------------------------------------------------------------------------
{
  mem_block_t* tb_src;
  mem_block_t* tb_dst;
  unsigned int tb_size;

  data_indP->no_tb = 0;

  while (data_requestP->data.nb_elements > 0) {
    tb_src = list_remove_head (&data_requestP->data);

    if (tb_src != NULL) {
      tb_size = ((struct mac_tb_req *) (tb_src->data))->tb_size_in_bits >> 3;
      printf("[RLC-LOOP] FOUND TB SIZE IN BITS %d IN BYTES %u sizeof (mac_rlc_max_rx_header_size_t) %d\n",
             ((struct mac_tb_req *) (tb_src->data))->tb_size_in_bits,
             tb_size, sizeof (mac_rlc_max_rx_header_size_t));

      *tx_packetsP = *tx_packetsP + 1;

      if (*drop_countP == 0) {
        tb_dst  = get_free_mem_block(sizeof (mac_rlc_max_rx_header_size_t) + tb_size, __func__);

        if (tb_dst != NULL) {
          ((struct mac_tb_ind *) (tb_dst->data))->first_bit        = 0;
          ((struct mac_tb_ind *) (tb_dst->data))->data_ptr         = &tb_dst->data[sizeof (mac_rlc_max_rx_header_size_t)];
          ((struct mac_tb_ind *) (tb_dst->data))->size             = tb_size;
          ((struct mac_tb_ind *) (tb_dst->data))->error_indication = 0;

          memcpy(((struct mac_tb_ind *) (tb_dst->data))->data_ptr,
                 &((struct mac_tb_req *) (tb_src->data))->data_ptr[0],
                 tb_size);

          list_add_tail_eurecom(tb_dst, &data_indP->data);
          data_indP->no_tb  += 1;
        } else {
          printf("Out of memory error\n");
//          exit(-1);
        }
      } else {
        printf("[RLC-LOOP] DROPPING 1 TB\n");
        *drop_countP = *drop_countP - 1;
        *dropped_tx_packetsP = *dropped_tx_packetsP + 1;
      }

      free_mem_block(tb_src, __func__);

      if (data_indP->no_tb > 0) {
        printf("[RLC-LOOP] Exchange %d TBs\n",data_indP->no_tb);
      }
    }
  }
}
//-----------------------------------------------------------------------------
void pdcp_rlc_test_exchange_pdus(rlc_um_entity_t *um_txP,
                                 rlc_um_entity_t *um_rxP,
                                 u16_t           bytes_txP,
                                 u16_t           bytes_rxP)
//-----------------------------------------------------------------------------
{
  struct mac_data_req    data_request_tx;
  struct mac_data_req    data_request_rx;
  struct mac_data_ind    data_ind_tx;
  struct mac_data_ind    data_ind_rx;
  struct mac_status_ind  tx_status;
  struct mac_status_resp mac_rlc_status_resp_tx;
  struct mac_status_resp mac_rlc_status_resp_rx;


  memset(&data_request_tx, 0, sizeof(struct mac_data_req));
  memset(&data_request_rx, 0, sizeof(struct mac_data_req));
  memset(&data_ind_tx,     0, sizeof(struct mac_data_ind));
  memset(&data_ind_rx,     0, sizeof(struct mac_data_ind));
  memset(&tx_status,       0, sizeof(struct mac_status_ind));
  memset(&mac_rlc_status_resp_tx, 0, sizeof(struct mac_status_resp));
  memset(&mac_rlc_status_resp_rx, 0, sizeof(struct mac_status_resp));

  mac_rlc_status_resp_tx = rlc_um_mac_status_indication(um_txP, bytes_txP, tx_status, ENB_FLAG_YES);
  data_request_tx        = rlc_um_mac_data_request(um_txP);
  mac_rlc_status_resp_rx = rlc_um_mac_status_indication(um_rxP, bytes_rxP, tx_status, ENB_FLAG_YES);
  data_request_rx        = rlc_um_mac_data_request(um_rxP);


  pdcp_rlc_test_mac_rlc_loop(&data_ind_rx, &data_request_tx, &g_drop_tx, &g_tx_packets, &g_dropped_tx_packets);
  pdcp_rlc_test_mac_rlc_loop(&data_ind_tx, &data_request_rx, &g_drop_rx, &g_rx_packets, &g_dropped_rx_packets);
  rlc_um_mac_data_indication(um_rxP, data_ind_rx);
  rlc_um_mac_data_indication(um_txP, data_ind_tx);
  mac_xface->frame += 1;

  //rlc_um_tx_buffer_display(um_txP,NULL);
  //assert(um_txP->t_status_prohibit.time_out != 1);
  //assert(um_rxP->t_status_prohibit.time_out != 1);
  //assert(!((um_txP->vt_a == 954) && (um_txP->vt_us == 53)));
  //assert(mac_xface->frame <= 151);
  //check_mem_area(NULL);
  //display_mem_load();
}
//-----------------------------------------------------------------------------
void pdcp_rlc_test_send_sdu(module_id_t module_idP, rb_id_t rab_idP, int sdu_indexP)
//-----------------------------------------------------------------------------
{
  pdcp_data_req(module_idP, rab_idP, strlen(g_sdus[sdu_indexP]) + 1, g_sdus[sdu_indexP]);
}
//-----------------------------------------------------------------------------
int main(int argc, char **argv)
//-----------------------------------------------------------------------------
{
  unsigned long   index = 0;
  rlc_op_status_t rlc_op_status;
  rlc_info_t      rlc_info;
  char * g_log_level = "trace"; // by default global log level is set to trace

  mac_xface = malloc(sizeof(MAC_xface));
  Mac_rlc_xface = (MAC_RLC_XFACE*)malloc16(sizeof(MAC_RLC_XFACE));

  rlc_module_init ();
  pdcp_module_init();
  logInit();

  if (init_pdcp_entity(&pdcp_array[0][1]) == TRUE && init_pdcp_entity(&pdcp_array[1][1]) == TRUE)
    msg("PDCP entity initialization OK\n");
  else {
    msg("Cannot initialize PDCP entities!\n");
    return 1;
  }

  /* Initialize PDCP state variables */
  for (index = 0; index < 2; ++index) {
    if (pdcp_init_seq_numbers(&pdcp_array[index][1]) == FALSE) {
      msg("Cannot initialize %s PDCP entity!\n", ((index == 0) ? "first" : "second"));
      exit(1);
    }
  }


  /* Configure RLCs */
  rlc_info.rlc_mode = RLC_UM;
  rlc_info.rlc.rlc_um_info.timer_reordering = 20;
  rlc_info.rlc.rlc_um_info.sn_field_length  = 10;
  rlc_info.rlc.rlc_um_info.is_mXch          = 0;

  rlc_op_status = rrc_rlc_config_req   (0, ACTION_ADD, 1, RADIO_ACCESS_BEARER, rlc_info);
  assert(rlc_op_status == RLC_OP_STATUS_OK);

  rlc_op_status = rrc_rlc_config_req   (1, ACTION_ADD, 1, RADIO_ACCESS_BEARER, rlc_info);
  assert(rlc_op_status == RLC_OP_STATUS_OK);


  /* Simulate data traffic */
  pdcp_rlc_test_send_sdu(0, 1, 2);
  pdcp_rlc_test_exchange_pdus(&rlc[0].m_rlc_um_array[rlc[0].m_rlc_pointer[1].rlc_index],
                              &rlc[1].m_rlc_um_array[rlc[1].m_rlc_pointer[1].rlc_index],
                              1000,
                              1000);
  pdcp_rlc_test_exchange_pdus(&rlc[1].m_rlc_um_array[rlc[1].m_rlc_pointer[1].rlc_index],
                              &rlc[0].m_rlc_um_array[rlc[0].m_rlc_pointer[1].rlc_index],
                              1000,
                              1000);


  return 0;
}

//-----------------------------------------------------------------------------
BOOL init_pdcp_entity(pdcp_t *pdcp_entity)
//-----------------------------------------------------------------------------
{
  if (pdcp_entity == NULL)
    return FALSE;

  /*
   * Initialize sequence number state variables of relevant PDCP entity
   */
  pdcp_entity->next_pdcp_tx_sn = 0;
  pdcp_entity->next_pdcp_rx_sn = 0;
  pdcp_entity->tx_hfn = 0;
  pdcp_entity->rx_hfn = 0;
  /* SN of the last PDCP SDU delivered to upper layers */
  pdcp_entity->last_submitted_pdcp_rx_sn = 4095;
  pdcp_entity->seq_num_size = 12;

  return TRUE;
}

