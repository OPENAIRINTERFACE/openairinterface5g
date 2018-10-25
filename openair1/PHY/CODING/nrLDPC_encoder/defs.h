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

/*!\file defs.h
 * \brief LDPC encoder forward declarations
 * \author Florian Kaltenberger, Raymond Knopp, Kien le Trung (Eurecom)
 * \email openair_tech@eurecom.fr
 * \date 27-03-2018
 * \version 1.0
 * \note
 * \warning
 */

#include "PHY/TOOLS/time_meas.h"

/*ldpc_encoder.c*/
int encode_parity_check_part_orig(unsigned char *c,unsigned char *d, short BG,short Zc,short Kb,short block_length);

/*ldpc_encoder2.c*/
void encode_parity_check_part_optim(uint8_t *c,uint8_t *d, short BG,short Zc,short Kb);
int ldpc_encoder_optim(unsigned char *test_input,unsigned char *channel_input,short block_length,short BG,time_stats_t *tinput,time_stats_t *tprep,time_stats_t *tparity,time_stats_t *toutput);
int ldpc_encoder_optim_8seg(unsigned char **test_input,unsigned char **channel_input,short block_length,short BG,int n_segments,time_stats_t *tinput,time_stats_t *tprep,time_stats_t *tparity,time_stats_t *toutput);

/*ldpc_generate_coefficient.c*/
int ldpc_encoder_orig(unsigned char *test_input,unsigned char *channel_input,short block_length,short BG,unsigned char gen_code);

/*
int encode_parity_check_part(unsigned char *c,unsigned char *d, short BG,short Zc,short Kb);
int encode_parity_check_part_orig(unsigned char *c,unsigned char *d, short BG,short Zc,short Kb,short block_length);
int ldpc_encoder(unsigned char *test_input,unsigned char *channel_input,short block_length, double rate);
int ldpc_encoder_orig(unsigned char *test_input,unsigned char *channel_input,short block_length,int nom_rate,int denom_rate,unsigned char gen_code);
int ldpc_encoder_multi_segment(unsigned char **test_input,unsigned char **channel_input,short block_length,double rate,uint8_t n_segments);
int ldpc_encoder_optim(unsigned char *test_input,unsigned char *channel_input,short block_length,int nom_rate,int denom_rate,time_stats_t *tinput,time_stats_t *tprep,time_stats_t *tparity,time_stats_t *toutput);
int ldpc_encoder_optim_8seg(unsigned char **test_input,unsigned char **channel_input,short block_length,int nom_rate,int denom_rate,int n_segments,time_stats_t *tinput,time_stats_t *tprep,time_stats_t *tparity,time_stats_t *toutput);
*/
