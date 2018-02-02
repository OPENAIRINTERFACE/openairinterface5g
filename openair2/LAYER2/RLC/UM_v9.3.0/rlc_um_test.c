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

#define RLC_UM_MODULE
#define RLC_C
#define RLC_UM_TEST_C
#include <execinfo.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <assert.h>
#include <signal.h>
#include <sys/time.h>
//#include <curses.h>

#include "platform_types.h"
//-----------------------------------------------------------------------------
#include "rlc.h"
#include "rlc_um.h"
#include "list.h"
#include "rlc_um_control_primitives.h"
#include "LAYER2/MAC/extern.h"


#define TEST1
#define TEST2
#define TEST3
#define TEST4
#define TEST5

#define INCREMENT_FRAME_YES 1
#define INCREMENT_FRAME_NO  0

#define TEST_MAX_SEND_SDU 8192
#define TARGET_MAX_RX_ERROR_RATE 10
#define TARGET_MAX_TX_ERROR_RATE 10
static int  g_frame = 0;
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

// time in frame numbers
#define MAX_TIME_DELAYED_PDU_DUE_TO_HARQ     256
static struct mac_data_ind g_tx_delayed_indications[MAX_TIME_DELAYED_PDU_DUE_TO_HARQ];
static struct mac_data_ind g_rx_delayed_indications[MAX_TIME_DELAYED_PDU_DUE_TO_HARQ];


static int8_t *g_sdus[] = {"En dépit de son volontarisme affiché, le premier ministre est de plus en plus décrié pour son incompétence. La tension politique et dans l'opinion publique est encore montée d'un cran au Japon, sur fond d'inquiétantes nouvelles, avec du plutonium détecté dans le sol autour de la centrale de Fukushima. Le premier ministre Naoto Kan a solennellement déclaré que son gouvernement était «en état d'alerte maximum». Tout en reconnaissant que la situation restait «imprévisible». Ce volontarisme affiché par le premier ministre - que Nicolas Sarkozy rencontrera demain lors d'une visite au Japon - ne l'a pas empêché d'être la cible de violentes critiques de la part de parlementaires sur sa gestion de la crise. Attaqué sur le manque de transparence, il a assuré qu'il rendait publiques toutes les informations en sa possession. Un député de l'opposition, Yosuke Isozaki, a aussi reproché à Naoto Kan de ne pas avoir ordonné l'évacuation des populations dans la zone comprise entre 20 et 30 km autour de la centrale. «Peut-il y avoir quelque chose de plus irresponsable que cela ?», a-t-il lancé. Pour l'heure, la zone d'évacuation est limitée à un rayon de 20 km, seul le confinement étant recommandé pour les 10 km suivants. Sur ce sujet, les autorités japonaises ont été fragilisées mardi par les déclarations de Greenpeace, affirmant que ses experts avaient détecté une radioactivité dangereuse à 40 km de la centrale. L'organisation écologiste a appelé à une extension de la zone d'évacuation, exhortant Tokyo à «cesser de privilégier la politique aux dépens de la science». L'Agence japonaise de sûreté nucléaire a balayé ces critiques.",

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

#define RLC_2_PRINT_BUFFER_LEN 10000
static char rlc_2_print_buffer[RLC_2_PRINT_BUFFER_LEN];
//-----------------------------------------------------------------------------
void rlc_util_print_hex_octets(comp_name_t componentP, unsigned char* dataP, unsigned long sizeP)
//-----------------------------------------------------------------------------
{
  unsigned long octet_index = 0;
  unsigned long buffer_marker = 0;
  unsigned char aindex;

  if (dataP == NULL) {
    return;
  }


  LOG_D(RLC, "------+-------------------------------------------------|\n");
  LOG_D(RLC, "      |  0  1  2  3  4  5  6  7  8  9  a  b  c  d  e  f |\n");
  LOG_D(RLC, "------+-------------------------------------------------|\n");

  for (octet_index = 0; octet_index < sizeP; octet_index++) {
    if ((octet_index % 16) == 0) {
      if (octet_index != 0) {
        buffer_marker+=snprintf(&rlc_2_print_buffer[buffer_marker], RLC_2_PRINT_BUFFER_LEN - buffer_marker, " |\n");
        LOG_D(RLC, "%s", rlc_2_print_buffer);
        buffer_marker = 0;
      }

      buffer_marker+=snprintf(&rlc_2_print_buffer[buffer_marker], RLC_2_PRINT_BUFFER_LEN - buffer_marker, " %04ld |", octet_index);
    }

    /*
     * Print every single octet in hexadecimal form
     */
    buffer_marker+=snprintf(&rlc_2_print_buffer[buffer_marker], RLC_2_PRINT_BUFFER_LEN - buffer_marker, " %02x", dataP[octet_index]);
    /*
     * Align newline and pipes according to the octets in groups of 2
     */
  }

  /*
   * Append enough spaces and put final pipe
   */
  for (aindex = octet_index; aindex < 16; ++aindex) {
    buffer_marker+=snprintf(&rlc_2_print_buffer[buffer_marker], RLC_2_PRINT_BUFFER_LEN - buffer_marker, "   ");
  }

  // LOG_D(RLC, "   ");
  buffer_marker+=snprintf(&rlc_2_print_buffer[buffer_marker], RLC_2_PRINT_BUFFER_LEN - buffer_marker, " |\n");
  LOG_D(RLC, "%s",rlc_2_print_buffer);
}

//-----------------------------------------------------------------------------
void rlc_um_v9_3_0_test_windows_10()
//-----------------------------------------------------------------------------
{
  rlc_um_entity_t um1;
  rlc_um_entity_t um2;
  unsigned int    h,w, sn, result;

  uint32_t             timer_reordering = 2000;
  uint32_t             sn_field_length  = 10;
  uint32_t             is_mXch          = 0; // boolean, true if configured for MTCH or MCCH

  rlc_um_init(&um1);
  rlc_um_init(&um2);

  rlc_um_set_debug_infos(&um1, g_frame, 0, 0, 0, 1, 1 /*LC-id = RAB-id*/);
  rlc_um_set_debug_infos(&um2, g_frame, 1, 1, 1, 1, 1 /*LC-id = RAB-id*/);

  rlc_um_configure(&um1, g_frame, timer_reordering, sn_field_length, sn_field_length, is_mXch);
  rlc_um_configure(&um2, g_frame, timer_reordering, sn_field_length, sn_field_length, is_mXch);

  // RX window with vr_uh > vr_ur
  for (h = 0; h < RLC_UM_SN_10_BITS_MODULO; h++) {
    um1.vr_uh = h;

    for (w = 1; w < RLC_UM_WINDOW_SIZE_SN_10_BITS; w++) {
      um1.vr_ur = (um1.vr_uh - w) & RLC_UM_SN_10_BITS_MASK;

      for (sn = ((um1.vr_uh - um1.rx_um_window_size) & RLC_UM_SN_10_BITS_MASK) ; sn != um1.vr_uh; sn = ((sn+1) & RLC_UM_SN_10_BITS_MASK)) {
        assert(rlc_um_in_reordering_window(&um1, g_frame, sn) >= 0);
        // returns -2 if lower_bound  > sn
        // returns -1 if higher_bound < sn
        // returns  0 if lower_bound  < sn < higher_bound
        // returns  1 if lower_bound  == sn
        // returns  2 if higher_bound == sn
        // returns  3 if higher_bound == sn == lower_bound
        result = rlc_um_in_window(&um1, g_frame, (um1.vr_uh - um1.rx_um_window_size) & RLC_UM_SN_10_BITS_MASK, sn, um1.vr_uh);
        assert((result < 2) && (result >=0));
      }

      for (sn = um1.vr_uh ; sn != ((um1.vr_uh - um1.rx_um_window_size) & RLC_UM_SN_10_BITS_MASK) ; sn = ((sn+1) & RLC_UM_SN_10_BITS_MASK)) {
        // returns -2 if lower_bound  > sn
        // returns -1 if higher_bound < sn
        // returns  0 if lower_bound  < sn < higher_bound
        // returns  1 if lower_bound  == sn
        // returns  2 if higher_bound == sn
        // returns  3 if higher_bound == sn == lower_bound
        assert(rlc_um_in_window(&um1, g_frame, (um1.vr_uh - um1.rx_um_window_size) & RLC_UM_SN_10_BITS_MASK, sn, (um1.vr_uh -1) & RLC_UM_SN_10_BITS_MASK) < 0);
        assert(rlc_um_in_reordering_window(&um1, g_frame, sn) < 0);
      }

    }
  }
}
//-----------------------------------------------------------------------------
void rlc_um_v9_3_0_test_windows_5()
//-----------------------------------------------------------------------------
{
  rlc_um_entity_t um1;
  rlc_um_entity_t um2;
  unsigned int    h,w, sn, result;

  uint32_t             timer_reordering = 2000;
  uint32_t             sn_field_length  = 5;
  uint32_t             is_mXch          = 0; // boolean, true if configured for MTCH or MCCH

  rlc_um_init(&um1);
  rlc_um_init(&um2);

  rlc_um_set_debug_infos(&um1, g_frame, 0, 0, 0, 1, 1 /* LC-id = DRB-id */);
  rlc_um_set_debug_infos(&um2, g_frame, 1, 1, 1, 1, 1 /* LC-id = DRB-id */);

  rlc_um_configure(&um1, g_frame, timer_reordering, sn_field_length, sn_field_length, is_mXch);
  rlc_um_configure(&um2, g_frame, timer_reordering, sn_field_length, sn_field_length, is_mXch);

  // RX window with vr_uh > vr_ur
  for (h = 0; h < RLC_UM_SN_5_BITS_MODULO; h++) {
    um1.vr_uh = h;

    for (w = 1; w < RLC_UM_WINDOW_SIZE_SN_5_BITS; w++) {
      um1.vr_ur = (um1.vr_uh - w) & RLC_UM_SN_5_BITS_MASK;

      for (sn = ((um1.vr_uh - um1.rx_um_window_size) & RLC_UM_SN_5_BITS_MASK) ; sn != um1.vr_uh; sn = ((sn+1) & RLC_UM_SN_5_BITS_MASK)) {
        assert(rlc_um_in_reordering_window(&um1, g_frame, sn) >= 0);
        // returns -2 if lower_bound  > sn
        // returns -1 if higher_bound < sn
        // returns  0 if lower_bound  < sn < higher_bound
        // returns  1 if lower_bound  == sn
        // returns  2 if higher_bound == sn
        // returns  3 if higher_bound == sn == lower_bound
        result = rlc_um_in_window(&um1, g_frame, (um1.vr_uh - um1.rx_um_window_size) & RLC_UM_SN_5_BITS_MASK, sn, um1.vr_uh);
        assert((result < 2) && (result >=0));
      }

      for (sn = um1.vr_uh ; sn != ((um1.vr_uh - um1.rx_um_window_size) & RLC_UM_SN_5_BITS_MASK) ; sn = ((sn+1) & RLC_UM_SN_5_BITS_MASK)) {
        // returns -2 if lower_bound  > sn
        // returns -1 if higher_bound < sn
        // returns  0 if lower_bound  < sn < higher_bound
        // returns  1 if lower_bound  == sn
        // returns  2 if higher_bound == sn
        // returns  3 if higher_bound == sn == lower_bound
        assert(rlc_um_in_window(&um1, g_frame, (um1.vr_uh - um1.rx_um_window_size) & RLC_UM_SN_5_BITS_MASK, sn, (um1.vr_uh -1) & RLC_UM_SN_5_BITS_MASK) < 0);
        assert(rlc_um_in_reordering_window(&um1, g_frame, sn) < 0);
      }

    }
  }
}
//-----------------------------------------------------------------------------
void rlc_um_v9_3_0_test_reset_sdus()
//-----------------------------------------------------------------------------
{
  int i, j;

  for (j = 0; j < 2; j++) {
    for (i = 0; i < TEST_MAX_SEND_SDU; i++) {
      g_send_sdu_ids[i][j]= -1;
    }

    g_send_id_write_index[j] = 0;
    g_send_id_read_index[j]  = 0;
  }
}
//-----------------------------------------------------------------------------
void rlc_um_v9_3_0_test_send_sdu(rlc_um_entity_t *um_txP, int sdu_indexP)
//-----------------------------------------------------------------------------
{
  mem_block_t *sdu;
  sdu = get_free_mem_block (strlen(g_sdus[sdu_indexP]) + 1 + sizeof (struct rlc_um_data_req_alloc), __func__);

  if (sdu != NULL) {
    // PROCESS OF COMPRESSION HERE:
    printf("[FRAME %05d][RLC][MOD %02d][RB %02d] TX SDU %d %04d bytes\n",g_frame,um_txP->module_id, um_txP->rb_id, sdu_indexP, strlen(g_sdus[sdu_indexP]) + 1);
    memset (sdu->data, 0, sizeof (struct rlc_um_data_req_alloc));
    strcpy (&sdu->data[sizeof (struct rlc_um_data_req_alloc)],g_sdus[sdu_indexP]);

    ((struct rlc_um_data_req *) (sdu->data))->data_size = strlen(g_sdus[sdu_indexP])+ 1;
    ((struct rlc_um_data_req *) (sdu->data))->data_offset = sizeof (struct rlc_um_data_req_alloc);
    rlc_um_data_req(um_txP, g_frame, sdu);

    g_send_sdu_ids[g_send_id_write_index[um_txP->rb_id]++][um_txP->rb_id] = sdu_indexP;
    assert(g_send_id_write_index[um_txP->rb_id] < TEST_MAX_SEND_SDU);
  } else {
    printf("Out of memory error\n");
    exit(-1);
  }
}

//-----------------------------------------------------------------------------
void rlc_um_v9_3_0_buffer_delayed_rx_mac_data_ind(struct mac_data_ind* data_indP, signed int time_delayedP)
//-----------------------------------------------------------------------------
{
  int   frame_modulo;
  mem_block_t* tb;

  if (time_delayedP <= 0) {
    frame_modulo = g_frame % MAX_TIME_DELAYED_PDU_DUE_TO_HARQ;
  } else {
    frame_modulo = (g_frame + time_delayedP) % MAX_TIME_DELAYED_PDU_DUE_TO_HARQ;
  }

  while (data_indP->data.nb_elements > 0) {
    tb = list_remove_head (&data_indP->data);

    if (tb != NULL) {
      if (time_delayedP < 0) {
        list_add_head(tb, &g_rx_delayed_indications[frame_modulo].data);
      } else {
        list_add_tail_eurecom(tb, &g_rx_delayed_indications[frame_modulo].data);
      }

      g_rx_delayed_indications[frame_modulo].no_tb  += 1;
    }
  }

  assert(data_indP->data.head == NULL);
}
//-----------------------------------------------------------------------------
void rlc_um_v9_3_0_buffer_delayed_tx_mac_data_ind(struct mac_data_ind* data_indP, signed int time_delayedP)
//-----------------------------------------------------------------------------
{
  int   frame_modulo;
  mem_block_t* tb;

  if (time_delayedP <= 0) {
    frame_modulo = g_frame % MAX_TIME_DELAYED_PDU_DUE_TO_HARQ;
  } else {
    frame_modulo = (g_frame + time_delayedP) % MAX_TIME_DELAYED_PDU_DUE_TO_HARQ;
  }

  while (data_indP->data.nb_elements > 0) {
    tb = list_remove_head (&data_indP->data);

    if (tb != NULL) {
      if (time_delayedP < 0) {
        list_add_head(tb, &g_rx_delayed_indications[frame_modulo].data);
      } else {
        list_add_tail_eurecom(tb, &g_rx_delayed_indications[frame_modulo].data);
      }

      g_rx_delayed_indications[frame_modulo].no_tb  += 1;
    }
  }

  assert(data_indP->data.head == NULL);
}

//-----------------------------------------------------------------------------
void rlc_um_v9_3_0_test_mac_rlc_loop (struct mac_data_ind* data_indP,  struct mac_data_req* data_requestP, int* drop_countP, int* tx_packetsP,
                                      int* dropped_tx_packetsP)
//-----------------------------------------------------------------------------
{


  mem_block_t* tb_src;
  mem_block_t* tb_dst;
  unsigned int tb_size;

  data_indP->no_tb = 0;

  while (data_requestP->data.nb_elements > 0) {
    tb_src = list_remove_head (&data_requestP->data);

    if (tb_src != NULL) {
      tb_size = ((struct mac_tb_req *) (tb_src->data))->tb_size_in_bits >> 3;
      printf("[RLC-LOOP] FOUND TB SIZE IN BITS %d IN BYTES %d sizeof (mac_rlc_max_rx_header_size_t) %d\n",
             ((struct mac_tb_req *) (tb_src->data))->tb_size_in_bits,
             tb_size, sizeof (mac_rlc_max_rx_header_size_t));

      *tx_packetsP = *tx_packetsP + 1;

      if (*drop_countP == 0) {
        tb_dst  = get_free_mem_block(sizeof (mac_rlc_max_rx_header_size_t) + tb_size, __func__);
        memset(tb_dst->data, 0, sizeof (mac_rlc_max_rx_header_size_t) + tb_size);

        if (tb_dst != NULL) {
          //printf("[RLC-LOOP] Testing tb_dst (1)\n");
          check_free_mem_block(tb_dst, __func__);
          tb_dst->next = NULL;
          ((struct mac_tb_ind *) (tb_dst->data))->first_bit        = 0;
          ((struct mac_tb_ind *) (tb_dst->data))->data_ptr         = &tb_dst->data[sizeof (mac_rlc_max_rx_header_size_t)];
          ((struct mac_tb_ind *) (tb_dst->data))->size             = tb_size;
          ((struct mac_tb_ind *) (tb_dst->data))->error_indication = 0;

          memcpy(((struct mac_tb_ind *) (tb_dst->data))->data_ptr,
                 &((struct mac_tb_req *) (tb_src->data))->data_ptr[0],
                 tb_size);

          list_add_tail_eurecom(tb_dst, &data_indP->data);
          data_indP->no_tb  += 1;
          //printf("[RLC-LOOP] Testing tb_dst (2)\n");
          check_free_mem_block(tb_dst, __func__);
        } else {
          printf("Out of memory error\n");
          exit(-1);
        }
      } else {
        printf("[RLC-LOOP] DROPPING 1 TB\n");
        *drop_countP = *drop_countP - 1;
        *dropped_tx_packetsP = *dropped_tx_packetsP + 1;
      }


      //printf("[RLC-LOOP] Testing tb_src\n");
      check_free_mem_block(tb_src, __func__);

      free_mem_block(tb_src, __func__);

      if (data_indP->no_tb > 0) {
        printf("[RLC-LOOP] Exchange %d TBs\n",data_indP->no_tb);
      }
    }
  }
}
//-----------------------------------------------------------------------------
void rlc_um_v9_3_0_test_exchange_pdus(rlc_um_entity_t *um_txP,
                                      rlc_um_entity_t *um_rxP,
                                      uint16_t           bytes_txP,
                                      uint16_t           bytes_rxP)
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

  mac_rlc_status_resp_tx = rlc_um_mac_status_indication(um_txP, g_frame, 1, bytes_txP, tx_status,ENB_FLAG_YES);
  data_request_tx        = rlc_um_mac_data_request(um_txP, g_frame);
  mac_rlc_status_resp_rx = rlc_um_mac_status_indication(um_rxP, g_frame, 0, bytes_rxP, tx_status,ENB_FLAG_YES);
  data_request_rx        = rlc_um_mac_data_request(um_rxP, g_frame);


  rlc_um_v9_3_0_test_mac_rlc_loop(&data_ind_rx, &data_request_tx, &g_drop_tx, &g_tx_packets, &g_dropped_tx_packets);
  rlc_um_v9_3_0_test_mac_rlc_loop(&data_ind_tx, &data_request_rx, &g_drop_rx, &g_rx_packets, &g_dropped_rx_packets);
  rlc_um_mac_data_indication(um_rxP, g_frame, um_rxP->is_enb, data_ind_rx);
  rlc_um_mac_data_indication(um_txP, g_frame, um_txP->is_enb, data_ind_tx);
  g_frame += 1;
  //check_mem_area();
  //display_mem_load();
}
//-----------------------------------------------------------------------------
void rlc_um_v9_3_0_test_exchange_delayed_pdus(rlc_um_entity_t *um_txP,
    rlc_um_entity_t *um_rxP,
    uint16_t           bytes_txP,
    uint16_t           bytes_rxP,
    signed int      time_tx_delayedP, // if -1 added to the head of current frame tx mac data ind if any
    // if 0  added to the tail of current frame tx mac data ind if any
    // if > 0  added to the tail of frame+time_tx_delayedP mac data ind if any
    signed int      time_rx_delayedP, // if -1 added to the head of current frame rx mac data ind if any
    // if 0  added to the tail of current frame rx mac data ind if any
    // if > 0  added to the tail of frame+time_rx_delayedP mac data ind if any
    int             is_frame_incrementedP)
//-----------------------------------------------------------------------------
{
  struct mac_data_req    data_request_tx;
  struct mac_data_req    data_request_rx;
  struct mac_data_ind    data_ind_tx;
  struct mac_data_ind    data_ind_rx;
  struct mac_status_ind  tx_status;
  struct mac_status_resp mac_rlc_status_resp_tx;
  struct mac_status_resp mac_rlc_status_resp_rx;
  int                    frame_modulo = g_frame % MAX_TIME_DELAYED_PDU_DUE_TO_HARQ;


  memset(&data_request_tx, 0, sizeof(struct mac_data_req));
  memset(&data_request_rx, 0, sizeof(struct mac_data_req));
  memset(&data_ind_tx,     0, sizeof(struct mac_data_ind));
  memset(&data_ind_rx,     0, sizeof(struct mac_data_ind));
  memset(&tx_status,       0, sizeof(struct mac_status_ind));
  memset(&mac_rlc_status_resp_tx, 0, sizeof(struct mac_status_resp));
  memset(&mac_rlc_status_resp_rx, 0, sizeof(struct mac_status_resp));

  mac_rlc_status_resp_tx = rlc_um_mac_status_indication(um_txP, g_frame, 1, bytes_txP, tx_status,ENB_FLAG_YES);
  data_request_tx        = rlc_um_mac_data_request(um_txP, g_frame);
  mac_rlc_status_resp_rx = rlc_um_mac_status_indication(um_rxP, g_frame, 0, bytes_rxP, tx_status,ENB_FLAG_YES);
  data_request_rx        = rlc_um_mac_data_request(um_rxP, g_frame);


  rlc_um_v9_3_0_test_mac_rlc_loop(&data_ind_rx, &data_request_tx, &g_drop_tx, &g_tx_packets, &g_dropped_tx_packets);
  rlc_um_v9_3_0_test_mac_rlc_loop(&data_ind_tx, &data_request_rx, &g_drop_rx, &g_rx_packets, &g_dropped_rx_packets);

  rlc_um_v9_3_0_buffer_delayed_rx_mac_data_ind(&data_ind_rx, time_tx_delayedP);
  rlc_um_v9_3_0_buffer_delayed_tx_mac_data_ind(&data_ind_tx, time_rx_delayedP);


  rlc_um_mac_data_indication(um_rxP, g_frame, um_rxP->is_enb, g_rx_delayed_indications[frame_modulo]);
  memset(&g_rx_delayed_indications[frame_modulo], 0, sizeof(struct mac_data_ind));

  rlc_um_mac_data_indication(um_txP, g_frame, um_txP->is_enb, g_tx_delayed_indications[frame_modulo]);
  memset(&g_tx_delayed_indications[frame_modulo], 0, sizeof(struct mac_data_ind));

  if (is_frame_incrementedP) {
    g_frame += 1;
  }
}

//-----------------------------------------------------------------------------
void rlc_um_v9_3_0_test_data_conf(module_id_t module_idP, rb_id_t rb_idP, mui_t muiP, rlc_tx_status_t statusP)
//-----------------------------------------------------------------------------
{
  if (statusP == RLC_SDU_CONFIRM_YES) {
    printf("[FRAME %05d][RLC][MOD %02d][RB %02d]  CONFIRM SEND SDU MUI %05d\n",g_frame,module_idP, rb_idP, muiP);
  } else {
    printf("[FRAME %05d][RLC][MOD %02d][RB %02d]  CONFIRM LOST SDU MUI %05d\n",g_frame,module_idP, rb_idP, muiP);
  }
}
//-----------------------------------------------------------------------------
void rlc_um_v9_3_0_test_data_ind (module_id_t module_idP, rb_id_t rb_idP, sdu_size_t sizeP, mem_block_t *sduP)
//-----------------------------------------------------------------------------
{
  int i;

  if (g_error_on_phy == 0) {
    for (i = 0; i < 37; i++) {
      if (strcmp(g_sdus[i], sduP->data) == 0) {
        printf("[FRAME %05d][RLC][MOD %02d][RB %02d] RX SDU %d %04d bytes\n",g_frame,module_idP, rb_idP, i, sizeP);
        assert(TEST_MAX_SEND_SDU > g_send_id_read_index[rb_idP]);
        assert(g_send_id_write_index[rb_idP^1] > g_send_id_read_index[rb_idP]);

        if (g_send_sdu_ids[g_send_id_read_index[rb_idP]][rb_idP^1] != i) {

          printf("[FRAME %05d][RLC][MOD %d][RB %d][DATA-IND] g_send_sdu_ids[%d] = %d\n",g_frame,module_idP, rb_idP,  g_send_id_read_index[rb_idP]-2,
                 g_send_sdu_ids[g_send_id_read_index[rb_idP]-2][rb_idP^1]);
          printf("[FRAME %05d][RLC][MOD %d][RB %d][DATA-IND] g_send_sdu_ids[%d] = %d\n",g_frame,module_idP, rb_idP,  g_send_id_read_index[rb_idP]-1,
                 g_send_sdu_ids[g_send_id_read_index[rb_idP]-1][rb_idP^1]);
          printf("[FRAME %05d][RLC][MOD %d][RB %d][DATA-IND] g_send_sdu_ids[%d] = %d\n",g_frame,module_idP, rb_idP,  g_send_id_read_index[rb_idP],
                 g_send_sdu_ids[g_send_id_read_index[rb_idP]][rb_idP^1]);

          printf("[FRAME %05d][RLC][MOD %d][RB %d][DATA-IND] g_send_id_read_index = %d sdu sent = %d\n",g_frame,module_idP, rb_idP,  g_send_id_read_index[rb_idP], i);
        }

        assert(g_send_sdu_ids[g_send_id_read_index[rb_idP]][rb_idP^1] == i);
        g_send_id_read_index[rb_idP] += 1;
        free_mem_block(sduP, __func__);
        return;
      }
    }

    printf("[FRAME %05d][RLC][MOD %d][RB %d] RX UNKNOWN SDU %04d bytes\n",g_frame,module_idP, rb_idP,  sizeP);
    free_mem_block(sduP, __func__);
    assert(1==2);
  } else {
    for (i = 0; i < 37; i++) {
      if (strcmp(g_sdus[i], sduP->data) == 0) {
        printf("[FRAME %05d][RLC][MOD %02d][RB %02d] RX SDU %d %04d bytes\n",g_frame,module_idP, rb_idP, i, sizeP);
        assert(TEST_MAX_SEND_SDU > g_send_id_read_index[rb_idP]);
        g_send_id_read_index[rb_idP] += 1;
        free_mem_block(sduP, __func__);
        return;
      }
    }

    printf("[FRAME %05d][RLC][MOD %d][RB %d] RX UNKNOWN SDU %04d bytes\n",g_frame,module_idP, rb_idP,  sizeP);
    free_mem_block(sduP, __func__);
    return;
  }
}
//-----------------------------------------------------------------------------
void rlc_um_v9_3_0_test_reordering(uint32_t sn_field_lengthP)
//-----------------------------------------------------------------------------
{
  rlc_um_info_t     um_info;
  int                   i,j,r;

  um_info.timer_reordering = (32 * sn_field_lengthP * sn_field_lengthP)/100;
  um_info.sn_field_length  = sn_field_lengthP;
  um_info.is_mXch          = 0;

  srand (0);
  config_req_rlc_um (&um_tx, 0,0,0, &um_info, 0, SIGNALLING_RADIO_BEARER, SIGNALLING_RADIO_BEARER /*LC-id = DRB-id*/);
  config_req_rlc_um (&um_rx, 0,1,1, &um_info, 1, SIGNALLING_RADIO_BEARER, SIGNALLING_RADIO_BEARER /*LC-id = DRB-id*/);

  rlc_um_display_rx_window(&um_tx);

  rlc_um_display_rx_window(&um_rx);


  srand (0);


  // BIG SDU SMALL PDUS NO ERRORS
  rlc_um_v9_3_0_test_reset_sdus();

  for (i = 0; i < 128; i++) {
    rlc_um_v9_3_0_test_send_sdu(&um_tx, 1);
    rlc_um_v9_3_0_test_exchange_delayed_pdus(&um_tx, &um_rx, 3,    200, 0, 0, INCREMENT_FRAME_YES);
    rlc_um_v9_3_0_test_exchange_delayed_pdus(&um_tx, &um_rx, 3,    200, 0, 0, INCREMENT_FRAME_YES);
    rlc_um_v9_3_0_test_exchange_delayed_pdus(&um_tx, &um_rx, 3,    200, um_info.timer_reordering - 5, 0, INCREMENT_FRAME_YES);
    rlc_um_v9_3_0_test_exchange_delayed_pdus(&um_tx, &um_rx, 4,    200, 7, 0, INCREMENT_FRAME_YES);
    rlc_um_v9_3_0_test_exchange_delayed_pdus(&um_tx, &um_rx, 5,    200, 5, 0, INCREMENT_FRAME_YES);
    rlc_um_v9_3_0_test_exchange_delayed_pdus(&um_tx, &um_rx, 6,    200, 3, 0, INCREMENT_FRAME_YES);
    rlc_um_v9_3_0_test_exchange_delayed_pdus(&um_tx, &um_rx, 7,    200, 1, 0, INCREMENT_FRAME_YES);
    rlc_um_v9_3_0_test_exchange_delayed_pdus(&um_tx, &um_rx, 8,    200, 0, 0, INCREMENT_FRAME_YES);
    rlc_um_v9_3_0_test_exchange_delayed_pdus(&um_tx, &um_rx, 9,    200, 0, 0, INCREMENT_FRAME_YES);
    rlc_um_v9_3_0_test_exchange_delayed_pdus(&um_tx, &um_rx, 10,   200, 7, 0, INCREMENT_FRAME_YES);
    rlc_um_v9_3_0_test_exchange_delayed_pdus(&um_tx, &um_rx, 11,   200, 0, 0, INCREMENT_FRAME_YES);
    rlc_um_v9_3_0_test_exchange_delayed_pdus(&um_tx, &um_rx, 12,   200, 5, 0, INCREMENT_FRAME_YES);
    rlc_um_v9_3_0_test_exchange_delayed_pdus(&um_tx, &um_rx, 13,   200, 0, 0, INCREMENT_FRAME_YES);
    rlc_um_v9_3_0_test_exchange_delayed_pdus(&um_tx, &um_rx, 14,   200, 3, 0, INCREMENT_FRAME_YES);
    rlc_um_v9_3_0_test_exchange_delayed_pdus(&um_tx, &um_rx, 15,   200, 0, 0, INCREMENT_FRAME_YES);
    rlc_um_v9_3_0_test_exchange_delayed_pdus(&um_tx, &um_rx, 16,   200, 1, 0, INCREMENT_FRAME_YES);
    rlc_um_v9_3_0_test_exchange_delayed_pdus(&um_tx, &um_rx, 17,   200, 0, 0, INCREMENT_FRAME_YES);
    rlc_um_v9_3_0_test_exchange_delayed_pdus(&um_tx, &um_rx, 18,   200, 3, 0, INCREMENT_FRAME_YES);
    rlc_um_v9_3_0_test_exchange_delayed_pdus(&um_tx, &um_rx, 19,   200, 0, 0, INCREMENT_FRAME_YES);
    rlc_um_v9_3_0_test_exchange_delayed_pdus(&um_tx, &um_rx, 20,   200, 0, 0, INCREMENT_FRAME_YES);
    rlc_um_v9_3_0_test_exchange_delayed_pdus(&um_tx, &um_rx, 21,   200, 0, 0, INCREMENT_FRAME_YES);
    rlc_um_v9_3_0_test_exchange_delayed_pdus(&um_tx, &um_rx, 22,   200, 0, 0, INCREMENT_FRAME_YES);
    rlc_um_v9_3_0_test_exchange_delayed_pdus(&um_tx, &um_rx, 23,   200, 0, 0, INCREMENT_FRAME_YES);
    rlc_um_v9_3_0_test_exchange_delayed_pdus(&um_tx, &um_rx, 24,   200, 0, 0, INCREMENT_FRAME_YES);
    rlc_um_v9_3_0_test_exchange_delayed_pdus(&um_tx, &um_rx, 25,   200, 0, 0, INCREMENT_FRAME_YES);
    rlc_um_v9_3_0_test_exchange_delayed_pdus(&um_tx, &um_rx, 26,   200, 0, 0, INCREMENT_FRAME_YES);
    rlc_um_v9_3_0_test_exchange_delayed_pdus(&um_tx, &um_rx, 27,   200, 0, 0, INCREMENT_FRAME_YES);
    rlc_um_v9_3_0_test_exchange_delayed_pdus(&um_tx, &um_rx, 28,   200, 0, 0, INCREMENT_FRAME_YES);
    rlc_um_v9_3_0_test_exchange_delayed_pdus(&um_tx, &um_rx, 29,   200, 0, 0, INCREMENT_FRAME_YES);
    rlc_um_v9_3_0_test_exchange_delayed_pdus(&um_tx, &um_rx, 2000, 200, 0, 0, INCREMENT_FRAME_YES);
    rlc_um_v9_3_0_test_exchange_delayed_pdus(&um_tx, &um_rx, 2000, 200, 0, 0, INCREMENT_FRAME_YES);
    rlc_um_v9_3_0_test_exchange_delayed_pdus(&um_tx, &um_rx, 2000, 200, 0, 0, INCREMENT_FRAME_YES);
    rlc_um_v9_3_0_test_exchange_delayed_pdus(&um_tx, &um_rx, 2000, 200, 0, 0, INCREMENT_FRAME_YES);
    rlc_um_v9_3_0_test_exchange_delayed_pdus(&um_tx, &um_rx, 2000, 200, 0, 0, INCREMENT_FRAME_YES);
    rlc_um_v9_3_0_test_exchange_delayed_pdus(&um_tx, &um_rx, 2000, 200, 0, 0, INCREMENT_FRAME_YES);
    rlc_um_v9_3_0_test_exchange_delayed_pdus(&um_tx, &um_rx, 2000, 200, 0, 0, INCREMENT_FRAME_YES);

    assert (g_send_id_read_index[1] == g_send_id_write_index[0]);
    printf("\n\n\n\n\n\n\n\n");

  }

  printf("\n\n\n\n\n\n-----------------------------------------------------------------------------------------rlc_um_v9_3_0_test_reordering 3: END OF TEST BIG SDU, SMALL PDUs\n\n\n\n");

  rlc_um_v9_3_0_test_reset_sdus();

  for (j = 0; j < 16; j++) {
    //i = getchar();

    rlc_um_v9_3_0_test_reset_sdus();

    rlc_um_v9_3_0_test_send_sdu(&um_tx, 1);

    for (i = 0; i < 32; i++) {
      rlc_um_v9_3_0_test_exchange_delayed_pdus(&um_tx, &um_rx, 3, 200, 0, 0, INCREMENT_FRAME_YES);
    }

    rlc_um_v9_3_0_test_exchange_delayed_pdus(&um_tx, &um_rx, 2000, 200, 0, 0, INCREMENT_FRAME_YES);
    assert (g_send_id_read_index[1] == g_send_id_write_index[0]);


    rlc_um_v9_3_0_test_send_sdu(&um_tx, 1);
    rlc_um_v9_3_0_test_exchange_delayed_pdus(&um_tx, &um_rx, 3, 200, 0, 0, INCREMENT_FRAME_YES);
    rlc_um_v9_3_0_test_exchange_delayed_pdus(&um_tx, &um_rx, 3, 200, 0, 0, INCREMENT_FRAME_YES);
    rlc_um_v9_3_0_test_exchange_delayed_pdus(&um_tx, &um_rx, 3, 200, um_info.timer_reordering + 5, 0, INCREMENT_FRAME_YES);

    for (i = 0; i < 600; i++) {
      rlc_um_v9_3_0_test_exchange_delayed_pdus(&um_tx, &um_rx, 3, 200, 0, 0, INCREMENT_FRAME_YES);
    }

    rlc_um_v9_3_0_test_exchange_delayed_pdus(&um_tx, &um_rx, 2000, 200, 0, 0, INCREMENT_FRAME_YES);
    printf("g_send_id_read_index[1]=%d g_send_id_write_index[0]=%d Loop %d (1)\n", g_send_id_read_index[1], g_send_id_write_index[0], j);
    assert (g_send_id_read_index[1] != g_send_id_write_index[0]);

    rlc_um_v9_3_0_test_send_sdu(&um_tx, 1);
    rlc_um_v9_3_0_test_exchange_delayed_pdus(&um_tx, &um_rx, 3, 200, 0, 0, INCREMENT_FRAME_YES);

    for (i = 0; i < 600; i++) {
      if ((i % 32) == 0) {
        rlc_um_v9_3_0_test_exchange_delayed_pdus(&um_tx, &um_rx, 3, 200, um_info.timer_reordering + 5, 0, INCREMENT_FRAME_YES);
      } else {
        rlc_um_v9_3_0_test_exchange_delayed_pdus(&um_tx, &um_rx, 3, 200, 0, 0, INCREMENT_FRAME_YES);
      }
    }

    rlc_um_v9_3_0_test_exchange_delayed_pdus(&um_tx, &um_rx, 2000, 200, 0, 0, INCREMENT_FRAME_YES);
    printf("g_send_id_read_index[1]=%d g_send_id_write_index[0]=%d Loop %d (2)\n", g_send_id_read_index[1], g_send_id_write_index[0], j);

    assert (g_send_id_read_index[1] != g_send_id_write_index[0]);
  }

  printf("\n\n\n\n\n\n-----------------------------------------------------------------------------------------rlc_um_v9_3_0_test_reordering 4: END OF TEST BIG SDU, SMALL PDUs\n\n\n\n");
}
//-----------------------------------------------------------------------------
void rlc_um_v9_3_0_test_tx_rx_10(void)
//-----------------------------------------------------------------------------
{
  rlc_um_info_t     um_info;
  int                   i,j,r;


  um_info.timer_reordering = 32;
  um_info.sn_field_length  = 10;
  um_info.is_mXch          = 0;

  srand (0);
  config_req_rlc_um (&um_tx, 0,0,0, &um_info, 0, SIGNALLING_RADIO_BEARER, SIGNALLING_RADIO_BEARER /*LC-id = DRB-id*/);
  config_req_rlc_um (&um_rx, 0,1,1, &um_info, 1, SIGNALLING_RADIO_BEARER, SIGNALLING_RADIO_BEARER /*LC-id = DRB-id*/);

  rlc_um_display_rx_window(&um_tx);

  rlc_um_display_rx_window(&um_rx);


#ifdef TEST1
  srand (0);
  rlc_um_v9_3_0_test_reset_sdus();
  rlc_um_v9_3_0_test_send_sdu(&um_tx, 1);
  rlc_um_v9_3_0_test_exchange_pdus(&um_tx, &um_rx, 8000, 200);
  rlc_um_v9_3_0_test_exchange_pdus(&um_tx, &um_rx, 8000, 200);
  rlc_um_v9_3_0_test_send_sdu(&um_tx, 2);
  rlc_um_v9_3_0_test_exchange_pdus(&um_tx, &um_rx, 8000, 200);
  rlc_um_v9_3_0_test_send_sdu(&um_tx, 1);
  rlc_um_v9_3_0_test_send_sdu(&um_tx, 2);
  rlc_um_v9_3_0_test_exchange_pdus(&um_tx, &um_rx, 8000, 200);
  rlc_um_v9_3_0_test_send_sdu(&um_tx, 1);
  rlc_um_v9_3_0_test_send_sdu(&um_tx, 2);
  rlc_um_v9_3_0_test_send_sdu(&um_tx, 3);
  rlc_um_v9_3_0_test_send_sdu(&um_tx, 4);
  rlc_um_v9_3_0_test_send_sdu(&um_tx, 5);
  rlc_um_v9_3_0_test_send_sdu(&um_tx, 6);
  rlc_um_v9_3_0_test_exchange_pdus(&um_tx, &um_rx, 1000, 200);
  rlc_um_v9_3_0_test_exchange_pdus(&um_tx, &um_rx, 1000, 200);
  rlc_um_v9_3_0_test_exchange_pdus(&um_tx, &um_rx, 1000, 200);
  rlc_um_v9_3_0_test_exchange_pdus(&um_tx, &um_rx, 1000, 200);
  rlc_um_v9_3_0_test_exchange_pdus(&um_tx, &um_rx, 1000, 200);

  assert (g_send_id_read_index[1] == g_send_id_write_index[0]);
  printf("\n\n\n\n\n\n-----------------------------------------------------------------------------------------rlc_um_v9_3_0_test 1: END OF SIMPLE TEST SEVERAL SDUs IN PDU\n\n\n\n");
  sleep(2);
  rlc_um_v9_3_0_test_reset_sdus();

  // RANDOM TESTS
  for (i = g_send_id_write_index[0]; g_send_id_write_index[0] < TEST_MAX_SEND_SDU-12; i++) {
    rlc_um_v9_3_0_test_send_sdu(&um_tx, 1);
    rlc_um_v9_3_0_test_exchange_pdus(&um_tx, &um_rx, 8000, 200);
    rlc_um_v9_3_0_test_exchange_pdus(&um_tx, &um_rx, 8000, 200);
    rlc_um_v9_3_0_test_send_sdu(&um_tx, 2);
    rlc_um_v9_3_0_test_exchange_pdus(&um_tx, &um_rx, 8000, 200);
    rlc_um_v9_3_0_test_send_sdu(&um_tx, 1);
    rlc_um_v9_3_0_test_send_sdu(&um_tx, 2);
    rlc_um_v9_3_0_test_exchange_pdus(&um_tx, &um_rx, 8000, 200);
    rlc_um_v9_3_0_test_send_sdu(&um_tx, 1);
    rlc_um_v9_3_0_test_send_sdu(&um_tx, 2);
    rlc_um_v9_3_0_test_send_sdu(&um_tx, 3);
    rlc_um_v9_3_0_test_send_sdu(&um_tx, 4);
    rlc_um_v9_3_0_test_send_sdu(&um_tx, 5);
    rlc_um_v9_3_0_test_send_sdu(&um_tx, 6);

    for (i = 0; i < 50; i++) {
      rlc_um_v9_3_0_test_exchange_pdus(&um_tx, &um_rx, 200, 200);
    }

    assert (g_send_id_read_index[1] == g_send_id_write_index[0]);
  }

  assert (g_send_id_read_index[1] == g_send_id_write_index[0]);
  printf("\n\n\n\n\n\n-----------------------------------------------------------------------------------------rlc_um_v9_3_0_test 1: END OF TEST SEVERAL SDUs IN PDU\n\n\n\n");
  sleep(2);
#endif
#ifdef TEST2
  srand (0);
  rlc_um_v9_3_0_test_reset_sdus();
  // BIG SDU SMALL PDUS NO ERRORS
  rlc_um_v9_3_0_test_send_sdu(&um_tx, 1);
  rlc_um_v9_3_0_test_exchange_pdus(&um_tx, &um_rx, 3, 200);
  rlc_um_v9_3_0_test_exchange_pdus(&um_tx, &um_rx, 3, 200);
  rlc_um_v9_3_0_test_exchange_pdus(&um_tx, &um_rx, 3, 200);
  rlc_um_v9_3_0_test_exchange_pdus(&um_tx, &um_rx, 4, 200);
  rlc_um_v9_3_0_test_exchange_pdus(&um_tx, &um_rx, 5, 200);
  rlc_um_v9_3_0_test_exchange_pdus(&um_tx, &um_rx, 6, 200);
  rlc_um_v9_3_0_test_exchange_pdus(&um_tx, &um_rx, 7, 200);
  rlc_um_v9_3_0_test_exchange_pdus(&um_tx, &um_rx, 8, 200);
  rlc_um_v9_3_0_test_exchange_pdus(&um_tx, &um_rx, 9, 200);
  rlc_um_v9_3_0_test_exchange_pdus(&um_tx, &um_rx, 10, 200);
  rlc_um_v9_3_0_test_exchange_pdus(&um_tx, &um_rx, 11, 200);
  rlc_um_v9_3_0_test_exchange_pdus(&um_tx, &um_rx, 12, 200);
  rlc_um_v9_3_0_test_exchange_pdus(&um_tx, &um_rx, 13, 200);
  rlc_um_v9_3_0_test_exchange_pdus(&um_tx, &um_rx, 14, 200);
  rlc_um_v9_3_0_test_exchange_pdus(&um_tx, &um_rx, 15, 200);
  rlc_um_v9_3_0_test_exchange_pdus(&um_tx, &um_rx, 16, 200);
  rlc_um_v9_3_0_test_exchange_pdus(&um_tx, &um_rx, 17, 200);
  rlc_um_v9_3_0_test_exchange_pdus(&um_tx, &um_rx, 18, 200);
  rlc_um_v9_3_0_test_exchange_pdus(&um_tx, &um_rx, 19, 200);
  rlc_um_v9_3_0_test_exchange_pdus(&um_tx, &um_rx, 20, 200);
  rlc_um_v9_3_0_test_exchange_pdus(&um_tx, &um_rx, 21, 200);
  rlc_um_v9_3_0_test_exchange_pdus(&um_tx, &um_rx, 22, 200);
  rlc_um_v9_3_0_test_exchange_pdus(&um_tx, &um_rx, 23, 200);
  rlc_um_v9_3_0_test_exchange_pdus(&um_tx, &um_rx, 24, 200);
  rlc_um_v9_3_0_test_exchange_pdus(&um_tx, &um_rx, 25, 200);
  rlc_um_v9_3_0_test_exchange_pdus(&um_tx, &um_rx, 26, 200);
  rlc_um_v9_3_0_test_exchange_pdus(&um_tx, &um_rx, 27, 200);
  rlc_um_v9_3_0_test_exchange_pdus(&um_tx, &um_rx, 28, 200);
  rlc_um_v9_3_0_test_exchange_pdus(&um_tx, &um_rx, 29, 200);
  rlc_um_v9_3_0_test_exchange_pdus(&um_tx, &um_rx, 2000, 200);
  rlc_um_v9_3_0_test_exchange_pdus(&um_tx, &um_rx, 2000, 200);
  rlc_um_v9_3_0_test_exchange_pdus(&um_tx, &um_rx, 2000, 200);
  rlc_um_v9_3_0_test_exchange_pdus(&um_tx, &um_rx, 2000, 200);

  rlc_um_v9_3_0_test_send_sdu(&um_tx, 1);
  rlc_um_v9_3_0_test_exchange_pdus(&um_tx, &um_rx, 30, 200);
  rlc_um_v9_3_0_test_exchange_pdus(&um_tx, &um_rx, 31, 200);
  rlc_um_v9_3_0_test_exchange_pdus(&um_tx, &um_rx, 32, 200);
  rlc_um_v9_3_0_test_exchange_pdus(&um_tx, &um_rx, 33, 200);
  rlc_um_v9_3_0_test_exchange_pdus(&um_tx, &um_rx, 34, 200);
  rlc_um_v9_3_0_test_exchange_pdus(&um_tx, &um_rx, 35, 200);
  rlc_um_v9_3_0_test_exchange_pdus(&um_tx, &um_rx, 36, 200);
  rlc_um_v9_3_0_test_exchange_pdus(&um_tx, &um_rx, 37, 200);
  rlc_um_v9_3_0_test_exchange_pdus(&um_tx, &um_rx, 38, 200);
  rlc_um_v9_3_0_test_exchange_pdus(&um_tx, &um_rx, 39, 200);
  rlc_um_v9_3_0_test_exchange_pdus(&um_tx, &um_rx, 40, 200);
  rlc_um_v9_3_0_test_exchange_pdus(&um_tx, &um_rx, 41, 200);
  rlc_um_v9_3_0_test_exchange_pdus(&um_tx, &um_rx, 42, 200);
  rlc_um_v9_3_0_test_exchange_pdus(&um_tx, &um_rx, 43, 200);
  rlc_um_v9_3_0_test_exchange_pdus(&um_tx, &um_rx, 44, 200);
  rlc_um_v9_3_0_test_exchange_pdus(&um_tx, &um_rx, 45, 200);
  rlc_um_v9_3_0_test_exchange_pdus(&um_tx, &um_rx, 46, 200);
  rlc_um_v9_3_0_test_exchange_pdus(&um_tx, &um_rx, 47, 200);
  rlc_um_v9_3_0_test_exchange_pdus(&um_tx, &um_rx, 48, 200);
  rlc_um_v9_3_0_test_exchange_pdus(&um_tx, &um_rx, 49, 200);
  rlc_um_v9_3_0_test_exchange_pdus(&um_tx, &um_rx, 50, 200);
  rlc_um_v9_3_0_test_exchange_pdus(&um_tx, &um_rx, 51, 200);
  rlc_um_v9_3_0_test_exchange_pdus(&um_tx, &um_rx, 52, 200);
  rlc_um_v9_3_0_test_exchange_pdus(&um_tx, &um_rx, 53, 200);
  rlc_um_v9_3_0_test_exchange_pdus(&um_tx, &um_rx, 54, 200);
  rlc_um_v9_3_0_test_exchange_pdus(&um_tx, &um_rx, 55, 200);
  rlc_um_v9_3_0_test_exchange_pdus(&um_tx, &um_rx, 56, 200);
  rlc_um_v9_3_0_test_exchange_pdus(&um_tx, &um_rx, 57, 200);
  rlc_um_v9_3_0_test_exchange_pdus(&um_tx, &um_rx, 58, 200);
  rlc_um_v9_3_0_test_exchange_pdus(&um_tx, &um_rx, 59, 200);

  //rlc_um_v9_3_0_test_exchange_pdus(&um_tx, &um_rx, 60, 200); if uncomment: error because too many segments of SDU
  for (i = 0; i < 24; i++) {
    rlc_um_v9_3_0_test_exchange_pdus(&um_tx, &um_rx, 2000, 200);
  }

  assert (g_send_id_read_index[1] == g_send_id_write_index[0]);
  printf("\n\n\n\n\n\n-----------------------------------------------------------------------------------------rlc_um_v9_3_0_test 2: END OF TEST BIG SDU SMALL PDUs\n\n\n\n");
#endif

  for (r = 0; r < 32; r++) {
    //for (r = 0; r < 1024; r++) {
    srand (r);
#ifdef TEST3
    g_error_on_phy = 0;
    g_tx_packets = 0;
    g_rx_packets = 0;
    rlc_um_v9_3_0_test_reset_sdus();

    // RANDOM TESTS
    for (i = g_send_id_write_index[0]; g_send_id_write_index[0] < TEST_MAX_SEND_SDU-1; i++) {
      printf("UM.TX SDU %d\n", um_tx.nb_sdu);

      if (um_tx.nb_sdu < (um_tx.size_input_sdus_buffer - 2)) {
        g_random_sdu = rand() % 37;
        rlc_um_v9_3_0_test_send_sdu(&um_tx, g_random_sdu);
      }

      g_random_nb_frames   = (rand() % 10) + 1;

      //g_random_nb_frames   = 1;
      for (j = 0; j < g_random_nb_frames; j++) {
        g_random_tx_pdu_size = (rand() % RLC_SDU_MAX_SIZE)  / ((rand () % 4)+1);
        g_random_rx_pdu_size = (rand() % RLC_SDU_MAX_SIZE)  / ((rand () % 4)+1);
        rlc_um_v9_3_0_test_exchange_pdus(&um_tx, &um_rx, g_random_tx_pdu_size, g_random_rx_pdu_size);
      }
    }

    for (j = 0; j < 400; j++) {
      rlc_um_v9_3_0_test_exchange_pdus(&um_tx, &um_rx, 500, 500);
    }

    printf("\n\n\n\n\n\n-----------------------------------------------------------------------------------------rlc_um_v9_3_0_test 3: END OF TEST RANDOM (SEED=%d ) TX ONLY :\n\n\n\n",
           r);
    assert (g_send_id_read_index[1] == g_send_id_write_index[0]);
#endif
#ifdef TEST4
    g_error_on_phy = 0;
    g_tx_packets = 0;
    g_rx_packets = 0;
    rlc_um_v9_3_0_test_reset_sdus();

    for (i = g_send_id_write_index[0]; g_send_id_write_index[0] < TEST_MAX_SEND_SDU-1; i++) {
      if (um_tx.nb_sdu < (um_rx.size_input_sdus_buffer - 2)) {
        g_random_sdu = rand() % 37;
        rlc_um_v9_3_0_test_send_sdu(&um_tx, g_random_sdu);

        if (um_rx.nb_sdu < (um_rx.size_input_sdus_buffer - 2)) {
          g_random_sdu = rand() % 37;
          rlc_um_v9_3_0_test_send_sdu(&um_rx, g_random_sdu);
        } else {
          i = i-1;
        }
      } else {
        if (um_rx.nb_sdu < (um_rx.size_input_sdus_buffer - 2)) {
          g_random_sdu = rand() % 37;
          rlc_um_v9_3_0_test_send_sdu(&um_rx, g_random_sdu);
        } else {
          i = i-1;
        }
      }

      g_random_nb_frames   = rand() % 4;

      for (j = 0; j < g_random_nb_frames; j++) {
        g_random_tx_pdu_size = (rand() % RLC_SDU_MAX_SIZE)  / ((rand () % 4)+1);
        g_random_rx_pdu_size = (rand() % RLC_SDU_MAX_SIZE)  / ((rand () % 4)+1);
        rlc_um_v9_3_0_test_exchange_pdus(&um_tx, &um_rx, g_random_tx_pdu_size, g_random_rx_pdu_size);
      }
    }

    for (j = 0; j < 100; j++) {
      g_random_tx_pdu_size = (rand() % RLC_SDU_MAX_SIZE)  / ((rand () % 4)+1);
      g_random_rx_pdu_size = (rand() % RLC_SDU_MAX_SIZE)  / ((rand () % 4)+1);
      rlc_um_v9_3_0_test_exchange_pdus(&um_tx, &um_rx, g_random_tx_pdu_size, g_random_rx_pdu_size);
    }

    printf("\n\n\n\n\n\n-----------------------------------------------------------------------------------------rlc_um_v9_3_0_test 4: END OF TEST RANDOM (SEED=%d) TX RX:\n\n\n\n",r);
    assert (g_send_id_read_index[1] == g_send_id_write_index[0]);
    assert (g_send_id_read_index[0] == g_send_id_write_index[1]);
#endif
  }

#ifdef TEST5
  rlc_um_display_rx_window(&um_tx);
  rlc_um_display_rx_window(&um_rx);

  for (r = 0; r < 1024; r++) {
    srand (r);
    g_error_on_phy = 1;

    for (g_target_tx_error_rate = 0; g_target_tx_error_rate < TARGET_MAX_TX_ERROR_RATE; g_target_tx_error_rate++) {
      for (g_target_rx_error_rate = 0; g_target_rx_error_rate < TARGET_MAX_RX_ERROR_RATE; g_target_rx_error_rate++) {
        g_tx_packets = 0;
        g_dropped_tx_packets = 0;
        g_rx_packets = 0;
        g_dropped_rx_packets = 0;
        rlc_um_v9_3_0_test_reset_sdus();

        for (i = g_send_id_write_index[0]; g_send_id_write_index[0] < TEST_MAX_SEND_SDU-1; i++) {
          g_random_sdu = rand() % 37;
          rlc_um_v9_3_0_test_send_sdu(&um_tx, g_random_sdu);
          g_random_sdu = rand() % 37;
          //rlc_um_v9_3_0_test_send_sdu(&um_rx, g_random_sdu);


          g_random_nb_frames   = rand() % 4;

          for (j = 0; j < g_random_nb_frames; j++) {
            g_random_tx_pdu_size = (rand() % RLC_SDU_MAX_SIZE)  / ((rand () % 4)+1);
            g_random_rx_pdu_size = (rand() % RLC_SDU_MAX_SIZE)  / ((rand () % 4)+1);
            rlc_um_v9_3_0_test_exchange_pdus(&um_tx, &um_rx, g_random_tx_pdu_size, g_random_rx_pdu_size);
          }

          //rlc_um_display_rx_window(&um_tx);
          rlc_um_display_rx_window(&um_rx);

          int dropped = (rand() % 3);

          if ((dropped == 0) && (g_tx_packets > 0)) {
            if ((((g_dropped_tx_packets + 1)*100) / g_tx_packets) <= g_target_tx_error_rate) {
              g_drop_tx = 1;
            }
          }

          dropped = (rand() % 3);

          if ((dropped == 0) && (g_rx_packets > 0)) {
            if ((((g_dropped_rx_packets + 1)*100) / g_rx_packets) <= g_target_rx_error_rate) {
              g_drop_rx = 1;
            }
          }
        }

        for (j = 0; j < 100; j++) {
          g_random_tx_pdu_size = (rand() % RLC_SDU_MAX_SIZE)  / ((rand () % 4)+1);
          g_random_rx_pdu_size = (rand() % RLC_SDU_MAX_SIZE)  / ((rand () % 4)+1);
          rlc_um_v9_3_0_test_exchange_pdus(&um_tx, &um_rx, g_random_tx_pdu_size, g_random_rx_pdu_size);
        }

        printf("\n\n\n\n\n\n-----------------------------------------------------------------------------------------rlc_um_v9_3_0_test 5: END OF TEST RANDOM (SEED=%d BLER TX=%d BLER RX=%d ) TX RX WITH ERRORS ON PHY LAYER:\n\n\n\n",
               r, g_target_tx_error_rate, g_target_rx_error_rate);
        //assert (g_send_id_read_index[1] == g_send_id_write_index[0]);
        //assert (g_send_id_read_index[0] == g_send_id_write_index[1]);
        printf("REAL BLER TX=%d (TARGET=%d) BLER RX=%d (TARGET=%d) \n",
               (g_tx_packets >0)?(g_dropped_tx_packets*100)/g_tx_packets:0,
               g_target_tx_error_rate,
               (g_rx_packets >0)?(g_dropped_rx_packets*100)/g_rx_packets:0,
               g_target_rx_error_rate);

      }
    }

    g_drop_tx = 0;
    g_drop_rx = 0;
    g_tx_packets = 0;
    g_dropped_tx_packets = 0;
    g_rx_packets = 0;
    g_dropped_rx_packets = 0;
    rlc_um_v9_3_0_test_reset_sdus();

    for (j = 0; j < 100; j++) {
      rlc_um_v9_3_0_test_send_sdu(&um_rx, 1);
      g_random_tx_pdu_size = (rand() % RLC_SDU_MAX_SIZE)  / ((rand () % 4)+1);
      g_random_rx_pdu_size = (rand() % RLC_SDU_MAX_SIZE)  / ((rand () % 4)+1);
      rlc_um_v9_3_0_test_exchange_pdus(&um_tx, &um_rx, g_random_tx_pdu_size, g_random_rx_pdu_size);
    }
  }

  g_error_on_phy = 0;
#endif
}
//-----------------------------------------------------------------------------
void rlc_um_v9_3_0_test_tx_rx_5(void)
//-----------------------------------------------------------------------------
{
  rlc_um_info_t     um_info;
  int                   i,j,r;


  um_info.timer_reordering = 32;
  um_info.sn_field_length  = 5;
  um_info.is_mXch          = 0;

  srand (0);
  config_req_rlc_um (&um_tx, 0,0,0, &um_info, 0, SIGNALLING_RADIO_BEARER, SIGNALLING_RADIO_BEARER /*LC-id = DRB-id*/);
  config_req_rlc_um (&um_rx, 0,1,1, &um_info, 1, SIGNALLING_RADIO_BEARER, SIGNALLING_RADIO_BEARER /*LC-id = DRB-id*/);

  rlc_um_display_rx_window(&um_tx);

  rlc_um_display_rx_window(&um_rx);


#ifdef TEST1
  srand (0);
  printf("\n\n\n\n\n\n-----------------------------------------------------------------------------------------rlc_um_v9_3_0_test_5 1: START OF SIMPLE TEST SEVERAL SDUs IN PDU\n\n\n\n");
  rlc_um_v9_3_0_test_reset_sdus();
  rlc_um_v9_3_0_test_send_sdu(&um_tx, 1);
  rlc_um_v9_3_0_test_exchange_pdus(&um_tx, &um_rx, 8000, 200);
  rlc_um_v9_3_0_test_exchange_pdus(&um_tx, &um_rx, 8000, 200);
  rlc_um_v9_3_0_test_send_sdu(&um_tx, 2);
  rlc_um_v9_3_0_test_exchange_pdus(&um_tx, &um_rx, 8000, 200);
  rlc_um_v9_3_0_test_send_sdu(&um_tx, 1);
  rlc_um_v9_3_0_test_send_sdu(&um_tx, 2);
  rlc_um_v9_3_0_test_exchange_pdus(&um_tx, &um_rx, 8000, 200);
  rlc_um_v9_3_0_test_send_sdu(&um_tx, 1);
  rlc_um_v9_3_0_test_send_sdu(&um_tx, 2);
  rlc_um_v9_3_0_test_send_sdu(&um_tx, 3);
  rlc_um_v9_3_0_test_send_sdu(&um_tx, 4);
  rlc_um_v9_3_0_test_send_sdu(&um_tx, 5);
  rlc_um_v9_3_0_test_send_sdu(&um_tx, 6);
  rlc_um_v9_3_0_test_exchange_pdus(&um_tx, &um_rx, 1000, 200);
  rlc_um_v9_3_0_test_exchange_pdus(&um_tx, &um_rx, 1000, 200);
  rlc_um_v9_3_0_test_exchange_pdus(&um_tx, &um_rx, 1000, 200);
  rlc_um_v9_3_0_test_exchange_pdus(&um_tx, &um_rx, 1000, 200);
  rlc_um_v9_3_0_test_exchange_pdus(&um_tx, &um_rx, 1000, 200);

  assert (g_send_id_read_index[1] == g_send_id_write_index[0]);
  printf("\n\n\n\n\n\n-----------------------------------------------------------------------------------------rlc_um_v9_3_0_test_5 1: END OF SIMPLE TEST SEVERAL SDUs IN PDU\n\n\n\n");
  sleep(2);
  rlc_um_v9_3_0_test_reset_sdus();

  // RANDOM TESTS
  for (i = g_send_id_write_index[0]; g_send_id_write_index[0] < TEST_MAX_SEND_SDU-12; i++) {
    rlc_um_v9_3_0_test_send_sdu(&um_tx, 1);
    rlc_um_v9_3_0_test_exchange_pdus(&um_tx, &um_rx, 8000, 200);
    rlc_um_v9_3_0_test_exchange_pdus(&um_tx, &um_rx, 8000, 200);
    rlc_um_v9_3_0_test_send_sdu(&um_tx, 2);
    rlc_um_v9_3_0_test_exchange_pdus(&um_tx, &um_rx, 8000, 200);
    rlc_um_v9_3_0_test_send_sdu(&um_tx, 1);
    rlc_um_v9_3_0_test_send_sdu(&um_tx, 2);
    rlc_um_v9_3_0_test_exchange_pdus(&um_tx, &um_rx, 8000, 200);
    rlc_um_v9_3_0_test_send_sdu(&um_tx, 1);
    rlc_um_v9_3_0_test_send_sdu(&um_tx, 2);
    rlc_um_v9_3_0_test_send_sdu(&um_tx, 3);
    rlc_um_v9_3_0_test_send_sdu(&um_tx, 4);
    rlc_um_v9_3_0_test_send_sdu(&um_tx, 5);
    rlc_um_v9_3_0_test_send_sdu(&um_tx, 6);

    for (i = 0; i < 50; i++) {
      rlc_um_v9_3_0_test_exchange_pdus(&um_tx, &um_rx, 400, 200);
    }

    assert (g_send_id_read_index[1] == g_send_id_write_index[0]);
  }

  assert (g_send_id_read_index[1] == g_send_id_write_index[0]);
  printf("\n\n\n\n\n\n-----------------------------------------------------------------------------------------rlc_um_v9_3_0_test 1: END OF TEST SEVERAL SDUs IN PDU\n\n\n\n");
  sleep(2);
#endif
#ifdef TEST2
  srand (0);
  rlc_um_v9_3_0_test_reset_sdus();
  // BIG SDU SMALL PDUS NO ERRORS
  rlc_um_v9_3_0_test_send_sdu(&um_tx, 1);
  rlc_um_v9_3_0_test_exchange_pdus(&um_tx, &um_rx, 3, 200);
  rlc_um_v9_3_0_test_exchange_pdus(&um_tx, &um_rx, 3, 200);
  rlc_um_v9_3_0_test_exchange_pdus(&um_tx, &um_rx, 3, 200);
  rlc_um_v9_3_0_test_exchange_pdus(&um_tx, &um_rx, 4, 200);
  rlc_um_v9_3_0_test_exchange_pdus(&um_tx, &um_rx, 5, 200);
  rlc_um_v9_3_0_test_exchange_pdus(&um_tx, &um_rx, 6, 200);
  rlc_um_v9_3_0_test_exchange_pdus(&um_tx, &um_rx, 7, 200);
  rlc_um_v9_3_0_test_exchange_pdus(&um_tx, &um_rx, 8, 200);
  rlc_um_v9_3_0_test_exchange_pdus(&um_tx, &um_rx, 9, 200);
  rlc_um_v9_3_0_test_exchange_pdus(&um_tx, &um_rx, 10, 200);
  rlc_um_v9_3_0_test_exchange_pdus(&um_tx, &um_rx, 11, 200);
  rlc_um_v9_3_0_test_exchange_pdus(&um_tx, &um_rx, 12, 200);
  rlc_um_v9_3_0_test_exchange_pdus(&um_tx, &um_rx, 13, 200);
  rlc_um_v9_3_0_test_exchange_pdus(&um_tx, &um_rx, 14, 200);
  rlc_um_v9_3_0_test_exchange_pdus(&um_tx, &um_rx, 15, 200);
  rlc_um_v9_3_0_test_exchange_pdus(&um_tx, &um_rx, 16, 200);
  rlc_um_v9_3_0_test_exchange_pdus(&um_tx, &um_rx, 17, 200);
  rlc_um_v9_3_0_test_exchange_pdus(&um_tx, &um_rx, 18, 200);
  rlc_um_v9_3_0_test_exchange_pdus(&um_tx, &um_rx, 19, 200);
  rlc_um_v9_3_0_test_exchange_pdus(&um_tx, &um_rx, 20, 200);
  rlc_um_v9_3_0_test_exchange_pdus(&um_tx, &um_rx, 21, 200);
  rlc_um_v9_3_0_test_exchange_pdus(&um_tx, &um_rx, 22, 200);
  rlc_um_v9_3_0_test_exchange_pdus(&um_tx, &um_rx, 23, 200);
  rlc_um_v9_3_0_test_exchange_pdus(&um_tx, &um_rx, 24, 200);
  rlc_um_v9_3_0_test_exchange_pdus(&um_tx, &um_rx, 25, 200);
  rlc_um_v9_3_0_test_exchange_pdus(&um_tx, &um_rx, 2000, 200);
  rlc_um_v9_3_0_test_exchange_pdus(&um_tx, &um_rx, 2000, 200);
  rlc_um_v9_3_0_test_exchange_pdus(&um_tx, &um_rx, 2000, 200);
  rlc_um_v9_3_0_test_exchange_pdus(&um_tx, &um_rx, 2000, 200);

  rlc_um_v9_3_0_test_send_sdu(&um_tx, 1);
  rlc_um_v9_3_0_test_exchange_pdus(&um_tx, &um_rx, 26, 200);
  rlc_um_v9_3_0_test_exchange_pdus(&um_tx, &um_rx, 27, 200);
  rlc_um_v9_3_0_test_exchange_pdus(&um_tx, &um_rx, 28, 200);
  rlc_um_v9_3_0_test_exchange_pdus(&um_tx, &um_rx, 29, 200);
  rlc_um_v9_3_0_test_exchange_pdus(&um_tx, &um_rx, 30, 200);
  rlc_um_v9_3_0_test_exchange_pdus(&um_tx, &um_rx, 31, 200);
  rlc_um_v9_3_0_test_exchange_pdus(&um_tx, &um_rx, 32, 200);
  rlc_um_v9_3_0_test_exchange_pdus(&um_tx, &um_rx, 33, 200);
  rlc_um_v9_3_0_test_exchange_pdus(&um_tx, &um_rx, 34, 200);
  rlc_um_v9_3_0_test_exchange_pdus(&um_tx, &um_rx, 35, 200);
  rlc_um_v9_3_0_test_exchange_pdus(&um_tx, &um_rx, 36, 200);
  rlc_um_v9_3_0_test_exchange_pdus(&um_tx, &um_rx, 37, 200);
  rlc_um_v9_3_0_test_exchange_pdus(&um_tx, &um_rx, 38, 200);
  rlc_um_v9_3_0_test_exchange_pdus(&um_tx, &um_rx, 39, 200);
  rlc_um_v9_3_0_test_exchange_pdus(&um_tx, &um_rx, 40, 200);
  rlc_um_v9_3_0_test_exchange_pdus(&um_tx, &um_rx, 41, 200);
  rlc_um_v9_3_0_test_exchange_pdus(&um_tx, &um_rx, 42, 200);
  rlc_um_v9_3_0_test_exchange_pdus(&um_tx, &um_rx, 43, 200);
  rlc_um_v9_3_0_test_exchange_pdus(&um_tx, &um_rx, 44, 200);
  rlc_um_v9_3_0_test_exchange_pdus(&um_tx, &um_rx, 45, 200);
  rlc_um_v9_3_0_test_exchange_pdus(&um_tx, &um_rx, 46, 200);
  rlc_um_v9_3_0_test_exchange_pdus(&um_tx, &um_rx, 47, 200);
  rlc_um_v9_3_0_test_exchange_pdus(&um_tx, &um_rx, 48, 200);

  //rlc_um_v9_3_0_test_exchange_pdus(&um_tx, &um_rx, 60, 200); if uncomment: error because too many segments of SDU
  for (i = 0; i < 24; i++) {
    rlc_um_v9_3_0_test_exchange_pdus(&um_tx, &um_rx, 2000, 200);
  }

  assert (g_send_id_read_index[1] == g_send_id_write_index[0]);
  printf("\n\n\n\n\n\n-----------------------------------------------------------------------------------------rlc_um_v9_3_0_test 2: END OF TEST BIG SDU SMALL PDUs\n\n\n\n");
#endif

  for (r = 0; r < 32; r++) {
    //for (r = 0; r < 1024; r++) {
    srand (r);
#ifdef TEST3
    g_error_on_phy = 0;
    g_tx_packets = 0;
    g_rx_packets = 0;
    rlc_um_v9_3_0_test_reset_sdus();

    // RANDOM TESTS
    for (i = g_send_id_write_index[0]; g_send_id_write_index[0] < TEST_MAX_SEND_SDU-1; i++) {
      printf("UM.TX SDU %d\n", um_tx.nb_sdu);

      if (um_tx.nb_sdu < (um_tx.size_input_sdus_buffer - 2)) {
        g_random_sdu = rand() % 37;
        rlc_um_v9_3_0_test_send_sdu(&um_tx, g_random_sdu);
      }

      g_random_nb_frames   = (rand() % 10) + 1;

      //g_random_nb_frames   = 1;
      for (j = 0; j < g_random_nb_frames; j++) {
        g_random_tx_pdu_size = (rand() % RLC_SDU_MAX_SIZE)  / ((rand () % 4)+1);
        g_random_rx_pdu_size = (rand() % RLC_SDU_MAX_SIZE)  / ((rand () % 4)+1);
        rlc_um_v9_3_0_test_exchange_pdus(&um_tx, &um_rx, g_random_tx_pdu_size, g_random_rx_pdu_size);
      }
    }

    for (j = 0; j < 400; j++) {
      rlc_um_v9_3_0_test_exchange_pdus(&um_tx, &um_rx, 500, 500);
    }

    printf("\n\n\n\n\n\n-----------------------------------------------------------------------------------------rlc_um_v9_3_0_test 3: END OF TEST RANDOM (SEED=%d ) TX ONLY :\n\n\n\n",r);
    assert (g_send_id_read_index[1] == g_send_id_write_index[0]);
#endif
#ifdef TEST4
    g_error_on_phy = 0;
    g_tx_packets = 0;
    g_rx_packets = 0;
    rlc_um_v9_3_0_test_reset_sdus();

    for (i = g_send_id_write_index[0]; g_send_id_write_index[0] < TEST_MAX_SEND_SDU-1; i++) {
      if (um_tx.nb_sdu < (um_rx.size_input_sdus_buffer - 2)) {
        g_random_sdu = rand() % 37;
        rlc_um_v9_3_0_test_send_sdu(&um_tx, g_random_sdu);

        if (um_rx.nb_sdu < (um_rx.size_input_sdus_buffer - 2)) {
          g_random_sdu = rand() % 37;
          rlc_um_v9_3_0_test_send_sdu(&um_rx, g_random_sdu);
        } else {
          i = i-1;
        }
      } else {
        if (um_rx.nb_sdu < (um_rx.size_input_sdus_buffer - 2)) {
          g_random_sdu = rand() % 37;
          rlc_um_v9_3_0_test_send_sdu(&um_rx, g_random_sdu);
        } else {
          i = i-1;
        }
      }

      g_random_nb_frames   = rand() % 4;

      for (j = 0; j < g_random_nb_frames; j++) {
        g_random_tx_pdu_size = (rand() % RLC_SDU_MAX_SIZE)  / ((rand () % 4)+1);
        g_random_rx_pdu_size = (rand() % RLC_SDU_MAX_SIZE)  / ((rand () % 4)+1);
        rlc_um_v9_3_0_test_exchange_pdus(&um_tx, &um_rx, g_random_tx_pdu_size, g_random_rx_pdu_size);
      }
    }

    for (j = 0; j < 100; j++) {
      g_random_tx_pdu_size = (rand() % RLC_SDU_MAX_SIZE)  / ((rand () % 4)+1);
      g_random_rx_pdu_size = (rand() % RLC_SDU_MAX_SIZE)  / ((rand () % 4)+1);
      rlc_um_v9_3_0_test_exchange_pdus(&um_tx, &um_rx, g_random_tx_pdu_size, g_random_rx_pdu_size);
    }

    printf("\n\n\n\n\n\n-----------------------------------------------------------------------------------------rlc_um_v9_3_0_test 4: END OF TEST RANDOM (SEED=%d) TX RX:\n\n\n\n",r);
    assert (g_send_id_read_index[1] == g_send_id_write_index[0]);
    assert (g_send_id_read_index[0] == g_send_id_write_index[1]);
#endif
  }

#ifdef TEST5
  rlc_um_display_rx_window(&um_tx);
  rlc_um_display_rx_window(&um_rx);

  for (r = 0; r < 1024; r++) {
    srand (r);
    g_error_on_phy = 1;

    for (g_target_tx_error_rate = 0; g_target_tx_error_rate < TARGET_MAX_TX_ERROR_RATE; g_target_tx_error_rate++) {
      for (g_target_rx_error_rate = 0; g_target_rx_error_rate < TARGET_MAX_RX_ERROR_RATE; g_target_rx_error_rate++) {
        g_tx_packets = 0;
        g_dropped_tx_packets = 0;
        g_rx_packets = 0;
        g_dropped_rx_packets = 0;
        rlc_um_v9_3_0_test_reset_sdus();

        for (i = g_send_id_write_index[0]; g_send_id_write_index[0] < TEST_MAX_SEND_SDU-1; i++) {
          g_random_sdu = rand() % 37;
          rlc_um_v9_3_0_test_send_sdu(&um_tx, g_random_sdu);
          g_random_sdu = rand() % 37;
          //rlc_um_v9_3_0_test_send_sdu(&um_rx, g_random_sdu);


          g_random_nb_frames   = rand() % 4;

          for (j = 0; j < g_random_nb_frames; j++) {
            g_random_tx_pdu_size = (rand() % RLC_SDU_MAX_SIZE)  / ((rand () % 4)+1);
            g_random_rx_pdu_size = (rand() % RLC_SDU_MAX_SIZE)  / ((rand () % 4)+1);
            rlc_um_v9_3_0_test_exchange_pdus(&um_tx, &um_rx, g_random_tx_pdu_size, g_random_rx_pdu_size);
          }

          //rlc_um_display_rx_window(&um_tx);
          rlc_um_display_rx_window(&um_rx);

          int dropped = (rand() % 3);

          if ((dropped == 0) && (g_tx_packets > 0)) {
            if ((((g_dropped_tx_packets + 1)*100) / g_tx_packets) <= g_target_tx_error_rate) {
              g_drop_tx = 1;
            }
          }

          dropped = (rand() % 3);

          if ((dropped == 0) && (g_rx_packets > 0)) {
            if ((((g_dropped_rx_packets + 1)*100) / g_rx_packets) <= g_target_rx_error_rate) {
              g_drop_rx = 1;
            }
          }
        }

        for (j = 0; j < 100; j++) {
          g_random_tx_pdu_size = (rand() % RLC_SDU_MAX_SIZE)  / ((rand () % 4)+1);
          g_random_rx_pdu_size = (rand() % RLC_SDU_MAX_SIZE)  / ((rand () % 4)+1);
          rlc_um_v9_3_0_test_exchange_pdus(&um_tx, &um_rx, g_random_tx_pdu_size, g_random_rx_pdu_size);
        }

        printf("\n\n\n\n\n\n-----------------------------------------------------------------------------------------rlc_um_v9_3_0_test 5: END OF TEST RANDOM (SEED=%d BLER TX=%d BLER RX=%d ) TX RX WITH ERRORS ON PHY LAYER:\n\n\n\n",
               r, g_target_tx_error_rate, g_target_rx_error_rate);
        //assert (g_send_id_read_index[1] == g_send_id_write_index[0]);
        //assert (g_send_id_read_index[0] == g_send_id_write_index[1]);
        printf("REAL BLER TX=%d (TARGET=%d) BLER RX=%d (TARGET=%d) \n",
               (g_tx_packets >0)?(g_dropped_tx_packets*100)/g_tx_packets:0,
               g_target_tx_error_rate,
               (g_rx_packets >0)?(g_dropped_rx_packets*100)/g_rx_packets:0,
               g_target_rx_error_rate);

      }
    }

    g_drop_tx = 0;
    g_drop_rx = 0;
    g_tx_packets = 0;
    g_dropped_tx_packets = 0;
    g_rx_packets = 0;
    g_dropped_rx_packets = 0;
    rlc_um_v9_3_0_test_reset_sdus();

    for (j = 0; j < 100; j++) {
      rlc_um_v9_3_0_test_send_sdu(&um_rx, 1);
      g_random_tx_pdu_size = (rand() % RLC_SDU_MAX_SIZE)  / ((rand () % 4)+1);
      g_random_rx_pdu_size = (rand() % RLC_SDU_MAX_SIZE)  / ((rand () % 4)+1);
      rlc_um_v9_3_0_test_exchange_pdus(&um_tx, &um_rx, g_random_tx_pdu_size, g_random_rx_pdu_size);
    }
  }

  g_error_on_phy = 0;
#endif
}

//-----------------------------------------------------------------------------
void rlc_um_v9_3_0_test_print_trace (void)
//-----------------------------------------------------------------------------
{
  void *array[100];
  size_t size;
  char **strings;
  size_t i;

  size = backtrace (array, 100);
  strings = backtrace_symbols (array, size);

  printf ("Obtained %d stack frames.\n", size);

  for (i = 0; i < size; i++) {
    printf ("%s\n", strings[i]);
  }

  free (strings);
}
//-----------------------------------------------------------------------------
void rlc_um_v9_3_0_test(void)
//-----------------------------------------------------------------------------
{
  pool_buffer_init();

  set_comp_log(RLC, LOG_TRACE, LOG_MED, 1);


  // tested OK
  rlc_um_v9_3_0_test_reordering(10);

  rlc_um_v9_3_0_test_tx_rx_10();

  // tested OK
  rlc_um_v9_3_0_test_windows_10();

  rlc_um_v9_3_0_test_tx_rx_5();
  rlc_um_v9_3_0_test_windows_5();
  rlc_um_v9_3_0_test_reordering(5);


  printf("rlc_um_v9_3_0_test: END OF TESTS\n");
  exit(0);
}
